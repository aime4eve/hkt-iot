
/* Includes ------------------------------------------------------------------*/
#include "sound.h"
#include "communicate.h"
#include "control_center.h"
#include "gpio.h"
#include "multi_button.h"
#include "systick.h"
#include "uart.h"

loudspeaker_t sound_t;

/**
 * @brief  初始化语音播放GPIO
 * @retval
 */
void Sound_Init(void)
{
    CMU_OPCCR1_EXTICKSEL_Set(CMU_OPCCR1_EXTICKSEL_LSCLK); // EXTI中断采样时钟选择
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);                 // IO控制时钟寄存器使能

    /** 初始化语音IC IO **/
    OutputIO(SOUND_DATA_PORT, SOUND_DATA_PIN, OUT_PUSHPULL);
    InputtIO(SOUND_BUSY_PORT, SOUND_BUSY_PIN, IN_NORMAL); // 播放语音时输出低电平

    SOUND_DATA_H;
}

void Sound_BitH(void)
{
    SOUND_DATA_L;
    TicksDelayUs(500);
    SOUND_DATA_H;
    TicksDelayUs(500);
    TicksDelayUs(500);
    TicksDelayUs(500);
}

void Sound_BitL(void)
{
    SOUND_DATA_L;
    TicksDelayUs(500); // 主频过高时 delay时间<500ms
    TicksDelayUs(500);
    TicksDelayUs(500);
    SOUND_DATA_H;
    TicksDelayUs(500);
}

void Sound_Play(u8 num)
{
#if FUNC_VOICE_EN
    ;
#endif

    SOUND_DATA_L;
    __disable_irq();
    // TicksDelayMs(6, NULL);
    for (u8 p = 0; p < 12; p++) {
        TicksDelayUs(500);
    }
    SOUND_DATA_H;
    TicksDelayUs(500);
    for (int i = 0; i < 8; i++) {
        if ((num << i) & 0x80) {
            Sound_BitH();
        } else {
            Sound_BitL();
        }
    }
    __enable_irq();
}

void Sound_Insert_Number(u8 insert, u8 voice_index)
{
    if (insert == 1) {
        sound_t.voice_list[sound_t.voice_number++] = voice_index; // 插入语音列表
    } else {
        sound_t.voice_number = 1;     // 清除语音号
        sound_t.voice_play_index = 0; // 清除当前播放序号
        memset(sound_t.voice_list, 0, sizeof(sound_t.voice_list));
        sound_t.voice_list[0] = voice_index;
        Sound_Play(VOICE_STOP);
    }
#if HIGH_LEVEL_DEBUG_ENABLE
    DEBUG_TRACE(LOG_TAG, "Sound Insert Number %d ,Play Index %d", sound_t.voice_number, sound_t.voice_play_index);
#endif
}

/**
 * @brief  循环语音播放事件
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_sound_play(ULONG thread_input)
{
    (void)thread_input;
    ULONG actual_events;
    UINT status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
                                     BIT_SOUND,        /* 等待标志 */
                                     TX_AND_CLEAR,     /* 等待任意bit满足即可 */
                                     &actual_events,   /* 获取实际值 */
                                     TX_WAIT_FOREVER); /* 永久等待 */
    insertspeaker_t voice_loop = {0};
    tx_thread_sleep(2000);
    if (READ_RST_KEY_STATUS) { // 上电检测到有人按等待3秒确认动作
        tx_thread_sleep(3000);
    }

    tx_thread_sleep(10);

    Sound_Play(sound_t.volume_number);
    Sound_Play(VOICE_TONE_TOUCH);
    tx_event_flags_set(&event_group, BIT_LED, TX_OR);
    tx_event_flags_set(&event_group, BIT_FINGER, TX_OR);
    // tx_event_flags_set(&event_group, BIT_TOUCH, TX_OR);
    tx_event_flags_set(&event_group, BIT_CARD, TX_OR);
    tx_event_flags_set(&event_group, BIT_BLE, TX_OR);

    tx_event_flags_set(&event_group, BIT_BLE, TX_OR);

    tx_event_flags_set(&main_event_group, MAIN_BIT_RUN, TX_OR);

    // for (int i = 0; i < VOICE_HELP_INFO + 1; i++)
    // {
    //     Sound_Play(i);
    //     while (1)
    //     {
    //         tx_thread_sleep(500);
    //         if (READ_SOUND_BUSY_STATUS)
    //             break;
    //     }
    // }

    while (1) {
        tx_thread_sleep(20);
        u8 voice_busy_state = READ_SOUND_BUSY_STATUS;
        if (!READ_SOUND_BUSY_STATUS) // 播放语音时不进入睡眠
        {
            if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        }
        tx_thread_sleep(20);
        if (sound_t.timely) {
            voice_loop.insert = 0;
            voice_loop.voice_index = sound_t.voice_list[0];
            sound_t.timely = 0;
        } else if (voice_busy_state && READ_SOUND_BUSY_STATUS) // 防抖
        {
            if (sound_t.voice_play_index < sound_t.voice_number) {
#if HIGH_LEVEL_DEBUG_ENABLE
                DEBUG_TRACE(LOG_TAG, "Sound Play Index %d", sound_t.voice_play_index);
#endif
                voice_loop.insert = 0;
                voice_loop.voice_index = sound_t.voice_list[sound_t.voice_play_index];
                sound_t.voice_play_index++;
                if (sound_t.voice_play_index >= sound_t.voice_number) {
                    sound_t.voice_number = 0;
                    sound_t.voice_play_index = 0;
                    memset(sound_t.voice_list, 0, sizeof(sound_t.voice_list));
                }
                if (voice_loop.voice_index != VOICE_TONE_ALARM) {
                    tx_thread_sleep(100);
                }
            } else if (device_t.tamperAlarm) {
                /* 语音 */
                // 防拆报警
                Sound_Insert_Number(1, VOICE_TONE_ALARM);
            }
        }
        if (voice_loop.voice_index) {
            Sound_Play(voice_loop.voice_index);
            voice_loop.voice_index = 0;
        }
    }
}
