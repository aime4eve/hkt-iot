#ifndef __AN001_H__
#define __AN001_H__

#include "config.h"

#define DEFAULT_AS_NUMBER 10        // 最大支持空开数量
#define DEFAULT_AS_REPORT_TIME 36000 // 默认上报间隔36000秒

#define TRIGGER_REAL_TIME_DATA_CMD 6 // 被动上报实时数据
#define REPORT_REAL_TIME_DATA_CMD 7  // 主动上报实时数据

typedef enum {
    READ_AS_CONTACT_STATUS = 1, // 读取开关分合闸状态
    READ_AS_IS_REMOTE_CONTROL,  // 读取能否远程控制
    READ_AS_REAL_TIME_DATA,     // 读取空开实时数据
    READ_AS_SLAVE_PARAM,        // 读取空开已设定参数

} as_event_t;

typedef enum {
    CMD_AS_ENTER_ADDR_MODE = 1,  // 进入地址模式
    CMD_AS_OUT_ADDR_MODE,        // 退出地址模式
    CMD_AS_REPORT_INTERVAL,      // 设置上报间隔
    CMD_AS_SET_CONTACT,          // 设置空开开合状态
    CMD_AS_RESET_FACTORY_MODE_2, // 恢复出厂设置2
    CMD_AS_READ_SLAVE_PARAM,     // 读取指定空开阈值
    CMD_AS_LEAKAGE_PROTECTION,   // 漏保自检

} as_cmd_t;

typedef union {
    struct
    {
        u16 bit0 : 1;
        u16 bit1 : 1;
        u16 bit2 : 1;
        u16 bit3 : 1;
        u16 bit4 : 1;
        u16 bit5 : 1;
        u16 bit6 : 1;
        u16 bit7 : 1;
        u16 bit8 : 1;
        u16 bit9 : 1;
        u16 bit10 : 1;
        u16 bit11 : 1;
        u16 bit12 : 1;
        u16 bit13 : 1;
        u16 bit14 : 1;
        u16 bit15 : 1;
    };
    u16 bit_value;
} bit_16_t;

typedef union {
    struct
    {
        u32 bit0 : 1;
        u32 bit1 : 1;
        u32 bit2 : 1;
        u32 bit3 : 1;
        u32 bit4 : 1;
        u32 bit5 : 1;
        u32 bit6 : 1;
        u32 bit7 : 1;
        u32 bit8 : 1;
        u32 bit9 : 1;
        u32 bit10 : 1;
        u32 bit11 : 1;
        u32 bit12 : 1;
        u32 bit13 : 1;
        u32 bit14 : 1;
        u32 bit15 : 1;
    };
    u32 bit_value;
} bit_32_t;

typedef struct // 空开已设定参数
{
    u16 max_voltage;             // 电压上限       	0
    u16 min_voltage;             // 电压下限       	1
    u16 max_leakage;             // 漏电流上限     	2
    u16 max_power;               // 功率上限       	3
    u16 max_temperature;         // 温度上限       	4
    u16 max_electricity;         // 电流上限       	5
    u16 item;                    // 规格型号       	6
    u16 version;                 // 版本号         	7
    u16 electricity_coefficient; // 电流系数       	8
    u16 max_a_electricity;       // A相电流上限    	9
    u16 max_b_electricity;       // B相电流上限    	10
    u16 max_c_electricity;       // C相电流上限    	11
    u16 max_a_power;             // A相功率上限    	12
    u16 max_b_power;             // B相功率上限    	13
    u16 max_c_power;             // C相功率上限    	14
    u16 max_voltage_alarm;       // 电压预警上限   	15
    u16 min_voltage_alarm;       // 电压预警下限   	16
    u16 max_leakage_alarm;       // 漏电预警上限 	17
    u16 max_temperature_alarm;   // 温度预警上限 	18
    u16 a_electricity_alarm;     // A相电流预警  	19
    u16 b_electricity_alarm;     // B相电流预警  	20
    u16 c_electricity_alarm;     // C相电流预警  	21
    u16 electricity_alarm;       // 电流预警     	22
    u16 ID1;                     //        		   23
    u16 ID2;                     //         	   24
    u16 ID3;                     //        		   25

    union {
        struct
        {
            u16 bit0 : 1;  // 断电恢复分闸保护
            u16 bit1 : 1;  // 允许本地检修模式
            u16 bit2 : 1;  // 1P 恶性负载
            u16 bit3 : 1;  // 允许远程锁定模式
            u16 bit4 : 1;  // N 线前端断线4
            u16 bit5 : 1;  // 温度保护
            u16 bit6 : 1;  // 不平衡保护/3-4P
            u16 bit7 : 1;  // 相序保护/3-4P
            u16 bit8 : 1;  // 外部DI报警
            u16 bit9 : 1;  // 电流缺相/3-4P
            u16 bit10 : 1; // 保留
            u16 bit11 : 1; // 保留
            u16 bit12 : 1; // 保留
            u16 bit13 : 1; // 保留
            u16 bit14 : 1; // N 线后端断线4P
            u16 bit15 : 1; // 保留
        };
        u16 value;
    } enable_0; // 功能使能0    	26

    union {
        struct
        {
            u16 bit0 : 1;  // 短路报警
            u16 bit1 : 1;  // 浪涌报警
            u16 bit2 : 1;  // 过载报警
            u16 bit3 : 1;  // 温度预警
            u16 bit4 : 1;  // 漏电报警
            u16 bit5 : 1;  // 过流报警
            u16 bit6 : 1;  // 过压报警
            u16 bit7 : 1;  // 保留
            u16 bit8 : 1;  // 保留
            u16 bit9 : 1;  // 电压缺相/3-4P
            u16 bit10 : 1; // 打火报警
            u16 bit11 : 1; // 欠压报警
            u16 bit12 : 1; // 过压预警
            u16 bit13 : 1; // 欠压预警
            u16 bit14 : 1; // 漏电预警
            u16 bit15 : 1; // 电流预警
        };
        u16 value;
    } enable_1; // 功能使能0    	27

    union {
        struct
        {
            u16 bit0 : 1;  // 断电恢复分闸保护
            u16 bit1 : 1;  // 保留
            u16 bit2 : 1;  // 1P 恶性负载
            u16 bit3 : 1;  // 保留
            u16 bit4 : 1;  // N 线前端断线4
            u16 bit5 : 1;  // 温度保护
            u16 bit6 : 1;  // 不平衡保护/3-4P
            u16 bit7 : 1;  // 相序保护/3-4P
            u16 bit8 : 1;  // 外部DI报警
            u16 bit9 : 1;  // 电流缺相/3-4P
            u16 bit10 : 1; // 保留
            u16 bit11 : 1; // 保留
            u16 bit12 : 1; // 保留
            u16 bit13 : 1; // 保留
            u16 bit14 : 1; // N 线后端断线4P
            u16 bit15 : 1; // 保留
        };
        u16 value;
    } open_enable_0; // 分闸使能0    	28
    union {
        struct
        {
            u16 bit0 : 1;  // 短路报警
            u16 bit1 : 1;  // 浪涌报警
            u16 bit2 : 1;  // 过载报警
            u16 bit3 : 1;  // 温度预警
            u16 bit4 : 1;  // 漏电报警
            u16 bit5 : 1;  // 过流报警
            u16 bit6 : 1;  // 过压报警
            u16 bit7 : 1;  // 保留
            u16 bit8 : 1;  // 保留
            u16 bit9 : 1;  // 电压缺相/3-4P
            u16 bit10 : 1; // 打火报警
            u16 bit11 : 1; // 欠压报警
            u16 bit12 : 1; // 过压预警
            u16 bit13 : 1; // 欠压预警
            u16 bit14 : 1; // 漏电预警
            u16 bit15 : 1; // 电流预警
        };
        u16 value;
    } open_enable_1; // 分闸使能1    	29

    union {
        struct
        {
            u16 bit0 : 1;  // 断电恢复分闸保护
            u16 bit1 : 1;  // 保留
            u16 bit2 : 1;  // 保留
            u16 bit3 : 1;  // 保留
            u16 bit4 : 1;  // N 线前端断线4
            u16 bit5 : 1;  // 温度保护
            u16 bit6 : 1;  // 不平衡保护/3-4P
            u16 bit7 : 1;  // 相序保护/3-4P
            u16 bit8 : 1;  // 保留
            u16 bit9 : 1;  // 电流缺相/3-4P
            u16 bit10 : 1; // 保留
            u16 bit11 : 1; // 保留
            u16 bit12 : 1; // 保留
            u16 bit13 : 1; // 保留
            u16 bit14 : 1; // 保留
            u16 bit15 : 1; // 保留
        };
        u16 value;
    } close_enable_0; // 允许合闸0    	30

    union {
        struct
        {
            u16 bit0 : 1;  // 短路报警
            u16 bit1 : 1;  // 浪涌报警
            u16 bit2 : 1;  // 过载报警
            u16 bit3 : 1;  // 温度预警
            u16 bit4 : 1;  // 漏电报警
            u16 bit5 : 1;  // 过流报警
            u16 bit6 : 1;  // 过压报警
            u16 bit7 : 1;  // 保留
            u16 bit8 : 1;  // 保留
            u16 bit9 : 1;  // 电压缺相/3-4P
            u16 bit10 : 1; // 打火报警
            u16 bit11 : 1; // 欠压报警
            u16 bit12 : 1; // 过压预警
            u16 bit13 : 1; // 欠压预警
            u16 bit14 : 1; // 漏电预警
            u16 bit15 : 1; // 电流预警
        };
        u16 value;
    } close_enable_1; // 允许合闸1    	31
} slave_param_t;

typedef struct // 实时数据结构体
{
    u16 total_voltage;       // 总电压
    u16 leakage_electricity; // 漏电流
    u16 power;               // 功率
    u16 temperature;         // 温度
    u16 electricity;         // 电流
    union {
        struct
        {
            u16 shortcircuit_alarm : 1;                      // 短路报警
            u16 surge_alarm : 1;                             // 浪涌报警
            u16 overload_alarm : 1;                          // 过载报警
            u16 temperature_warning : 1;                     // 温度预警
            u16 leakage_alarm : 1;                           // 漏电报警
            u16 over_electricity_alarm : 1;                  // 过流报警
            u16 over_voltage_alarm : 1;                      // 过压报警
            u16 leakage_protection_function_normal : 1;      // 漏电保护功能正常
            u16 leakage_protection_self_check_completed : 1; // 漏电保护自检完成
            u16 phaseloss_alarm : 1;                         // 输入缺相报警
            u16 fire_alarm : 1;                              // 打火报警
            u16 under_voltage_alarm : 1;                     // 欠压报警
            u16 over_voltage_warning : 1;                    // 过压预警
            u16 under_voltage_warning : 1;                   // 欠压预警
            u16 leakage_warning : 1;                         // 漏电预警
            u16 electricity_warning : 1;                     // 电流预警
        };
        u16 alarm;
    } alarm_t; // 告警位

    u16 power_low_byte;  // 电量低字节
    u16 power_high_byte; // 电量高字节
    u16 a_phase_voltage; // a相电压
    u16 b_phase_voltage; // b相电压
    u16 c_phase_voltage; // c相电压
    u16 a_electricity;   // a相电流
    u16 b_electricity;   // b相电流
    u16 c_electricity;   // c相电流
    u16 n_electricity;   // n相电流
    u16 a_phase_power;   // a相功率
    u16 b_phase_power;   // b相功率
    u16 c_phase_power;   // c相功率
    union {
        struct
        {
            u16 shortcircuit_alarm : 1; // 短路报警
            u16 bit1 : 1;
            u16 overload_alarm : 1;       // 过载报警
            u16 temperature_t3_alarm : 1; // 温度报警
            u16 bit4 : 1;
            u16 over_electricity_alarm : 1; // 过流报警
            u16 over_voltage_alarm : 1;     // 过压报警
            u16 bit7 : 1;
            u16 bit8 : 1;
            u16 phaseloss_alarm : 1;       // 输入缺相报警
            u16 fire_alarm : 1;            // 打火报警
            u16 under_voltage_alarm : 1;   // 欠压报警
            u16 over_voltage_warning : 1;  // 过压预警
            u16 under_voltage_aarning : 1; // 欠压预警
            u16 bit14 : 1;
            u16 electricity_warning : 1; // 电流预警
        };
        u16 phase_alarm;
    } a_phase_alarm_t; // a相告警

    union {
        struct
        {
            u16 shortcircuit_alarm : 1; // 短路报警
            u16 bit1 : 1;
            u16 overload_alarm : 1;       // 过载报警
            u16 temperature_t3_alarm : 1; // 温度报警
            u16 bit4 : 1;
            u16 over_electricity_alarm : 1; // 过流报警
            u16 over_voltage_alarm : 1;     // 过压报警
            u16 bit7 : 1;
            u16 bit8 : 1;
            u16 phaseloss_alarm : 1;       // 输入缺相报警
            u16 fire_alarm : 1;            // 打火报警
            u16 under_voltage_alarm : 1;   // 欠压报警
            u16 over_voltage_warning : 1;  // 过压预警
            u16 under_voltage_aarning : 1; // 欠压预警
            u16 bit14 : 1;
            u16 electricity_warning : 1; // 电流预警
        };
        u16 phase_alarm;
    } b_phase_alarm_t; // b相告警

    union {
        struct
        {
            u16 shortcircuit_alarm : 1; // 短路报警
            u16 bit1 : 1;
            u16 overload_alarm : 1;       // 过载报警
            u16 temperature_t3_alarm : 1; // 温度报警
            u16 bit4 : 1;
            u16 over_electricity_alarm : 1; // 过流报警
            u16 over_voltage_alarm : 1;     // 过压报警
            u16 bit7 : 1;
            u16 bit8 : 1;
            u16 phaseloss_alarm : 1;       // 输入缺相报警
            u16 fire_alarm : 1;            // 打火报警
            u16 under_voltage_alarm : 1;   // 欠压报警
            u16 over_voltage_warning : 1;  // 过压预警
            u16 under_voltage_aarning : 1; // 欠压预警
            u16 bit14 : 1;
            u16 electricity_warning : 1; // 电流预警
        };
        u16 phase_alarm;
    } c_phase_alarm_t;          // c相告警
    u16 bit_state;              // 分合闸状态
    u16 a_phase_power_factor;   // a相功率因数
    u16 b_phase_power_factor;   // b相功率因数
    u16 c_phase_power_factor;   // c相功率因数
    u16 a_phase_temperature;    // a相温度
    u16 b_phase_temperature;    // b相温度
    u16 c_phase_temperature;    // c相温度
    u16 n_phase_temperature;    // n相温度
    u16 a_Terminal_temperature; // 端子温度a
    u16 b_terminal_temperature; // 端子温度b
    u16 c_terminal_temperature; // 端子温度c
    u16 n_terminal_temperature; // 端子温度n	0x1C(28)
} slave_realtime_data_t;

typedef struct as_an001 {
    u8 number;
    u8 run_mode;                             // 运行模式  1进入地址设置模式 0正常模式
    u8 is_alive[DEFAULT_AS_NUMBER];          // 空开位置
    u8 open_state[DEFAULT_AS_NUMBER];        // 空开分合闸状态
    u8 is_remote_control[DEFAULT_AS_NUMBER]; // 空开远程使能状态

    // bit_16_t is_alive;				 //空开位置
    // bit_16_t open_state;			 //空开分合闸状态
    // bit_16_t is_remote_control;		 //空开远程使能状态
    // bit_16_t is_user_remote_control; //空开用户远程使能状态
    slave_realtime_data_t slave_realtime_data[DEFAULT_AS_NUMBER]; // 空开实时数据
    slave_param_t slave_param[DEFAULT_AS_NUMBER];                 // 空开参数

    u32 period_refresh_real_time;   // 周期刷新时间
    u32 period_upload_real_time;    // 周期上报时间
    u32 period_upload_real_timeout; // 周期上报时间计时
    u32 addr_mode_timeout;          // 地址设置模式运行超时事件
    u16 change_mask;                // 变化掩码，bit j = 空开j有待上报的变化

} as_an001_s;

extern struct as_an001 as_an001_t;

u16 crc_cal_value(u8 *data_value, u16 data_length);
#if !FUNC_BOARD_VERSION_CL
ErrStatus as_check_crc16(u8 *buf, u16 len);
#else
ErrorStatus as_check_crc16(u8 *buf, u16 len);
#endif
u8 as_check_slave_number(void);
void as_get_slave_info(as_event_t event);
void as_period_upload(void);
void as_write_cmd(u8 event, u8 addr, u8 cmd);
void as_event(void);

#endif
