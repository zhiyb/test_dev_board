#include <cstdint>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "pmic.h"

#define I2C_ADDR    ((uint8_t)0x39)

typedef enum {
    I2cRegId = 0,
    I2cRegPwrKey,
} i2c_reg_t;

static const unsigned int gpio_sda = 4;
static const unsigned int gpio_scl = 5;

static void pmic_write(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission(true);
    delayMicroseconds(10);
}

static uint8_t pmic_read(uint8_t reg)
{
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(I2C_ADDR, 1u, true);
    uint8_t v = 0;
    if (Wire.available())
        v = Wire.read();
    delayMicroseconds(10);
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
    pmic_write(I2cRegPwrKey, 0);
}

void pmic_update(void)
{
    // TODO
#if 0
    if (pmic_read(I2cRegId) != 0x5d) {
        // Cannot communicate with PMU?
        return;
    }
#endif
}
