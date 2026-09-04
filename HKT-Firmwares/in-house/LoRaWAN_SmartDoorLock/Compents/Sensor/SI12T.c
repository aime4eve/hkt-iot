#include "SI12T.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "iic.h"
#include "lwrb.h"
#include "sound.h"
#include "systick.h"
#include "uart.h"

#include "LoRaWAN_APPLY.h"
#include "MBI5120.h"
#include "W25X40CLSNIG.h"
#include "ZS902R.h"

s_si12t_t s_touch;
const u8 voice_volume_enum[] = {VOICE_VOLUME1, VOICE_VOLUME2, VOICE_VOLUME3, VOICE_VOLUME4, VOICE_VOLUME5, VOICE_VOLUME6, VOICE_VOLUME7, VOICE_VOLUME8};

/**
 * @brief  寻找字符串并进行对比
 * @param  src: 字符串源
 * @param  str: 待对比字符串
 * @retval true 匹配成功 false 匹配失败
 */
bool str_cmp(char *src, char *str)
{
    if (src == NULL || str == NULL) {
        return false;
    }
    int src_len = strlen(src);
    int str_len = strlen(str);
    int match_cnt = 0;
    char *index = str;
    char *src_index = src;
    char *match_index = NULL;
    int match_i;

    if (src_len < str_len) {
        return false;
    }

    for (int i = 0; i < src_len; i++) {
        if (*src_index == *index) // 若对应位置字符大小相等，则进入循环
        {
            match_cnt++;
            index++;
            if (match_cnt == 1) {
                match_index = src_index;
                match_i = i;
            }
        } else {
            match_cnt = 0; // 连续匹配
            index = str;

            if (match_index) {
                src_index = match_index;
                i = match_i;
                match_index = NULL;
            }
        }
        src_index++;
        if (match_cnt == str_len)
            return true;
    }
    return false;
}

/**
 * @brief  硬复位触摸传感器
 * @param
 * @retval
 **/
void si12tch_rst(void)
{
    TCH_RST_H;
    tx_thread_sleep(1);
    TCH_RST_L;
    tx_thread_sleep(1);
}

/**
 * @brief  触摸传感器初始化
 * @param
 * @retval
 **/
void si14t_init(void)
{
    TCH_IIC_EN_L;
    TCH_SCT_L;
    si12tch_rst();

    // 停止感应和校准
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Ch_hold1, 0xFF);
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Ch_hold2, 0xFF);

    // // 软件复位
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL2, 0x0F);
    // tx_thread_sleep(10);

    // 使能能睡眠模式
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL2, 0x07);

    // 关闭睡眠模式
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL2, 0x03);
    tx_thread_sleep(10);

    // 灵敏度设置 1.6%
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense1, 0xBA); // ch1 1.2%
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense1, 0xAA); // ch1/2 1.2%
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense2, 0xBB);
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense3, 0xBB);
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense4, 0xBB);
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense5, 0xBB);
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense6, 0xBB);

    // 灵敏度设置
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense1, 0x88);
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense2, 0x88);
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense3, 0x88);
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense4, 0x88);
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense5, 0x88);
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Sense6, 0x88);

    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL1, 0x18);
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL1, 0x3B);// 中断在高输出
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL1, 0x73); // 0x6B);// 中断输出：低、中或者高灵敏度中断
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL1, 0x13); // 中或者高灵敏度中断
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL1, 0x0B); // 低、中或者高灵敏度中断 感测时间适中
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL1, 0x08); // 低、中或者高灵敏度中断 感测时间最短
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL1, 0x38); // 中断在高输出 感测时间最短
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL1, 0x39);
    // i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL1, 0x3A); // 中断在高输出 感测时间适中

    // 感测校准
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Ch_hold1, 0x00);
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Ch_hold2, 0xF0);

    // 失能参考值复位
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Ref_rst1, 0x00);
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Ref_rst2, 0xF0);

    // 启用参考校准
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Cal_hold1, 0x00);
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, Cal_hold2, 0x00);
    tx_thread_sleep(40);
    TCH_IIC_EN_H;
}

void si12t_sleep(void)
{
    TCH_IIC_EN_L;
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL2, 0x07); // 使能睡眠模式
    TCH_IIC_EN_H;
}

void si12t_wakeup(void)
{
    TCH_IIC_EN_L;
    i2c_hard_write_onebyte(SI12TCH_IIC_ADDR, CTRL2, 0x03); // 不使能睡眠模式
    TCH_IIC_EN_H;
}

void out_state_printf(u8 ch, u8 state)
{
    INFO("\r\nCH[%d]: ", ch);
    switch (state) {
    case 0:
        INFO("No Out");
        break;
    case 1:
        INFO("Low Out");
        break;
    case 2:
        INFO("Mid Out");
        break;
    case 3:
        INFO("High Out");
        break;
    default:
        break;
    }
}

void si12t_clear_read(void)
{
    u8 buffer[3];
    TCH_IIC_EN_L;
    i2c_hard_write_read(SI12TCH_IIC_ADDR, Output1, buffer, 3);
    TCH_IIC_EN_H;
}

/**
 * @brief  读取各个按键的触摸状态
 *      =======================================================
 *      |      output3     |     output2    |    output1     |
 *      |       8bit       |       8bit     |      8bit      |
 *      | 00   00  00   00 | 00  00  00  00 | 00  00  00  00 |
 *      | t12  t11 t10  t9 | t8  t7  t6  t5 | t4  t3  t2  t1 |
 *      =======================================================
 * @param
 * @retval true 有效  false 无效
 **/
bool si12t_output_read(void)
{
    int i = 0;
    u8 buffer[3];
    u8 state, value, valid = 0;
    static u8 lastKey[12];
    static u8 wait_clean;
    static u32 intervel;

    TCH_IIC_EN_L;
    i2c_hard_write_read(SI12TCH_IIC_ADDR, Output1, buffer, 3);
    TCH_IIC_EN_H;
    // DEBUG_TRACE(LOG_TAG, "Read Touch Status: %02X %02X %02X", buffer[0], buffer[1], buffer[2]);

    if (buffer[0] + buffer[1] + buffer[2] == 0) {
        wait_clean = 0;
    } else {
        if (wait_clean && intervel > get_syspant_ms()) {
            DEBUG_TRACE(WARN_TAG, "Read Touch Invaild");
            return false;
        }
    }

    for (i = 0; i < 12; i++) {
        value = buffer[i / 4];
        state = (value >> (i % 4) * 2) & 0x03;
        if (state >= 0x03) {
            // if (lastKey[i] <= state)
            valid = 1;
            // 转ASCII展示
#if EU_TOUCH_LED
            if (i == 0)
                s_touch.password[s_touch.cnt] = 0x23; // #
            else if (i == 1)
                s_touch.password[s_touch.cnt] = 0x30; // 0
            else if (i == 2)
                s_touch.password[s_touch.cnt] = 0x2A; // *
            else
                s_touch.password[s_touch.cnt] = 0x3C - i;
            break;
#else
            if (i == 11)
                s_touch.password[s_touch.cnt] = 0x23; // #
            else if (i == 9)
                s_touch.password[s_touch.cnt] = 0x2A; // *
            else if (i == 10)
                s_touch.password[s_touch.cnt] = 0x30; // 0
            else
                s_touch.password[s_touch.cnt] = i + 0x31;
            break;
#endif
        }
        lastKey[i] = state;
        // out_state_printf(i + 1, state); // 打印状态
    }
    if (valid) {
        wait_clean = 1;
        intervel = get_syspant_ms() + 100;
        s_touch.cnt++;
        s_touch.timeout = 10 * SEC_DELAY;
        return true;
    }
    return false;
}

#if FUNC_OPERATIONAL_VERSION_ENABLE

/**
 * @brief  触摸事件
 * @param  thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_touch(ULONG thread_input)
{
    (void)thread_input;
    extern u8 uart_touch_press;
    ULONG actual_events;
    ULONG mian_bit;

    UINT status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
                                     BIT_TOUCH,        /* 等待标志 */
                                     TX_AND_CLEAR,     /* 等待特定bit满足 */
                                     &actual_events,   /* 获取实际值 */
                                     TX_WAIT_FOREVER); /* 永久等待 */

    u8 userNum, i, action;
    bool ret;

    u8 buffer[50];
    u8 sendLen;

    while (1) {
    __back:
        tx_thread_sleep(50);
        if (device_t.sleepState) {
            goto __back;
        }

        if (device_t.isLedShow)
            MBI5120_Display(0xFFFF);

        // INFO("\r\n>>>>>>>>>>>>>>>  si12t  start <<<<<<<<<<<<<<<<<<<\r\n");
        si12t_clear_read(); // 清除读取状态
        GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, ENABLE);
        UINT status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
                                         BIT_TOUCH,        /* 等待标志 */
                                         TX_AND_CLEAR,     /* 等待特定bit满足 */
                                         &actual_events,   /* 获取实际值 */
                                         TX_WAIT_FOREVER); /* 永久等待 */

        tx_event_flags_get(&main_event_group, MAIN_BIT_DISPLAY_ALL, TX_OR, &mian_bit, TX_NO_WAIT);
        if (mian_bit & MAIN_BIT_NO_TOUCH) {
            goto __back;
        }

        // INFO("\r\n>>>>>>>>>>>>>>>  si12t  end <<<<<<<<<<<<<<<<<<<\r\n");
        if (uart_touch_press != 0xFF) { // 串口模拟按键输入
            s_touch.password[s_touch.cnt++] = uart_touch_press;
            uart_touch_press = 0xFF;
        } else {
            bool ret = si12t_output_read();
            if (ret == false)
                goto __back;
        }

        INFO("\r\n>>>>>>>>>>>>>>>touch data[%d]: %s<<<<<<<<<<<<<<<<<<<\r\n", s_touch.cnt, s_touch.password);
        if (device_t.lockTimeout) { // 锁定状态
            DEBUG_TRACE(WARN_TAG, "The Lock is Locked, %s", __func__);
            memset(&s_touch, 0, sizeof(s_touch));
            /* 语音 */
            // 失败提示音
            // 系统已锁定
            Sound_Insert_Number(0, VOICE_TONE_FAIL);
            Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);

            // MBI5120_Display(0);
            tx_thread_sleep(2000); // 失败后强制延时播放语音
            // MBI5120_Display(0xFFFF);
            goto __back;
        }

        if (device_t.lockDoorTimeout && s_touch.password[s_touch.cnt - 1] == 0x2A) {
            device_t.lockDoorTimeout = 0; // 立马上锁
            memset(&s_touch, 0, sizeof(s_touch));
            DEBUG_TRACE(LOG_TAG, "Use * For Lock");
            tx_thread_sleep(1000);
            goto __back;
        }

        if (s_touch.clear_signal) {
            MBI5120_Display(0xFFFF);
            tx_thread_sleep(1000);
            memset(&s_touch, 0, sizeof(s_touch));
            goto __back;
        }

        // 在已开锁状态下不允许按除*号外的按键
        if (device_t.lockDoorTimeout && s_touch.password[s_touch.cnt - 1] != 0x2A) {
            memset(&s_touch, 0, sizeof(s_touch));
            goto __back;
        }

        /* 语音 */
        // 键盘触摸提示音
        if (s_touch.password[0] != 0x23 || device_t.lockMode)
            Sound_Insert_Number(0, VOICE_TONE_TOUCH);

        u_mbi5120_t.dat = getLedShowNum(s_touch.password[s_touch.cnt - 1]);
        MBI5120_Display(u_mbi5120_t.dat);
        device_t.isLedShow = 1;

        /* 操作 */
        if (s_touch.cnt == 2 && s_touch.password[0] == 0x2A && s_touch.password[1] == 0x2A && !device_t.lockMode) {
            MBI5120_Display(0); // 关闭显示
            goto __back;
        } else if (s_touch.password[s_touch.cnt - 1] == 0x2A && s_touch.cnt >= 2) { // 回删密码
            memset(&s_touch, 0, sizeof(s_touch));
            DEBUG_TRACE(LOG_TAG, "Clear Password: %s", s_touch.password);
            goto __back;
        }

        i = 0;
        memset(buffer, 0, sizeof(buffer));

        /* 判断触摸按键操作结束 */
        if (s_touch.password[s_touch.cnt - 1] == 0x23 || s_touch.cnt >= 20) {
            if (s_touch.cnt == 1) { // 叮咚提示音
#if FUNC_DOORBELL_ENABLE
                /* 语音 */
                Sound_Insert_Number(0, VOICE_TONE_DINGDONG);
#else
                tx_event_flags_set(&main_event_group, MAIN_BIT_NO_TOUCH, TX_OR);
                if (LoRaWAN.joinState) { // 入网成功才可以发起
                    if (device_t.sleepDelay < 15)
                        device_t.sleepDelay = 15;
                    Sound_Insert_Number(1, VOICE_EXECUTING); // 执行中，请稍后
                    MBI5120_Display(0);
#if FUNC_FINGERPRINT_ENABLE
                    tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                    // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                    u8 buf[2] = {0x32, 0x00};
                    u8 waitCnt = 100;
                    union u_mbi5120 t_mbi5120;

                    tx_event_flags_get(&event_group, BIT_SYNC, TX_OR_CLEAR, NULL, TX_NO_WAIT); // 发送之前清除事件
                    if (sendLoRaWANData(buf, 2) == true) {
                        if (device_t.sleepDelay < 15)
                            device_t.sleepDelay = 15;
                        while (--waitCnt) { // 等待密码下行
                            t_mbi5120.dat = Mbi_ShowNum(waitCnt / 10);
                            MBI5120_Display(t_mbi5120.dat);
                            tx_thread_sleep(100);
                            tx_event_flags_get(&event_group, BIT_SYNC, TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
                            if (actual_events & BIT_SYNC) {
                                break;
                            }
                        }
                        if (waitCnt > 0)
                            Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);
                        else
                            Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                    } else {
                        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                    }
                    MBI5120_Display(0xFFFF);
                }
#endif
                memset(&s_touch, 0, sizeof(s_touch));
                goto __back;
            } else if (s_touch.cnt == 3 && s_touch.password[0] == 0x37 && s_touch.password[1] == 0x37) { // 77 + # 检查时间同步状态
                if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                // 2024-12-20 00:00:00
                if (Timestamp < 1734624000) {
                    MBI5120_Display(0);
                    tx_thread_sleep(500);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(500);
                    MBI5120_Display(0);
                    tx_thread_sleep(500);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(500);
                    MBI5120_Display(0);
                    tx_thread_sleep(500);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(500);
                } else {
                    MBI5120_Display(0);
                    tx_thread_sleep(500);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(500);
                }
                goto __back;
            } else if (s_touch.cnt == 2 && s_touch.password[0] == 0x31) { // 1 + # 主动同步数据
                tx_event_flags_set(&main_event_group, MAIN_BIT_NO_TOUCH, TX_OR);
                if (device_t.sleepDelay < 15)
                    device_t.sleepDelay = 15;
                if (LoRaWAN.joinState) {                     // 入网成功才可以发起
                    Sound_Insert_Number(1, VOICE_EXECUTING); // 执行中，请稍后
                    MBI5120_Display(0);
#if FUNC_FINGERPRINT_ENABLE
                    tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                    // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif

                    u8 buf[2] = {0x32, 0x00};
                    u8 waitCnt = 100;
                    union u_mbi5120 t_mbi5120;

                    tx_event_flags_get(&event_group, BIT_SYNC, TX_OR_CLEAR, NULL, TX_NO_WAIT); // 发送之前清除事件
                    if (sendLoRaWANData(buf, 2) == true) {
                        if (device_t.sleepDelay < 15)
                            device_t.sleepDelay = 15;
                        while (--waitCnt) { // 等待密码下行
                            t_mbi5120.dat = Mbi_ShowNum(waitCnt / 10);
                            MBI5120_Display(t_mbi5120.dat);
                            tx_thread_sleep(100);
                            tx_event_flags_get(&event_group, BIT_SYNC, TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
                            if (actual_events & BIT_SYNC) {
                                break;
                            }
                        }
                        if (waitCnt > 0)
                            Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);
                        else
                            Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                    } else {
                        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                    }
                    MBI5120_Display(0xFFFF);
                } else {
                    reconnect_stamp = Timestamp;
                    LoRaWAN.joinState = 0;
                    LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
                    // Sound_Insert_Number(1, VOICE_EXECUTING);

                    MBI5120_Display(0);
                    tx_thread_sleep(2000);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(1000);
                }
                memset(&s_touch, 0, sizeof(s_touch));
                tx_event_flags_set(&main_event_group, ~(MAIN_BIT_NO_TOUCH), TX_AND);
                // if (!LoRaWAN.joinState) { // 入网成功才可以发起
                //     reconnect_stamp = Timestamp;
                //     LoRaWAN.joinState = 0;
                //     LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                //     LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
                //     // Sound_Insert_Number(1, VOICE_EXECUTING);

                //     MBI5120_Display(0);
                //     tx_thread_sleep(2000);
                //     MBI5120_Display(0xFFFF);
                //     tx_thread_sleep(1000);
                // }
                // memset(&s_touch, 0, sizeof(s_touch));
                // tx_event_flags_set(&main_event_group, ~(MAIN_BIT_NO_TOUCH), TX_AND);
                goto __back;
            } else if (s_touch.cnt == 2 && s_touch.password[0] == 0x32) { // 2 + # 重启LoRaWAN模组
                tx_event_flags_set(&main_event_group, MAIN_BIT_NO_TOUCH, TX_OR);
                setLoRaWANStatus(LoRaWAN_WAKE_STATUS); // 唤醒模块
                AT_LoRaWAN_setATEnter();
                AT_LoRaWAN_reboot(); // 保存配置参数
                LoRa_DelayMs(3000);

                memset(&s_touch, 0, sizeof(s_touch));
                tx_event_flags_set(&main_event_group, ~(MAIN_BIT_NO_TOUCH), TX_AND);
                goto __back;
            } else if (s_touch.cnt == 3 && s_touch.password[s_touch.cnt - 2] == 0x35 && s_touch.password[s_touch.cnt - 3] == 0x33) {
                /* 语音 */
                // 关锁提示音
                // 已关锁
                Sound_Insert_Number(0, VOICE_TONE_OFF_LOCK);
                Sound_Insert_Number(1, VOICE_OFF_LOCK);
                /* 反转 */
                MOTOR_REVERSAL();
                tx_thread_sleep(150);
                /* 刹车 */
                MOTOR_BRAKE();
                /* 待机 */
                MOTOR_STANDBY();
            } else if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card) {
                userNum = 0xFF;
                if (str_cmp(s_touch.password, "123456") == true) {
                    DEBUG_TRACE(LOG_TAG, "Password Match[%d]: %s", userNum, s_touch.password); // 密码匹配成功
                    action = 0x02;                                                             // 密码开门
                    tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
                    tx_thread_sleep(2000);
                } else if (coded_lock_t.temp_pass.isable && str_cmp(s_touch.password, coded_lock_t.temp_pass.word) == true) { // 临时密码解锁
                    DEBUG_TRACE(LOG_TAG, "Password Match[%d]: %s", userNum, s_touch.password);                                // 密码匹配成功
                    action = 0x02;                                                                                            // 密码开门
                    tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
                    tx_thread_sleep(2000);
                } else {
                    action = 0xC2; // 密码报警
                    device_t.openFailCnt++;
                    /* 语音 */
                    // 失败提示音
                    Sound_Insert_Number(1, VOICE_TONE_FAIL);
                    if (device_t.openFailCnt >= 5) {
                        device_t.lockTimeout = 100 * SEC_DELAY;
                        DEBUG_TRACE(LOG_TAG, "System Enter Lock Status");
                        /* 语音 */
                        // 系统已锁定
                        Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                    } else {
                        /* 语音 */
                        // 开锁失败
                        Sound_Insert_Number(1, VOICE_OPEN_LOCK_FAIL);
                    }
                    DEBUG_TRACE(LOG_TAG, "Password Error: %s", s_touch.password);

                    MBI5120_Display(0);
                    tx_thread_sleep(2000); // 失败后强制延时播放语音
                    MBI5120_Display(0xFFFF);
                }
            } else {
                userNum = 0xFF;
                action = 0;
                for (i = 0; i < MAX_SECRET_TIMELINESS_NUM + 1; i++) {
                    if (coded_lock_t.pass[i].isable) {
                        if (coded_lock_t.pass[i].user_num == 0) {
                            if (str_cmp(s_touch.password, coded_lock_t.pass[i].word) == true) {
                                userNum = coded_lock_t.pass[i].user_num;
                                action = 0x02;                                                              // 密码开门
                                tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);                         // 关闭灯光
                                DEBUG_TRACE(LOG_TAG, "Password Unlock[%d]: %s", userNum, s_touch.password); // 密码匹配成功
                                tx_thread_sleep(2000);
                                break;
                            }
                        } else {
                            if (Timestamp > coded_lock_t.pass[i].start_timestamp && Timestamp < coded_lock_t.pass[i].end_timestamp &&
                                str_cmp(s_touch.password, coded_lock_t.pass[i].word) == true) {
                                if (coded_lock_t.pass[i].valid_cnt) { // 有效次数使用完后，直接变为无效状态
                                    coded_lock_t.pass[i].valid_cnt--;
                                    if (!coded_lock_t.pass[i].valid_cnt) {
                                        coded_lock_t.pass[i].isable = 0;
                                    }
                                    FLASH_Write_Door_Lock_Coded();
                                }
                                userNum = coded_lock_t.pass[i].user_num;
                                action = 0x02;                                                              // 密码开门
                                tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);                         // 关闭灯光
                                DEBUG_TRACE(LOG_TAG, "Password Unlock[%d]: %s", userNum, s_touch.password); // 密码匹配成功
                                tx_thread_sleep(2000);
                                break;
                            }
                        }
                    }
                }
                if (i >= MAX_SECRET_TIMELINESS_NUM + 1) {
                    if (coded_lock_t.temp_pass.isable && str_cmp(s_touch.password, coded_lock_t.temp_pass.word) == true) { // 临时密码解锁
                        DEBUG_TRACE(LOG_TAG, "Password Match[%d]: %s", userNum, s_touch.password);                         // 密码匹配成功
                        action = 0x02;                                                                                     // 密码开门
                        tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
                        tx_thread_sleep(2000);
                    } else if (i >= 11 && action != 0x02) {
                        action = 0xC2; // 密码报警
                        device_t.openFailCnt++;
                        /* 语音 */
                        // 失败提示音
                        Sound_Insert_Number(1, VOICE_TONE_FAIL);
                        if (device_t.openFailCnt >= 5) {
                            device_t.lockTimeout = 100 * SEC_DELAY;
                            DEBUG_TRACE(LOG_TAG, "System Enter Lock Status");
                            /* 语音 */
                            // 系统已锁定
                            Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                        } else {
                            /* 语音 */
                            // 开锁失败
                            Sound_Insert_Number(1, VOICE_OPEN_LOCK_FAIL);
                        }
                        DEBUG_TRACE(LOG_TAG, "Password Error: %s", s_touch.password);
                        MBI5120_Display(0);
                        tx_thread_sleep(2000); // 失败后强制延时播放语音
                        MBI5120_Display(0xFFFF);
                    }
                }
            }
            memset(&s_touch, 0, sizeof(s_touch));
            sendLen = 7;

            buffer[0] = 0x30;
            buffer[1] = action;  // 操作记录
            buffer[2] = userNum; // 用户编号
            buffer[3] = (Timestamp >> 24) & 0xFF;
            buffer[4] = (Timestamp >> 16) & 0xFF;
            buffer[5] = (Timestamp >> 8) & 0xFF;
            buffer[6] = Timestamp & 0xFF;

            int free_number = lwrb_get_free(&lwrbBuff_t);
            if (free_number >= (sendLen + 1)) // 检查缓冲区是否已满
            {
                lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                lwrb_write(&lwrbBuff_t, buffer, sendLen);
                lwrb_write_count++;
            } else {
                DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            }

            if (action == 0xC2) { // 写入开锁失败记录
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.unlockFail = 1;
                exc_lock_info_t.type = 0;
                exc_lock_info_t.number = userNum;
                if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card) {
                    strcat(exc_lock_info_t.word, "123456");
                } else {
                    strcat(exc_lock_info_t.word, coded_lock_t.pass[i].word);
                }
                exc_lock_info_t.stamp = Timestamp;
                // nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            } else if (action == 0x02) { // 写入开锁成功记录
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.unlockSuccess = 1;
                exc_lock_info_t.type = 0;
                exc_lock_info_t.number = userNum;
                if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card) {
                    strcat(exc_lock_info_t.word, "123456");
                } else {
                    strcat(exc_lock_info_t.word, coded_lock_t.pass[i].word);
                }
                exc_lock_info_t.stamp = Timestamp;
                // nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            }

            /* 输入开锁密码解除防拆报警*/
            if (device_t.tamperAlarm && action == 0x02) {
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
                exc_lock_info_t.type = 0;
                exc_lock_info_t.number = userNum;
                if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card) {
                    strcat(exc_lock_info_t.word, "123456");
                } else {
                    strcat(exc_lock_info_t.word, coded_lock_t.pass[i].word);
                }
                exc_lock_info_t.stamp = Timestamp;
                // nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            }
        }
    }
}

#else

/**
 * @brief  分配空闲编号(按#号键自动生成编号)
 * @param  type 0密码类型 1卡类型 2指纹类型
 * @param  isRoot 0普通用户 1管理员用户
 * @retval 编号
 */
u8 distribute_idle_number(u8 type, u8 isRoot)
{
    u8 number = 0xFF, i;
    switch (type) {
    case 0:
        if (isRoot) {
            for (i = 0; i < 10; i++) {
                if (!coded_lock_t.pass[i].isable) {
                    number = i;
                    break;
                }
            }
        } else {
            for (i = 10; i < 100; i++) {
                if (!coded_lock_t.pass[i].isable) {
                    number = i;
                    break;
                }
            }
        }
        break;
    case 1:
        if (isRoot) {
            for (i = 0; i < 10; i++) {
                if (!coded_lock_t.card[i].isable) {
                    number = i;
                    break;
                }
            }
        } else {
            for (i = 10; i < 100; i++) {
                if (!coded_lock_t.card[i].isable) {
                    number = i;
                    break;
                }
            }
        }
        break;
    case 2:
        if (isRoot) {
            for (i = 0; i < 10; i++) {
                if (!coded_lock_t.finger[i].isable) {
                    number = i;
                    break;
                }
            }
        } else {
            for (i = 10; i < 100; i++) {
                if (!coded_lock_t.finger[i].isable) {
                    number = i;
                    break;
                }
            }
        }
        break;
    default:
        break;
    }
    return number;
}

/**
 * @brief  系统模式处理
 * @param
 * @retval true 监听下一次触摸  false 无符合条件
 */
bool lock_mode_event(void)
{
    /* * 操作 */
    if (!device_t.lockMode)
        return false;
    u16 i;
    static char pass[10] = {0};
    static u8 pass_len = 0;

    device_t.operateTimeout = 20 * SEC_DELAY;
    if (s_touch.password[s_touch.cnt - 1] == 0x2A) {
        /* 密码回删 */
        if ((device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_PASSWORD ||
             device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_PASSWORD ||
             device_t.lockMode == SYSTEM_MODE_INPUT_USER_PASSWORD ||
             device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_PASSWORD_2 ||
             device_t.lockMode == SYSTEM_MODE_INPUT_USER_PASSWORD_2) &&
            s_touch.cnt >= 2) {
            DEBUG_TRACE(LOG_TAG, "Clear Password: %s", s_touch.password);
        } else { // 模式返回 重播语音
            if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_PASSWORD) {
                device_t.lockMode--;
            } else if (device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_MODE) {
                device_t.lockMode = SYSTEM_MODE_WAIT_INPUT_PASSWORD;
                /* 语音 */
                // 请输入管理员信息
                Sound_Insert_Number(1, VOICE_PLS_INPUT_ROOT_INFO);
            } else if (device_t.lockMode == SYSTEM_MODE_SYSTEM_FACTORY) {
                device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_SYSTEMCONFIG;
                /* 语音 */
                // 系统设置
                // 1 音量加
                // 2 音量减
                // 3 联网开启
                // 4 联网关闭
                // 9 系统初始化
                Sound_Insert_Number(1, VOICE_SYSTEM_CONFIG);
                Sound_Insert_Number(1, VOICE_NUM_1);
                Sound_Insert_Number(1, VOICE_VOLUME_ADD);
                Sound_Insert_Number(1, VOICE_NUM_2);
                Sound_Insert_Number(1, VOICE_VOLUME_LESS);
#if FUNC_CONFIG_NETWORK_ENABLE
                Sound_Insert_Number(1, VOICE_NUM_3);
                Sound_Insert_Number(1, VOICE_NETWORK_ON);
                Sound_Insert_Number(1, VOICE_NUM_4);
                Sound_Insert_Number(1, VOICE_NETWORK_OFF);
#endif
                // Sound_Insert_Number(1, VOICE_NUM_9);
                // Sound_Insert_Number(1, VOICE_SYSTEM_INIT);
            } else if (device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_SYSTEMCONFIG) {
                device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_MODE;
                /* 语音 */
                // 1 密码用户
                // 2 卡用户
                // 3 指纹用户
                // 4 系统设置
                Sound_Insert_Number(1, VOICE_NUM_1);
                Sound_Insert_Number(1, VOICE_PASSWORD_USER);
                Sound_Insert_Number(1, VOICE_NUM_2);
                Sound_Insert_Number(1, VOICE_CARD_USER);
                Sound_Insert_Number(1, VOICE_NUM_3);
                Sound_Insert_Number(1, VOICE_FINGERPRINT_USER);
                Sound_Insert_Number(1, VOICE_NUM_4);
                Sound_Insert_Number(1, VOICE_SYSTEM_CONFIG);
            } else if (device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_USER_PASSWORD ||
                       device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_USER_CARD ||
                       device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_USER_FINGER) {
                device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_MODE;
                /* 语音 */
                // 1 密码用户
                // 2 卡用户
                // 3 指纹用户
                // 4 系统设置
                Sound_Insert_Number(1, VOICE_NUM_1);
                Sound_Insert_Number(1, VOICE_PASSWORD_USER);
                Sound_Insert_Number(1, VOICE_NUM_2);
                Sound_Insert_Number(1, VOICE_CARD_USER);
                Sound_Insert_Number(1, VOICE_NUM_3);
                Sound_Insert_Number(1, VOICE_FINGERPRINT_USER);
                Sound_Insert_Number(1, VOICE_NUM_4);
                Sound_Insert_Number(1, VOICE_SYSTEM_CONFIG);
            } else if (device_t.lockMode >= SYSTEM_MODE_WAIT_INPUT_ROOT_PASSWORD_NUMBER && device_t.lockMode <= SYSTEM_MODE_WAIT_DELETE_PASSWORD_NUMBER) {
                device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_USER_PASSWORD;
                /* 语音 */
                // 密码用户
                // 1 录入管理信息
                // 2 录入用户信息
                // 3 删除用户信息
                Sound_Insert_Number(1, VOICE_PASSWORD_USER);
                Sound_Insert_Number(1, VOICE_NUM_1);
                Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
                Sound_Insert_Number(1, VOICE_NUM_2);
                Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
                Sound_Insert_Number(1, VOICE_NUM_3);
                Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
            } else if (device_t.lockMode >= SYSTEM_MODE_WAIT_INPUT_ROOT_CARD_NUMBER && device_t.lockMode <= SYSTEM_MODE_WAIT_DELETE_CARD_NUMBER) {
                device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_USER_CARD;
                /* 语音 */
                // 卡用户
                // 1 录入管理信息
                // 2 录入用户信息
                // 3 删除用户信息
                Sound_Insert_Number(1, VOICE_CARD_USER);
                Sound_Insert_Number(1, VOICE_NUM_1);
                Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
                Sound_Insert_Number(1, VOICE_NUM_2);
                Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
                Sound_Insert_Number(1, VOICE_NUM_3);
                Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
            } else if (device_t.lockMode >= SYSTEM_MODE_WAIT_INPUT_ROOT_FINGER_NUMBER && device_t.lockMode <= SYSTEM_MODE_WAIT_DELETE_FINGER_NUMBER) {
                device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_USER_FINGER;
                /* 语音 */
                // 指纹用户
                // 1 录入管理信息
                // 2 录入用户信息
                // 3 删除用户信息
                Sound_Insert_Number(1, VOICE_FINGERPRINT_USER);
                Sound_Insert_Number(1, VOICE_NUM_1);
                Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
                Sound_Insert_Number(1, VOICE_NUM_2);
                Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
                Sound_Insert_Number(1, VOICE_NUM_3);
                Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
            } else if (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_PASSWORD && device_t.lockMode <= SYSTEM_MODE_DELETE_FINGER) {
                device_t.lockMode -= 9;
                /* 语音 */
                // 请输入编号
                Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            } else if (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_PASSWORD_2 && device_t.lockMode <= SYSTEM_MODE_INPUT_USER_FINGER_2) {
                device_t.lockMode -= 16;
                /* 语音 */
                // 请输入编号
                Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
#if FUNC_FINGERPRINT_ENABLE
                cancel_cmd();
#endif
            } else if (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_FINGER_3 && device_t.lockMode <= SYSTEM_MODE_INPUT_USER_FINGER_3) {
                device_t.lockMode -= 18;
                /* 语音 */
                // 请输入编号
                Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
#if FUNC_FINGERPRINT_ENABLE
                cancel_cmd();
#endif
            } else if (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_FINGER_4 && device_t.lockMode <= SYSTEM_MODE_INPUT_USER_FINGER_4) {
                device_t.lockMode -= 20;
                /* 语音 */
                // 请输入编号
                Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
#if FUNC_FINGERPRINT_ENABLE
                cancel_cmd();
#endif
            } else if (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_FINGER_5 && device_t.lockMode <= SYSTEM_MODE_INPUT_USER_FINGER_5) {
                device_t.lockMode -= 22;
                /* 语音 */
                // 请输入编号
                Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
#if FUNC_FINGERPRINT_ENABLE
                cancel_cmd();
#endif
            }
            DEBUG_TRACE(LOG_TAG, "Exec * For Back, Lock Mode %d", device_t.lockMode);
        }
        memset(&s_touch, 0, sizeof(s_touch));
        return true;
    }
    /* 等待选择模式 */
    else if (device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_MODE) {
        if (s_touch.password[0] == 0x31) {                             // 1 密码用户
            device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_USER_PASSWORD; // 选择密码方式
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Password Way, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 密码用户
            // 1 录入管理信息
            // 2 录入用户信息
            // 3 删除用户信息
            Sound_Insert_Number(1, VOICE_PASSWORD_USER);
            Sound_Insert_Number(1, VOICE_NUM_1);
            Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
            Sound_Insert_Number(1, VOICE_NUM_2);
            Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
            Sound_Insert_Number(1, VOICE_NUM_3);
            Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
        } else if (s_touch.password[0] == 0x32) {                  // 2 卡用户
            device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_USER_CARD; // 选择卡方式
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Card Way, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 卡用户
            // 1 录入管理信息
            // 2 录入用户信息
            // 3 删除用户信息
            Sound_Insert_Number(1, VOICE_CARD_USER);
            Sound_Insert_Number(1, VOICE_NUM_1);
            Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
            Sound_Insert_Number(1, VOICE_NUM_2);
            Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
            Sound_Insert_Number(1, VOICE_NUM_3);
            Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
        } else if (s_touch.password[0] == 0x33) {                    // 3 指纹用户
            device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_USER_FINGER; // 选择指纹方式
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Finger Way, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 指纹用户
            // 1 录入管理信息
            // 2 录入用户信息
            // 3 删除用户信息
            Sound_Insert_Number(1, VOICE_FINGERPRINT_USER);
            Sound_Insert_Number(1, VOICE_NUM_1);
            Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
            Sound_Insert_Number(1, VOICE_NUM_2);
            Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
            Sound_Insert_Number(1, VOICE_NUM_3);
            Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
        } else if (s_touch.password[0] == 0x34) {                     // 4 系统设置
            device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_SYSTEMCONFIG; //
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose System Config, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 系统设置
            // 1 音量加
            // 2 音量减
            // 3 联网开启
            // 4 联网关闭
            // 9 系统初始化
            Sound_Insert_Number(1, VOICE_SYSTEM_CONFIG);
            Sound_Insert_Number(1, VOICE_NUM_1);
            Sound_Insert_Number(1, VOICE_VOLUME_ADD);
            Sound_Insert_Number(1, VOICE_NUM_2);
            Sound_Insert_Number(1, VOICE_VOLUME_LESS);
#if FUNC_CONFIG_NETWORK_ENABLE
            Sound_Insert_Number(1, VOICE_NUM_3);
            Sound_Insert_Number(1, VOICE_NETWORK_ON);
            Sound_Insert_Number(1, VOICE_NUM_4);
            Sound_Insert_Number(1, VOICE_NETWORK_OFF);
#endif
            // Sound_Insert_Number(1, VOICE_NUM_9);
            // Sound_Insert_Number(1, VOICE_SYSTEM_INIT);
        } else {
            DEBUG_TRACE(WARN_TAG, "Exec Fail, Choose Way Not Standard, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 操作失败
            // 1 密码用户
            // 2 卡用户
            // 3 指纹用户
            // 4 系统设置
            Sound_Insert_Number(1, VOICE_EXCE_FAIL);
            Sound_Insert_Number(1, VOICE_NUM_1);
            Sound_Insert_Number(1, VOICE_PASSWORD_USER);
            Sound_Insert_Number(1, VOICE_NUM_2);
            Sound_Insert_Number(1, VOICE_CARD_USER);
            Sound_Insert_Number(1, VOICE_NUM_3);
            Sound_Insert_Number(1, VOICE_FINGERPRINT_USER);
            Sound_Insert_Number(1, VOICE_NUM_4);
            Sound_Insert_Number(1, VOICE_SYSTEM_CONFIG);
        }
        memset(&s_touch, 0, sizeof(s_touch));
        return true;
    }
    /* 等待选择密码录入方式 */
    else if (device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_USER_PASSWORD) {
        if (s_touch.password[0] == 0x31) { // 录入管理信息
            device_t.lockMode = SYSTEM_MODE_WAIT_INPUT_ROOT_PASSWORD_NUMBER;
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Password Input Root Message, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 请输入编号
            // 以#号键结束
            Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
            Sound_Insert_Number(1, VOICE_AUTO_NUMBER);
        } else if (s_touch.password[0] == 0x32) { // 录入用户信息
            device_t.lockMode = SYSTEM_MODE_WAIT_INPUT_USER_PASSWORD_NUMBER;
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Password Input User Message, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 请输入编号
            // 以#号键结束
            Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
            Sound_Insert_Number(1, VOICE_AUTO_NUMBER);
        } else if (s_touch.password[0] == 0x33) { // 删除用户信息
            device_t.lockMode = SYSTEM_MODE_WAIT_DELETE_PASSWORD_NUMBER;
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Password Delete Action, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 请输入编号
            // 以#号键结束
            Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
        } else {
            DEBUG_TRACE(WARN_TAG, "Exec Fail, Choose Password Way Not Standard, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 操作失败
            // 1 录入管理信息
            // 2 录入用户信息
            // 3 删除用户信息
            Sound_Insert_Number(1, VOICE_EXCE_FAIL);
            Sound_Insert_Number(1, VOICE_NUM_1);
            Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
            Sound_Insert_Number(1, VOICE_NUM_2);
            Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
            Sound_Insert_Number(1, VOICE_NUM_3);
            Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
        }
        memset(&s_touch, 0, sizeof(s_touch));
        return true;
    }
    /* 等待选择卡录入方式 */
    else if (device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_USER_CARD) {
        if (s_touch.password[0] == 0x31) { // 录入管理信息
            device_t.lockMode = SYSTEM_MODE_WAIT_INPUT_ROOT_CARD_NUMBER;
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Card Input Root Message, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 请输入编号
            // 以#号键结束
            Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
            Sound_Insert_Number(1, VOICE_AUTO_NUMBER);
        } else if (s_touch.password[0] == 0x32) { // 录入用户信息
            device_t.lockMode = SYSTEM_MODE_WAIT_INPUT_USER_CARD_NUMBER;
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Card Input User Message, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 请输入编号
            // 以#号键结束
            Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
            Sound_Insert_Number(1, VOICE_AUTO_NUMBER);
        } else if (s_touch.password[0] == 0x33) { // 删除用户信息
            device_t.lockMode = SYSTEM_MODE_WAIT_DELETE_CARD_NUMBER;
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Card Delete Action, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 请输入编号
            // 以#号键结束
            Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
        } else {
            DEBUG_TRACE(WARN_TAG, "Exec Fail, Choose Card Way Not Standard, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 操作失败
            // 1 录入管理信息
            // 2 录入用户信息
            // 3 删除用户信息
            Sound_Insert_Number(1, VOICE_EXCE_FAIL);
            Sound_Insert_Number(1, VOICE_NUM_1);
            Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
            Sound_Insert_Number(1, VOICE_NUM_2);
            Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
            Sound_Insert_Number(1, VOICE_NUM_3);
            Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
        }
        memset(&s_touch, 0, sizeof(s_touch));
        return true;
    }
    /* 等待选择指纹录入方式 */
    else if (device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_USER_FINGER) {
        if (s_touch.password[0] == 0x31) { // 录入管理信息
            device_t.lockMode = SYSTEM_MODE_WAIT_INPUT_ROOT_FINGER_NUMBER;
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Finger Input Root Message, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 请输入编号
            // 以#号键结束
            Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
            Sound_Insert_Number(1, VOICE_AUTO_NUMBER);
        } else if (s_touch.password[0] == 0x32) { // 录入用户信息
            device_t.lockMode = SYSTEM_MODE_WAIT_INPUT_USER_FINGER_NUMBER;
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Finger Input User Message, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 请输入编号
            // 以#号键结束
            Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
            Sound_Insert_Number(1, VOICE_AUTO_NUMBER);
        } else if (s_touch.password[0] == 0x33) { // 删除用户信息
            device_t.lockMode = SYSTEM_MODE_WAIT_DELETE_FINGER_NUMBER;
            DEBUG_TRACE(LOG_TAG, "Exec Success, Choose Finger Delete Action, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 请输入编号
            // 以#号键结束
            Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
            Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
        } else {
            DEBUG_TRACE(WARN_TAG, "Exec Fail, Choose Finger Way Not Standard, Lock Mode %d", device_t.lockMode);
            /* 语音 */
            // 操作失败
            // 1 录入管理信息
            // 2 录入用户信息
            // 3 删除用户信息
            Sound_Insert_Number(1, VOICE_EXCE_FAIL);
            Sound_Insert_Number(1, VOICE_NUM_1);
            Sound_Insert_Number(1, VOICE_INPUT_ROOT_INFO);
            Sound_Insert_Number(1, VOICE_NUM_2);
            Sound_Insert_Number(1, VOICE_INPUT_USER_INFO);
            Sound_Insert_Number(1, VOICE_NUM_3);
            Sound_Insert_Number(1, VOICE_DELETE_USER_INFO);
        }
        memset(&s_touch, 0, sizeof(s_touch));
        return true;
    }
    /* 系统设置模式 */
    else if (device_t.lockMode == SYSTEM_MODE_WAIT_CHOOSE_SYSTEMCONFIG) {
        // 1 音量加
        if (s_touch.password[0] == 0x31) {
            if (sound_t.volume_number < VOICE_VOLUME8)
                sound_t.volume_number++;

            /* 语音 */
            // 音量加
            Sound_Insert_Number(0, sound_t.volume_number);
            Sound_Insert_Number(1, VOICE_VOLUME_ADD);
            FLASH_Write_Device_Param();
            DEBUG_TRACE(LOG_TAG, "Volume[%d], Lock Mode %d", sound_t.volume_number, device_t.lockMode);
        }
        // 2 音量减
        else if (s_touch.password[0] == 0x32) {
            if (sound_t.volume_number > VOICE_VOLUME2)
                sound_t.volume_number--;

            /* 语音 */
            // 音量减
            Sound_Insert_Number(0, sound_t.volume_number);
            Sound_Insert_Number(1, VOICE_VOLUME_LESS);
            FLASH_Write_Device_Param();
            DEBUG_TRACE(LOG_TAG, "Volume[%d], Lock Mode %d", sound_t.volume_number, device_t.lockMode);
        }
#if FUNC_CONFIG_NETWORK_ENABLE
        else if (s_touch.password[0] == 0x33) {
            /* 语音 */
            // 联网开启
            Sound_Insert_Number(0, VOICE_NETWORK_ON);
            FLASH_Write_Device_Param();
            DEBUG_TRACE(LOG_TAG, "network on");
            Delay_Ms(2000);
            NVIC_SystemReset(); // 复位
        } else if (s_touch.password[0] == 0x34) {
            /* 语音 */
            // 联网关闭
            Sound_Insert_Number(0, VOICE_NETWORK_OFF);
            FLASH_Write_Device_Param();
            DEBUG_TRACE(LOG_TAG, "network off");
            Delay_Ms(2000);
            NVIC_SystemReset(); // 复位
        }
#endif
        // 9 恢复出厂设置
        // else if (s_touch.password[0] == 0x39) {
        //     device_t.lockMode = SYSTEM_MODE_SYSTEM_FACTORY;
        //     /* 语音 */
        //     // 系统初始化
        //     Sound_Insert_Number(1, VOICE_INPUT_SECOND);
        // }
        memset(&s_touch, 0, sizeof(s_touch));
        return true;
    }
    // /* 恢复出厂设置 */
    // else if (device_t.lockMode == SYSTEM_MODE_SYSTEM_FACTORY) {
    //     if (s_touch.password[s_touch.cnt - 1] == 0x23) { // 按#号键确认
    //         // 恢复初始化状态
    //         device_t.lockMode = 0;
    //         device_t.operateTimeout = 0;
    //         /* 语音 */
    //         // 系统初始化
    //         Sound_Insert_Number(1, VOICE_SYSTEM_INIT);

    //         reset_coded_lock();
    //         reset_card_binding();
    //         reset_fingerprint();
    //         coded_lock_init();

    //         device_t.factoryFlag = 1;
    //         FLASH_Write_Factory_Flag();
    //         FLASH_Reset_Param();

    //         GPIO_EXTI_Close(FINGER_DETECT_PORT, FINGER_DETECT_PIN);
    //         tx_thread_sleep(200);
    //         // GPIO_EXTI_Init(FINGER_DETECT_PORT, FINGER_DETECT_PIN, EXTI_RISING, ENABLE);
    //         // W25X40_ChipErase();

    //         // 写入操作记录
    //         struct lock_log exc_lock_info_t;
    //         memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
    //         exc_lock_info_t.u_exec.isLocal = 1;
    //         exc_lock_info_t.u_exec.factory = 1;
    //         exc_lock_info_t.type = 4;
    //         exc_lock_info_t.stamp = Timestamp;
    //         nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash

    //         memset(&s_touch, 0, sizeof(s_touch));
    //         return true;
    //     }
    // }
    /* 判断触摸按键操作结束 */
    else if (s_touch.password[s_touch.cnt - 1] == 0x23 || s_touch.cnt >= 20) {
        /* 等待录入编号 */
        if (device_t.lockMode >= SYSTEM_MODE_WAIT_INPUT_ROOT_PASSWORD_NUMBER && device_t.lockMode <= SYSTEM_MODE_WAIT_DELETE_FINGER_NUMBER) {
            /* 判断输入编号长度 */
            if (s_touch.cnt == 4) {
                device_t.inputNum = (s_touch.password[0] - 0x30) * 100 + (s_touch.password[1] - 0x30) * 10 + (s_touch.password[2] - 0x30);
            __input:
                if (device_t.inputNum == 0xFF) {
                    DEBUG_TRACE(WARN_TAG, "Exec Fail, Password Total Number More Than 99, Lock Mode %d", device_t.lockMode);
                    /* 语音 */
                    // 存量已满
                    tx_thread_sleep(100);
                    Sound_Insert_Number(1, VOICE_FULL_STOCK);
                } else {
                    /* 语音 */
                    // 编号 0
                    // 编号 0
                    // 编号 1
                    tx_thread_sleep(100);
                    Sound_Insert_Number(1, VOICE_NUM_0 + (device_t.inputNum / 100));
                    Sound_Insert_Number(1, VOICE_NUM_0 + (device_t.inputNum / 10 % 10));
                    Sound_Insert_Number(1, VOICE_NUM_0 + (device_t.inputNum % 10));
                }

                DEBUG_TRACE(LOG_TAG, "Lock Mode Input Number: %d", device_t.inputNum);
                /* 管理员操作管理员用户 */
                if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_ROOT_PASSWORD_NUMBER ||
                    device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_ROOT_CARD_NUMBER ||
                    device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_ROOT_FINGER_NUMBER) {
                    if (device_t.inputNum < 10) {
                        if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_ROOT_PASSWORD_NUMBER) // 录入管理员密码
                        {
                            if (coded_lock_t.pass[(u8)device_t.inputNum].isable == 0) {
                                device_t.lockMode = SYSTEM_MODE_INPUT_ROOT_PASSWORD;
                                memset(pass, 0, sizeof(pass));
                                DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Input Root Password[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 请输入新密码
                                Sound_Insert_Number(1, VOICE_INPUT_NEW_PASSWORD);
                                Sound_Insert_Number(1, VOICE_END_WITH_HASH_KEY);
                            } else {
                                DEBUG_TRACE(WARN_TAG, "Exec Fail, Password Root Number Already Exist[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 重复用户
                                Sound_Insert_Number(1, VOICE_REPEAT_USER);
                            }
                        } else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_ROOT_CARD_NUMBER) {
                            if (coded_lock_t.card[(u8)device_t.inputNum].isable == 0) {
                                device_t.lockMode = SYSTEM_MODE_INPUT_ROOT_CARD;
                                DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Input Root Card[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 请刷卡
                                Sound_Insert_Number(1, VOICE_INPUT_CARD);
                            } else {
                                DEBUG_TRACE(WARN_TAG, "Exec Fail, Card Root Number Already Exist[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 重复用户
                                Sound_Insert_Number(1, VOICE_REPEAT_USER);
                            }
                        } else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_ROOT_FINGER_NUMBER) {
                            if (coded_lock_t.finger[(u8)device_t.inputNum].isable == 0) {
                                device_t.lockMode = SYSTEM_MODE_INPUT_ROOT_FINGER;
                                DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Input Root Finger[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 请按手指
                                Sound_Insert_Number(1, VOICE_INPUT_FINGER);
                            } else {
                                DEBUG_TRACE(WARN_TAG, "Exec Fail, Finger Root Number Already Exist[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 重复用户
                                Sound_Insert_Number(1, VOICE_REPEAT_USER);
                            }
                        }
                    } else {
                        DEBUG_TRACE(WARN_TAG, "Exec Fail, Input Root Number More Than Max Number[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                        /* 语音 */
                        // 操作失败
                        // 请输入编号
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                        Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
                    }
                }
                /* 管理员操作普通用户 */
                else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_USER_PASSWORD_NUMBER ||
                         device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_USER_CARD_NUMBER ||
                         device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_USER_FINGER_NUMBER) {
                    if (device_t.inputNum >= 10 && device_t.inputNum < 100) {
                        if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_USER_PASSWORD_NUMBER) { // 录入用户密码
                            if (coded_lock_t.pass[(u8)device_t.inputNum].isable == 0) {
                                device_t.lockMode = SYSTEM_MODE_INPUT_USER_PASSWORD;
                                memset(pass, 0, sizeof(pass));
                                memset(&s_touch, 0, sizeof(s_touch));
                                DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Input User Password[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 请输入新密码
                                Sound_Insert_Number(1, VOICE_INPUT_NEW_PASSWORD);
                            }
                            // else if (coded_lock_t.total_num_pass == 99)
                            // {
                            //     DEBUG_TRACE(WARN_TAG, "Exec Fail, Password Total Number More Than 99, Lock Mode %d", device_t.lockMode);
                            //     /* 语音 */
                            //     // 存量已满
                            // }
                            else {
                                DEBUG_TRACE(WARN_TAG, "Exec Fail, User Number Already Exist[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 重复用户
                                Sound_Insert_Number(1, VOICE_REPEAT_USER);
                            }
                        } else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_USER_CARD_NUMBER) {
                            if (coded_lock_t.card[(u8)device_t.inputNum].isable == 0) {
                                device_t.lockMode = SYSTEM_MODE_INPUT_USER_CARD;
                                DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Input User Card[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 请刷卡
                                Sound_Insert_Number(1, VOICE_INPUT_CARD);
                            } else {
                                DEBUG_TRACE(WARN_TAG, "Exec Fail, Card User Number Already Exist[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 重复用户
                                Sound_Insert_Number(1, VOICE_REPEAT_USER);
                            }
                        } else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_USER_FINGER_NUMBER) {
                            if (coded_lock_t.finger[(u8)device_t.inputNum].isable == 0) {
                                device_t.lockMode = SYSTEM_MODE_INPUT_USER_FINGER;
                                DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Input User Finger[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 请按手指
                                Sound_Insert_Number(1, VOICE_INPUT_FINGER);
                            } else {
                                DEBUG_TRACE(WARN_TAG, "Exec Fail, Finger User Number Already Exist[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 重复用户
                                Sound_Insert_Number(1, VOICE_REPEAT_USER);
                            }
                        }
                    } else {
                        DEBUG_TRACE(WARN_TAG, "Exec Fail, Input User Number More Than Max Number[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                        /* 语音 */
                        // 操作失败
                        // 请输入编号
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                        Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
                    }
                }
                /* 删除用户 */
                else {
                    // 不能删除自己
                    if (device_t.inputNum < 100) {
                        if (device_t.lockMode == SYSTEM_MODE_WAIT_DELETE_PASSWORD_NUMBER && (device_t.inputNum != device_t.enterRootNum || device_t.enterRootType != 0)) {
                            if (coded_lock_t.total_num_pass && coded_lock_t.pass[(u8)device_t.inputNum].isable) {
                                coded_lock_t.total_num_pass--;
                                coded_lock_t.pass[(u8)device_t.inputNum].isable = 0;
                                memset(coded_lock_t.pass[(u8)device_t.inputNum].word, 0, 10);
                                del_coded_lock((u8)device_t.inputNum, 2);

                                DEBUG_TRACE(LOG_TAG, "Exec Success, Delete Password[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 删除成功
                                Sound_Insert_Number(1, VOICE_DELETE_SUCCESS);
                                device_t.lockMode = 0;
                                device_t.operateTimeout = 0;
                            } else {
                                DEBUG_TRACE(WARN_TAG, "Exec Fail, Delete Password[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 操作失败
                                // 请输入编号
                                Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                                Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
                            }
                            // 写入操作记录
                            struct lock_log exc_lock_info_t;
                            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                            exc_lock_info_t.u_exec.isLocal = 1;
                            exc_lock_info_t.u_exec.deleteAction = 1;
                            exc_lock_info_t.type = 0;
                            exc_lock_info_t.number = device_t.inputNum;
                            strcat(exc_lock_info_t.word, coded_lock_t.pass[i].word);
                            exc_lock_info_t.stamp = Timestamp;
                            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                        } else if (device_t.lockMode == SYSTEM_MODE_WAIT_DELETE_CARD_NUMBER && (device_t.inputNum != device_t.enterRootNum || device_t.enterRootType != 1)) {
                            if (coded_lock_t.total_num_card && coded_lock_t.card[(u8)device_t.inputNum].isable) {
                                coded_lock_t.total_num_card--;
                                coded_lock_t.card[(u8)device_t.inputNum].isable = 0;
                                memset(coded_lock_t.card[(u8)device_t.inputNum].word, 0, 10);
                                del_card_binding((u8)device_t.inputNum, 2);

                                DEBUG_TRACE(LOG_TAG, "Exec Success, Delete Card[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 删除成功
                                Sound_Insert_Number(1, VOICE_DELETE_SUCCESS);
                                device_t.lockMode = 0;
                                device_t.operateTimeout = 0;
                            } else {
                                DEBUG_TRACE(WARN_TAG, "Exec Fail, Delete Card[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 操作失败
                                // 请输入编号
                                Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                                Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
                            }
                            // 写入操作记录
                            struct lock_log exc_lock_info_t;
                            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                            exc_lock_info_t.u_exec.isLocal = 1;
                            exc_lock_info_t.u_exec.deleteAction = 1;
                            exc_lock_info_t.type = 2;
                            exc_lock_info_t.number = device_t.inputNum;
                            strcat(exc_lock_info_t.word, coded_lock_t.pass[i].word);
                            exc_lock_info_t.stamp = Timestamp;
                            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                        } else if (device_t.lockMode == SYSTEM_MODE_WAIT_DELETE_FINGER_NUMBER && (device_t.inputNum != device_t.enterRootNum || device_t.enterRootType != 2)) {
                            if (coded_lock_t.total_num_finger && coded_lock_t.finger[(u8)device_t.inputNum].isable) {
                                coded_lock_t.total_num_finger--;
                                coded_lock_t.finger[(u8)device_t.inputNum].isable = 0;
                                memset(coded_lock_t.finger[(u8)device_t.inputNum].word, 0, 10);
#if FUNC_FINGERPRINT_ENABLE
                                delete_template(device_t.inputNum, 1);
                                del_fingerprint((u8)device_t.inputNum, 2);
#endif
                                DEBUG_TRACE(LOG_TAG, "Exec Success, Delete finger[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 删除成功
                                Sound_Insert_Number(1, VOICE_DELETE_SUCCESS);
                                device_t.lockMode = 0;
                                device_t.operateTimeout = 0;
                            } else {
                                DEBUG_TRACE(WARN_TAG, "Exec Fail, Delete Finger[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                                /* 语音 */
                                // 操作失败
                                // 请输入编号
                                Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                                Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
                            }
                            // 写入操作记录
                            struct lock_log exc_lock_info_t;
                            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                            exc_lock_info_t.u_exec.isLocal = 1;
                            exc_lock_info_t.u_exec.deleteAction = 1;
                            exc_lock_info_t.type = 1;
                            exc_lock_info_t.number = device_t.inputNum;
                            strcat(exc_lock_info_t.word, coded_lock_t.pass[i].word);
                            exc_lock_info_t.stamp = Timestamp;
                            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                        } else {
                            DEBUG_TRACE(WARN_TAG, "Exec Fail, Input User Number More Than Max Number[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                            /* 语音 */
                            // 操作失败
                            // 请输入编号
                            Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                            Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
                        }
                    } else {
                        DEBUG_TRACE(WARN_TAG, "Exec Fail, Input User Number More Than Max Number[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                        /* 语音 */
                        // 操作失败
                        // 请输入编号
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                        Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
                    }
                    memset(&s_touch, 0, sizeof(s_touch));
                    return true;
                }
            }
            /* 自动分配编号 */
            else if (s_touch.password[0] == 0x23 && s_touch.cnt == 1 &&
                     device_t.lockMode != SYSTEM_MODE_WAIT_DELETE_PASSWORD_NUMBER &&
                     device_t.lockMode != SYSTEM_MODE_WAIT_DELETE_CARD_NUMBER &&
                     device_t.lockMode != SYSTEM_MODE_WAIT_DELETE_FINGER_NUMBER) {
                if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_ROOT_PASSWORD_NUMBER) {
                    device_t.inputNum = distribute_idle_number(0, 1);
                } else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_ROOT_CARD_NUMBER) {
                    device_t.inputNum = distribute_idle_number(1, 1);
                } else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_ROOT_FINGER_NUMBER) {
                    device_t.inputNum = distribute_idle_number(2, 1);
                } else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_USER_PASSWORD_NUMBER) {
                    device_t.inputNum = distribute_idle_number(0, 0);
                } else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_USER_CARD_NUMBER) {
                    device_t.inputNum = distribute_idle_number(1, 0);
                } else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_USER_FINGER_NUMBER) {
                    device_t.inputNum = distribute_idle_number(2, 0);
                }
                memset(&s_touch, 0, sizeof(s_touch));
                goto __input;
            } else {
                DEBUG_TRACE(WARN_TAG, "Exec Fail, Input Number More Than Max Len, Lock Mode %d", device_t.lockMode);
                /* 语音 */
                // 操作失败
                // 请输入编号
                Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                Sound_Insert_Number(1, VOICE_INPUT_NUMBER);
                memset(&s_touch, 0, sizeof(s_touch));
            }
        }
        /* 等待录入密码 */
        else if (device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_PASSWORD || device_t.lockMode == SYSTEM_MODE_INPUT_USER_PASSWORD) {
            if (s_touch.cnt >= 7 && s_touch.cnt <= 9) { // 密码长度6-8
                memset(pass, 0, sizeof(pass));
                memcpy(pass, s_touch.password, s_touch.cnt - 1);
                pass_len = s_touch.cnt - 1;
                device_t.lockMode += 9;
                DEBUG_TRACE(LOG_TAG, "Exec Success, Save Temp Password[%s], Lock Mode %d", pass, device_t.lockMode);
                /* 语音 */
                // 请再次输入
                Sound_Insert_Number(1, VOICE_INPUT_SECOND);
            } else {
                DEBUG_TRACE(WARN_TAG, "Exec Fail, Input Password Not Standard, Lock Mode %d", device_t.lockMode);
                /* 语音 */
                // 操作失败
                // 请输入密码
                Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                Sound_Insert_Number(1, VOICE_INPUT_NEW_PASSWORD);
            }
            memset(&s_touch, 0, sizeof(s_touch));
        }
        /* 等待2次录入密码 */
        else if (device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_PASSWORD_2 || device_t.lockMode == SYSTEM_MODE_INPUT_USER_PASSWORD_2) {
            if (s_touch.cnt >= 7 && s_touch.cnt <= 9) // 密码长度6-8
            {
                if (!strncmp(pass, s_touch.password, s_touch.cnt - 1) && (pass_len == s_touch.cnt - 1)) {
                    set_coded_lock((u8)device_t.inputNum, (char *)pass, 1);
                    // 更新缓存
                    memset(coded_lock_t.pass[(u8)device_t.inputNum].word, 0, sizeof(coded_lock_t.pass[(u8)device_t.inputNum].word));
                    strncpy(coded_lock_t.pass[(u8)device_t.inputNum].word, (char *)pass, s_touch.cnt - 1);
                    coded_lock_t.total_num_pass++;
                    coded_lock_t.pass[(u8)device_t.inputNum].isable = 1;
                    DEBUG_TRACE(LOG_TAG, "Exec Success, Save Password[%s], Lock Mode %d", pass, device_t.lockMode);

                    // 退出系统设置
                    device_t.lockMode = 0;
                    device_t.operateTimeout = 0;
                    /* 语音 */
                    // 操作成功
                    Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);

                    // 写入操作记录
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.inputAction = 1;
                    exc_lock_info_t.type = 0;
                    exc_lock_info_t.number = device_t.inputNum;
                    strncpy(exc_lock_info_t.word, (char *)pass, s_touch.cnt - 1);
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                } else {
                    device_t.lockMode -= 9; // 返回输入第一次密码
                    memset(pass, 0, sizeof(pass));
                    DEBUG_TRACE(WARN_TAG, "Exec Fail, Please Input Password, Back Lock Mode %d", device_t.lockMode);
                    /* 语音 */
                    // 操作失败
                    // 请输入密码
                    Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                    Sound_Insert_Number(1, VOICE_INPUT_NEW_PASSWORD);
                }
            } else {
                device_t.lockMode -= 9; // 返回输入第一次密码
                memset(pass, 0, sizeof(pass));
                DEBUG_TRACE(WARN_TAG, "Exec Fail, Please Input Password, Back Lock Mode %d", device_t.lockMode);
                /* 语音 */
                // 操作失败
                // 请输入密码
                Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                Sound_Insert_Number(1, VOICE_INPUT_NEW_PASSWORD);
            }
        }
        /* 验证管理员密码 */
        else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_PASSWORD) {
            // 无管理员用户时 使用初始密码进入管理员模式
            if (!cb_root_num()) {
                i = 0xFF;
                if (str_cmp(s_touch.password, "123456") == true) {
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
                } else {
                    DEBUG_TRACE(LOG_TAG, "Exec Fail, Password Not Match[%d]: %s, Lock Mode %d", i, s_touch.password, device_t.lockMode);
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
            } else {
                for (i = 0; i < 10; i++) {
                    if (coded_lock_t.pass[i].isable && str_cmp(s_touch.password, coded_lock_t.pass[i].word) == true) {
                        device_t.enterRootNum = i;
                        device_t.enterRootType = 0;
                        DEBUG_TRACE(LOG_TAG, "Password Match[%d]: %s, Lock Mode %d", i, s_touch.password, device_t.lockMode); // 密码匹配成功
                        break;
                    }
                }
                // if (str_cmp(s_touch.password, "10001000") == true) // 特殊自用密码
                // {
                //     i = 0xFF;
                //     goto __choose_mode;
                // }
                if (i >= 10) {
                    DEBUG_TRACE(LOG_TAG, "Exec Fail, Password Not Match[%d]: %s, Lock Mode %d", i, s_touch.password, device_t.lockMode);
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
                } else {
                __choose_mode:
                    device_t.tamperAlarm = 0;
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
                }
            }
            // 写入操作记录
            struct lock_log exc_lock_info_t;
            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
            exc_lock_info_t.u_exec.isLocal = 1;
            exc_lock_info_t.u_exec.enterRoot = 1;
            exc_lock_info_t.type = 0;
            exc_lock_info_t.number = i;
            if (i < 100 || i == 0xFF)
                exc_lock_info_t.u_exec.unlockSuccess = 1;
            else
                exc_lock_info_t.u_exec.unlockFail = 1;
            exc_lock_info_t.stamp = Timestamp;
            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
        }
        memset(&s_touch, 0, sizeof(s_touch));
        return true;
    }

    return false;
}

/**
 * @brief  触摸事件
 * @param  thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_touch(ULONG thread_input)
{
    (void)thread_input;
    extern u8 uart_touch_press;
    ULONG actual_events;

    UINT status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
                                     BIT_TOUCH,        /* 等待标志 */
                                     TX_AND_CLEAR,     /* 等待特定bit满足 */
                                     &actual_events,   /* 获取实际值 */
                                     TX_WAIT_FOREVER); /* 永久等待 */

    ULONG mian_bit;

    u8 userNum, i, action;
    bool ret;

    u8 buffer[50];
    u8 sendLen;

    while (1) {
    __back:
        tx_thread_sleep(50);
        if (device_t.sleepState) {
            goto __back;
        }

        if (device_t.isLedShow)
            MBI5120_Display(0xFFFF);

        // INFO("\r\n>>>>>>>>>>>>>>>  si12t  start <<<<<<<<<<<<<<<<<<<\r\n");
        si12t_clear_read(); // 清除读取状态
        GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, ENABLE);
        UINT status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
                                         BIT_TOUCH,        /* 等待标志 */
                                         TX_AND_CLEAR,     /* 等待特定bit满足 */
                                         &actual_events,   /* 获取实际值 */
                                         TX_WAIT_FOREVER); /* 永久等待 */

        tx_event_flags_get(&main_event_group, MAIN_BIT_DISPLAY_ALL, TX_OR, &mian_bit, TX_NO_WAIT);
        if (mian_bit & MAIN_BIT_NO_TOUCH) {
            goto __back;
        }

        // INFO("\r\n>>>>>>>>>>>>>>>  si12t  end <<<<<<<<<<<<<<<<<<<\r\n");
        if (uart_touch_press != 0xFF) { // 串口模拟按键输入
            s_touch.password[s_touch.cnt++] = uart_touch_press;
            uart_touch_press = 0xFF;
        } else {
            bool ret = si12t_output_read();
            if (ret == false)
                goto __back;
        }

        INFO("\r\n>>>>>>>>>>>>>>>touch data[%d]: %s<<<<<<<<<<<<<<<<<<<\r\n", s_touch.cnt, s_touch.password);

        if (device_t.lockTimeout) { // 锁定状态
            DEBUG_TRACE(WARN_TAG, "The Lock is Locked, %s", __func__);
            memset(&s_touch, 0, sizeof(s_touch));
            /* 语音 */
            // 失败提示音
            // 系统已锁定
            Sound_Insert_Number(0, VOICE_TONE_FAIL);
            Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);

            // MBI5120_Display(0);
            tx_thread_sleep(2000); // 失败后强制延时播放语音
            // MBI5120_Display(0xFFFF);
            goto __back;
        }

        if (device_t.lockDoorTimeout && s_touch.password[s_touch.cnt - 1] == 0x2A) {
            device_t.lockDoorTimeout = 0; // 立马上锁
            memset(&s_touch, 0, sizeof(s_touch));
            DEBUG_TRACE(LOG_TAG, "Use * For Lock");
            tx_thread_sleep(1000);
            goto __back;
        }

        if (s_touch.clear_signal) {
            MBI5120_Display(0xFFFF);
#if FUNC_FINGERPRINT_ENABLE
            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_BLUE, TX_OR);
            // finger_led_control(3, 1, 1, 0); // 蓝色常亮
#endif
            tx_thread_sleep(500);
            memset(&s_touch, 0, sizeof(s_touch));
            goto __back;
        }

        // 在已开锁状态下不允许按除*号外的按键
        if (device_t.lockDoorTimeout && s_touch.password[s_touch.cnt - 1] != 0x2A) {
            memset(&s_touch, 0, sizeof(s_touch));
            goto __back;
        }

        /* 语音 */
        // 键盘触摸提示音
        if (s_touch.password[0] != 0x23 || device_t.lockMode)
            Sound_Insert_Number(0, VOICE_TONE_TOUCH);

        u_mbi5120_t.dat = getLedShowNum(s_touch.password[s_touch.cnt - 1]);
        MBI5120_Display(u_mbi5120_t.dat);
        device_t.isLedShow = 1;
        // mDelay(20);
        // MBI5120_Display(0xFFFF);
        // #if FUNC_FINGERPRINT_ENABLE
        //         // finger_led_control(3, 1, 1, 0); // 蓝色常亮
        // #endif

        /* 模式处理 */
        ret = lock_mode_event();
        if (ret == true)
            goto __back;

        /* 操作 */
        if (s_touch.cnt == 2 && s_touch.password[0] == 0x2A && s_touch.password[1] == 0x2A && !device_t.lockMode) {
            MBI5120_Display(0); // 关闭显示
#if FUNC_FINGERPRINT_ENABLE
            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
            // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
            goto __back;
            // Sound_Insert_Number(0, VOICE_STOP);
        } else if (s_touch.password[s_touch.cnt - 1] == 0x2A && s_touch.cnt >= 2) { // 回删密码
            memset(&s_touch, 0, sizeof(s_touch));
            DEBUG_TRACE(LOG_TAG, "Clear Password: %s", s_touch.password);
            goto __back;
        }

        i = 0;
        memset(buffer, 0, sizeof(buffer));

        /* 判断触摸按键操作结束 */
        if (s_touch.password[s_touch.cnt - 1] == 0x23 || s_touch.cnt >= 20) {
            if (s_touch.cnt == 1) { // 叮咚提示音
#if FUNC_DOORBELL_ENABLE
                /* 语音 */
                Sound_Insert_Number(0, VOICE_TONE_DINGDONG);
#else
                tx_event_flags_set(&main_event_group, MAIN_BIT_NO_TOUCH, TX_OR);
                if (LoRaWAN.joinState) { // 入网成功才可以发起
                    if (device_t.sleepDelay < 15)
                        device_t.sleepDelay = 15;
                    Sound_Insert_Number(1, VOICE_EXECUTING); // 执行中，请稍后
                    MBI5120_Display(0);
#if FUNC_FINGERPRINT_ENABLE
                    tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                    // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                    u8 buf[2] = {0x32, 0x00};
                    u8 waitCnt = 100;
                    union u_mbi5120 t_mbi5120;

                    tx_event_flags_get(&event_group, BIT_SYNC, TX_OR_CLEAR, NULL, TX_NO_WAIT); // 发送之前清除事件
                    if (sendLoRaWANData(buf, 2) == true) {
                        if (device_t.sleepDelay < 15)
                            device_t.sleepDelay = 15;
                        while (--waitCnt) { // 等待密码下行
                            t_mbi5120.dat = Mbi_ShowNum(waitCnt / 10);
                            MBI5120_Display(t_mbi5120.dat);
                            tx_thread_sleep(100);
                            tx_event_flags_get(&event_group, BIT_SYNC, TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
                            if (actual_events & BIT_SYNC) {
                                break;
                            }
                        }
                        if (waitCnt > 0)
                            Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);
                        else
                            Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                    } else {
                        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                    }
                    MBI5120_Display(0xFFFF);
                }
#endif
                memset(&s_touch, 0, sizeof(s_touch));
                goto __back;
            } else if (s_touch.cnt == 2 && s_touch.password[0] == 0x2A) { // * + # 管理操作
                device_t.operateTimeout = 20 * SEC_DELAY;
                device_t.lockMode = SYSTEM_MODE_WAIT_INPUT_PASSWORD;
                device_t.enterRootNum = 0;
                device_t.enterRootType = 0;
                memset(&s_touch, 0, sizeof(s_touch));
                DEBUG_TRACE(LOG_TAG, "Wait Enter System Mode Input Root Password, Lock Mode %d", device_t.lockMode);
                /* 语音 */
                // 请输入管理员信息
                Sound_Insert_Number(1, VOICE_PLS_INPUT_ROOT_INFO);
                Sound_Insert_Number(1, VOICE_HELP_INFO);
                goto __back;
            } else if (s_touch.cnt == 3 && s_touch.password[0] == 0x37 && s_touch.password[1] == 0x37) { // 77 + # 检查时间同步状态
                if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                // 2024-12-20 00:00:00
                if (Timestamp < 1734624000) {
                    MBI5120_Display(0);
                    tx_thread_sleep(500);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(500);
                    MBI5120_Display(0);
                    tx_thread_sleep(500);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(500);
                    MBI5120_Display(0);
                    tx_thread_sleep(500);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(500);
                } else {
                    MBI5120_Display(0);
                    tx_thread_sleep(500);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(500);
                }
                goto __back;
            } else if (s_touch.cnt == 2 && s_touch.password[0] == 0x31) { // 1 + # 主动同步数据
                tx_event_flags_set(&main_event_group, MAIN_BIT_NO_TOUCH, TX_OR);
                if (device_t.sleepDelay < 15)
                    device_t.sleepDelay = 15;
                if (LoRaWAN.joinState) {                     // 入网成功才可以发起
                    Sound_Insert_Number(1, VOICE_EXECUTING); // 执行中，请稍后
                    MBI5120_Display(0);
#if FUNC_FINGERPRINT_ENABLE
                    tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                    // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif

                    u8 buf[2] = {0x32, 0x00};
                    u8 waitCnt = 100;
                    union u_mbi5120 t_mbi5120;

                    tx_event_flags_get(&event_group, BIT_SYNC, TX_OR_CLEAR, NULL, TX_NO_WAIT); // 发送之前清除事件
                    if (sendLoRaWANData(buf, 2) == true) {
                        if (device_t.sleepDelay < 15)
                            device_t.sleepDelay = 15;
                        while (--waitCnt) { // 等待密码下行
                            t_mbi5120.dat = Mbi_ShowNum(waitCnt / 10);
                            MBI5120_Display(t_mbi5120.dat);
                            tx_thread_sleep(100);
                            tx_event_flags_get(&event_group, BIT_SYNC, TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
                            if (actual_events & BIT_SYNC) {
                                break;
                            }
                        }
                        if (waitCnt > 0)
                            Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);
                        else
                            Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                    } else {
                        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                    }
                    MBI5120_Display(0xFFFF);
                } else {
                    reconnect_stamp = Timestamp;
                    LoRaWAN.joinState = 0;
                    LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
                    // Sound_Insert_Number(1, VOICE_EXECUTING);

                    MBI5120_Display(0);
                    tx_thread_sleep(2000);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(1000);
                }
                memset(&s_touch, 0, sizeof(s_touch));
                tx_event_flags_set(&main_event_group, ~(MAIN_BIT_NO_TOUCH), TX_AND);
                goto __back;
            } else if (s_touch.cnt == 2 && s_touch.password[0] == 0x32) { // 2 + # 重启LoRaWAN模组
                tx_event_flags_set(&main_event_group, MAIN_BIT_NO_TOUCH, TX_OR);
                setLoRaWANStatus(LoRaWAN_WAKE_STATUS); // 唤醒模块
                AT_LoRaWAN_setATEnter();
                AT_LoRaWAN_reboot(); // 保存配置参数
                LoRa_DelayMs(3000);

                memset(&s_touch, 0, sizeof(s_touch));
                tx_event_flags_set(&main_event_group, ~(MAIN_BIT_NO_TOUCH), TX_AND);
                goto __back;
            } else if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                userNum = 0xFF;
                if (str_cmp(s_touch.password, "123456") == true) {
                    DEBUG_TRACE(LOG_TAG, "Password Match[%d]: %s", userNum, s_touch.password); // 密码匹配成功
                    action = 0x02;                                                             // 密码开门
                    tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
#if FUNC_FINGERPRINT_ENABLE
                    tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                    // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                    tx_thread_sleep(2000);
                } else if (coded_lock_t.temp_pass.isable && str_cmp(s_touch.password, coded_lock_t.temp_pass.word) == true) // 临时密码解锁
                {
                    DEBUG_TRACE(LOG_TAG, "Password Match[%d]: %s", userNum, s_touch.password); // 密码匹配成功
                    action = 0x02;                                                             // 密码开门
                    tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
#if FUNC_FINGERPRINT_ENABLE
                    tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                    // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                    tx_thread_sleep(2000);
                } else {
                    action = 0xC2; // 密码报警
                    device_t.openFailCnt++;
                    /* 语音 */
                    // 失败提示音
                    Sound_Insert_Number(1, VOICE_TONE_FAIL);
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
                    DEBUG_TRACE(LOG_TAG, "Password Error: %s", s_touch.password);

                    MBI5120_Display(0);
                    tx_thread_sleep(2000); // 失败后强制延时播放语音
                    MBI5120_Display(0xFFFF);
                }
            } else {
                userNum = 0xFF;
                action = 0;
                for (i = 0; i < 100; i++) {
                    if (coded_lock_t.pass[i].isable) {
                        if (str_cmp(s_touch.password, coded_lock_t.pass[i].word) == true) {
                            userNum = i;
                            action = 0x02; // 密码开门
                            tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
#if FUNC_FINGERPRINT_ENABLE
                            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                            // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                            DEBUG_TRACE(LOG_TAG, "Password Unlock[%d]: %s", userNum, s_touch.password); // 密码匹配成功
                            tx_thread_sleep(2000);
                            break;
                        }
                    }
                }
                if (i >= 100) {
                    //                     if (str_cmp(s_touch.password, "10001000") == true) // 特殊自用密码
                    //                     {
                    //                         DEBUG_TRACE(LOG_TAG, "Password Match[%d]: %s", userNum, s_touch.password); // 密码匹配成功
                    //                         action = 0x02;                                                             // 密码开门
                    //                         tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
                    // #if FUNC_FINGERPRINT_ENABLE
                    //                         tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                    //                         // finger_led_control(3, 0, 0, 0); // 关闭灯光
                    // #endif
                    //                     }
                    if (coded_lock_t.temp_pass.isable && str_cmp(s_touch.password, coded_lock_t.temp_pass.word) == true) // 临时密码解锁
                    {
                        DEBUG_TRACE(LOG_TAG, "Password Match[%d]: %s", userNum, s_touch.password); // 密码匹配成功
                        action = 0x02;                                                             // 密码开门
                        tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
#if FUNC_FINGERPRINT_ENABLE
                        tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                        // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                        tx_thread_sleep(2000);
                    } else if (i >= 100 && action != 0x02) {
                        action = 0xC2; // 密码报警
                        device_t.openFailCnt++;
                        /* 语音 */
                        // 失败提示音
                        Sound_Insert_Number(1, VOICE_TONE_FAIL);
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
                        DEBUG_TRACE(LOG_TAG, "Password Error: %s", s_touch.password);

                        MBI5120_Display(0);
                        tx_thread_sleep(2000); // 失败后强制延时播放语音
                        MBI5120_Display(0xFFFF);
                    }
                }
            }
            memset(&s_touch, 0, sizeof(s_touch));
            sendLen = 7;

            buffer[0] = 0x30;
            buffer[1] = action;  // 操作记录
            buffer[2] = userNum; // 用户编号
            buffer[3] = (Timestamp >> 24) & 0xFF;
            buffer[4] = (Timestamp >> 16) & 0xFF;
            buffer[5] = (Timestamp >> 8) & 0xFF;
            buffer[6] = Timestamp & 0xFF;

            int free_number = lwrb_get_free(&lwrbBuff_t);
            if (free_number >= (sendLen + 1)) {       // 检查缓冲区是否已满
                lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                lwrb_write(&lwrbBuff_t, buffer, sendLen);
                lwrb_write_count++;
            } else {
                DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            }

            if (action == 0xC2) { // 写入开锁失败记录
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.unlockFail = 1;
                exc_lock_info_t.type = 0;
                exc_lock_info_t.number = userNum;
                if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                    strcat(exc_lock_info_t.word, "123456");
                } else {
                    strcat(exc_lock_info_t.word, coded_lock_t.pass[i].word);
                }
                exc_lock_info_t.stamp = Timestamp;
                nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            } else if (action == 0x02) {                  // 写入开锁成功记录
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.unlockSuccess = 1;
                exc_lock_info_t.type = 0;
                exc_lock_info_t.number = userNum;
                if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                    strcat(exc_lock_info_t.word, "123456");
                } else {
                    strcat(exc_lock_info_t.word, coded_lock_t.pass[i].word);
                }
                exc_lock_info_t.stamp = Timestamp;
                nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            }

            /* 输入开锁密码解除防拆报警*/
            if (device_t.tamperAlarm && action == 0x02) {
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
                exc_lock_info_t.type = 0;
                exc_lock_info_t.number = userNum;
                if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                    strcat(exc_lock_info_t.word, "123456");
                } else {
                    strcat(exc_lock_info_t.word, coded_lock_t.pass[i].word);
                }
                exc_lock_info_t.stamp = Timestamp;
                nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            }
        }
    }
}
#endif
