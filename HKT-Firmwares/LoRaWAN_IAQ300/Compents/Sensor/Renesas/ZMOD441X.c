
#include "zmod441x.h"

#include "zmod4410_config_iaq2.h"
#include "zmod4xxx.h"
#include "zmod4xxx_cleaning.h"
//#include "zmod4xxx_hal.h"
#include "iaq_2nd_gen.h"
#include "zmod4xxx_i2c_hal.h"

#include "uart.h"
#include "control_center.h"
#include "systick.h"
#include "iic.h"

/* Counter to check POR Event */
static uint32_t polling_counter = 0;

/* Sensor target variables */
static zmod4xxx_dev_t dev;
static uint8_t zmod4xxx_status;
static uint8_t prod_data[ZMOD4410_PROD_DATA_LEN];
static uint8_t adc_result[32] = {0};
static iaq_2nd_gen_handle_t algo_handle;
static iaq_2nd_gen_results_t algo_results;

/**
 * @brief	初始化TVOC传感器
 * @param
 * @retval
 */
void ZMOD4410_Init(void)
{
    int8_t ret;
    u8 count = 0;

    /****TARGET SPECIFIC FUNCTION ****/
    /*
     * Customers have to write their own init_hardware function in
     * init_hardware (located in dependencies/zmod4xxx_api/HAL directory)
     * variable of dev has *read, *write and *delay function pointer. Three
     * functions have to be generated for corresponding hardware and assign
     * in init_hardware. For more information, read the programming manual.
     */
    dev.read = zmod4xxx_i2c_read;
    dev.write = zmod4xxx_i2c_write;
    dev.delay_ms = zmod4xxx_sleep;
    /****TARGET SPECIFIC FUNCTION ****/

    /* Sensor Related Data */
    dev.i2c_addr = ZMOD4410_I2C_ADDR;
    dev.pid = ZMOD4410_PID;
    dev.init_conf = &zmod_sensor_type[INIT];
    dev.meas_conf = &zmod_sensor_type[MEASUREMENT];
    dev.prod_data = prod_data;

    ret = zmod4xxx_read_sensor_info(&dev);
    if (ret)
    {
        DEBUG_TRACE(ERROR_TAG, "Error %d during reading zmod4410 information, exiting program!", ret);
        return;
    }

    /* This function starts the cleaning procedure. It's
     * recommended to be executed after product assembly. This
     * helps to clean the metal oxide surface from assembly.
     * IMPORTANT NOTE: The cleaning procedure can be run only once
     * during the modules lifetime and takes 10 minutes. */

    // ret = zmod4xxx_cleaning_run(&dev);
    // if (ret) {
    //     DEBUG_TRACE(ERROR_TAG,"Error %d during cleaning procedure, exiting program!\n", ret);
    //     return;
    // }

    /* Preperation of sensor */
    ret = zmod4xxx_prepare_sensor(&dev);
    if (ret)
    {
        DEBUG_TRACE(ERROR_TAG, "Error %d during preparation of the zmod4410, exiting program!", ret);
        return;
    }

    /* One time initialization of the algorithm */
    ret = init_iaq_2nd_gen(&algo_handle);
    if (ret)
    {
        DEBUG_TRACE(ERROR_TAG, "Error %d when zmod4410 initializing algorithm, exiting program!", ret);
        return;
    }

    ret = zmod4xxx_start_measurement(&dev);
    if (ret)
    {
        DEBUG_TRACE(ERROR_TAG, "Error %d when zmod4410 starting measurement, exiting program!", ret);
        return;
    }
    // do
    // {
    /* Instead of polling STATUS REGISTER, the INTERRUPT PIN can be used.
     * For more information, please look at Interrupt Usage chapter
     * in Programming Manual */
    count++;
    do
    {
        ret = zmod4xxx_read_status(&dev, &zmod4xxx_status);
        if (ret)
        {
            DEBUG_TRACE(ERROR_TAG, "Error %d during read of zmod4410 status, exiting program!", ret);
            return;
        }
        /* Increase polling counter */
        polling_counter++;
        /* Delay for 200 milliseconds */
        dev.delay_ms(200);
    } while ((zmod4xxx_status & STATUS_SEQUENCER_RUNNING_MASK) &&
             (polling_counter <= ZMOD4410_IAQ2_COUNTER_LIMIT));

    /* Check if timeout has occured */
    if (ZMOD4410_IAQ2_COUNTER_LIMIT <= polling_counter)
    {
        ret = zmod4xxx_check_error_event(&dev);
        if (ret)
        {
            DEBUG_TRACE(ERROR_TAG, "Error %d during read of zmod4410 status, exiting program!", ret);
            return;
        }
        DEBUG_TRACE(ERROR_TAG, "Error %d, exiting program!\n", ret);
        return;
    }
    else
    {
        polling_counter = 0;
    }

    ret = zmod4xxx_read_adc_result(&dev, adc_result);
    if (ret)
    {
        DEBUG_TRACE(ERROR_TAG, "Error %d during read zmod4410 of ADC results, exiting program!", ret);
        return;
    }

    /* calculate the algorithm */
    ret = calc_iaq_2nd_gen(&algo_handle, &dev, adc_result, &algo_results);
    if ((ret != IAQ_2ND_GEN_OK) && (ret != IAQ_2ND_GEN_STABILIZATION))
    {
        DEBUG_TRACE(ERROR_TAG, "Error %d when zmod4410 calculating algorithm, exiting program!", ret);
        return;
        /* IAQ 2nd Gen algorithm skips first 60 samples for stabilization */
    }
    else
    {
        // INFO("*********** Measurements ***********\n");
        // for (int i = 0; i < 13; i++)
        // {
        //     INFO(" Rmox[%d] = ", i);
        //     INFO("%.3f kOhm\n", algo_results.rmox[i] / 1e3);
        // }
        // INFO(" log_Rcda = %5.3f logOhm\n", algo_results.log_rcda);
        // INFO(" EtOH = %6.3f ppm\n", algo_results.etoh);
        // INFO(" TVOC = %6.3f mg/m^3\n", algo_results.tvoc);
        // INFO(" eCO2 = %4.0f ppm\n", algo_results.eco2);
        // INFO(" IAQ  = %4.1f\n", algo_results.iaq);
        DEBUG_TRACE(LOG_TAG, " ZMOD44XX Convert TVOC Results: %f mg/m^3, IAQ: %4.1f, Level: %d, Count: %d", algo_results.tvoc, algo_results.iaq, device_t.sensor_t.tvoc_level, count);
        if (ret == IAQ_2ND_GEN_STABILIZATION)
        {
            DEBUG_TRACE(WARN_TAG, "ZMOD44XX Warmup...");
        }
        else
        {
            DEBUG_TRACE(WARN_TAG, "ZMOD44XX Valid...");
            device_t.sensor_t.tvoc = algo_results.tvoc * 1000;
            //       break;
        }

        // INFO("************************************\n");
    }

    // /* wait 1.99 seconds before starting the next measurement */
    // dev.delay_ms(1990);

    // /* start a new measurement before result calculation */
    // ret = zmod4xxx_start_measurement(&dev);
    // if (ret)
    // {
    //     DEBUG_TRACE(ERROR_TAG, "Error %d when starting measurement, exiting program!\n",
    //                 ret);
    //     return;
    // }
    // dev.delay_ms(50);

    //  } while (1);
}

/**
 * @brief	获取TVOC传感器转换结果
 * @param
 * @retval
 */
void readZMOD4410ConvertResult(void)
{
    int ret;
    ret = zmod4xxx_start_measurement(&dev);
    if (ret)
    {
        DEBUG_TRACE(ERROR_TAG, "Error %d when zmod4410 starting measurement, exiting program!", ret);
        return;
    }

    /* Instead of polling STATUS REGISTER, the INTERRUPT PIN can be used.
     * For more information, please look at Interrupt Usage chapter
     * in Programming Manual */
    do
    {
        ret = zmod4xxx_read_status(&dev, &zmod4xxx_status);
        if (ret)
        {
            DEBUG_TRACE(ERROR_TAG, "Error %d zmod4410 during read of sensor status, exiting program!", ret);
            return;
        }
        /* Increase polling counter */
        polling_counter++;
        /* Delay for 200 milliseconds */
        dev.delay_ms(200);
    } while ((zmod4xxx_status & STATUS_SEQUENCER_RUNNING_MASK) && (polling_counter <= ZMOD4410_IAQ2_COUNTER_LIMIT));

    /* Check if timeout has occured */
    if (ZMOD4410_IAQ2_COUNTER_LIMIT <= polling_counter)
    {
        ret = zmod4xxx_check_error_event(&dev);
        if (ret)
        {
            DEBUG_TRACE(ERROR_TAG, "Error %d zmod4410 during read of sensor status, exiting program!", ret);
            return;
        }
        DEBUG_TRACE(ERROR_TAG, "Error %d, zmod4410 exiting program!", ret);
        return;
    }
    else
    {
        polling_counter = 0;
    }

    ret = zmod4xxx_read_adc_result(&dev, adc_result);
    if (ret)
    {
        DEBUG_TRACE(ERROR_TAG, "Error %d zmod4410 during read of ADC results, exiting program!", ret);
        return;
    }

    /* calculate the algorithm */
    ret = calc_iaq_2nd_gen(&algo_handle, &dev, adc_result, &algo_results);
    if ((ret != IAQ_2ND_GEN_OK) && (ret != IAQ_2ND_GEN_STABILIZATION))
    {
        DEBUG_TRACE(ERROR_TAG, "Error %d when zmod4410 calculating algorithm, exiting program!", ret);
        return;
        /* IAQ 2nd Gen algorithm skips first 60 samples for stabilization */
    }
    else
    {
        // INFO("*********** Measurements ***********\n");
        // for (int i = 0; i < 13; i++)
        // {
        //     INFO(" Rmox[%d] = ", i);
        //     INFO("%.3f kOhm\n", algo_results.rmox[i] / 1e3);
        // }
        // INFO(" log_Rcda = %5.3f logOhm\n", algo_results.log_rcda);
        // INFO(" EtOH = %6.3f ppm\n", algo_results.etoh);
        // INFO(" TVOC = %6.3f mg/m^3\n", algo_results.tvoc);
        // INFO(" eCO2 = %4.0f ppm\n", algo_results.eco2);
        // INFO(" IAQ  = %4.1f\n", algo_results.iaq);
        if (ret == IAQ_2ND_GEN_STABILIZATION)
        {
            DEBUG_TRACE(WARN_TAG, "ZMOD44XX Warmup...");
        }
        else
        {
            DEBUG_TRACE(LOG_TAG, "ZMOD44XX Valid...");

        }
        device_t.sensor_t.tvoc = algo_results.tvoc * 1000;

        // 根据 AQI 空气质量指数划分,按等级0-5上报：
        // IAQ: 1-3 非常好
        // IAQ: 3-4 一般
        // IAQ: 4-5 较差
        // IAQ: 5-7 非常差
        if (algo_results.iaq < 2.5)
        {
            device_t.sensor_t.tvoc_level = 0;
        }
        else if (algo_results.iaq < 3)
        {
            device_t.sensor_t.tvoc_level = 1;
        }
        else if (algo_results.iaq < 3.5)
        {
            device_t.sensor_t.tvoc_level = 2;
        }
        else if (algo_results.iaq < 4)
        {
            device_t.sensor_t.tvoc_level = 3;
        }
        else if (algo_results.iaq < 5)
        {
            device_t.sensor_t.tvoc_level = 4;
        }
        else
        {
            device_t.sensor_t.tvoc_level = 5;
        }
        DEBUG_TRACE(LOG_TAG, " ZMOD44XX Convert TVOC Results: %f mg/m^3, IAQ: %4.1f, Level: %d\n", algo_results.tvoc, algo_results.iaq, device_t.sensor_t.tvoc_level);
        // INFO("************************************\n");
    }
}

u8 ZMOD4410_Check(void)
{
    int8_t ret;
    ret = zmod4xxx_read_sensor_info(&dev);
    if (ret)
    {
        return 1;
    }
    return 0;
}
