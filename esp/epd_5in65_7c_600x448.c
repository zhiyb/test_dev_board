#include "epd_private.h"

#define EPD_WIDTH       600
#define EPD_HEIGHT      448

static void epd_reset(void)
{
    spi_master_enable(0);
    gpio_set_rst(1);
    systick_delay(200);
    gpio_set_rst(0);
    systick_delay(2);
    gpio_set_rst(1);
    sleep_ms(200);
}

static void epd_busy(void)
{
    do
        sleep_ms(10);
    while (!gpio_get_busy());
}

static void epd_cmd(uint8_t cmd)
{
    spi_master_enable(0);
    gpio_set_dc(0);
    spi_master_enable(1);
    spi_master_transmit(cmd);
}

static void epd_data(uint8_t v)
{
    spi_master_enable(0);
    gpio_set_dc(1);
    spi_master_enable(1);
    spi_master_transmit(v);
}

static void epd_update_start(void)
{
    epd_cmd(0x04);
    epd_busy();

    epd_cmd(0x12);
    //epd_busy();
}

static void epd_func_update(const uint8_t *pimg, uint32_t ofs, uint32_t len)
{
    if (ofs == 0 && len != 0)
        epd_cmd(0x10);
    for (uint32_t i = 0; i < len; i++)
        epd_data(pimg[i]);
    if (ofs + len >= EPD_HEIGHT * ((EPD_WIDTH + 1) / 2))
        epd_update_start();
}

static void epd_func_init(void)
{
    epd_reset();
    epd_busy();

    epd_cmd(0x00);
    epd_data(0xEF);
    epd_data(0x08);

    epd_cmd(0x01);
    epd_data(0x37);
    epd_data(0x00);
    epd_data(0x23);
    epd_data(0x23);

    epd_cmd(0x03);
    epd_data(0x00);

    epd_cmd(0x06);
    epd_data(0xC7);
    epd_data(0xC7);
    epd_data(0x1D);

    epd_cmd(0x30);
    epd_data(0x39);

    epd_cmd(0x41);
    epd_data(0x00);

    epd_cmd(0x50);
    epd_data(0x37);

    epd_cmd(0x60);
    epd_data(0x22);

    epd_cmd(0x61);
    epd_data(0x02);
    epd_data(0x58);
    epd_data(0x01);
    epd_data(0xC0);

    epd_cmd(0xE3);
    epd_data(0xAA);

    sleep_ms(100);

    epd_cmd(0x50);
    epd_data(0x37);

    epd_cmd(0x61);//Set Resolution setting
    epd_data(0x02);
    epd_data(0x58);
    epd_data(0x01);
    epd_data(0xC0);

    epd_busy();
}

const epd_func_t *epd_5in65_7c_600x448(void)
{
    static const epd_func_t func = {
        .init = &epd_func_init,
        .wait = &epd_busy,
        .update = &epd_func_update,
    };
    return &func;
}
