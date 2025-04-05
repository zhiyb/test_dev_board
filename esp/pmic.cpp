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

    I2cRegSchdDev,                  // Select a dev for configuration
    I2cRegSchdScheduled,
    I2cRegSchdPeriodic0,            // u16
    I2cRegSchdPeriodic1,
    I2cRegSchdTimeout0,             // u16
    I2cRegSchdTimeout1,
    I2cRegSchdNextTick0,            // u32
    I2cRegSchdNextTick1,
    I2cRegSchdNextTick2,
    I2cRegSchdNextTick3,

    I2cRegWdtScratch,
    I2cRegWdtTick0,                 // u32
    I2cRegWdtTick1,
    I2cRegWdtTick2,
    I2cRegWdtTick3,

    I2cRegAdcTemp0,                 // u16
    I2cRegAdcTemp1,
    I2cRegAdcVcc0,                  // u16
    I2cRegAdcVcc1,

    I2cRegShtLastMeasurement0,      // u16 T
    I2cRegShtLastMeasurement1,
    I2cRegShtLastMeasurement2,      // u16 RH
    I2cRegShtLastMeasurement3,
    I2cRegShtLastMeasurement4,      // u16 WDT tick
    I2cRegShtLastMeasurement5,
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
    static const uint32_t wdt_tick_delta_max = 2 * 24 * 60 * 60 / (wdt_cal_typical / 1000000);

    uint32_t wdt_cal;
    uint32_t wdt_cal_min;
    uint32_t wdt_cal_max;
    uint32_t ref_ntp_ts;
    uint32_t ref_wdt_tick;
    uint32_t wdt_tick;
    bool ts_valid = true;

    // WDT tick timing calibration
    {
        bool rec_valid = true;

        // Latest values
        uint32_t ntp_ts;
        if (ts_valid) {
            ts_valid &= ntpClient.update();
        }
        if (ts_valid) {
            ntp_ts = ntpClient.getEpochTime();
            ts_valid &= pmic_read_multi(I2cRegWdtTick0, (uint8_t *)&wdt_tick, 4) == sizeof(wdt_tick);
            rec_valid &= pmic_read(I2cRegWdtScratch);
        }
        rec_valid &= ts_valid;

        // Fetch previous calibration values, unit is us
        uint32_t wdt_cal_weight;
        if (rec_valid) {
            sprintf(topic, "var/%s/ref_ntp_ts", id);
            rec_valid &= mqtt_get(mqttClient, topic, &ref_ntp_ts, sizeof(ref_ntp_ts)) == sizeof(ref_ntp_ts);
        }
        if (rec_valid) {
            sprintf(topic, "var/%s/ref_wdt_tick", id);
            rec_valid &= mqtt_get(mqttClient, topic, &ref_wdt_tick, sizeof(ref_wdt_tick)) == sizeof(ref_wdt_tick);
        }

        // Previous calibration results may be valid anyway
        bool prev_cal_valid = true;
        if (prev_cal_valid) {
            sprintf(topic, "var/%s/wdt_cal", id);
            prev_cal_valid &= mqtt_get(mqttClient, topic, &wdt_cal, sizeof(wdt_cal)) == sizeof(wdt_cal);
        }
        if (prev_cal_valid) {
            sprintf(topic, "var/%s/wdt_cal_min", id);
            prev_cal_valid &= mqtt_get(mqttClient, topic, &wdt_cal_min, sizeof(wdt_cal_min)) == sizeof(wdt_cal_min);
        }
        if (prev_cal_valid) {
            sprintf(topic, "var/%s/wdt_cal_max", id);
            prev_cal_valid &= mqtt_get(mqttClient, topic, &wdt_cal_max, sizeof(wdt_cal_max)) == sizeof(wdt_cal_max);
        }
        if (prev_cal_valid) {
            sprintf(topic, "var/%s/wdt_cal_weight", id);
            prev_cal_valid &= mqtt_get(mqttClient, topic, &wdt_cal_weight, sizeof(wdt_cal_weight)) == sizeof(wdt_cal_weight);
        }
        rec_valid &= prev_cal_valid;
        if (!prev_cal_valid) {
            // Revert to default values
            wdt_cal = wdt_cal_typical;
            wdt_cal_min = wdt_cal - wdt_cal_margin;
            wdt_cal_max = wdt_cal + wdt_cal_margin;
            wdt_cal_weight = 1;
        }


        bool upd_ref = false;
        bool upd_cal = false;
        if (!rec_valid) {
            // Invalid MQTT record or PMIC rebooted, calibration not possible
            // Update reference to current time
            if (ts_valid) {
                upd_ref = true;
                upd_cal = true;
                ref_wdt_tick = wdt_tick;
                ref_ntp_ts = ntp_ts;
            }

        } else {
            // Calibration possible
            uint32_t wdt_tick_delta = wdt_tick - ref_wdt_tick;
            if (wdt_tick_delta >= wdt_tick_delta_max) {
                // Calibration reference too old
                // Interpolate to current tick - wdt_tick_delta_max / 2
                upd_ref = true;

                uint32_t wdt_tick_inc = wdt_tick_delta - wdt_tick_delta_max / 2;
                wdt_tick_delta = wdt_tick_delta_max / 2;
                ref_wdt_tick += wdt_tick_inc;

                uint64_t ts_delta_us = wdt_cal;
                ts_delta_us *= wdt_tick_inc;
                ref_ntp_ts += (ts_delta_us + 500000) / 1000000;
            }

            if (wdt_tick_delta > 0) {
                // Reference valid, calibrate
                upd_cal = true;

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

#if 1
                // Calibration debug values
                sprintf(topic, "debug/%s/wdt_tick", id);
                sprintf(data, "%lu", wdt_tick);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/ntp_ts", id);
                sprintf(data, "%lu", ntp_ts);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/ref_wdt_tick", id);
                sprintf(data, "%lu", ref_wdt_tick);
                mqttClient.publish(topic, data, true);
                sprintf(topic, "debug/%s/ref_ntp_ts", id);
                sprintf(data, "%lu", ref_ntp_ts);
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
        }


        // Update calibration records
        bool upd_success = true;
        if (upd_ref) {
            if (upd_success) {
                sprintf(topic, "var/%s/ref_wdt_tick", id);
                upd_success &= mqttClient.publish(topic, (uint8_t *)&ref_wdt_tick, sizeof(ref_wdt_tick), true);
            }
            if (upd_success) {
                sprintf(topic, "var/%s/ref_ntp_ts", id);
                upd_success &= mqttClient.publish(topic, (uint8_t *)&ref_ntp_ts, sizeof(ref_ntp_ts), true);
            }
        }

        if (upd_cal) {
            if (upd_success) {
                sprintf(topic, "var/%s/wdt_cal", id);
                upd_success &= mqttClient.publish(topic, (uint8_t *)&wdt_cal, sizeof(wdt_cal), true);
            }
            if (upd_success) {
                sprintf(topic, "var/%s/wdt_cal_min", id);
                upd_success &= mqttClient.publish(topic, (uint8_t *)&wdt_cal_min, sizeof(wdt_cal_min), true);
            }
            if (upd_success) {
                sprintf(topic, "var/%s/wdt_cal_max", id);
                upd_success &= mqttClient.publish(topic, (uint8_t *)&wdt_cal_max, sizeof(wdt_cal_max), true);
            }
            if (upd_success) {
                sprintf(topic, "var/%s/wdt_cal_weight", id);
                upd_success &= mqttClient.publish(topic, (uint8_t *)&wdt_cal_weight, sizeof(wdt_cal_weight), true);
            }
        }

        // If update failed, records need to be re-initialised next time
        if (upd_ref | upd_cal)
            pmic_write(I2cRegWdtScratch, upd_success);
    }

    // Update VBAT
    if (ts_valid) {
        // Read calibration value
        char vbg_str[8];
        sprintf(topic, "config/%s/pmic/adc_vbg", id);
        uint8_t vbg_str_len = mqtt_get(mqttClient, topic, &vbg_str[0], sizeof(vbg_str));
        vbg_str_len = max(vbg_str_len, (uint8_t)(sizeof(vbg_str) - 1));
        vbg_str[vbg_str_len] = 0;
        float adc_vbg = atof(vbg_str);

        uint16_t adc_val[2] = {0};
        if (pmic_read_multi(I2cRegAdcTemp0, (uint8_t *)&adc_val[0], 4) == 4) {
            // Calculate current timestamp
            uint32_t tick_delta = wdt_tick - ref_wdt_tick;
            uint64_t ts_delta = tick_delta;
            ts_delta *= wdt_cal;
            ts_delta = (ts_delta + 500000) / 1000000;
            uint32_t ts = ref_ntp_ts + ts_delta;

            // Report battery voltage
            if (adc_vbg == 0) {
                // Invalid calibration value, report as adc_raw
                sprintf(topic, "sensor/%s/adc_raw/pmic_vcc", id);
                sprintf(data, "{\"ts\":%lu,\"value\":%u}", ts, (uint32_t)adc_val[1]);
                mqttClient.publish(topic, data, true);
            } else {
                // Convert to voltage
                float adc_vcc = adc_vbg * 1024.0 / (float)adc_val[1];
                sprintf(topic, "sensor/%s/voltage/pmic_vcc", id);
                sprintf(data, "{\"ts\":%lu,\"value\":%g}", ts, adc_vcc);
                mqttClient.publish(topic, data, true);
            }

            // Report pmic temperature TODO conversion
            sprintf(topic, "sensor/%s/adc_raw/pmic_temp", id);
            sprintf(data, "{\"ts\":%lu,\"value\":%u}", ts, (uint32_t)adc_val[0]);
            mqttClient.publish(topic, data, true);
        }
    }

    // Update sensor logs
    if (ts_valid) {
        // Send all sensor measurement logs
        uint8_t len = pmic_read(I2cRegShtMeasurementLogLength);
        while (len--) {
            union {
                struct {
                    uint16_t t;
                    uint16_t rh;
                    uint16_t tick;
                };
                uint8_t raw[0];
            } sht;

            if (pmic_read_multi(I2cRegShtReadMeasurementLog0, &sht.raw[0], 6) == 6) {
                // Assuming sht.tick must be within 0x10000 of current wdt_tick
                uint32_t tick = ((wdt_tick - 0x10000u) & 0xffff0000u) | sht.tick;
                tick += (uint32_t)(wdt_tick - tick) & 0xffff0000u;
                int32_t tick_delta = tick - ref_wdt_tick;
                int64_t ts_delta = tick_delta;
                ts_delta *= wdt_cal;
                ts_delta = (ts_delta + 500000) / 1000000;
                uint32_t ts = ref_ntp_ts + ts_delta;

                // Report temperature
                sprintf(topic, "sensor/%s/temperature/pmic_sht", id);
                sprintf(data, "{\"ts\":%lu,\"value\":%g}", ts, -45 + (uint32_t)sht.t * 175 / 65535.0);
                mqttClient.publish(topic, data, true);

                // Report humidity
                sprintf(topic, "sensor/%s/humidity/pmic_sht", id);
                sprintf(data, "{\"ts\":%lu,\"value\":%g}", ts, -6 + (uint32_t)sht.rh * 125 / 65535.0);
                mqttClient.publish(topic, data, true);
            }
        }
    }

#if 0   // Update sensor latest measurements
    if (ts_valid) {
        union {
            struct {
                uint16_t t;
                uint16_t rh;
                uint16_t tick;
            };
            uint8_t raw[0];
        } sht;
        pmic_read_multi(I2cRegShtLastMeasurement0, &sht.raw[0], 6);

        // Assuming sht.tick must be within 0x10000 of current wdt_tick
        uint32_t tick = ((wdt_tick - 0x10000u) & 0xffff0000u) | sht.tick;
        tick += (uint32_t)(wdt_tick - tick) & 0xffff0000u;
        int32_t tick_delta = tick - ref_wdt_tick;
        int64_t ts_delta = tick_delta;
        ts_delta *= wdt_cal;
        ts_delta = (ts_delta + 500000) / 1000000;
        uint32_t ts = ref_ntp_ts + ts_delta;

#if 1
        // Timestamp calculation debug values
        sprintf(topic, "debug/%s/sht_tick_raw", id);
        sprintf(data, "%lu", (uint32_t)sht.tick);
        mqttClient.publish(topic, data, true);
        sprintf(topic, "debug/%s/sht_tick", id);
        sprintf(data, "%lu", (uint32_t)tick);
        mqttClient.publish(topic, data, true);
        sprintf(topic, "debug/%s/sht_ts_delta", id);
        sprintf(data, "%ld", (int32_t)ts_delta);
        mqttClient.publish(topic, data, true);
        sprintf(topic, "debug/%s/sht_ts", id);
        sprintf(data, "%lu", (uint32_t)ts);
        mqttClient.publish(topic, data, true);
#endif

        // Report temperature
        sprintf(topic, "sensor/%s/temperature/pmic_sht", id);
        sprintf(data, "{\"ts\":%lu,\"value\":%g}", ts, -45 + (uint32_t)sht.t * 175 / 65535.0);
        mqttClient.publish(topic, data, true);

        // Report humidity
        sprintf(topic, "sensor/%s/humidity/pmic_sht", id);
        sprintf(data, "{\"ts\":%lu,\"value\":%g}", ts, -6 + (uint32_t)sht.rh * 125 / 65535.0);
        mqttClient.publish(topic, data, true);
    }
#endif

    // TODO Update scheduling info
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
