

#include "zmod451x.h"
#include "zmod4510_config_oaq2.h"
#include "zmod4xxx.h"
#include "zmod4xxx_i2c_hal.h"
//#include "zmod4xxx_cleaning.h"
#include "oaq_2nd_gen.h"

#include "control_center.h"
#include "uart.h"
#include "systick.h"
#include "communicate.h"

float humidity_pct;
float temperature_degc;

int8_t ret;
static uint32_t polling_counter = 0;
/* Sensor target variables */
static zmod4xxx_dev_t dev;
static uint8_t track_number[6];
static uint8_t zmod4xxx_status;
static uint8_t adc_result[ZMOD4510_ADC_DATA_LEN];
static uint8_t prod_data[ZMOD4510_PROD_DATA_LEN];
static oaq_2nd_gen_handle_t algo_handle;
static oaq_2nd_gen_results_t algo_results;

/**
 * @brief	初始化臭氧O3传感器
 * @param
 * @retval
 */
void ZMOD4510_Init(void)
{
    dev.read = zmod4xxx_i2c_read;
    dev.write = zmod4xxx_i2c_write;
    dev.delay_ms = zmod4xxx_sleep;

    dev.i2c_addr = ZMOD4510_I2C_ADDR;
    dev.pid = ZMOD4510_PID;
    dev.init_conf = &zmod_oaq_sensor_type[INIT];
    dev.meas_conf = &zmod_oaq_sensor_type[MEASURE];
    dev.prod_data = prod_data;

    /* Read product ID and configuration parameters. */
    ret = zmod4xxx_read_sensor_info(&dev);
    if (ret)
    {
        INFO("Error %d during reading sensor information, exiting program!\n",
             ret);
        return;
    }

    /* Each sensor has a unique tracking number and individual trimming
     * information. Please provide these information when you need support by
     * Renesas. */
    ret = zmod4xxx_read_tracking_number(&dev, track_number);
    if (ret)
    {
        INFO("Error %d during reading tracking number, exiting program!\n",
             ret);
        return;
    }
    INFO("sensor tracking number: x0000");
    for (int i = 0; i < sizeof(track_number); i++)
    {
        INFO("%02X", track_number[i]);
    }
    INFO("\n");
    INFO("sensor trimming data:");
    for (int i = 0; i < sizeof(prod_data); i++)
    {
        INFO(" %i", prod_data[i]);
    }
    INFO("\n");

    /* This function starts the cleaning procedure. Please
     * check the Programming Manual on indications for usage.
     * IMPORTANT NOTE: The cleaning procedure can be run only once
     * during the modules lifetime and takes 10 minutes (blocking). */
    // ret = zmod4xxx_cleaning_run(&dev);
    // if (ret) {
    //      INFO("Error %d during cleaning procedure, exiting program!\n", ret);
    //     return;
    // }

    /* Calibration parameters are determined and measurement is configured. */
    ret = zmod4xxx_prepare_sensor(&dev);
    if (ret)
    {
        INFO("Error %d during preparation of the sensor, exiting program!\n",
             ret);
        return;
    }

    /* One time initialization of the algorithm. Pass handle to calculation function. */
    ret = init_oaq_2nd_gen(&algo_handle, &dev);
    if (ret)
    {
        INFO("Error %d when initializing algorithm, exiting program!\n", ret);
        return;
    }

    /* Start one measurement. */
    ret = zmod4xxx_start_measurement(&dev);
    if (ret)
    {
        INFO("Error %d during start of measurement, exiting program!\n", ret);
        return;
    }

    /* Main Measurement Loop */
    // do
    {
        /* Instead of polling STATUS REGISTER, interrupts can be used.
         * For more information, please check Programming Manual, section
         * “Interrupt Usage” */
        do
        {
            ret = zmod4xxx_read_status(&dev, &zmod4xxx_status);
            if (ret)
            {
                INFO(
                    "Error %d during read of sensor status, exiting program!\n",
                    ret);
                return;
            }
            /* Increase polling counter */
            polling_counter++;
            /* Delay for 15 milliseconds */
            dev.delay_ms(15);
            IWDT_Clr();
        } while ((zmod4xxx_status & STATUS_SEQUENCER_RUNNING_MASK) &&
                 (polling_counter <= ZMOD4510_OAQ2_COUNTER_LIMIT));

        /* Check if timeout is occured */
        if (ZMOD4510_OAQ2_COUNTER_LIMIT <= polling_counter)
        {
            ret = zmod4xxx_check_error_event(&dev);
            if (ret)
            {
                INFO("Error %d during check event, exiting program!\n", ret);
                return;
            }
            INFO("Error %d, exiting program!\n", ERROR_GAS_TIMEOUT);
            return;
        }
        else
        {
            polling_counter = 0;
        }

        /* Read sensor ADC output. */
        ret = zmod4xxx_read_adc_result(&dev, adc_result);
        if (ret)
        {
            INFO("Error %d during read of ADC results, exiting program!\n",
                 ret);
            return;
        }

        /* Humidity and temperature measurements are needed for ambient
         * compensation. It is highly recommented to have a real humidity and
         * temperature sensor for these values! */
        humidity_pct = 50.0;     // 50% RH
        temperature_degc = 20.0; // 20 degC

        /* calculate algorithm results */
        ret = calc_oaq_2nd_gen(&algo_handle, &dev, adc_result, humidity_pct,
                               temperature_degc, &algo_results);
        if ((ret != OAQ_2ND_GEN_OK) && (ret != OAQ_2ND_GEN_STABILIZATION))
        {
            INFO("Error %d when calculating algorithm, exiting program!\n",
                 ret);
            return;
            /* OAQ 2nd Gen algorithm skips first samples for sensor stabilization */
        }
        else
        {
            INFO("*********** Measurements ***********\n");
            for (int i = 0; i < 8; i++)
            {
                INFO(" Rmox[%d] = ", i);
                INFO("%.3f kOhm\n", algo_results.rmox[i] / 1e3);
            }
            INFO(" O3_conc_ppb = %6.3f\n", algo_results.O3_conc_ppb);
            INFO(" Fast AQI = %i\n", algo_results.FAST_AQI);
            INFO(" EPA AQI = %i\n", algo_results.EPA_AQI);
            if (ret == OAQ_2ND_GEN_STABILIZATION)
            {
                INFO("Warmup!\n");
            }
            else
            {
                INFO("Valid!\n");
                device_t.sensor_t.o3_aqi = algo_results.FAST_AQI;
                DEBUG_TRACE(LOG_TAG, " ZMOD45XX Convert AQI Results: %f\n", algo_results.FAST_AQI);
            }
            INFO("************************************\n");
        }

        // /* wait 1.98 seconds before starting the next measurement */
        // dev.delay_ms(1980);

        //  /* start a new measurement before result calculation */
        //  ret = zmod4xxx_start_measurement(&dev);
        //  if (ret)
        // {
        //      INFO("Error %d when starting measurement, exiting program!\n",
        //          ret);
        //      return;
    }
    //   } while (1);
}

/**
 * @brief	获取臭氧O3传感器转换结果
 * @param
 * @retval
 */
void readZMOD4510ConvertResult(void)
{
    /* Start one measurement. */
    ret = zmod4xxx_start_measurement(&dev);
    if (ret)
    {
        INFO("Error %d during start of measurement, exiting program!\n", ret);
        return;
    }
    /* Instead of polling STATUS REGISTER, interrupts can be used.
     * For more information, please check Programming Manual, section
     * “Interrupt Usage” */
    do
    {
        ret = zmod4xxx_read_status(&dev, &zmod4xxx_status);
        if (ret)
        {
            INFO(
                "Error %d during read of sensor status, exiting program!\n",
                ret);
            return;
        }
        /* Increase polling counter */
        polling_counter++;
        /* Delay for 15 milliseconds */
        dev.delay_ms(15);
    } while ((zmod4xxx_status & STATUS_SEQUENCER_RUNNING_MASK) &&
             (polling_counter <= ZMOD4510_OAQ2_COUNTER_LIMIT));

    /* Check if timeout is occured */
    if (ZMOD4510_OAQ2_COUNTER_LIMIT <= polling_counter)
    {
        ret = zmod4xxx_check_error_event(&dev);
        if (ret)
        {
            INFO("Error %d during check event, exiting program!\n", ret);
            return;
        }
        INFO("Error %d, exiting program!\n", ERROR_GAS_TIMEOUT);
        return;
    }
    else
    {
        polling_counter = 0;
    }

    /* Read sensor ADC output. */
    ret = zmod4xxx_read_adc_result(&dev, adc_result);
    if (ret)
    {
        INFO("Error %d during read of ADC results, exiting program!\n",
             ret);
        return;
    }

    /* Humidity and temperature measurements are needed for ambient
     * compensation. It is highly recommented to have a real humidity and
     * temperature sensor for these values! */
    humidity_pct = device_t.sensor_t.humidity / 1000;        // 50% RH
    temperature_degc = device_t.sensor_t.temperature / 1000; // 20 degC

    /* calculate algorithm results */
    ret = calc_oaq_2nd_gen(&algo_handle, &dev, adc_result, humidity_pct,
                           temperature_degc, &algo_results);
    if ((ret != OAQ_2ND_GEN_OK) && (ret != OAQ_2ND_GEN_STABILIZATION))
    {
        INFO("Error %d when calculating algorithm, exiting program!\n",
             ret);
        return;
        /* OAQ 2nd Gen algorithm skips first samples for sensor stabilization */
    }
    else
    {
        INFO("*********** Measurements ***********\n");
        for (int i = 0; i < 8; i++)
        {
            INFO(" Rmox[%d] = ", i);
            INFO("%.3f kOhm\n", algo_results.rmox[i] / 1e3);
        }
        INFO(" O3_conc_ppb = %6.3f\n", algo_results.O3_conc_ppb);
        INFO(" Fast AQI = %i\n", algo_results.FAST_AQI);
        INFO(" EPA AQI = %i\n", algo_results.EPA_AQI);
        if (ret == OAQ_2ND_GEN_STABILIZATION)
        {
            INFO("Warmup!\n");
        }
        else
        {
            INFO("Valid!\n");
            device_t.sensor_t.o3_aqi = algo_results.FAST_AQI;
            DEBUG_TRACE(LOG_TAG, " ZMOD45XX Convert AQI Results: %f\n", algo_results.FAST_AQI);
        }
        INFO("************************************\n");
    }
}
