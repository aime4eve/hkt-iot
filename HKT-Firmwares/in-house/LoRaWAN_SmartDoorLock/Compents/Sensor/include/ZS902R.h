
#ifndef __ZS902R_H__
#define __ZS902R_H__

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if FUNC_FINGERPRINT_ENABLE

#define FINGER_BIT_DISPLAY_STOP (1 << 0)
#define FINGER_BIT_DISPLAY_GREEN (1 << 1)
#define FINGER_BIT_DISPLAY_BLUE (1 << 2)
#define FINGER_BIT_DISPLAY_RED (1 << 3)
#define FINGER_BIT_SLEEP (1 << 4)
#define FINGER_BIT_SLEEP_DONE (1 << 5)
#define FINGER_BIT_IDLE (1 << 6)

#define FINGER_BIT_DISPLAY_ALL (FINGER_BIT_DISPLAY_STOP | FINGER_BIT_DISPLAY_GREEN | FINGER_BIT_DISPLAY_BLUE | FINGER_BIT_DISPLAY_RED | FINGER_BIT_IDLE | FINGER_BIT_SLEEP | FINGER_BIT_SLEEP_DONE)

enum zs902r_validation_code {
    ZS_VALIDATION_CODE_EXEC_FINSH = 0,                  // 执行成功
    ZS_VALIDATION_CODE_PACK_RECV_ERROR,                 // 数据包接收错误
    ZS_VALIDATION_CODE_NO_FINGER,                       // 传感器上无手指
    ZS_VALIDATION_CODE_INPUT_IMAGE_FAIL,                // 录入指纹图像失败
    ZS_VALIDATION_CODE_SPANNING_CHARACTERISTICS_FAIL_1, // 指纹图像太干、太淡而生不成特征
    ZS_VALIDATION_CODE_SPANNING_CHARACTERISTICS_FAIL_2, // 指纹图像太湿、太糊而生不成特征
    ZS_VALIDATION_CODE_SPANNING_CHARACTERISTICS_FAIL_3, // 指纹图像太乱而生不成特征
    ZS_VALIDATION_CODE_SPANNING_CHARACTERISTICS_FAIL_4, // 指纹图像正常，但特征点太少（或面积太小）而生不成特征
    ZS_VALIDATION_CODE_FINGER_NO_MATCH,                 // 指纹不匹配
    ZS_VALIDATION_CODE_FINGER_NOT_FIND,                 // 未搜索到指纹
    ZS_VALIDATION_CODE_MERGE_CHARACTERISTICS_FAIL,      // 特征合并失败
    ZS_VALIDATION_CODE_ADDR_FAIL,                       // 访问指纹库时地址序号超出指纹库范围
    ZS_VALIDATION_CODE_READ_TEMPLATE_ERROR,             // 从指纹库读模板出错或无效
    ZS_VALIDATION_CODE_UPLOAD_CHARACTERISTICS_FAIL,     // 上传特征失败
    ZS_VALIDATION_CODE_CAN_NOT_RECV_NEXT_DATA,          // 模组不能接收后续数据包
    ZS_VALIDATION_CODE_UPLOAD_IMAGE_FAIL,               // 上传图像失败
    ZS_VALIDATION_CODE_DELETE_TEMPLATE_FAIL,            // 删除模板失败
    ZS_VALIDATION_CODE_CLEAR_FINGER_FAIL,               // 清空指纹库失败
    ZS_VALIDATION_CODE_ENTER_LOW_POWER_MODE_FAIL,       // 不能进入低功耗状态
    ZS_VALIDATION_CODE_CMD_ERROR,                       // 口令不正确
    ZS_VALIDATION_CODE_RESET_SYSTEM_ERROR,              // 系统复位失败
    ZS_VALIDATION_CODE_MERGE_IMAGE_FAIL,                // 缓冲区内没有有效原始图而生不成图像
    ZS_VALIDATION_CODE_OTA_FAIL,                        // 在线升级失败
    ZS_VALIDATION_CODE_FINGER_NOT_MOVE,                 // 残留指纹或两次采集之间手指没有移动过
    ZS_VALIDATION_CODE_WR_FLASH_ERROR,                  // 读写FLASH出错
    ZS_VALIDATION_CODE_RANDOM_NUM_FAIL,                 // 随机数生成失败
    ZS_VALIDATION_CODE_REGISTER_INVALID,                // 无效寄存器号
    ZS_VALIDATION_CODE_REGISTER_DATA_ERROR,             // 寄存器设定内容错误号
    ZS_VALIDATION_CODE_NOTE_PAGE_ERROR,                 // 记事本页码指定错误
    ZS_VALIDATION_CODE_PORT_EXCE_ERROR,                 // 端口操作失败
    ZS_VALIDATION_CODE_AUTO_ENROLL_ERROR,               // 自动注册失败
    ZS_VALIDATION_CODE_FINGER_FULL,                     // 指纹库满
    ZS_VALIDATION_CODE_DEVICE_ADDR_ERROR,               // 设备地址错误
    ZS_VALIDATION_CODE_PASSWORD_ERROR,                  // 密码有误
    ZS_VALIDATION_CODE_FINGER_TEMPLATE_VALID,           // 指纹模板非空
    ZS_VALIDATION_CODE_FINGER_TEMPLATE_INVALID,         // 指纹模板为空
    ZS_VALIDATION_CODE_FINGER_INVALID,                  // 指纹库为空
    ZS_VALIDATION_CODE_SET_INPUT_CNT_ERROR,             // 录入次数设置错误
    ZS_VALIDATION_CODE_TIMEOUT,                         // 超时
    ZS_VALIDATION_CODE_FINGER_ALREADY_EXISTED,          // 指纹已经存在
    ZS_VALIDATION_CODE_FINGER_CHARACTERISTICS_RELEVANT, // 指纹特征有关联
    ZS_VALIDATION_CODE_INIT_FAIL,                       // 传感器初始化失败
    ZS_VALIDATION_CODE_INFO_VALID,                      // 模组信息非空
    ZS_VALIDATION_CODE_INFO_INVALID,                    // 模组信息为空
    ZS_VALIDATION_CODE_OTP_FAIL,                        // OTP 操作失败
    ZS_VALIDATION_CODE_SECRET_KEY_CREAT_FAIL,           // 秘钥生成失败
    ZS_VALIDATION_CODE_SECRET_KEY_VALID,                // 秘钥不存在
    ZS_VALIDATION_CODE_NTRU_EXCE_FAIL,                  // 安全算法执行失败
    ZS_VALIDATION_CODE_NTRU_RESULT_ERROR,               // 安全算法加解密结果有误
    ZS_VALIDATION_CODE_FUNC_SECRET_LEVEL_NOT_MATCH,     // 功能与加密等级不匹配
    ZS_VALIDATION_CODE_SECRET_KEY_LOCKED,               // 密钥已锁定
    ZS_VALIDATION_CODE_IMAGE_SMALL,                     // 图像面积小
};

extern TX_EVENT_FLAGS_GROUP zs_display_event_group;

u8 read_template(u16 location_num);
u8 delete_template(u16 page, u16 delete_num);
void clear_fingerprint_maps(void);
u8 enter_sleep_mode(void);
u8 finger_led_control(u8 mode, u8 start_color, u8 end_color, u8 cycle);
u8 auto_identify(u16 *id_num);
void thread_fingerprint(ULONG thread_input);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
