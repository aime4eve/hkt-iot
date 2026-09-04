
/* Includes ------------------------------------------------------------------*/
#include "acc.h"
#include "control_center.h"
#include "math.h"
#include "systick.h"
#include "uart.h"

static unsigned int f_step = 0;

static int32_t acc_iic_read(uint8_t reg, uint8_t *bufp, uint16_t len)
{
    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        ;
    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置
    LL_I2C_HandleTransfer(I2C1, (ACC_I2C_ADDRESS << 1), LL_I2C_ADDRSLAVE_7BIT, 1, I2C_AUTOEND_MODE, I2C_GENERATE_START_WRITE);

    // 等待发送完成
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
        ;
    // 写数据
    LL_I2C_TransmitData8(I2C1, reg);

    LL_I2C_HandleTransfer(I2C1, (ACC_I2C_ADDRESS << 1) | 1, LL_I2C_ADDRSLAVE_7BIT, len, I2C_AUTOEND_MODE, I2C_GENERATE_START_READ);

    // 读取数据
    for (int i = 0; i < len; i++) {
        // 等待接收非空
        while (!LL_I2C_IsActiveFlag_RXNE(I2C1))
            ;
        *(bufp + i) = LL_I2C_ReceiveData8(I2C1);
    }

    // 等待停止完成
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
        ;
    LL_I2C_ClearFlag_STOP(I2C1);

    return 0;
}

static int32_t acc_iic_write(uint8_t reg, uint8_t *bufp, uint16_t len)
{
    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        ;
    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置为写
    LL_I2C_HandleTransfer(I2C1, ACC_I2C_ADDRESS << 1, LL_I2C_ADDRSLAVE_7BIT, len, I2C_AUTOEND_MODE, I2C_GENERATE_START_WRITE);
    // 等待发送完成
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
        ;
    LL_I2C_TransmitData8(I2C1, reg);
    // 写数据
    for (int i = 0; i < len - 1; i++) {
        // 等待发送完成
        while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
            ;
        LL_I2C_TransmitData8(I2C1, *(bufp + i));
    }
    // 等待停止完成
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
        ;
    LL_I2C_ClearFlag_STOP(I2C1);
    return 0;
}

/*交换两个整数的值*/
void swap(short *p, short *q)
{
    short buf;
    buf = *p;
    *p = *q;
    *q = buf;
    return;
}

/*快速排序*/
void quick_sort(short *a, int low, int high)
{
    int i = low;
    int j = high;
    short key = a[low];
    if (low >= high) // 如果low >= high说明排序结束了
    {
        return;
    }
    while (low < high) // 该while循环结束一次表示比较了一轮
    {
        while (low < high && key <= a[high]) {
            --high; // 向前寻找
        }
        if (key > a[high]) {
            swap(&a[low], &a[high]);
            ++low;
        }
        while (low < high && key >= a[low]) {
            ++low; // 向后寻找
        }
        if (key < a[low]) {
            swap(&a[low], &a[high]);
            --high;
        }
    }
    quick_sort(a, i, low - 1); // 用同样的方式对分出来的左边的部分进行同上的做法
    quick_sort(a, low + 1, j); // 用同样的方式对分出来的右边的部分进行同上的做法
}

/*查找一个有序数组中的众数*/
short find_mode_number(short *arr, int len)
{
    short many = 1, less = 1;
    short value = 0;
    for (int i = 0; i < len; i++) {
        for (int j = i; j < len; j++) {
            if (arr[j] == arr[j + 1]) {
                less++;
            } else {
                if (many < less) {
                    swap(&many, &less);
                    value = arr[j];
                }
                less = 1;
                break;
            }
        }
    }
    return value;
}

// 软件滤波
short getAccRaw(short *accVals, int cnt)
{
    int value_collection = 0;
    short max = accVals[0];
    short min = accVals[0];

    for (int n = 0; n < cnt; n++) {
        max = (accVals[n] < max) ? max : accVals[n];
        min = (min < accVals[n]) ? min : accVals[n];
        value_collection += accVals[n];
    }
    value_collection = (value_collection - max - min) / (cnt - 2);
    if (value_collection == 0)
        value_collection = 1;
    return value_collection;
}

short getAccCentre(short *accVals, int cnt)
{
    int value = 0;
    for (int i = 0; i < 12 - 6; i++) {
        value += accVals[i + 3];
    }
    value /= 12 - 6;
    return value;
}

int mir3da_register_read(uint8_t addr, uint8_t *data_m, uint8_t len)
{
    // To do i2c read api
    int rc = 0;
    acc_iic_read(addr, data_m, len);
    return rc;
}

int mir3da_register_write(uint8_t addr, uint8_t data_m)
{
    int rc = 0;
    // To do i2c write api

    acc_iic_write(addr, &data_m, 2);
    return rc;
}

// Initialization
int mir3da_init(void)
{
    int res = 0;
    uint8_t data_m = 0;

    // Retry 3 times
    res = mir3da_register_read(NSA_REG_WHO_AM_I, &data_m, 1);
    if (data_m != 0x13) {
        res = mir3da_register_read(NSA_REG_WHO_AM_I, &data_m, 1);
        if (data_m != 0x13) {
            res = mir3da_register_read(NSA_REG_WHO_AM_I, &data_m, 1);
            if (data_m != 0x13) {
                INFO("\r\n------mir3da read chip id  error= %x-----\r\n", data_m);
                return -1;
            }
        }
    }

    INFO("\r\n------mir3da chip id = %x-----\r\n", data_m);
    res |= mir3da_register_write(NSA_REG_G_RANGE, 0x00); //+/-2G,14bit
    // res |= mir3da_register_write(NSA_REG_G_RANGE, 0x80); //+/-2G,14bit  //开启高通滤波器
    // res |= mir3da_register_write(NSA_REG_POWERMODE_BW, 0x95);          //sleep mode
    res |= mir3da_register_write(NSA_REG_POWERMODE_BW, 0x01); // normal mode  1/2 ODR
    res |= mir3da_register_write(NSA_REG_ODR_AXIS_DISABLE, 0x07); // ODR = 125hz
    // res |= mir3da_register_write(NSA_REG_ODR_AXIS_DISABLE, 0x05); // ODR = 31.25hz
    res |= mir3da_register_write(NSA_REG_FIFO_CTRL, 0x80);

    // // Engineering mode
    // res |= mir3da_register_write(NSA_REG_ENGINEERING_MODE, 0x83);
    // res |= mir3da_register_write(NSA_REG_ENGINEERING_MODE, 0x69);
    // res |= mir3da_register_write(NSA_REG_ENGINEERING_MODE, 0xBD);

    // Reduce power consumption
    if (ACC_I2C_ADDRESS == 0x26) {
        mir3da_register_write(NSA_REG_SENS_COMP, 0x00);
    }
    return res;
}

int mir3da_open_step_counter(void)
{
    int res = 0;
    // res |= mir3da_register_write(NSA_REG_STEP_CONGIF1, 0x01);
    // res |= mir3da_register_write(NSA_REG_STEP_CONGIF2, 0x62);
    // res |= mir3da_register_write(NSA_REG_STEP_CONGIF3, 0x46);
    // res |= mir3da_register_write(NSA_REG_STEP_CONGIF4, 0x33);
    // res |= mir3da_register_write(NSA_REG_STEP_FILTER, 0xa2);

    res |= mir3da_register_write(NSA_REG_STEP_CONGIF1, 0x01);
    res |= mir3da_register_write(NSA_REG_STEP_CONGIF2, 0x00);
    res |= mir3da_register_write(NSA_REG_STEP_CONGIF3, 0x00);
    res |= mir3da_register_write(NSA_REG_STEP_CONGIF4, 0x00);
    res |= mir3da_register_write(NSA_REG_STEP_FILTER, 0x80);
    return res;
}

int mir3da_close_step_counter(void)
{ 
     
    int res = 0;
    res = mir3da_register_write(NSA_REG_STEP_FILTER, 0x22);
    // res = mir3da_register_write(NSA_REG_STEP_FILTER, 0x0);
    return res;
}

int mir3da_get_step(void)
{
    unsigned char tmp_data[2] = {0};
    if (mir3da_register_read(NSA_REG_STEPS_MSB, tmp_data, 2) == 0)
        f_step = (tmp_data[0] << 8 | tmp_data[1]) / 2;
    return f_step;
}

int mir3da_clear_step(void)
{
    int res = 0;
    res = mir3da_register_write(NSA_REG_RESET_STEP, 0x80);
    f_step = 0;
    return res;
}

// enable/disable the chip
int mir3da_set_enable(uint8_t enable)
{
    int res = 0;
    if (enable)
        // res = mir3da_register_write(NSA_REG_POWERMODE_BW,0x30);
        res = mir3da_register_write(NSA_REG_POWERMODE_BW, 0x0);
    else
        res = mir3da_register_write(NSA_REG_POWERMODE_BW, 0x80);
    return res;
}

// Read three axis data, 1024 LSB = 1 g
int mir3da_read_data(short *x, short *y, short *z)
{
    uint8_t tmp_data[6] = {0};
    if (mir3da_register_read(NSA_REG_ACC_X_LSB, tmp_data, 6) != 0) {
        return -1;
    }
    *x = ((short)(tmp_data[1] << 8 | tmp_data[0])) >> 4;
    *y = ((short)(tmp_data[3] << 8 | tmp_data[2])) >> 4;
    *z = ((short)(tmp_data[5] << 8 | tmp_data[4])) >> 4;
    return 0;
}

int mir3da_new_data_ready(void)
{
    uint8_t tmp_data;
    if (mir3da_register_read(NSA_REG_NEWDATA_FLAG, &tmp_data, 1) != 0) {
        return -1;
    }
    if (tmp_data != 1)
        return -1;
    return 0;
}

int mir3da_open_active_interrupt(uint8_t th)
{
    int res = 0;
    res |= mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS1, 0x87);
    res |= mir3da_register_write(NSA_REG_ACTIVE_DURATION, 0x00);
    res |= mir3da_register_write(NSA_REG_ACTIVE_THRESHOLD, th);
    res |= mir3da_register_write(NSA_REG_INTERRUPT_MAPPING1, 0x04);
    res |= mir3da_register_write(NSA_REG_INT_LATCH, 0x01); // 中断锁存250ms
    return res;
}

int mir3da_close_active_interrupt(void)
{
    int res = 0;
    res = mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS1, 0x00);
    res |= mir3da_register_write(NSA_REG_INTERRUPT_MAPPING1, 0x00);
    return res;
}

int mir3da_open_sm_interrupt(uint8_t th)
{
    int res = 0;
    res = mir3da_register_write(NSA_REG_SM_THRESHOL, th);
    res |= mir3da_register_write(NSA_REG_INTERRUPT_MAPPING1, 0x80);
    res |= mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS0, 0x02);
    return res;
}

int mir3da_close_sm_interrupt(void)
{
    int res = 0;
    res = mir3da_register_write(NSA_REG_INTERRUPT_MAPPING1, 0x00);
    res |= mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS0, 0x00);
    return res;
}

void Acc_Handler(void)
{
    static u32 covert_time;
    static raw_data_t filter_raw[FILTER_CNT];
    static u8 count = 0;
    raw_data_t raw;
    raw_data_t sample;

    if (covert_time >= get_syspant_ms())
        return;
    covert_time = get_syspant_ms() + 100;
    // read raw data sample
    if (mir3da_new_data_ready() != 0)
        return;
    if (mir3da_read_data(&raw.ax, &raw.ay, &raw.az) != 0)
        return;
    INFO("\nx= %d, y= %d, z=%d\n", raw.ax, raw.ay, raw.az);
    // INFO("%d,%d,%d\n", raw.ax, raw.ay, raw.az);

    filter_raw[count].ax = raw.ax;
    filter_raw[count].ay = raw.ay;
    filter_raw[count].az = raw.az;
    count++;

#if 1
    short x_value[FILTER_CNT];
    short y_value[FILTER_CNT];
    short z_value[FILTER_CNT];

    if (count >= FILTER_CNT - 1) {
        count = 0;

        for (u8 i = 0; i < FILTER_CNT; i++) {
            x_value[i] = filter_raw[i].ax;
            y_value[i] = filter_raw[i].ay;
            z_value[i] = filter_raw[i].az;
        }
        int step = mir3da_get_step();
        INFO("\r\n--- get step: %d---\r\n", step);

        quick_sort(x_value, 0, sizeof(x_value) / sizeof(short) - 1);
        quick_sort(y_value, 0, sizeof(y_value) / sizeof(short) - 1);
        quick_sort(z_value, 0, sizeof(z_value) / sizeof(short) - 1);
        raw.ax = getAccCentre(x_value, FILTER_CNT);
        raw.ay = getAccCentre(y_value, FILTER_CNT);
        raw.az = getAccCentre(z_value, FILTER_CNT);
#endif
    }
    // INFO("\nx= %d, y= %d, z=%d", raw.ax, raw.ay, raw.az);
    // INFO("%d,%d,%d\n", raw.ax, raw.ay, raw.az);

    sample.ax = FS_2g_HR_TO_mg(raw.ax);
    sample.ay = FS_2g_HR_TO_mg(raw.ay);
    sample.az = FS_2g_HR_TO_mg(raw.az);
    // INFO("\nx= %d, y= %d, z=%d", sample.x, sample.y, sample.z);

    float roll = (float)(atan2((float)(sample.ax), sample.ax) * DEG_PER_RAD);  // 转换为度数
    float pitch = (float)(atan2((float)(sample.ay), sample.az) * DEG_PER_RAD); // 转换为度数
    if (roll < 0)
        roll += 360;
    if (pitch < 0)
        pitch += 360;
}

void Acc_Init(void)
{
    mir3da_init();
    mir3da_open_step_counter();
}

void convertAccData(void)
{
    raw_data_t raw;
    raw_data_t sample;
    while (mir3da_new_data_ready() != 0)
        ;
    while (mir3da_read_data(&raw.ax, &raw.ay, &raw.az) != 0)
        ;
    sample.ax = FS_2g_HR_TO_mg(raw.ax);
    sample.ay = FS_2g_HR_TO_mg(raw.ay);
    sample.az = FS_2g_HR_TO_mg(raw.az);

    device_t.sensor_t.ax = sample.ax;
    device_t.sensor_t.ay = sample.ay;
    device_t.sensor_t.az = sample.az;

    int step = mir3da_get_step();
    mir3da_clear_step();
    device_t.sensor_t.gastricMotility += step;
    DEBUG_TRACE(LOG_TAG, "Convert X= %d, Y= %d, Z= %dG, Step = %d %d", device_t.sensor_t.ax, device_t.sensor_t.ay, device_t.sensor_t.az, step, device_t.sensor_t.gastricMotility);
}
