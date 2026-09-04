
#include "systick.h"
#include "gpio.h"
#include "adc.h"
#include "uart.h"
#include "control_center.h"
#include "communicate.h"
#include "LoRaWAN_APPLY.h"

struct device_sensor device_t;

u16 ledGreenBlinkTime;
u16 ledBlueBlinkTime;
u16 ledOrangeBlinkTime;
u16 ledRedBlinkTime;
u32 countDelay = 0; // delay决定两人之间最少间隔（灵敏度）,延时：delay*10ms,不可太大会影响下一次计数

/**
 * @brief  红外计数器事件处理
 * @param
 * @retval
 */
void People_Counter_Event(void)
{
    if (getLoRaWANStatus() == LoRaWAN_WAKE_STATUS)
    {
        if (!uartLoRa.sendDelay && (LoRaWAN.joinState == 1 || (LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE)))
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    }
    if ((device_t.reportMode & PERIOD_REPORT_MODE) == 1 && device_t.mode == COUNTING_MODE) // 计数模式下才开始上报，验证和对齐模式不上报
    {
        if ((device_t.reportIRInterval * 60) <= device_t.counter_run_time)
        {
            device_t.counter_run_time = 0;
            device_t.updateCounter = 1;
        }
    }
}
#if 0
void People_Counter_Event(void)
{
    static u32 start_run_time = 5 * SEC_DELAY; // 等待红外计数器稳定
    static u8 unusual_a_count, unusual_b_count;
    static u8 fault_cnt;
    u32 end_run_time;
    if (start_run_time > get_syspant_ms())
        return;
    // start_run_time = get_syspant_ms() + 2 * 10; //红外发射周期2ms 10个周期判断一次
    if (device_t.counter_a_sense_count < 4 && device_t.counter_b_sense_count > device_t.counter_a_sense_count)
    {//屏蔽脉冲不正常的误计数
        DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense A Unusual, Count A: %d, Count B: %d", device_t.counter_a_sense_count, device_t.counter_b_sense_count);
        if (unusual_b_count == 0)
        {
            unusual_a_count++;
            if (unusual_a_count >= 1 && fault_cnt < 3)
            {
                unusual_a_count = 0;
                device_t.counter_b_value++;
                device_t.counter_b_count++;
                device_t.direction = PASSAGE_DIRECTION_A_TO_B;
                DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense Direction: A to B\n");
                ledGreenBlinkTime = 200;
                ledBlueBlinkTime = 200;//蓝+绿=青
                goto sense;
            }
            goto foot;
        }
        else
            unusual_b_count = 0;
    }
    else if (device_t.counter_b_sense_count < 4 && device_t.counter_a_sense_count > device_t.counter_b_sense_count)
    {//屏蔽脉冲不正常脉冲的误计数
        DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense B Unusual, Count A: %d, Count B: %d", device_t.counter_a_sense_count, device_t.counter_b_sense_count);
        if (unusual_a_count == 0)
        {
            unusual_b_count++;
            if (unusual_b_count >= 1 && fault_cnt < 3)
            {
                unusual_b_count = 0;
                device_t.counter_a_value++;
                device_t.counter_a_count++;
                device_t.direction = PASSAGE_DIRECTION_B_TO_A;
                DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense Direction: B to A\n");
                ledBlueBlinkTime = 200;
                ledRedBlinkTime = 200;//红+蓝=紫色
                goto sense;
            }
            goto foot;
        }
        else
            unusual_a_count = 0;
    }
foot:
    // DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense Count A: %d, Count B: %d\n", device_t.counter_a_sense_count, device_t.counter_b_sense_count);
    device_t.counter_a_sense_count = 0; // 清除触发记录
    device_t.counter_b_sense_count = 0;
    fault_cnt = 0;

    if (getLoRaWANStatus() == LoRaWAN_WAKE_STATUS)
    {
        if (!uartLoRa.sendDelay && (LoRaWAN.joinState == 1 || (LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE)))
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    }

    start_run_time = get_syspant_ms() + 2 * 5;

    if ((device_t.reportMode & PERIOD_REPORT_MODE) == 1)
    {
        if ((device_t.reportIRInterval * 60) <= device_t.counter_run_time)
        {
            device_t.counter_run_time = 0;
            device_t.updateCounter = 1;
        }
    }

    // if (device_t.reportMode >= PERIOD_REPORT_MODE) //周期上报
    // {
    //     if ((device_t.reportIRInterval * 60) <= device_t.counter_run_time)
    //     {
    //         device_t.counter_run_time = 0;
    //         device_t.updateCounter = 1;
    //     }
    // }
    // else if (device_t.reportMode >= (REPORT_COUNT_MORETHAN_SETTING + PERIOD_REPORT_MODE)) //周期上报+累计人数上报
    // {
    //     if ((device_t.counter_a_value + device_t.counter_b_value) >= device_t.reportTotalCount || (device_t.reportIRInterval * 60) <= device_t.counter_run_time)
    //     {
    //         device_t.counter_run_time = 0;
    //         device_t.updateCounter = 1;
    //     }
    // }

    return;

sense:
    if (device_t.mode == COUNTING_MODE)
    //    ledGreenBlinkTime = 200;
    //else
    {
        if (!device_t.updateCounter)
        {
            if (device_t.reportMode >= REPORT_COUNT_CHANGED) // 计数值改变
            {
                device_t.updateCounter = 1;
            }
            else if (device_t.reportMode >= REPORT_COUNT_MORETHAN_SETTING) // 累计人数上报
            {
                if ((device_t.counter_a_value + device_t.counter_b_value) >= device_t.reportTotalCount)
                {
                    device_t.updateCounter = 1;
                }
            }
        }
    }
    device_t.counter_a_sense_count = 0; // 清除触发记录
    device_t.counter_b_sense_count = 0;

    // 等待人走开开始下一次计数，或超时
    end_run_time = get_syspant_ms() + 10 * SEC_DELAY;    // 设定最大超时时间
    start_run_time = get_syspant_ms() + 2 * IR_DEC_DUTY; // 50ms

    while (1)
    {
        if (start_run_time >= end_run_time)
        {
            fault_cnt++;
            if (fault_cnt >= 3)
            {
                fault_cnt = 0;
                if (!device_t.faultState) // 更新故障状态标志
                {
                    device_t.updateFault = 1;
                    device_t.faultState = 1;
                    DEBUG_TRACE(IR_TAG, "[DisSense] Infrared Sense Entry Fault");
                }
            }
            start_run_time = get_syspant_ms() + 2 * 5;
            DEBUG_TRACE(IR_TAG, "[DisSense] Infrared Sense Time Out"); // 超时自动退出检测
            break;
        }
        if (device_t.counter_a_sense_count >= IR_DEC_DUTY * 0.9 && device_t.counter_b_sense_count >= IR_DEC_DUTY * 0.9) // 确认人已离开
        {
            mDelay(IR_DEC_DUTY);                                                                                            // 防抖
            if (device_t.counter_a_sense_count >= IR_DEC_DUTY * 0.9 && device_t.counter_b_sense_count >= IR_DEC_DUTY * 0.9) // 确认人已离开
            {
                DEBUG_TRACE(IR_TAG, "[DisSense] Infrared Sense People Leave");
                if (device_t.faultState)
                {
                    device_t.updateFault = 1;
                    device_t.faultState = 0;
                    DEBUG_TRACE(IR_TAG, "[DisSense] Infrared Sense Exti Fault");
                }
                break;
            }
        }
        if (start_run_time > get_syspant_ms())
            continue;
        start_run_time = get_syspant_ms() + 2 * IR_DEC_DUTY; // 50ms
        device_t.counter_a_sense_count = 0;                  // 清除触发记录
        device_t.counter_b_sense_count = 0;
    }
    // DEBUG_TRACE(IR_TAG, "[Count] Infrared Counter A: %d, Counter B: %d\n", device_t.counter_a_value, device_t.counter_b_value);
    DEBUG_TRACE(IR_TAG, "[Count] Infrared Counter A: %d, Total Counter A: %d, Counter B: %d, Total Counter B: %d\n",
                device_t.counter_a_value, device_t.counter_a_count, device_t.counter_b_value, device_t.counter_b_count);
}
#endif

/**
 * @brief  红外计数器处理函数,在定时器1ms中断中调用
 * @param
 * @retval
 */
void People_Counter_Handle(void)
{
    static uint32_t sytm = 0, sumTotal_a = 0, sumTotal_b = 0;
    static uint8_t step1 = 0, step2 = 0; // 1:a-b,2:b-a step1决定哪边触发，step2决定是否走到另一边
    sytm++;

    if (device_t.mode <= ALIGNMENT_MODE) // 对齐模式不计数
    {
        sytm = 0;
    }
    else if (sytm % 10 == 0)
    {
        if (sytm > 29999)
        {
            if (sumTotal_a < 3000 || sumTotal_b < 3000) // 如果脉冲不正常（夜间单边脉冲、无脉冲或脉冲少，人长时间逗留30s以上遮住一半或全部，或发射端掉落）
            {
                if (!device_t.faultState)
                {
                    device_t.faultState = 1;
                    device_t.updateFault = 1; // 超过30s则发送异常掉落上报
                    // DEBUG_TRACE(IR_TAG, "[DisSense] Infrared Sense Entry Fault");
                }
                countDelay = 3000; // 不正常一直不计数
                step1 = 0;
                step2 = 0;
            }
            else if (device_t.faultState)
            {
                device_t.faultState = 0;
                // DEBUG_TRACE(IR_TAG, "[DisSense] Infrared Sense Exti Fault");
            }
            sytm = 0;
            sumTotal_a = 0;
            sumTotal_b = 0;
        }

        if (countDelay > 0) // 降低灵敏度，延时：delay*10ms
        {
            countDelay--;
        }
        else
        {
            if (step1 == 0)
            {
                if (device_t.counter_a_sense_count < 4 && device_t.counter_b_sense_count > device_t.counter_a_sense_count)
                {
                    // DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense A Unusual, Count A: %d, Count B: %d",
                    // device_t.counter_a_sense_count, device_t.counter_b_sense_count);
                    step1 = 1;
                }
                else if (device_t.counter_b_sense_count < 4 && device_t.counter_a_sense_count > device_t.counter_b_sense_count)
                {
                    // DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense B Unusual, Count A: %d, Count B: %d",
                    // device_t.counter_a_sense_count, device_t.counter_b_sense_count);
                    step1 = 2;
                }
            }
            else if (device_t.counter_b_sense_count < 4 && device_t.counter_a_sense_count > device_t.counter_b_sense_count && step1 == 1)
            {
                /*if (step2 == 0) // 只打印一次调试信息
                {
                    DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense Direction: A to B\n");
                }*/
                step2 = step1;
            }
            else if (device_t.counter_a_sense_count < 4 && device_t.counter_b_sense_count > device_t.counter_a_sense_count && step1 == 2)
            {
                /*if (step2 == 0) // 只打印一次调试信息
                {
                    DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense Direction: B to A\n");
                }*/
                step2 = step1;
            }

            if (device_t.counter_a_sense_count > 4 && device_t.counter_b_sense_count > 4)
            {
                if (step2)
                {
                    countDelay = 20; // 适当延时降低灵敏度
                    if (step2 == 1)  // 只有经历过A-B步骤才增加计数B
                    {
                        // if (device_t.mode == COUNTING_MODE) // 只有验证模式闪灯
                        {
                            ledGreenBlinkTime = 200;
                            ledBlueBlinkTime = 200; // 蓝+绿=青
                        }
                        device_t.counter_b_value++;
                        device_t.counter_b_count++;
                        // DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense CounterB++\n");
                        device_t.direction = PASSAGE_DIRECTION_A_TO_B;
                    }
                    else if (step2 == 2) // 只有经历过B-A步骤才增加计数A
                    {
                        // if (device_t.mode == COUNTING_MODE) // 只有验证模式闪灯
                        {
                            ledBlueBlinkTime = 200;
                            ledRedBlinkTime = 200; // 红+蓝=紫色
                        }
                        device_t.counter_a_value++;
                        device_t.counter_a_count++;
                        // DEBUG_TRACE(IR_TAG, "[Sense] Infrared Sense CounterA++\n");
                        device_t.direction = PASSAGE_DIRECTION_B_TO_A;
                    }
                    if (!device_t.updateCounter && device_t.mode == COUNTING_MODE) // 计数模式下才开始上报，验证和对齐模式不上报
                    {
                        if (device_t.reportMode >= REPORT_COUNT_CHANGED) // 计数值改变
                        {
                            device_t.updateCounter = 1;
                        }
                        else if (device_t.reportMode >= REPORT_COUNT_MORETHAN_SETTING) // 累计人数上报
                        {
                            if ((device_t.counter_a_value + device_t.counter_b_value) >= device_t.reportTotalCount)
                            {
                                device_t.updateCounter = 1;
                            }
                        }
                    }
                    // DEBUG_TRACE(IR_TAG, "[Count] Infrared Counter A: %d, Total Counter A: %d, Counter B: %d, Total Counter B: %d\n",
                    // device_t.counter_a_value, device_t.counter_a_count, device_t.counter_b_value, device_t.counter_b_count);
                }
                step1 = 0; // 只要未遮挡就清除步骤，只走到一半退回则不计数且清除步骤等下次计数
                step2 = 0;
            }
        }
        sumTotal_a += device_t.counter_a_sense_count;
        sumTotal_b += device_t.counter_b_sense_count;
        device_t.counter_a_sense_count = 0;
        device_t.counter_b_sense_count = 0;
    }
}

/**
 * @brief  客流量计数器对齐模式处理
 * @param
 * @retval
 */
void People_Counter_Alignment_MODE_Process(void)
{
    static u32 start_run_time;
    if (start_run_time > get_syspant_ms())
        return;
    start_run_time = get_syspant_ms() + 2 * 250;                                      // 红外发射周期小于2ms 250个周期判断一次
    if (device_t.counter_a_sense_count < 250 || device_t.counter_b_sense_count < 250) // 未对齐
    {
        ledRedBlinkTime = 200;
    }
    else
    {
        ledGreenBlinkTime = SEC_DELAY;
    }
    DEBUG_TRACE(LOG_TAG, "[Alignment mode] Infrared Sense Count A: %d, Count B: %d\n", device_t.counter_a_sense_count, device_t.counter_b_sense_count);
    device_t.counter_a_sense_count = 0;
    device_t.counter_b_sense_count = 0;
}

/**
 * @brief  客流量计数器模式切换控制，在1ms中断函数中执行
 * @param
 * @retval
 */
void People_Counter_Mode_Process(void)
{
    if (device_t.mode_timeout)
    {
        device_t.mode_timeout--;
        if (!device_t.mode_timeout)
        {
            switch (device_t.mode)
            {
            case ALIGNMENT_MODE: // 对齐模式
                device_t.mode = VALIDATION_MODE;
                device_t.mode_timeout = 2 * SEC_DELAY * 60;
                device_t.counter_a_value = 0; // 清零计数值
                device_t.counter_b_value = 0;
                break;
            case VALIDATION_MODE: // 验证模式
                device_t.mode = COUNTING_MODE;
                /*发送客流量计数器*/
                ledBlueBlinkTime = 2 * SEC_DELAY;
                // mDelay(250);
                // ledBlueBlinkTime = 250;
                device_t.updateCounter = 1;
                break;
            case COUNTING_MODE: // 计数模式
                break;
            default:
                device_t.mode = COUNTING_MODE;
                break;
            }
        }
    }
}

/**
 * @brief  客流量计数器运行事件控制函数
 * @param
 * @retval
 */
void Run_People_Counter_Handle(void)
{
    if (factoryMode || keepMode)
        return;
    if (device_t.mode == ALIGNMENT_MODE)
        People_Counter_Alignment_MODE_Process();
    else
        People_Counter_Event();
    static time_t stamp;
    if (stamp > Timestamp)
        return;

    stamp = Timestamp + 3;
    DEBUG_TRACE(IR_TAG, "[Count] Infrared Counter A: %d, Total Counter A: %d, Counter B: %d, Total Counter B: %d\n",
                device_t.counter_a_value, device_t.counter_a_count, device_t.counter_b_value, device_t.counter_b_count);
}

/**
 * @brief  LED显示控制，在1ms中断函数中执行
 * @param
 * @retval
 */
void Led_Blink_Process(void)
{
    static int cnt;
    if (ledGreenBlinkTime)
    {
        ledGreenBlinkTime--;
        LL_GPIO_WriteOutputPin(GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN, 1);
        if (!ledGreenBlinkTime)
        {
            LL_GPIO_WriteOutputPin(GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN, 0);
        }
    }

    if (ledBlueBlinkTime)
    {
        ledBlueBlinkTime--;
        LL_GPIO_WriteOutputPin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN, 1);
        if (!ledBlueBlinkTime)
        {
            LL_GPIO_WriteOutputPin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN, 0);
        }
    }

    if (ledRedBlinkTime)
    {
        ledRedBlinkTime--;
        LL_GPIO_WriteOutputPin(GPIO_LED_RED_PORT, GPIO_LED_RED_PIN, 1);
        if (!ledRedBlinkTime)
        {
            LL_GPIO_WriteOutputPin(GPIO_LED_RED_PORT, GPIO_LED_RED_PIN, 0);
        }
    }

    if (batteryStatus)
    {
        cnt++;
        if (cnt > 1800)
        {
            LL_GPIO_WriteOutputPin(GPIO_LED_ORANGE_PORT, GPIO_LED_ORANGE_PIN, 1);
            if (cnt > 2000)
                cnt = 0;
        }
        else
            LL_GPIO_WriteOutputPin(GPIO_LED_ORANGE_PORT, GPIO_LED_ORANGE_PIN, 0);
    }
    else if (ledOrangeBlinkTime)
    {
        ledOrangeBlinkTime--;
        LL_GPIO_WriteOutputPin(GPIO_LED_ORANGE_PORT, GPIO_LED_ORANGE_PIN, 1);
        if (!ledOrangeBlinkTime)
        {
            LL_GPIO_WriteOutputPin(GPIO_LED_ORANGE_PORT, GPIO_LED_ORANGE_PIN, 0);
        }
    }
}

/**
 * @brief  工厂测试模式
 * @param
 * @retval
 */
void factoryTestMode(void)
{
    mDelay(300);
    DEBUG_TRACE(ACK_TAG, "factoryMode People Counter Sensor Step:1"); // 开始测试
    ledGreenBlinkTime = 2 * SEC_DELAY;                                // LED常亮2秒
    mDelay(2000);
    ledBlueBlinkTime = 2 * SEC_DELAY;
    mDelay(2000);
    ledOrangeBlinkTime = 2 * SEC_DELAY;
    mDelay(2000);
    ledRedBlinkTime = 2 * SEC_DELAY;

    DEBUG_TRACE(ACK_TAG, "factoryMode People Counter Sensor Step:2"); // 等待门磁触发关闭
    int state = getDoorSensorState();
    if (state) // 已是闭合状态
    {
        while (getDoorSensorState() > 0)
            ; // 等待开启重新触发
        state = 0;
    }
    while (getDoorSensorState() == 0)
        ; // 等待闭合
    mDelay(500);

    DEBUG_TRACE(ACK_TAG, "factoryMode People Counter Sensor Step:3"); // 等待门磁触发开启
    state = getDoorSensorState();
    if (state == 0) // 已是开启状态
    {
        while (getDoorSensorState() == 0)
            ; // 等待闭合重新触发
    }
    while (getDoorSensorState() > 0)
        ; // 等待开启
    mDelay(500);

    DEBUG_TRACE(ACK_TAG, "factoryMode People Counter Sensor Step:4"); // 等待Counter A触发
    state = getPeopleCounterAState();
    while (state != getPeopleCounterAState())
        ;
    mDelay(300);
    DEBUG_TRACE(ACK_TAG, "factoryMode People Counter Sensor Step:5"); // 等待Counter B触发
    mDelay(500);
    state = getPeopleCounterBState();
    while (state != getPeopleCounterBState())
        ;
    mDelay(300);
    DEBUG_TRACE(ACK_TAG, "factoryMode People Counter Sensor Step:6"); // 等待设备入网
    mDelay(500);

    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 重新初始化设备
}
