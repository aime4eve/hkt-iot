
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "communicate.h"
#include "config.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "iic.h"
#include "lwrb.h"
#include "multi_button.h"
#include "sound.h"
#include "spi.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

#include "LoRaWAN_APPLY.h"
#include "SI12T.h"
#include "W25X40CLSNIG.h"
#include "ZS902R.h"
#if (FSV9522_CARD_ENABLE)
#include "fsv9522.h"
#else
#include "nz3801.h"
#endif

/*  任务优先级，数值越小优先级越高  */
#define thread_priority_deepsleep 4
#define thread_priority_start 2
#define thread_priority_msg 3

#define thread_priority_touch 1
#define thread_priority_sound_play 5
#define thread_priority_card 5
#define thread_priority_fingerprint 5
#define thread_priority_motor 6
#define thread_priority_bt 7
#define thread_priority_uart 8
#define thread_priority_communicate 9
#define thread_priority_local_sync 10

#define thread_priority_led 20
#define thread_priority_status 30

/*  任务栈大小，单位字节  */
#define thread_stack_size_start 2048
#define thread_stack_size_status 1024
#define thread_stack_size_idle 1024

#define thread_stack_size_communicate (4096 + 1024)
#define thread_stack_size_uart 2048
#define thread_stack_size_bt 2048
#define thread_stack_size_led 1024
#define thread_stack_size_motor 1024
#define thread_stack_size_touch 1024
#define thread_stack_size_card 2048
#define thread_stack_size_fingerprint 1024
#define thread_stack_size_local_sync 1024
#define thread_stack_size_sound_play 1024
#define thread_stack_size_deepsleep (4096 + 1024)

/*  静态全局变量 */
static TX_THREAD thread_block_start;
static uint64_t thread_stack_start[thread_stack_size_start / 8];
static TX_THREAD thread_block_communicate;
static uint64_t thread_stack_communicate[thread_stack_size_communicate / 8];
static TX_THREAD thread_block_uart;
static uint64_t thread_stack_uart[thread_stack_size_uart / 8];
static TX_THREAD thread_block_bt;
static uint64_t thread_stack_bt[thread_stack_size_bt / 8];
static TX_THREAD thread_block_led;
static uint64_t thread_stack_led[thread_stack_size_led / 8];
static TX_THREAD thread_block_motor;
static uint64_t thread_stack_motor[thread_stack_size_motor / 8];
static TX_THREAD thread_block_touch;
static uint64_t thread_stack_touch[thread_stack_size_touch / 8];
static TX_THREAD thread_block_card;
static uint64_t thread_stack_card[thread_stack_size_card / 8];
static TX_THREAD thread_block_fingerprint;
static uint64_t thread_stack_fingerprint[thread_stack_size_fingerprint / 8];
static TX_THREAD thread_block_local_sync;
static uint64_t thread_stack_local_sync[thread_stack_size_local_sync / 8];
static TX_THREAD thread_block_sound_play;
static uint64_t thread_stack_sound_play[thread_stack_size_sound_play / 8];
static TX_THREAD thread_block_deepsleep;
static uint64_t thread_stack_deepsleep[thread_stack_size_deepsleep / 8];

TX_TIMER AppTimer;

/*  全局变量 */
TX_MUTEX u_mutex_lock;  /* 用于printf互斥 */
TX_MUTEX f_mutex_lock;  /* 用于nor flash互斥 */
TX_MUTEX l_mutex_lock;  /* 用于lora互斥 */
TX_SEMAPHORE Semaphore; /* 用于指纹互斥 */

TX_EVENT_FLAGS_GROUP event_group;

/**
 * @brief  将ThreadX任务信息通过串口打印出来
 * @param
 * @retval
 */
void DispthreadInfo(void)
{
    TX_THREAD *p_tcb; /* 定义一个任务控制块指针 */

    p_tcb = &thread_block_start;

    /* 打印标题 */
    INFO("\r\n===============================================================\r\n");
    // INFO(" 任务优先级 任务栈大小 当前使用栈  最大栈使用   任务名\r\n");
    INFO("   Prio     StackSize   CurStack    MaxStack   ThreadName\r\n");

    /* 遍历任务控制列表TCB list)，打印所有的任务的优先级和名称 */
    while (p_tcb != (TX_THREAD *)0) {
        INFO("   %2d        %5d      %5d       %5d      %s\r\n",
             p_tcb->tx_thread_priority,
             p_tcb->tx_thread_stack_size,
             (int)p_tcb->tx_thread_stack_end - (int)p_tcb->tx_thread_stack_ptr,
             (int)p_tcb->tx_thread_stack_end - (int)p_tcb->tx_thread_stack_highest_ptr,
             p_tcb->tx_thread_name);

        p_tcb = p_tcb->tx_thread_created_next;

        if (p_tcb == &thread_block_start)
            break;
    }
}

void TimerCallback()
{
    static u16 SysTicker;
    SysTicker++;
    if (uartInfo.recvTimeout)
        uartInfo.recvTimeout--;
    if (uartBle.recvTimeout)
        uartBle.recvTimeout--;
    if (uartZs.recvTimeout)
        uartZs.recvTimeout--;
    if (uartLoRa.recvTimeout)
        uartLoRa.recvTimeout--;
    if (uartLoRa.sendDelay)
        uartLoRa.sendDelay--;
    if (device_t.lockDoorTimeout)
        device_t.lockDoorTimeout--;
    if (s_touch.timeout) {
        s_touch.timeout--;
        if (!s_touch.timeout && s_touch.cnt) { // 超时清除密码键盘操作
            memset(&s_touch, 0, sizeof(s_touch));
        }
    }
    if (device_t.lockTimeout) {
        device_t.lockTimeout--;
        if (!device_t.lockTimeout) {
            device_t.openFailCnt = 0;
        }
    }
    if (device_t.operateTimeout) {
        device_t.operateTimeout--;
    }

    if (SysTicker >= SEC_DELAY) {
        SysTicker = 0;
        SystemTimer();
        Timestamp++;
#if FUNC_OPERATIONAL_VERSION_ENABLE
        if (coded_lock_t.temp_pass.isable) {
            if (coded_lock_t.temp_pass.time < Timestamp) {
                coded_lock_t.temp_pass.isable = 0;
            }
        }
#endif
        if (!uartLoRa.sendDelay && !s_touch.timeout && !device_t.lockTimeout && !device_t.operateTimeout) {
            if (device_t.sleepDelay)
                device_t.sleepDelay--;
        }

        if (coded_lock_t.temp_pass.time < Timestamp && coded_lock_t.temp_pass.isable) {
            coded_lock_t.temp_pass.isable = 0;
        }
    }
    IWDT_Clr();
}

/**
 * @brief  启动任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
static void thread_start(ULONG thread_input)
{
    (void)thread_input;

    /* 外设初始化 */
    FuncInit();      // 系统软件初始化
    SpiFlash_Init(); // nor flash 初始化
    lwrbDataInit();  // 初始化缓冲区
    IWDT_Init();
    IWDT_Clr();

    /* 定时器组 */
    tx_timer_create(&AppTimer,
                    "App Timer",
                    TimerCallback,
                    0,                 /* 传递的参数 */
                    50,                /* 设置定时器时间溢出的初始延迟，单位 ThreadX 系统时间节拍数 */
                    50,                /* 设置初始延迟后的定时器运行周期，如果设置为 0，表示单次定时器 */
                    TX_AUTO_ACTIVATE); /* 激活定时器 */

    /* 创建通讯任务 */
    tx_thread_create(&thread_block_communicate,     /* 任务控制块地址 */
                     "thread communicate",          /* 任务名 */
                     thread_communicate,            /* 启动任务函数地址 */
                     0,                             /* 传递给任务的参数 */
                     &thread_stack_communicate[0],  /* 堆栈基地址 */
                     thread_stack_size_communicate, /* 堆栈空间大小 */
                     thread_priority_communicate,   /* 任务优先级*/
                     thread_priority_communicate,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,              /* 不开启时间片 */
                     TX_AUTO_START);                /* 创建后立即启动 */

    /* 创建通讯任务 */
    tx_thread_create(&thread_block_uart,     /* 任务控制块地址 */
                     "thread uart",          /* 任务名 */
                     thread_uart,            /* 启动任务函数地址 */
                     0,                      /* 传递给任务的参数 */
                     &thread_stack_uart[0],  /* 堆栈基地址 */
                     thread_stack_size_uart, /* 堆栈空间大小 */
                     thread_priority_uart,   /* 任务优先级*/
                     thread_priority_uart,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,       /* 不开启时间片 */
                     TX_AUTO_START);         /* 创建后立即启动 */

#if FUNC_BT_ENABLE
    /* 创建蓝牙通讯任务 */
    tx_thread_create(&thread_block_bt,     /* 任务控制块地址 */
                     "thread bt",          /* 任务名 */
                     thread_bt,            /* 启动任务函数地址 */
                     0,                    /* 传递给任务的参数 */
                     &thread_stack_bt[0],  /* 堆栈基地址 */
                     thread_stack_size_bt, /* 堆栈空间大小 */
                     thread_priority_bt,   /* 任务优先级*/
                     thread_priority_bt,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,     /* 不开启时间片 */
                     TX_AUTO_START);       /* 创建后立即启动 */
#endif

    /* 创建LED闪烁任务 */
    tx_thread_create(&thread_block_led,     /* 任务控制块地址 */
                     "thread led",          /* 任务名 */
                     thread_led,            /* 启动任务函数地址 */
                     0,                     /* 传递给任务的参数 */
                     &thread_stack_led[0],  /* 堆栈基地址 */
                     thread_stack_size_led, /* 堆栈空间大小 */
                     thread_priority_led,   /* 任务优先级*/
                     thread_priority_led,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,      /* 不开启时间片 */
                     TX_AUTO_START);        /* 创建后立即启动 */

    /* 创建电机控制任务 */
    tx_thread_create(&thread_block_motor,     /* 任务控制块地址 */
                     "thread motor",          /* 任务名 */
                     thread_motor,            /* 启动任务函数地址 */
                     0,                       /* 传递给任务的参数 */
                     &thread_stack_motor[0],  /* 堆栈基地址 */
                     thread_stack_size_motor, /* 堆栈空间大小 */
                     thread_priority_motor,   /* 任务优先级*/
                     thread_priority_motor,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,        /* 不开启时间片 */
                     TX_AUTO_START);          /* 创建后立即启动 */

    /* 创建触摸感应任务 */
    tx_thread_create(&thread_block_touch,     /* 任务控制块地址 */
                     "thread touch",          /* 任务名 */
                     thread_touch,            /* 启动任务函数地址 */
                     0,                       /* 传递给任务的参数 */
                     &thread_stack_touch[0],  /* 堆栈基地址 */
                     thread_stack_size_touch, /* 堆栈空间大小 */
                     thread_priority_touch,   /* 任务优先级*/
                     thread_priority_touch,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,        /* 不开启时间片 */
                     TX_AUTO_START);          /* 创建后立即启动 */

    /* 创建读卡器任务 */
    tx_thread_create(&thread_block_card,     /* 任务控制块地址 */
                     "thread card",          /* 任务名 */
                     thread_card,            /* 启动任务函数地址 */
                     0,                      /* 传递给任务的参数 */
                     &thread_stack_card[0],  /* 堆栈基地址 */
                     thread_stack_size_card, /* 堆栈空间大小 */
                     thread_priority_card,   /* 任务优先级*/
                     thread_priority_card,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,       /* 不开启时间片 */
                     TX_AUTO_START);         /* 创建后立即启动 */
#if FUNC_FINGERPRINT_ENABLE
    /* 创建指纹任务 */
    tx_thread_create(&thread_block_fingerprint,     /* 任务控制块地址 */
                     "thread fingerprint",          /* 任务名 */
                     thread_fingerprint,            /* 启动任务函数地址 */
                     0,                             /* 传递给任务的参数 */
                     &thread_stack_fingerprint[0],  /* 堆栈基地址 */
                     thread_stack_size_fingerprint, /* 堆栈空间大小 */
                     thread_priority_fingerprint,   /* 任务优先级*/
                     thread_priority_fingerprint,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,              /* 不开启时间片 */
                     TX_AUTO_START);                /* 创建后立即启动 */
#endif

#if !FUNC_OPERATIONAL_VERSION_ENABLE
    /* 创建云同步任务 */
    tx_thread_create(&thread_block_local_sync,     /* 任务控制块地址 */
                     "thread local sync",          /* 任务名 */
                     thread_local_sync,            /* 启动任务函数地址 */
                     0,                            /* 传递给任务的参数 */
                     &thread_stack_local_sync[0],  /* 堆栈基地址 */
                     thread_stack_size_local_sync, /* 堆栈空间大小 */
                     thread_priority_local_sync,   /* 任务优先级*/
                     thread_priority_local_sync,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,             /* 不开启时间片 */
                     TX_AUTO_START);               /* 创建后立即启动 */
#endif
    /* 创建语音播放任务 */
    tx_thread_create(&thread_block_sound_play,     /* 任务控制块地址 */
                     "thread sound play",          /* 任务名 */
                     thread_sound_play,            /* 启动任务函数地址 */
                     0,                            /* 传递给任务的参数 */
                     &thread_stack_sound_play[0],  /* 堆栈基地址 */
                     thread_stack_size_sound_play, /* 堆栈空间大小 */
                     thread_priority_sound_play,   /* 任务优先级*/
                     thread_priority_sound_play,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,             /* 不开启时间片 */
                     TX_AUTO_START);               /* 创建后立即启动 */

    /* 创建低功耗睡眠任务 */
    tx_thread_create(&thread_block_deepsleep,     /* 任务控制块地址 */
                     "thread deepsleep",          /* 任务名 */
                     thread_deepsleep,            /* 启动任务函数地址 */
                     0,                           /* 传递给任务的参数 */
                     &thread_stack_deepsleep[0],  /* 堆栈基地址 */
                     thread_stack_size_deepsleep, /* 堆栈空间大小 */
                     thread_priority_deepsleep,   /* 任务优先级*/
                     thread_priority_deepsleep,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,            /* 不开启时间片 */
                     TX_AUTO_START);              /* 创建后立即启动 */
    // 降低任务优先级
    //  UINT OldPriority;
    //  tx_thread_preemption_change(&thread_block_start, 10, &OldPriority);
    //  tx_thread_priority_change(&thread_block_start, 10, &OldPriority);

    tx_thread_delete(&thread_block_start);
}

/**
 * @brief  事件组通知函数
 * @param group_ptr 该事件传递的形参
 * @retval
 */
void event_group_notify(TX_EVENT_FLAGS_GROUP *group_ptr)
{
    ;
}

/**
 * @brief  ThreadX专用的任务创建，通信组件创建函数
 * @param first_unused_memory  未使用的地址空间
 * @retval
 */
void tx_application_define(void *first_unused_memory)
{
    /* 创建互斥信号量 */
    tx_mutex_create(&u_mutex_lock, "u_mutex_info", TX_NO_INHERIT);
    tx_mutex_create(&f_mutex_lock, "f_mutex_info", TX_NO_INHERIT);
    tx_mutex_create(&l_mutex_lock, "l_mutex_info", TX_NO_INHERIT);
    /* 创建信号量，初始值为1，用于二值信号量 */
    tx_semaphore_create(&Semaphore, "Semaphore", 1);

    /* 创建事件标志组 */
    tx_event_flags_create(&event_group, "event_group");
    tx_event_flags_set_notify(&event_group, event_group_notify);

    /* 创建启动任务 */
    tx_thread_create(&thread_block_start,     /* 任务控制块地址 */
                     "thread start",          /* 任务名 */
                     thread_start,            /* 启动任务函数地址 */
                     0,                       /* 传递给任务的参数 */
                     &thread_stack_start[0],  /* 堆栈基地址 */
                     thread_stack_size_start, /* 堆栈空间大小 */
                     thread_priority_start,   /* 任务优先级*/
                     thread_priority_start,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,        /* 不开启时间片 */
                     TX_AUTO_START);          /* 创建后立即启动 */
}

// 更改系统主频为64M
void Clockchange(void)
{
    uint32_t i = 0;
    CMU->PCLKCR1 |= 0x1 << 7; // PAD总线时钟使能
    // GPIOF->FCR |=0x3C; //PF1和PF2配置模拟功能

    //    //使能XTHF
    //     CMU_XTHFCR_XTHFEN_Setable(ENABLE); //使能XTHF
    //     CMU_XTHFCR_XTHF_CFG_Set(CMU_XTHFCR_XTHF_CFG_MAX); //振荡强度选择最强
    //     TicksDelayMs( 3, NULL );//起振需要时间

    ///*系统时钟超过24M后需要打开wait*/
    FLS_RDCR_WAIT_Set(FLS_RDCR_WAIT_2CYCLE);
    IWDT_Clr();

    // PLL配置
    //  CMU->PLLHCR |= 0x1 << 1; //PLLH输入选择XTHF
    CMU->PLLHCR &= ~(0x1 << 1); // PLLH输入选择RCHF

    CMU_PLLHCR_REFPRSC_Set(CMU_PLLHCR_REFPRSC_DIV8); // 8M晶体分频到1M,其他晶体需要修改分频系数

    CMU->PLLHCR &= ~(0x3ff << 16);
    CMU->PLLHCR |= 0x3f << 16; // PLLH升至64M

    CMU_PLLHCR_EN_Setable(ENABLE); // 使能PLLH

    while (!CMU_PLLHCR_LOCKED_Chk()) { // 等待PLL建立
        if (i >= 6400) {               // 超时设计，根据主时钟选择不同值，例程是8M
            break;
        }
        i++;
    }

    CMU_SYSCLKCR_SYSCLKSEL_Set(CMU_SYSCLKCR_SYSCLKSEL_PLL_H); // 系统时钟源选择PLLH
    CMU_SYSCLKCR_AHBPRES_Set(CMU_SYSCLKCR_AHBPRES_DIV1);
    CMU_SYSCLKCR_APBPRES_Set(CMU_SYSCLKCR_APBPRES_DIV1);
}

/**
 * @brief  程序入口
 * @param
 * @retval
 */
int main(void)
{
    /*系统初始化*/
    SCB->VTOR = 0x00004000;
    Init_System();
    Uartx_Init(UART0, 115200); // 串口打印初始化
#if FUNC_FINGERPRINT_ENABLE
    Uartx_Init(UART2, 57600); // 指纹串口初始化
#endif
#if FUNC_BT_ENABLE
    Uartx_Init(UART5, 115200); // 蓝牙串口初始化
#endif
#if FUNC_LORA_VERSION_LIR
    LPUART0_Init();
#else
    Uartx_Init(UART3, 115200); // 指纹串口初始化
#endif
    BSTIM_Init(1000, RCHFCLKCFG); // 定时1ms中断
#if LPT_CARD_ENABLE
    LPTIM_Init();
#endif
    App_PortCfg();
    I2C_Init(I2C0);
    SpiInit();
    Spi1Init();
    /* 进入ThreadX内核 */
    tx_kernel_enter();
}
