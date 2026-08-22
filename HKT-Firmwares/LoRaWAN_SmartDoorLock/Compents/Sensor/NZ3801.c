#include "NZ3801.h"
#include "SI12T.h"
#include "W25X40CLSNIG.h"
#include "communicate.h"
#include "control_center.h"
#include "gpio.h"
#include "sound.h"
#include "spi.h"
#include "systick.h"
#include "uart.h"

#include "errno.h"
#include "iso14443a.h"
#include "mbi5120.h"
#include "nz3801-ab.h"
#include "nz3801-ab_com.h"
#include "zs902r.h"

#if (!FSV9522_CARD_ENABLE)

/**
 * @brief  硬复位读卡器
 * @param
 * @retval
 **/
void nz3801_rst(void)
{
    CARD_RST_L;
    tx_thread_sleep(1);
    CARD_RST_H;
    tx_thread_sleep(1);
}

/**
 * @brief  读卡器初始化
 * @param
 * @retval
 **/
void nz3801_init(void)
{
    // iso14443AProximityCard_t gvCardA;
    iso14443AInitialize();
}

/**
 * @brief  读卡器sleep
 * @param
 * @retval
 **/
void nz3801_sleep(void)
{
    // nz3801HwReset();
    // nzWriteReg(COMMAND, 0x30);
    nz3801SoftPwrDown(1);
}

/**
 * @brief  读卡器读UID
 * @param  uid uid缓冲区
 * @param  len uid长度
 * @retval 返回执行结果 读取成功ERR_NONE
 **/
u8 nz3801_uid_read(u8 *uid, u8 *len)
{
    iso14443AProximityCard_t gvCardA;
    u8 err;
    // u8 buf[260];
    // u8 clength;

    // err = nz3801ActivateField(false);
    // if (err != ERR_NONE)
    // {
    //     DEBUG_TRACE(WARN_TAG, "Field OFF Fail, %s", __func__);
    //     return err;
    // }
    // tx_thread_sleep(5);

    err = nz3801ActivateField(true);
    if (err != ERR_NONE) {
        DEBUG_TRACE(WARN_TAG, "Field ON FAIL, %s", __func__);
        return err;
    }
    // tx_thread_sleep(15);
    // tx_thread_sleep(1);
    TicksDelayUs(200);

    if (device_t.isTouchInit == 0) {
        device_t.isTouchInit = 1;
        si14t_init();
        tx_event_flags_set(&event_group, BIT_TOUCH, TX_OR);
    }
    // 选卡
    err = iso14443ASelect(ISO14443A_CMD_WUPA, &gvCardA);
    if (err == ERR_NONE) {
        // DEBUG_TRACE(LOG_TAG, "REQA OK, %s", __func__);
        // PrintInfoDumpExt(gvCardA.uid, gvCardA.actlength, " UID:");
        memcpy(uid, gvCardA.uid, gvCardA.actlength);
        *len = gvCardA.actlength;

        // gvCardA.fsdi = 8; // fsdi=8 -> FSD=256
        // err = iso14443AEnterProtocolMode(&gvCardA, buf, &clength);
        // if (err == ERR_NONE)
        // {
        //     DEBUG_TRACE(LOG_TAG, ">RATS  OK.");
        //     PrintInfoDumpExt(buf, clength, " ATS:");
        //     // DEBUG_TRACE(" FSCI=%d\r\n FWI=%d\r\n TA=%02x\r\n",gvCardA.fsci,gvCardA.fwi,gvCardA.TA);
        // }
        // else
        // {
        //     DEBUG_TRACE(WARN_TAG,">RATS\r\n FAIL.[%d]\r\n", err);
        // }
    }
    // else
    // {
    //     DEBUG_TRACE(WARN_TAG, "REQA FAIL[%d] NO Card!!!, %s", err, __func__);
    // }
    return err;
}

/**
 * @brief  单次读卡
 * @param
 * @retval 1 读卡成功 0读卡失败
 */
u8 singleCardRead(void)
{
    iso14443AProximityCard_t gvCardA;
    u8 err;
    nz3801_init();

    // err = nz3801ActivateField(false);
    // if (err != ERR_NONE)
    // {
    //     return 0;
    // }
    // mDelay(5);
    err = nz3801ActivateField(true);
    if (err != ERR_NONE) {
        return 0;
    }
    // mDelay(1);
    TicksDelayUs(100);
    // 选卡
    err = iso14443ASelect(ISO14443A_CMD_WUPA, &gvCardA);
    nz3801_sleep();
    if (err == ERR_NONE) {
        PrintInfoDumpExt(gvCardA.uid, gvCardA.actlength, " UID:");
        return 1;
    }
    return 0;
}

/**
 * @brief  用于读卡器控制
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_card(ULONG thread_input)
{
    (void)thread_input;

    ULONG actual_events;

    u8 uid[ISO14443A_MAX_UID_LENGTH] = {0};
    char str_uid[20];
    char temp_str_uid[20];
    u8 i, err, action, userNum, uid_len;

    u8 buffer[50];
    u8 sendLen;

    UINT status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
                                     BIT_CARD,         /* 等待标志 */
                                     TX_AND_CLEAR,     /* 等待特定bit满足 */
                                     &actual_events,   /* 获取实际值 */
                                     TX_WAIT_FOREVER); /* 永久等待 */
    nz3801_rst();
    nz3801_init();

    while (1) {
    __back:
        action = 0;
        tx_thread_sleep(500); // 500ms 周期读取
        if (device_t.sleepState || device_t.lockDoorTimeout) {
            goto __back;
        }
        memset(uid, 0, sizeof(uid));
        memset(str_uid, 0, sizeof(str_uid));
        nz3801_init();
        err = nz3801_uid_read(uid, &uid_len);
        nz3801_sleep();
        if (err == ERR_NONE) // 读卡成功
        {
            if (device_t.lockTimeout) // 锁定状态
            {
                DEBUG_TRACE(WARN_TAG, "The Lock is Locked, %s", __func__);
                memset(&s_touch, 0, sizeof(s_touch));
                /* 语音 */
                // 失败提示音
                // 系统已锁定
                Sound_Insert_Number(0, VOICE_TONE_FAIL);
                Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                tx_thread_sleep(2000); // 失败后强制延时播放语音
                goto __back;
            }

            snprintf(str_uid, uid_len * 2, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                     uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6], uid[7], uid[8], uid[9]);
            DEBUG_TRACE(LOG_TAG, "Read Card UID: %s", str_uid);
            if (device_t.lockMode) {
                device_t.operateTimeout = 20 * SEC_DELAY;
                if (device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_CARD || device_t.lockMode == SYSTEM_MODE_INPUT_USER_CARD) {
                    device_t.lockMode += 7;
                    goto __success; // 刷卡仅校验一次

                    /* 语音 */
                    // 请再次输入
                    //    Sound_Insert_Number(1, VOICE_INPUT_SECOND);
                    //    memcpy(temp_str_uid, str_uid, uid_len * 2);
                    //    DEBUG_TRACE(LOG_TAG, "Exec Success, Card Number[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                } else if (device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_CARD_2 || device_t.lockMode == SYSTEM_MODE_INPUT_USER_CARD_2) {
                    if (!memcmp(temp_str_uid, str_uid, uid_len * 2)) {
                    __success:
                        set_card_binding((u8)device_t.inputNum, str_uid, 1);
                        // 更新缓存
                        strncpy(coded_lock_t.card[(u8)device_t.inputNum].word, str_uid, uid_len * 2);
                        coded_lock_t.total_num_card++;
                        coded_lock_t.card[(u8)device_t.inputNum].isable = 1;
                        DEBUG_TRACE(LOG_TAG, "Exec Success, Save Card[%s], Lock Mode %d", str_uid, device_t.lockMode);

                        // 退出系统设置
                        device_t.lockMode = 0;
                        /* 语音 */
                        // 操作成功
                        Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);

                        struct lock_log exc_lock_info_t;
                        memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                        exc_lock_info_t.u_exec.isLocal = 1;
                        exc_lock_info_t.u_exec.inputAction = 1;
                        exc_lock_info_t.type = 2;
                        exc_lock_info_t.number = userNum;
                        strcat(exc_lock_info_t.word, str_uid);
                        exc_lock_info_t.stamp = Timestamp;
                        nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash

                        tx_thread_sleep(2000); // 等2s再读卡
                    } else {
                        device_t.lockMode -= 8; // 返回输入第一次读卡
                        DEBUG_TRACE(WARN_TAG, "Exec Fail, Please Input Card, Back Lock Mode %d", device_t.lockMode);
                        /* 语音 */
                        // 操作失败
                        // 请放卡
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                        Sound_Insert_Number(1, VOICE_INPUT_CARD);
                    }
                }
                /* 验证管理员密码 */
                else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_PASSWORD) {
                    i = 0xFF;
                    if (!coded_lock_t.total_num_finger && !coded_lock_t.total_num_card && !coded_lock_t.total_num_pass) {
                        device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_MODE;
                        DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Choose Mode, Lock Mode %d", device_t.lockMode);
                        /* 语音 */
                        // 操作成功
                        // 1 密码用户
                        // 2 卡用户
                        // 3 指纹用户
                        // 4 系统设置
                        Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);
                        Sound_Insert_Number(1, VOICE_NUM_1);
                        Sound_Insert_Number(1, VOICE_PASSWORD_USER);
                        Sound_Insert_Number(1, VOICE_NUM_2);
                        Sound_Insert_Number(1, VOICE_CARD_USER);
                        Sound_Insert_Number(1, VOICE_NUM_3);
                        Sound_Insert_Number(1, VOICE_FINGERPRINT_USER);
                        Sound_Insert_Number(1, VOICE_NUM_4);
                        Sound_Insert_Number(1, VOICE_SYSTEM_CONFIG);

                        device_t.openFailCnt = 0;
                        device_t.enterRootFailCnt = 0;
                    } else if (coded_lock_t.total_num_card > 0) {
                        for (i = 0; i < 10; i++) {
                            if (coded_lock_t.card[i].isable) {
                                if (strncmp(str_uid, coded_lock_t.card[i].word, uid_len * 2) == 0) {
                                    device_t.tamperAlarm = 0;
                                    device_t.enterRootNum = i;
                                    device_t.enterRootType = 1;
                                    device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_MODE;
                                    DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Choose Mode, Lock Mode %d", device_t.lockMode);
                                    /* 语音 */
                                    // 操作成功
                                    // 1 密码用户
                                    // 2 卡用户
                                    // 3 指纹用户
                                    // 4 系统设置
                                    Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);
                                    Sound_Insert_Number(1, VOICE_NUM_1);
                                    Sound_Insert_Number(1, VOICE_PASSWORD_USER);
                                    Sound_Insert_Number(1, VOICE_NUM_2);
                                    Sound_Insert_Number(1, VOICE_CARD_USER);
                                    Sound_Insert_Number(1, VOICE_NUM_3);
                                    Sound_Insert_Number(1, VOICE_FINGERPRINT_USER);
                                    Sound_Insert_Number(1, VOICE_NUM_4);
                                    Sound_Insert_Number(1, VOICE_SYSTEM_CONFIG);

                                    device_t.openFailCnt = 0;
                                    device_t.enterRootFailCnt = 0;
                                    break;
                                }
                            }
                        }
                        if (i >= 10) {
                            goto __card_fail;
                        }
                    } else {
                    __card_fail:
                        /* 语音 */
                        // 操作失败
                        // 请输入管理员信息
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                        if (++device_t.enterRootFailCnt >= 5) {
                            device_t.enterRootFailCnt = 0;
                            device_t.lockTimeout = 100 * SEC_DELAY;

                            /* 语音 */
                            // 系统已锁定
                            Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
#if FUNC_FINGERPRINT_ENABLE
                            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                            // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                        } else
                            Sound_Insert_Number(1, VOICE_PLS_INPUT_ROOT_INFO);
                    }
                    // 写入操作记录
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.enterRoot = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = i;
                    if (i < 100 || i == 0xFF)
                        exc_lock_info_t.u_exec.unlockSuccess = 1;
                    else
                        exc_lock_info_t.u_exec.unlockFail = 1;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                    tx_thread_sleep(2000);
                }
            } else {
                userNum = 0xFF;
                if (coded_lock_t.total_num_card == 0) {
                    device_t.openFailCnt++;
                    action = 0xC3; // MF 卡报警
                    DEBUG_TRACE(WARN_TAG, "Card Uid Not Match!!!");

                    /* 语音 */
                    // 失败提示音
                    Sound_Insert_Number(0, VOICE_TONE_FAIL);
                    if (device_t.openFailCnt >= 5) {
                        device_t.lockTimeout = 100 * SEC_DELAY;
#if FUNC_FINGERPRINT_ENABLE
                        tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                        // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                        DEBUG_TRACE(LOG_TAG, "System Enter Lock Status");
                        /* 语音 */
                        // 系统已锁定
                        Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                    } else {
                        /* 语音 */
                        // 开锁失败
                        Sound_Insert_Number(1, VOICE_OPEN_LOCK_FAIL);
#if FUNC_FINGERPRINT_ENABLE
                        tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_BLUE, TX_OR);
                        // finger_led_control(3, 1, 1, 0); // 蓝色常亮
#endif
                    }
                    // memset(buffer, 0, sizeof(buffer));
                    // sendLen = 7;

                    // buffer[0] = 0x30;
                    // buffer[1] = action;  // 操作记录
                    // buffer[2] = userNum; // 用户编号
                    // buffer[3] = (Timestamp >> 24) & 0xFF;
                    // buffer[4] = (Timestamp >> 16) & 0xFF;
                    // buffer[5] = (Timestamp >> 8) & 0xFF;
                    // buffer[6] = Timestamp & 0xFF;

                    // int free_number = lwrb_get_free(&lwrbBuff_t);
                    // if (free_number >= (sendLen + 1))
                    // {
                    //     lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                    //     lwrb_write(&lwrbBuff_t, buffer, sendLen);
                    //     lwrb_write_count++;
                    // }
                    // else
                    // {
                    //     DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                    // }

                    //                    if (action == 0xC3) // 写入开锁失败记录
                    //                    {
                    //                        struct lock_log exc_lock_info_t;
                    //                        memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    //                        exc_lock_info_t.u_exec.isLocal = 1;
                    //                        exc_lock_info_t.u_exec.unlockFail = 1;
                    //                        exc_lock_info_t.type = 2;
                    //                        exc_lock_info_t.number = userNum;
                    //                        if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger)
                    //                        {
                    //                            strcat(exc_lock_info_t.word, "123456");
                    //                        }
                    //                        else
                    //                        {
                    //                            strcat(exc_lock_info_t.word, str_uid);
                    //                        }
                    //                        exc_lock_info_t.stamp = Timestamp;
                    //                        nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                    //                    }
                } else {
                    for (i = 0; i < 100; i++) {
                        if (coded_lock_t.card[i].isable) {
                            if (strncmp(str_uid, coded_lock_t.card[i].word, uid_len * 2) == 0) {
                                userNum = i;
                                action = 0x03; // MF卡开门
                                /* 卡验证成功 执行开锁 */
                                tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
                                DEBUG_TRACE(LOG_TAG, "Card Unlock[%d]: %s", userNum, str_uid); // UID匹配成功
#if FUNC_FINGERPRINT_ENABLE
                                tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                                // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                                break;
                            }
                        }
                    }
                    if (i >= 100) {
                        action = 0xC3; // MF卡报警
                        device_t.openFailCnt++;
                        /* 语音 */
                        // 失败提示音
                        Sound_Insert_Number(0, VOICE_TONE_FAIL);
                        if (device_t.openFailCnt >= 5) {
                            device_t.lockTimeout = 100 * SEC_DELAY;
#if FUNC_FINGERPRINT_ENABLE
                            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                            // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                            DEBUG_TRACE(LOG_TAG, "System Enter Lock Status");
                            /* 语音 */
                            // 系统已锁定
                            Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                        } else {
                            /* 语音 */
                            // 开锁失败
                            Sound_Insert_Number(1, VOICE_OPEN_LOCK_FAIL);
#if FUNC_FINGERPRINT_ENABLE
                            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_BLUE, TX_OR);
                            // finger_led_control(3, 1, 1, 0); // 蓝色常亮
#endif
                        }
                        DEBUG_TRACE(LOG_TAG, "Card Uid Not Match: %s", str_uid);
                    }
                }
                memset(buffer, 0, sizeof(buffer));
                sendLen = 7;

                buffer[0] = 0x30;
                buffer[1] = action;  // 操作记录
                buffer[2] = userNum; // 用户编号
                buffer[3] = (Timestamp >> 24) & 0xFF;
                buffer[4] = (Timestamp >> 16) & 0xFF;
                buffer[5] = (Timestamp >> 8) & 0xFF;
                buffer[6] = Timestamp & 0xFF;

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendLen + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, buffer, sendLen);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }

                // 写入开锁失败记录
                if (action == 0xC3) {
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.unlockFail = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = userNum;
                    if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                        strcat(exc_lock_info_t.word, "123456");
                    } else {
                        strcat(exc_lock_info_t.word, str_uid);
                    }
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                } else if (action == 0x03)                    // 写入开锁成功记录
                {
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.unlockSuccess = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = userNum;
                    if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                        strcat(exc_lock_info_t.word, "123456");
                    } else {
                        strcat(exc_lock_info_t.word, str_uid);
                    }
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }

                /* MF卡开锁解除防拆报警*/
                if (device_t.tamperAlarm && action == 0x03) {
                    device_t.tamperAlarm = 0;
                    tamperio_state = READ_FUNC_KEY_STATUS;

                    sendLen = 2;
                    buffer[0] = 0x84;
                    buffer[1] = 0;
                    int free_number = lwrb_get_free(&lwrbBuff_t);
                    if (free_number >= (sendLen + 1)) {
                        lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                        lwrb_write(&lwrbBuff_t, buffer, sendLen);
                        lwrb_write_count++;
                    } else {
                        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                    }

                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.tamperRecover = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = userNum;
                    if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                        strcat(exc_lock_info_t.word, "123456");
                    } else {
                        strcat(exc_lock_info_t.word, coded_lock_t.card[i].word);
                    }
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }

                if (action == 0xC3) // 开锁失败停止读卡一段时间
                {
                    // LED闪烁一次
                    MBI5120_Display(0);
                    tx_thread_sleep(2000);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(1000);
                }
            }
        }
    }
}
#endif
