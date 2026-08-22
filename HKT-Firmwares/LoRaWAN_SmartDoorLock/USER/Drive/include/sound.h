
#ifndef __SOUND_H__
#define __SOUND_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

    /*
    开锁提示音
    关锁提示音
    警报音
    失败提示音
    键盘触摸提示音


    0
    1
    2
    3
    4
    5
    6
    7
    8
    9
    已开锁
    已关锁
    操作成功
    操作失败
    开锁失败
    密码用户
    卡用户
    指纹用户
    重复用户
    存量已满
    录入成功
    录入管理信息
    录入用户信息
    请输入编号
    请放手指
    请输入新密码
    请刷卡
    请输入管理员密码
    请输入管理员信息
    请再次输入
    删除成功
    删除用户信息
    系统初始化
    系统设置
    系统已锁定
    */
    typedef enum
    {
        VOICE_TONE_OPEN_LOCK = 1, // 开锁提示音
        VOICE_TONE_OFF_LOCK,      // 关锁提示音
        VOICE_TONE_ALARM,         // 警报音
        VOICE_TONE_FAIL,          // 失败提示音
        VOICE_TONE_TOUCH,         // 键盘触摸提示音
        VOICE_TONE_DINGDONG,      // 门铃声
        VOICE_NUM_0 = 10,
        VOICE_NUM_1,
        VOICE_NUM_2,
        VOICE_NUM_3,
        VOICE_NUM_4,
        VOICE_NUM_5,
        VOICE_NUM_6,
        VOICE_NUM_7,
        VOICE_NUM_8,
        VOICE_NUM_9,
        VOICE_OPEN_LOCK,           // 已开锁
        VOICE_OFF_LOCK,            // 已关锁
        VOICE_EXCE_SUCCESS,        // 操作成功
        VOICE_EXCE_FAIL,           // 操作失败
        VOICE_OPEN_LOCK_FAIL,      // 开锁失败
        VOICE_PASSWORD_USER,       // 密码用户
        VOICE_CARD_USER,           // 卡用户
        VOICE_FINGERPRINT_USER,    // 指纹用户
        VOICE_REPEAT_USER,         // 重复用户
        VOICE_FULL_STOCK,          // 存量已满
        VOICE_INPUT_SUCCESS,       // 录入成功
        VOICE_INPUT_ROOT_INFO,     // 录入管理信息
        VOICE_INPUT_USER_INFO,     // 录入用户信息
        VOICE_INPUT_NUMBER,        // 请输入编号
        VOICE_INPUT_FINGER,        // 请放手指
        VOICE_INPUT_NEW_PASSWORD,  // 请输入新密码
        VOICE_INPUT_CARD,          // 请刷卡
        VOICE_INPUT_ROOT_PASSWORD, // 请输入管理员密码
        VOICE_PLS_INPUT_ROOT_INFO, // 请输入管理员信息
        VOICE_INPUT_SECOND,        // 请再次输入
        VOICE_DELETE_SUCCESS,      // 删除成功
        VOICE_DELETE_USER_INFO,    // 删除用户信息
        VOICE_SYSTEM_INIT,         // 系统初始化
        VOICE_SYSTEM_CONFIG,       // 系统设置
        VOICE_SYSTEM_LOCK,         // 系统已锁定
        VOICE_LOW_BATTERY,         // 电量低
        VOICE_VOLUME_ADD,          // 音量加
        VOICE_VOLUME_LESS,         // 音量减
        VOICE_END_WITH_HASH_KEY,   // 以#号键结束
        VOICE_AUTO_NUMBER,         // 按#号键自动编号
        VOICE_EXECUTING,           // 执行中,请稍后
        VOICE_HELP_INFO,           // 返回请按*号键,确认请按#号键
        VOICE_NETWORK_ON,          // 联网开启
        VOICE_NETWORK_OFF,         // 联网关闭

        VOICE_VOLUME1 = 0xE0,      // 音量1       //最小
        VOICE_VOLUME2,             // 音量2
        VOICE_VOLUME3,             // 音量3
        VOICE_VOLUME4,             // 音量4
        VOICE_VOLUME5,             // 音量5
        VOICE_VOLUME6,             // 音量6
        VOICE_VOLUME7,             // 音量7
        VOICE_VOLUME8,             // 音量8       //最大
        VOICE_STOP = 0xFE,         // STOP
    } VoiceSentence;

    // 执行中，请稍后
    // 电量低
    // 音量加
    // 音量减

    typedef struct loudspeaker
    {
        u8 timely : 1; // 实时播放
        u8 volume_number;
        u8 voice_number;
        u8 voice_play_index; // 正在播放语音序号
        u8 voice_list[20];   // 保存播放语音列表
    } loudspeaker_t;

    typedef struct insertspeaker
    {
        u8 insert : 1; // 选择是否加入播放列表
        u8 voice_index;
    } insertspeaker_t;

    extern loudspeaker_t sound_t;

    void Sound_Play(u8 num);
    void Sound_Insert_Number(u8 insert, u8 voice_index);
    void Sound_Init(void);
    void thread_sound_play(ULONG thread_input);

#ifdef __cplusplus
}
#endif

#endif
