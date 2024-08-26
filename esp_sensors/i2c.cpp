#include <ESP8266WiFi.h>
#include <Wire.h>
#include "i2c.h"

static const unsigned int gpio_sda = 4;
static const unsigned int gpio_scl = 5;

void i2c_init(void)
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

void i2c_shutdown(void)
{
    // Force I2C pins to GND to avoid powering on I2C devices
    digitalWrite(gpio_sda, LOW);
    digitalWrite(gpio_scl, LOW);
    pinMode(gpio_sda, OUTPUT);
    pinMode(gpio_scl, OUTPUT);
}

uint8_t i2c_write_multi(uint8_t addr, uint8_t reg, const uint8_t *buf, uint8_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    for (uint8_t i = 0; i < len; i++)
        Wire.write(buf[i]);
    Wire.endTransmission(true);
    delayMicroseconds(100);
    return len;
}

uint8_t i2c_read_multi(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(addr, (size_t)len, true);
    uint8_t i = 0;
    while (Wire.available()) {
        buf[i] = Wire.read();
        i += 1;
    }
    delayMicroseconds(100);
    return i;
}
