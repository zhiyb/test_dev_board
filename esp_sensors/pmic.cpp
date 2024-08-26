#include <ESP8266WiFi.h>
#include <cstdint>
#include "pmic.h"
#include "i2c.h"
#include "http.h"

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
    I2cRegEspScheduleSec0,
    I2cRegEspScheduleSec1,
    I2cRegEspScheduleSec2,
    I2cRegEspScheduleSec3,
    I2cRegPicoScheduleSec0,
    I2cRegPicoScheduleSec1,
    I2cRegPicoScheduleSec2,
    I2cRegPicoScheduleSec3,
    I2cRegPowerEn,
} i2c_reg_t;

static void pmic_write_multi(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    i2c_write_multi(I2C_ADDR, reg, buf, len);
}

static void pmic_write(uint8_t reg, uint8_t val)
{
    i2c_write_reg(I2C_ADDR, reg, val);
}

static uint8_t pmic_read_multi(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return i2c_read_multi(I2C_ADDR, reg, buf, len);
}

static uint8_t pmic_read(uint8_t reg)
{
    return i2c_read_reg(I2C_ADDR, reg);
}

void pmic_init(void)
{
}

void pmic_power_enable(bool sensors)
{
    uint8_t en = pmic_read(I2cRegPowerEn);
    en = (en & ~(1 << 2)) | (sensors << 2);
    pmic_write(I2cRegPowerEn, en);
}

void pmic_shutdown(void)
{
    // Shutdown everything
    pmic_write(I2cRegPowerEn, 0);
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
    uint32_t schedule_secs = 30;
    sprintf(url, "%s?token=%s&action=next", url_base, id);
    const String &next = http_get(url, nullptr);
    int outdated = next.indexOf("\"outdated\":");
    if (outdated < 0) {
        // Error parsing outdated value
        // Wakeup again after 1 hour
        outdated = 1;
        schedule_secs = 60 * 60;
    } else if (next[outdated + 11] != 'f') {
        // Outdated now
        // Wakeup again after 30 seconds
        outdated = 1;
        schedule_secs = 30;
    } else {
        // Wait for next update
        int sec = next.indexOf("\"next_secs\":");
        if (sec < 0) {
            // Host missed update, check again after 1 hour
            outdated = sec;
            schedule_secs = 60 * 60;
        } else {
            // Add some offset to scheduled wakeup time as PMIC's clock may not be accurate
            outdated = 0;
            sec = next.substring(sec + 12).toInt();
            schedule_secs = sec + 30;
        }
    }
    pmic_write_multi(I2cRegEspScheduleSec0, (uint8_t *)&schedule_secs, 4);
    Serial.printf("outdated=%d schedule=%d\n", outdated, schedule_secs);
}

void pmic_update(uint32_t schedule_secs)
{
    pmic_write_multi(I2cRegEspScheduleSec0, (uint8_t *)&schedule_secs, 4);
    Serial.printf("schedule=%d\n", schedule_secs);
}
