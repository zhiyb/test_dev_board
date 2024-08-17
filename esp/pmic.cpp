#include <cstdint>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "pmic.h"
#include "http.h"

#define I2C_ADDR    ((uint8_t)0x39)

typedef enum {
    I2cRegId = 0,
    I2cRegState,
    I2cRegAdcTempL,
    I2cRegAdcTempH,
    I2cRegAdcVbgL,
    I2cRegAdcVbgH,
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

void pmic_update(void)
{
    uint8_t state = pmic_read(I2cRegState);
    uint16_t adc_val[2] = {0};
    pmic_read_multi(I2cRegAdcTempL, (uint8_t *)&adc_val[0], 4);
    // Serial.printf("state=0x%02x adc_0=0x%04x adc_1=0x%04x\n",
    //     (int)state, (int)adc_val[0], (int)adc_val[1]);

    char url[96];
    char data[32];
    uint32_t len;
    String resp;

    // Upload Temperature TODO conversion
    sprintf(url, "%sid=%s&key=%s", url_base, id, "temp");
    len = sprintf(data, "%d", adc_val[0]);
    resp = http_post(url, nullptr, &data[0], len);
    // Upload Vcc
    sprintf(url, "%sid=%s&key=%s", url_base, id, "vcc");
    len = sprintf(data, "%g", (float)adc_val[1] / 4096.0);
    resp = http_post(url, nullptr, &data[0], len);

    // TODO Get next wakeup time
}
