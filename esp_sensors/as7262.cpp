#include <ESP8266WiFi.h>
#include "as7262.h"
#include "i2c.h"

#define I2C_ADDR    ((uint8_t)0x49)

typedef enum {
    RegHWVersion    = 0x00,
    RegFWVersion    = 0x02,
    RegControlSetup = 0x04,
    RegIntegTime    = 0x05,
    RegDeviceTemp   = 0x06,
    RegLEDControl   = 0x07,
    RegChannelV     = 0x08,
    RegChannelB     = 0x0a,
    RegChannelG     = 0x0c,
    RegChannelY     = 0x0e,
    RegChannelO     = 0x10,
    RegChannelR     = 0x12,
    RegCalV         = 0x14,
    RegCalB         = 0x18,
    RegCalG         = 0x1c,
    RegCalY         = 0x20,
    RegCalO         = 0x24,
    RegCalR         = 0x28,
} reg_t;

static uint8_t as7262_read(uint8_t addr)
{
    // Wait until write register ready
    for (;;) {
        uint8_t status = i2c_read_reg(I2C_ADDR, 0);
        if (!(status & (1 << 1)))
            break;
    }

    // Send virtual register address
    i2c_write_reg(I2C_ADDR, 1, addr);

    // Wait until read register has valid data
    for (;;) {
        uint8_t status = i2c_read_reg(I2C_ADDR, 0);
        if (status & (1 << 0))
            break;
    }

    // Read data
    return i2c_read_reg(I2C_ADDR, 2);
}

static void as7262_write(uint8_t addr, uint8_t val)
{
    // Wait until write register ready
    for (;;) {
        uint8_t status = i2c_read_reg(I2C_ADDR, 0);
        if (!(status & (1 << 1)))
            break;
    }

    // Send virtual register address
    i2c_write_reg(I2C_ADDR, 1, addr | 0x80);

    // Wait until read register has valid data
    for (;;) {
        uint8_t status = i2c_read_reg(I2C_ADDR, 0);
        if (!(status & (1 << 1)))
            break;
    }

    // Write data
    i2c_write_reg(I2C_ADDR, 1, val);
}

static void as7262_reset()
{
    // Special dance to synchronise I2C virtual register access
    // Serial.print("AS7262 sync\n");
    uint8_t step = 0;
    for (;;) {
        uint8_t status = i2c_read_reg(I2C_ADDR, 0);
        if (status & (1 << 0)) {
            // Read valid
            i2c_read_reg(I2C_ADDR, 2);
            if ((step & 3) == 3)
                break;
            step |= step << 1;
        } else if (!(status & (1 << 1))) {
            // Write valid, read control register
            i2c_write_reg(I2C_ADDR, 1, 0x04);
            step |= 1;
        }
    }

    // Soft reset sensor
    Serial.print("AS7262 reset\n");
    as7262_write(RegControlSetup, 0x84);
    delay(1000);
    while (as7262_read(RegControlSetup) & (1 << 7));
}

void as7262_led(bool drv_en, uint8_t drv_current)
{
    as7262_write(RegLEDControl, (drv_current << 4) | (drv_en << 3));
}

void as7262_init(void)
{
    delay(1000);    // Power-on delay
    as7262_reset();

    uint8_t ver[4];
    Serial.print("AS7262 ver=0x");
    for (int i = 0; i < 4; i++) {
        ver[i] = as7262_read(i);
        Serial.printf("%02x", (int)ver[i]);
    }
    Serial.println();

    // Turn on LEDs
    as7262_led(false, 0);
}

void as7262_print(void)
{
    // Config
    // Interrupt disabled, gain = 64, bank = mode 3 single shot
    as7262_write(RegControlSetup, 0b00111100);

    // Read temperature
    uint8_t temp = as7262_read(RegDeviceTemp);

    // Wait until data ready
    while (!(as7262_read(RegControlSetup) & (1 << 1)));

    // Read data
    int channel[6];
    for (int ch = 0; ch < 6; ch++) {
        uint16_t v = 0;
        v |= as7262_read(RegChannelV + ch * 2 + 0) << 8;
        v |= as7262_read(RegChannelV + ch * 2 + 1) << 0;
        channel[ch] = v;
    }

    // Print data
    Serial.printf("AS7262 T=%d V=%d B=%d G=%d Y=%d O=%d R=%d\n", (int)temp,
        channel[0], channel[1], channel[2], channel[3], channel[4], channel[5]);
}
