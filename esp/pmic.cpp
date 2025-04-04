#include <cstdint>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "pmic.h"
#include "http.h"
#include "common.h"

#define I2C_ADDR    ((uint8_t)0x39)

typedef enum {
    I2cRegId = 0,
    I2cRegState,
    I2cRegBootMode,

    I2cRegAdcTemp0,
    I2cRegAdcTemp1,
    I2cRegAdcVbg0,
    I2cRegAdcVbg1,

    I2cRegEspTimeoutSec0,
    I2cRegEspTimeoutSec1,
    I2cRegPicoTimeoutSec0,
    I2cRegPicoTimeoutSec1,
    I2cRegEspScheduleSec0,          // u32
    I2cRegEspScheduleSec1,
    I2cRegEspScheduleSec2,
    I2cRegEspScheduleSec3,
    I2cRegPicoScheduleSec0,         // u32
    I2cRegPicoScheduleSec1,
    I2cRegPicoScheduleSec2,
    I2cRegPicoScheduleSec3,

    I2cRegWdtScratch,
    I2cRegWdtTick0,                 // u32
    I2cRegWdtTick1,
    I2cRegWdtTick2,
    I2cRegWdtTick3,

    I2cRegShtLastMeasurement0,      // u16 T
    I2cRegShtLastMeasurement1,
    I2cRegShtLastMeasurement2,      // u16 RH
    I2cRegShtLastMeasurement3,
    I2cRegShtReadMeasurementLog0,   // u16 T
    I2cRegShtReadMeasurementLog1,
    I2cRegShtReadMeasurementLog2,   // u16 RH
    I2cRegShtReadMeasurementLog3,
    I2cRegShtReadMeasurementLog4,   // u16 WDT tick
    I2cRegShtReadMeasurementLog5,
    I2cRegShtMeasurementLogLength,
} i2c_reg_t;

static const unsigned int gpio_sda = 4;
static const unsigned int gpio_scl = 5;

static void pmic_write_multi(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(reg);
    for (uint8_t i = 0; i < len; i++)
        Wire.write(buf[i]);
    Wire.endTransmission(true);
    delayMicroseconds(10);
}

static void pmic_write(uint8_t reg, uint8_t val)
{
    pmic_write_multi(reg, &val, 1);
}

static uint8_t pmic_read_multi(uint8_t reg, uint8_t *buf, uint8_t len)
{
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(I2C_ADDR, (size_t)len, true);
    uint8_t i = 0;
    while (Wire.available()) {
        buf[i] = Wire.read();
        i += 1;
    }
    delayMicroseconds(10);
    return i;
}

static uint8_t pmic_read(uint8_t reg)
{
    uint8_t v = 0;
    pmic_read_multi(reg, &v, 1);
    return v;
}

void pmic_debug(const char *str)
{
    uint32_t s = strlen(str);
    pmic_write_multi(I2cRegShtMeasurementLogLength, (uint8_t *)str, s);
}

void pmic_init(void)
{
    // Toggle SCL to release stuck I2C bus
    digitalWrite(gpio_sda, LOW);
    digitalWrite(gpio_scl, LOW);
    pinMode(gpio_sda, INPUT);
    pinMode(gpio_scl, INPUT);
    delayMicroseconds(10);
    for (int i = 0; i < 10; i++) {
        pinMode(gpio_scl, OUTPUT);
        delayMicroseconds(10);
        pinMode(gpio_scl, INPUT);
        delayMicroseconds(10);
    }

    // Send stop condition
    pinMode(gpio_sda, OUTPUT);
    delayMicroseconds(2);
    pinMode(gpio_sda, INPUT);
    delayMicroseconds(2);

    // Init I2C
    Wire.begin(gpio_sda, gpio_scl);
    Wire.setClock(400000);
}

void pmic_shutdown(void)
{
    // Shutdown everything
    pmic_write(I2cRegState, 0);
}

uint8_t pmic_get_key(void)
{
    uint8_t state = pmic_read(I2cRegState);
    // b0: PICO_EN
    // b1: ESP_EN
    // b2,b3: KEY
    // b4: ADC_READY
    return (state >> 2) & 3;
}

void pmic_update(NTPClient &ntpClient, PubSubClient &mqttClient, const char *id)
{
    char topic[128];
    char data[128];

    // Calibrate PMIC watchdog timer
    static const uint32_t wdt_cal_typical = 8000000;
    static const uint32_t wdt_cal_margin = 4000000;
    static const uint32_t ntp_ts_max_delay = 1000000;

    uint32_t wdt_cal;
    uint32_t wdt_cal_min;
    uint32_t wdt_cal_max;

    {
        ntpClient.update();
        uint32_t ntp_ts = ntpClient.getEpochTime();
        uint32_t wdt_tick;
        pmic_read_multi(I2cRegWdtTick0, (uint8_t *)&wdt_tick, 4);

        // Fetch previous calibration value, unit is us
        bool valid = true;
        uint32_t wdt_cal_weight;
        sprintf(topic, "var/%s/wdt_cal", id);
        valid &= mqtt_get(mqttClient, topic, &wdt_cal, sizeof(wdt_cal)) == sizeof(wdt_cal);
        if (valid) {
            sprintf(topic, "var/%s/wdt_cal_min", id);
            valid &= mqtt_get(mqttClient, topic, &wdt_cal_min, sizeof(wdt_cal_min)) == sizeof(wdt_cal_min);
            sprintf(topic, "var/%s/wdt_cal_max", id);
            valid &= mqtt_get(mqttClient, topic, &wdt_cal_max, sizeof(wdt_cal_max)) == sizeof(wdt_cal_max);
            sprintf(topic, "var/%s/wdt_cal_weight", id);
            valid &= mqtt_get(mqttClient, topic, &wdt_cal_weight, sizeof(wdt_cal_weight)) == sizeof(wdt_cal_weight);
        }
        if (!valid) {
            wdt_cal = wdt_cal_typical;
            wdt_cal_min = wdt_cal - wdt_cal_margin;
            wdt_cal_max = wdt_cal + wdt_cal_margin;
            wdt_cal_weight = 1;
        }

        uint8_t wdt_scratch = pmic_read(I2cRegWdtScratch);
        if (valid && wdt_scratch) {
            // Calibration possible

            // Fetch and update MQTT timestamp records
            valid = true;
            uint32_t ref_wdt_tick;
            sprintf(topic, "var/%s/last_wdt_tick", id);
            valid &= mqtt_get(mqttClient, topic, &ref_wdt_tick, sizeof(ref_wdt_tick)) == sizeof(ref_wdt_tick);
            mqttClient.publish(topic, (uint8_t *)&wdt_tick, sizeof(wdt_tick), true);
            sprintf(topic, "var/%s/ref_wdt_tick", id);
            mqttClient.publish(topic, (uint8_t *)&ref_wdt_tick, sizeof(ref_wdt_tick), true);

            uint32_t ref_ntp_ts;
            sprintf(topic, "var/%s/last_ntp_ts", id);
            valid &= mqtt_get(mqttClient, topic, &ref_ntp_ts, sizeof(ref_ntp_ts)) == sizeof(ref_ntp_ts);
            mqttClient.publish(topic, (uint8_t *)&ntp_ts, sizeof(ntp_ts), true);
            sprintf(topic, "var/%s/ref_ntp_ts", id);
            mqttClient.publish(topic, (uint8_t *)&ref_ntp_ts, sizeof(ref_ntp_ts), true);

            uint32_t wdt_tick_delta = wdt_tick - ref_wdt_tick;
            if (valid && wdt_tick_delta != 0) {
                // Reference valid, calibrate
                uint32_t ntp_ts_delta = ntp_ts - ref_ntp_ts;
                uint32_t tick_delta_min;
                uint32_t tick_delta_max;
                uint64_t ts_delta;
                uint32_t new_wdt_cal_min;
                uint32_t new_wdt_cal_max;

                // Max
                // wdt_tick sample points:      0*----1----*2
                // ntp_ts sample points:        0----*1*----2
                // ntp_ts with network delay:   0---da1d---a2
                tick_delta_min = std::max(wdt_tick_delta, 1u) - 1;
                if (tick_delta_min != 0) {
                    ts_delta = ntp_ts_delta;
                    ts_delta = ts_delta * 1000000 + (2000000 + ntp_ts_max_delay);
                    new_wdt_cal_max = (ts_delta + tick_delta_min / 2) / tick_delta_min;
                    // Update max value if calculated value is sensible
                    if (new_wdt_cal_max > wdt_cal_typical - wdt_cal_margin) {
                        wdt_cal_max = ((uint64_t)wdt_cal_weight * wdt_cal_max +
                            (uint64_t)tick_delta_min * new_wdt_cal_max) / (wdt_cal_weight + tick_delta_min);
                    }
                }

                // Min
                // wdt_tick sample points:      0----*1*----2
                // ntp_ts sample points:        0*----1----*2
                // ntp_ts with network delay:   0da---1----d2----a
                tick_delta_max = wdt_tick_delta + 2;
                ts_delta = ntp_ts_delta;
                ts_delta = std::max(ts_delta * 1000000, 1000000ull + ntp_ts_max_delay) - (1000000ull + ntp_ts_max_delay);
                new_wdt_cal_min = (ts_delta + tick_delta_max / 2) / tick_delta_max;
                // Update min value if calculated value is sensible
                if (new_wdt_cal_min < wdt_cal_typical + wdt_cal_margin) {
                    wdt_cal_min = ((uint64_t)wdt_cal_weight * wdt_cal_min +
                        (uint64_t)tick_delta_min * new_wdt_cal_min) / (wdt_cal_weight + tick_delta_min);
                }

                // Use mean value for current session
                uint32_t ref_wdt_cal = wdt_cal;
                wdt_cal = (wdt_cal_min + wdt_cal_max + 1) / 2;
                uint32_t ref_wdt_cal_weight = wdt_cal_weight;
                wdt_cal_weight = ((uint64_t)wdt_cal_weight * wdt_cal_weight +
                    (uint64_t)tick_delta_min * tick_delta_min) / (wdt_cal_weight + tick_delta_min);

#define CAL_DEBUG   1
#if CAL_DEBUG
                // Debug logs
                sprintf(topic, "debug/%s/ref_wdt_tick", id);
                sprintf(data, "%lu", ref_wdt_tick);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/last_wdt_tick", id);
                sprintf(data, "%lu", wdt_tick);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/ref_ntp_ts", id);
                sprintf(data, "%lu", ref_ntp_ts);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/last_ntp_ts", id);
                sprintf(data, "%lu", ntp_ts);
                mqttClient.publish(topic, data, true);

                sprintf(topic, "debug/%s/new_wdt_cal_min", id);
                sprintf(data, "%lu", new_wdt_cal_min);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/new_wdt_cal_max", id);
                sprintf(data, "%lu", new_wdt_cal_max);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/ref_wdt_cal", id);
                sprintf(data, "%lu", ref_wdt_cal);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/ref_wdt_cal_weight", id);
                sprintf(data, "%lu", ref_wdt_cal_weight);
                mqttClient.publish(topic, data, true);

                sprintf(topic, "debug/%s/wdt_cal", id);
                sprintf(data, "%lu", wdt_cal);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/wdt_cal_min", id);
                sprintf(data, "%lu", wdt_cal_min);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/wdt_cal_max", id);
                sprintf(data, "%lu", wdt_cal_max);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/wdt_cal_weight", id);
                sprintf(data, "%lu", wdt_cal_weight);
                mqttClient.publish(topic, data, true);
#endif
            }

        } else {
            // Invalid MQTT record or PMIC rebooted, calibration not possible

            // Update MQTT timestamp records
            sprintf(topic, "var/%s/last_wdt_tick", id);
            mqttClient.publish(topic, (uint8_t *)&wdt_tick, sizeof(wdt_tick), true);

            sprintf(topic, "var/%s/last_ntp_ts", id);
            mqttClient.publish(topic, (uint8_t *)&ntp_ts, sizeof(ntp_ts), true);
        }

        // Update calibration values
        valid = true;
        sprintf(topic, "var/%s/wdt_cal", id);
        valid &= mqttClient.publish(topic, (uint8_t *)&wdt_cal, sizeof(wdt_cal), true);
        sprintf(topic, "var/%s/wdt_cal_min", id);
        valid &= mqttClient.publish(topic, (uint8_t *)&wdt_cal_min, sizeof(wdt_cal_min), true);
        sprintf(topic, "var/%s/wdt_cal_max", id);
        valid &= mqttClient.publish(topic, (uint8_t *)&wdt_cal_max, sizeof(wdt_cal_max), true);
        sprintf(topic, "var/%s/wdt_cal_weight", id);
        valid &= mqttClient.publish(topic, (uint8_t *)&wdt_cal_weight, sizeof(wdt_cal_weight), true);

        // Only update PMIC record if MQTT update was successful
        if (!wdt_scratch && valid)
            pmic_write(I2cRegWdtScratch, 1);
    }

    {
        uint16_t adc_val[2] = {0};
        pmic_read_multi(I2cRegAdcTemp0, (uint8_t *)&adc_val[0], 4);

        // Report battery voltage
        sprintf(topic, "sensor/%s/voltage/pmic_vcc", id);
        sprintf(data, "%g", (float)adc_val[1] / 4096.0);
        mqttClient.publish(topic, data, true);

        // Report pmic temperature TODO conversion
        sprintf(topic, "sensor/%s/adc_raw/pmic_temp", id);
        sprintf(data, "%d", adc_val[0]);
        mqttClient.publish(topic, data, true);
    }

    {
        union {
            struct {
                uint16_t t;
                uint16_t rh;
            };
            uint8_t raw[0];
        } sht;
        pmic_read_multi(I2cRegShtLastMeasurement0, &sht.raw[0], 4);

        // Report temperature
        sprintf(topic, "sensor/%s/temperature/pmic_sht", id);
        sprintf(data, "%g", -45 + (uint32_t)sht.t * 175 / 65535.0);
        mqttClient.publish(topic, data, true);

        // Report humidity
        sprintf(topic, "sensor/%s/humidity/pmic_sht", id);
        sprintf(data, "%g", -6 + (uint32_t)sht.rh * 125 / 65535.0);
        mqttClient.publish(topic, data, true);
    }
}

#if 0
void pmic_update(void)
{
    uint16_t adc_val[2] = {0};
    pmic_read_multi(I2cRegAdcTemp0, (uint8_t *)&adc_val[0], 4);

    char url[128];

    // Upload temperature TODO conversion
    sprintf(url, "%s?src=%s&inst=%s&type=adc&val=%d",
        sensor_url_base, id, "pmic_temp",
        adc_val[0]);
    http_get(url, nullptr);
    // Upload Vcc
    sprintf(url, "%s?src=%s&inst=%s&type=voltage&val=%g",
        sensor_url_base, id, "pmic_vcc",
        (float)adc_val[1] / 4096.0);
    http_get(url, nullptr);

    // Get next wakeup time
    uint32_t schedule_sec = 30;
    sprintf(url, "%s?token=%s&action=next", url_base, id);
    const String &next = http_get(url, nullptr);
    int outdated = next.indexOf("\"outdated\":");
    if (outdated < 0) {
        // Error parsing outdated value
        // Wakeup again after 1 hour
        outdated = 1;
        schedule_sec = 60 * 60;
    } else if (next[outdated + 11] != 'f') {
        // Outdated now
        // Wakeup again after 30 seconds
        outdated = 1;
        schedule_sec = 30;
    } else {
        // Wait for next update
        int sec = next.indexOf("\"next_secs\":");
        if (sec < 0) {
            // Host missed update, check again after 1 hour
            outdated = sec;
            schedule_sec = 60 * 60;
        } else {
            // Add some offset to scheduled wakeup time as PMIC's clock may not be accurate
            outdated = 0;
            sec = next.substring(sec + 12).toInt();
            schedule_sec = sec + 30;
        }
    }
    pmic_write_multi(I2cRegEspScheduleSec0, (uint8_t *)&schedule_sec, 4);
    Serial.printf("outdated=%d schedule=%d\n", outdated, schedule_sec);
}
#endif
