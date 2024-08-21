#include <cstdint>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "pmic.h"
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

    // Init I2C
    Wire.pins(gpio_sda, gpio_scl);
    Wire.begin();
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
