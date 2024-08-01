#include <SPI.h>
#include "epd_private.h"

static const unsigned int gpio_epd_busy = 16;
static const unsigned int gpio_epd_ncs  = 15;
static const unsigned int gpio_epd_dc   = 0;
static const unsigned int gpio_epd_rst  = 2;

void init_epd(void)
{
    SPI.begin();
    // Maximum frequency 20MHz
    SPI.beginTransaction(SPISettings(20 * 1000 * 1000, MSBFIRST, SPI_MODE0));

    digitalWrite(gpio_epd_rst, HIGH);
    digitalWrite(gpio_epd_dc,  HIGH);
    digitalWrite(gpio_epd_ncs, HIGH);
    pinMode(gpio_epd_busy, INPUT);
    pinMode(gpio_epd_rst,  OUTPUT);
    pinMode(gpio_epd_dc,   OUTPUT);
    pinMode(gpio_epd_ncs,  OUTPUT);
}

void systick_delay(uint32_t ms)
{
    delay(ms);
}

void sleep_ms(uint32_t ms)
{
    delay(ms);
}

void gpio_set_rst(bool val)
{
    digitalWrite(gpio_epd_rst, val ? HIGH : LOW);
}

void gpio_set_dc(bool val)
{
    digitalWrite(gpio_epd_dc, val ? HIGH : LOW);
}

bool gpio_get_busy(void)
{
    return digitalRead(gpio_epd_busy);
}

void spi_master_enable(bool val)
{
    digitalWrite(gpio_epd_ncs, val ? LOW : HIGH);
}

void spi_master_transmit(uint8_t val)
{
    SPI.transfer(val);
}
