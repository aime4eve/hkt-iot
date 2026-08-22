
/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "tim.h"
#include "uart.h"
#include "gpio.h"
#include "systick.h"
#include "lwrb.h"
#include "control_center.h"
#include "communicate.h"
#include "spi.h"
#include "adc.h"
#include "iic.h"
#include "multi_button.h"
#include "flash.h"

#include "LoRaWAN_APPLY.h"

#include "sht40x.h"
#include "scd4x.h"
#include "sgp4x.h"
#include "hp203b.h"
#include "im8601pa.h"
#include "LTR_3XXALS_01.h"
#include "EPD.h"
#include "PMSA003.h"
// #include "SEN0231.h"
#include "cb_hcho_v4c.h"
#include "W25X40CLSNIG.h"
#include "zmod451x.h"
#include "zmod441x.h"

/*  任务优先级，数值越小优先级越高  */
#define thread_priority_deepsleep 1
#define thread_priority_start 2
#define thread_priority_msg 3

#define thread_priority_display 4
#define thread_priority_sensor 5
#define thread_priority_tvoc 6
#define thread_priority_communicate 7
#define thread_priority_pir 8
#define thread_priority_led 9

#define thread_priority_status 30
#define thread_priority_idle 31

/*  任务栈大小，单位字节  */
#define thread_stack_size_start 4096
#define thread_stack_size_status 1024
#define thread_stack_size_idle 1024

#define thread_stack_size_communicate 4096
#define thread_stack_size_sensor 4096
#define thread_stack_size_tvoc 1024
#define thread_stack_size_led 1024
#define thread_stack_size_pir 1024
#define thread_stack_size_display 4096
#define thread_stack_size_deepsleep 4096

/*  静态全局变量 */
static TX_THREAD thread_block_start;
static uint64_t thread_stack_start[thread_stack_size_start / 8];
static TX_THREAD thread_block_status;
static uint64_t thread_stack_status[thread_stack_size_status / 8];
static TX_THREAD thread_block_idle;
static uint64_t thread_stack_idle[thread_stack_size_idle / 8];
static TX_THREAD thread_block_communicate;
static uint64_t thread_stack_communicate[thread_stack_size_communicate / 8];
static TX_THREAD thread_block_sensor;
static uint64_t thread_stack_sensor[thread_stack_size_sensor / 8];
static TX_THREAD thread_block_tvoc;
static uint64_t thread_stack_tvoc[thread_stack_size_tvoc / 8];
static TX_THREAD thread_block_led;
static uint64_t thread_stack_led[thread_stack_size_led / 8];
static TX_THREAD thread_block_pir;
static uint64_t thread_stack_pir[thread_stack_size_pir / 8];
static TX_THREAD thread_block_display;
static uint64_t thread_stack_display[thread_stack_size_display / 8];
static TX_THREAD thread_block_deepsleep;
static uint64_t thread_stack_deepsleep[thread_stack_size_deepsleep / 8];

TX_TIMER AppTimer;

/*  全局变量 */
TX_MUTEX mutex_info; /* 用于printf互斥 */
TX_MUTEX mutex_iic;  /* 用于IIC总线互斥 */

TX_EVENT_FLAGS_GROUP event_group;

__IO uint8_t OSStatRdy;  /* 统计任务就绪标志 */
__IO uint32_t OSIdleCtr; /* 空闲任务计数 */
__IO float OSCPUUsage;   /* CPU百分比 */
uint32_t OSIdleCtrMax;   /* 1秒内最大的空闲计数 */
uint32_t OSIdleCtrRun;   /* 1秒内空闲任务当前计数 */

/********************************************************************************************/

/**
 * @brief  统计任务，用于实现CPU利用率的统计。为了测试更加准确，可以开启注释调用的全局中断开关
 * @param
 * @retval
 */
void OSStatInit(void)
{
    OSStatRdy = FALSE;
    tx_thread_sleep(2u); /* 时钟同步 */

    __disable_irq();
    OSIdleCtr = 0uL; /* 清空闲计数 */
    __enable_irq();
    tx_thread_sleep(100); /* 统计100ms内，最大空闲计数 */

    __disable_irq();
    OSIdleCtrMax = OSIdleCtr; /* 保存最大空闲计数 */
    OSStatRdy = TRUE;
    __enable_irq();
}

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
    INFO("OS CPU Usage = %5.2f%%\r\n", OSCPUUsage);
    INFO("===============================================================\r\n");
    // INFO(" 任务优先级 任务栈大小 当前使用栈  最大栈使用   任务名\r\n");
    INFO("   Prio     StackSize   CurStack    MaxStack   ThreadName\r\n");

    /* 遍历任务控制列表TCB list)，打印所有的任务的优先级和名称 */
    while (p_tcb != (TX_THREAD *)0)
    {

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

/**
 * @brief  状态处理
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
static void thread_status(ULONG thread_input)
{
    (void)thread_input;

    while (OSStatRdy == FALSE)
    {
        tx_thread_sleep(200); /* 等待统计任务就绪 */
    }

    OSIdleCtrMax /= 100uL;
    if (OSIdleCtrMax == 0uL)
    {
        OSCPUUsage = 0u;
    }

    //__disable_irq();
    OSIdleCtr = OSIdleCtrMax * 100uL; /* 设置初始CPU利用率 0% */
    //__enable_irq();

    for (;;)
    {
        //__disable_irq();
        OSIdleCtrRun = OSIdleCtr; /* 获得100ms内空闲计数 */
        OSIdleCtr = 0uL;          /* 复位空闲计数 */
        //__enable_irq();           /* 计算100ms内的CPU利用率 */
        OSCPUUsage = (100uL - (float)OSIdleCtrRun / OSIdleCtrMax);
        tx_thread_sleep(100); /* 每100ms统计一次 */
    }
}

/**
 * @brief  空闲任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
static void thread_idle(ULONG thread_input)
{
    // TX_INTERRUPT_SAVE_AREA;

    (void)thread_input;
    while (1)
    {
        // TX_DISABLE
        OSIdleCtr++;
        // TX_RESTORE
    }
}

void thread_50ms_timer_init(void)
{
    /* 定时器组 */
    tx_timer_create(&AppTimer,
                    "App Timer",
                    tx_TimerCallback,
                    0,                 /* 传递的参数 */
                    50,                /* 设置定时器时间溢出的初始延迟，单位 ThreadX 系统时间节拍数 */
                    50,                /* 设置初始延迟后的定时器运行周期，如果设置为 0，表示单次定时器 */
                    TX_AUTO_ACTIVATE); /* 激活定时器 */
}

/**
 * @brief  启动任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
static void thread_start(ULONG thread_input)
{
    (void)thread_input;

    /* 优先执行任务统计 */
    OSStatInit();

    /* 外设初始化 */
    FuncInit();      // 系统软件初始化
    SpiFlash_Init(); // nor flash 初始化
    lwrbDataInit();  // 初始化缓冲区
    IWDT_Clr();

    thread_50ms_timer_init();

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

    /* 创建传感器任务 */
    tx_thread_create(&thread_block_sensor,     /* 任务控制块地址 */
                     "thread sensor",          /* 任务名 */
                     thread_sensor,            /* 启动任务函数地址 */
                     0,                        /* 传递给任务的参数 */
                     &thread_stack_sensor[0],  /* 堆栈基地址 */
                     thread_stack_size_sensor, /* 堆栈空间大小 */
                     thread_priority_sensor,   /* 任务优先级*/
                     thread_priority_sensor,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,         /* 不开启时间片 */
                     TX_AUTO_START);           /* 创建后立即启动 */

    // /* 创建传感器TVOC任务 */
    // tx_thread_create(&thread_block_tvoc,     /* 任务控制块地址 */
    //                  "thread tvoc",          /* 任务名 */
    //                  thread_tvoc,            /* 启动任务函数地址 */
    //                  0,                      /* 传递给任务的参数 */
    //                  &thread_stack_tvoc[0],  /* 堆栈基地址 */
    //                  thread_stack_size_tvoc, /* 堆栈空间大小 */
    //                  thread_priority_tvoc,   /* 任务优先级*/
    //                  thread_priority_tvoc,   /* 任务抢占阀值 */
    //                  TX_NO_TIME_SLICE,       /* 不开启时间片 */
    //                  TX_AUTO_START);         /* 创建后立即启动 */

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
    /* 创建pir任务 */
    tx_thread_create(&thread_block_pir,     /* 任务控制块地址 */
                     "thread pir",          /* 任务名 */
                     thread_pir,            /* 启动任务函数地址 */
                     0,                     /* 传递给任务的参数 */
                     &thread_stack_pir[0],  /* 堆栈基地址 */
                     thread_stack_size_pir, /* 堆栈空间大小 */
                     thread_priority_pir,   /* 任务优先级*/
                     thread_priority_pir,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,      /* 不开启时间片 */
                     TX_AUTO_START);        /* 创建后立即启动 */
    /* 创建屏幕显示任务 */
    tx_thread_create(&thread_block_display,     /* 任务控制块地址 */
                     "thread display",          /* 任务名 */
                     thread_display,            /* 启动任务函数地址 */
                     0,                         /* 传递给任务的参数 */
                     &thread_stack_display[0],  /* 堆栈基地址 */
                     thread_stack_size_display, /* 堆栈空间大小 */
                     thread_priority_display,   /* 任务优先级*/
                     thread_priority_display,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,          /* 不开启时间片 */
                     TX_AUTO_START);            /* 创建后立即启动 */
#if !FUNC_PM2_5
    /* 创建睡眠任务 */
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
#endif
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
    tx_mutex_create(&mutex_info, "mutex_info", TX_NO_INHERIT);
    tx_mutex_create(&mutex_iic, "mutex_iic", TX_NO_INHERIT);
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

    /* 创建统计任务 */
    tx_thread_create(&thread_block_status,     /* 任务控制块地址 */
                     "thread status",          /* 任务名 */
                     thread_status,            /* 启动任务函数地址 */
                     0,                        /* 传递给任务的参数 */
                     &thread_stack_status[0],  /* 堆栈基地址 */
                     thread_stack_size_status, /* 堆栈空间大小 */
                     thread_priority_status,   /* 任务优先级*/
                     thread_priority_status,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,         /* 不开启时间片 */
                     TX_AUTO_START);           /* 创建后立即启动 */

    /* 创建空闲任务 */
    tx_thread_create(&thread_block_idle,     /* 任务控制块地址 */
                     "thread idle",          /* 任务名 */
                     thread_idle,            /* 启动任务函数地址 */
                     0,                      /* 传递给任务的参数 */
                     &thread_stack_idle[0],  /* 堆栈基地址 */
                     thread_stack_size_idle, /* 堆栈空间大小 */
                     thread_priority_idle,   /* 任务优先级*/
                     thread_priority_idle,   /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,       /* 不开启时间片 */
                     TX_AUTO_START);         /* 创建后立即启动 */
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
    Uartx_Init(UART0, 115200);    // 串口打印初始化
    BSTIM_Init(1000, RCHFCLKCFG); // 定时1ms中断
    LPUART0_Init();
    App_PortCfg();
    I2C_Init(I2C0);
    SpiInit();
    /* 进入ThreadX内核 */
    tx_kernel_enter();
}
