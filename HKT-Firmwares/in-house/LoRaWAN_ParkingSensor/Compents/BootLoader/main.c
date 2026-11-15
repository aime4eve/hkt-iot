
/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "flash.h"
#include "gpio.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

#include "upgrade_common.h"
#include "upgrade_crc.h"
#include "upgrade_flash.h"

/*==============================================================================
 * Boot decision (protocol doc 蓝牙升级协议.docx, "上电启动与升级标志判定")
 *
 *   UPG_FLAG_JUMP            -> validate APP SP, jump
 *   UPG_FLAG_RECOVER         -> verify staged image on ext flash, transfer-only
 *                               install, then CONFIRM_PENDING + reset
 *   UPG_FLAG_CONFIRM_PENDING -> count unconfirmed boots; > M -> park
 *   unknown/blank            -> jump (old BL "mark old valid" equivalent)
 *
 * The BootLoader has NO BLE and NO diff synthesis: the APP receives the file,
 * synthesizes (diff) into external flash and writes the staging info before
 * resetting with UPG_FLAG_RECOVER (doc req.3 "合成移出 BOOT").
 *============================================================================*/

/* Millisecond tick for upgrade_flash timeouts (BL flavor). */
uint32_t upg_tick_ms(void)
{
    return get_syspant_ms();
}

static void IWDG_Init(void)
{
    /* Configure the IWDG with window option disabled */
    /* ------------------------------------------------------- */
    /* (1) Enable the IWDG by writing 0x0000 CCCC in the IWDG_KR register */
    /* (2) Enable register access by writing 0x0000 5555 in the IWDG_KR register */
    /* (3) Write the IWDG prescaler by programming IWDG_PR from 0 to 7 - LL_IWDG_PRESCALER_4 (0) is lowest divider*/
    /* (4) Write the reload register (IWDG_RLR) */
    /* (5) Wait for the registers to be updated (IWDG_SR = 0x0000 0000) */
    /* (6) Refresh the counter value with IWDG_RLR (IWDG_KR = 0x0000 AAAA) */
    LL_IWDG_Enable(IWDG);                              /* (1) */
    LL_IWDG_EnableWriteAccess(IWDG);                   /* (2) */
    LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_256); /* (3) */
    LL_IWDG_SetReloadCounter(IWDG, 0xFFFF);            /* (4) */
    while (LL_IWDG_IsReady(IWDG) != 1)                 /* (5) */
    {
    }
    LL_IWDG_ReloadCounter(IWDG); /* (6) */

    FLASH_OBProgramInitTypeDef OptionsBytesStruct;
    /* Get the configuration status */
    HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);
    if ((OptionsBytesStruct.USERConfig & (FLASH_OPTR_IWDG_STOP | FLASH_OPTR_IWDG_SW)) != 0) {
        // 没有配置过就配置一次，有必要判断一下是否配置过，因为每次配置完都会导致重启，不能每次上电都无条件配置一次
        OptionsBytesStruct.OptionType = OPTIONBYTE_USER;
        OptionsBytesStruct.USERType = OB_USER_IWDG_STOP | OB_USER_IWDG_SW;
        OptionsBytesStruct.USERConfig &= (~(FLASH_OPTR_IWDG_STOP | FLASH_OPTR_IWDG_SW)); // STOP模式下停止看门狗计数
        HAL_FLASH_Unlock();
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
        HAL_FLASH_OB_Unlock();
        if (HAL_FLASHEx_OBProgram(&OptionsBytesStruct) != HAL_OK) {
            // 配置失败，都说重启大法好，我就重启下试试...
            NVIC_SystemReset();
        }
        if (HAL_FLASH_OB_Launch() != HAL_OK) // 加载flash配置，这里会导致重启
        {
            NVIC_SystemReset();
        }
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
    }
}

/* Halt forever. feed_wdt: 1 while the IWDG is armed (RECO path). */
static void BL_Park(uint8_t feed_wdt)
{
    INFO("\nboot: PARK - halt, waiting for debug-port reflash\n");
    while (1) {
        if (feed_wdt) {
            LL_IWDG_ReloadCounter(IWDG);
        }
        __WFI();
    }
}

/* Bring-up aid: golden values cross-checked by Compents/Upgrade/test/
 * golden_vectors.ps1 (doc examples use placeholder CRCs). */
static void bl_self_test(void)
{
    static const uint8_t p0[4] = {0x01, 0x02, 0x03, 0x04};

    INFO("selftest: kermit(01020304)=0x%04X expect 0xC54F\n", upg_crc16(p0, 4));
}

/*==============================================================================
 * Staging image install (transfer-only)
 *============================================================================*/
#define BL_XFER_RETRY_MAX   3U

static int bl_read_staging(upg_staging_t *st)
{
    uint16_t calc;

    if (upg_flash_read(UPG_STAGING_ADDR, (uint8_t *)st, sizeof(*st)) != 0) {
        INFO("staging: ext read fail\n");
        return -1;
    }
    if (st->magic != UPG_STAGING_MAGIC) {
        INFO("staging: bad magic 0x%08lX\n", st->magic);
        return -1;
    }
    calc = upg_crc16((const uint8_t *)st, sizeof(*st) - 2U);
    if (calc != st->crc16) {
        INFO("staging: struct crc fail calc=0x%04X stored=0x%04X\n", calc, st->crc16);
        return -1;
    }
    /* range gates before anything drives an erase/write loop */
    if ((st->new_fw_len == 0U) || (st->new_fw_len > UPG_APP_MAX_SIZE)) {
        INFO("staging: len %lu out of range\n", st->new_fw_len);
        return -1;
    }
    if ((st->src_addr != UPG_EX_PATCH_BASE) && (st->src_addr != UPG_EX_NEWFW_BASE)) {
        INFO("staging: bad src 0x%08lX\n", st->src_addr);
        return -1;
    }
    if ((st->src_addr == UPG_EX_PATCH_BASE) && (st->new_fw_len > UPG_EX_PATCH_DATA_SIZE)) {
        return -1;
    }
    if ((st->src_addr == UPG_EX_NEWFW_BASE) && (st->new_fw_len > UPG_EX_NEWFW_SIZE)) {
        return -1;
    }
    return 0;
}

/* CRC32 of len bytes on external flash, chunked + WDT fed (doc req.1 idempotence:
 * "先判外部 NewFW 区是否已有 CRC 通过的新固件"). */
static uint32_t bl_ext_crc32(uint32_t addr, uint32_t len)
{
    static uint8_t buf[512];
    uint32_t state;
    uint32_t chunk;

    upg_crc32_start(&state);
    while (len != 0U) {
        chunk = UPG_MIN(len, sizeof(buf));
        if (upg_flash_read(addr, buf, chunk) != 0) {
            return 0xFFFFFFFFUL;    /* distinct from any legal CRC */
        }
        upg_crc32_update(&state, buf, chunk);
        addr += chunk;
        len -= chunk;
        LL_IWDG_ReloadCounter(IWDG);
    }
    return upg_crc32_finish(state);
}

/* Copy the staged image into the internal APP region, then read the copy back
 * and CRC it (the write API reports success without reading back). Restartable:
 * each page is erased before it is written, so a retried attempt rewrites the
 * whole image from the intact staged copy. */
static int bl_transfer(const upg_staging_t *st)
{
    static uint8_t buf[UPG_INT_PAGE_SIZE];
    uint32_t attempt;
    uint32_t pct = 0;

    for (attempt = 1U; attempt <= BL_XFER_RETRY_MAX; attempt++) {
        uint32_t off = 0;
        int ok = 1;

        while (off < st->new_fw_len) {
            uint32_t n = UPG_MIN(st->new_fw_len - off, UPG_INT_PAGE_SIZE);
            int dw = (int)((n + 7U) / 8U);
            int i;

            if (upg_flash_read(st->src_addr + off, buf, n) != 0) {
                INFO("xfer: ext read fail @%lu\n", off);
                ok = 0;
                break;
            }
            /* pad the tail double-word with erased-state 0xFF */
            for (i = (int)n; i < dw * 8; i++) {
                buf[i] = 0xFFU;
            }
            if (BL_FLASH_WriteAppPage(UPG_APP_BASE + off, (uint64_t *)buf, dw) != SUCCESS) {
                INFO("xfer: write fail @%lu\n", off);
                ok = 0;
                break;
            }
            off += n;
            LL_IWDG_ReloadCounter(IWDG);

            if (((off * 100U) / st->new_fw_len) >= (pct + 25U)) {
                pct = (off * 100U) / st->new_fw_len;
                INFO("xfer: %lu%%\n", pct);
            }
        }
        if (!ok) {
            continue;
        }

        /* read-back verification over exactly new_fw_len bytes */
        if (upg_crc32((const uint8_t *)UPG_APP_BASE, st->new_fw_len) != st->new_fw_crc) {
            INFO("xfer: readback CRC fail (attempt %lu)\n", attempt);
            LL_IWDG_ReloadCounter(IWDG);
            continue;
        }
        return 0;
    }
    return -1;
}

/* RECO branch: verify staged image, transfer, set CONFIRM_PENDING, reset.
 * Never returns. */
static void bl_install(void)
{
    upg_staging_t st;
    upg_flag_t flag;

    if (upg_flash_init() != 0) {
        INFO("install: no external flash\n");
        goto fallback;
    }
    if (bl_read_staging(&st) != 0) {
        goto fallback;
    }
    /* Idempotence gate: only touch the APP region once the staged image has
     * passed CRC. A power loss during the previous transfer lands right back
     * here with the staged copy still intact. */
    if (bl_ext_crc32(st.src_addr, st.new_fw_len) != st.new_fw_crc) {
        INFO("install: staged image CRC fail, skip upgrade\n");
        goto fallback;
    }
    INFO("install: %s len=%lu new=0x%04X old=0x%04X\n",
         (st.pkg_type == UPG_PKG_DIFF) ? "diff" : "full",
         st.new_fw_len, st.new_version, st.old_version);

    if (bl_transfer(&st) != 0) {
        /* APP region half-written, staged copy still valid: park so a power
         * cycle retries the transfer instead of booting a damaged image. */
        BL_Park(1);
    }

    flag.upgrade_flag = UPG_FLAG_CONFIRM_PENDING;
    flag.old_version = st.old_version;
    flag.new_version = st.new_version;
    flag.new_fw_len = st.new_fw_len;
    flag.confirm_attempts = 0;
    if (BL_FLASH_WriteFlag(&flag) != SUCCESS) {
        INFO("install: flag write fail\n");
        BL_Park(1);
    }
    INFO("install: done, resetting into new firmware\n");
    mDelay(50);
    NVIC_SystemReset();

fallback:
    /* Skip the upgrade and keep running the (untouched) old firmware
     * (doc req.7: 回退 = 跳过升级, not restore). */
    flag.upgrade_flag = UPG_FLAG_JUMP;
    flag.old_version = 0;
    flag.new_version = 0;
    flag.new_fw_len = 0;
    flag.confirm_attempts = 0;
    (void)BL_FLASH_WriteFlag(&flag);
    INFO("install: fallback to old firmware\n");
    mDelay(50);
    NVIC_SystemReset();
}

/*==============================================================================
 * main
 *============================================================================*/
int main(void)
{
    upg_flag_t flag;

    Systick_Init();
    SystemClock_Config();

    BL_FLASH_ReadFlag(&flag);

    if (flag.upgrade_flag == UPG_FLAG_RECOVER) {
        App_Uart3Cfg();
        TIM16_Init(1000, 16);
        systemTime_Init();
        INFO("\n=== BootLoader RECOVER ===\n");
        IWDG_Init();
        bl_self_test();
        bl_install();           /* never returns */
    }

    if (flag.upgrade_flag == UPG_FLAG_CONFIRM_PENDING) {
        App_Uart3Cfg();
        TIM16_Init(1000, 16);
        systemTime_Init();
        INFO("boot: fw pending confirm, boot %d/%d\n",
             flag.confirm_attempts + 1, UPG_CONFIRM_MAX_ATTEMPTS);
        flag.confirm_attempts++;
        if (flag.confirm_attempts > UPG_CONFIRM_MAX_ATTEMPTS) {
            /* New firmware never reported its version: park for a debug-port
             * reflash (doc req.4). */
            BL_Park(0);
        }
        (void)BL_FLASH_WriteFlag(&flag);
        if (BL_JumpToApp() != 0) {
            BL_Park(0);
        }
        BL_Park(0);
    }

    /* JUMP / unknown / blank: straight to the application */
    if (BL_JumpToApp() != 0) {
        App_Uart3Cfg();
        TIM16_Init(1000, 16);
        systemTime_Init();
        INFO("boot: invalid APP SP, park\n");
        BL_Park(0);
    }
    BL_Park(0);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}
