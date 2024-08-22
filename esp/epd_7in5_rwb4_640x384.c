#include "epd_private.h"

#define EPD_WIDTH       640
#define EPD_HEIGHT      384

static void epd_reset(void)
{
    spi_master_enable(0);
    gpio_set_rst(1);
    sleep_ms(100);
    gpio_set_rst(0);
    sleep_ms(2);
    gpio_set_rst(1);
    sleep_ms(100);
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
    spi_master_enable(1);
    gpio_set_dc(0);
    spi_master_transmit(cmd);
}

static void epd_data(uint8_t v)
{
    gpio_set_dc(1);
    spi_master_transmit(v);
}

static void epd_update_start(void)
{
    epd_cmd(0x12); // DISPLAY_REFRESH
}

static void epd_func_update(const uint8_t *pimg, uint32_t ofs, uint32_t len)
{
    if (ofs == 0 && len != 0)
        epd_cmd(0x10);
    for (uint32_t i = 0; i < len; i++)
        epd_data(pimg[i]);
    if (ofs + len >= EPD_WIDTH * EPD_HEIGHT / 2)
        epd_update_start();
}

static void epd_func_init(void)
{
    epd_reset();
    epd_busy();

    epd_cmd(0x01); // POWER_SETTING
    epd_data(0x37);
    epd_data(0x00);

    epd_cmd(0x00); // PANEL_SETTING
    epd_data(0xcf);
    epd_data(0x08);

    epd_cmd(0x30); // PLL_CONTROL
    epd_data(0x3c); // PLL:  0-15:0x3C, 15+:0x3A

    epd_cmd(0x82); // VCM_DC_SETTING
    epd_data(0x28); //all temperature  range

    epd_cmd(0x06); // BOOSTER_SOFT_START
    epd_data(0xc7);
    epd_data(0xcc);
    epd_data(0x15);

    epd_cmd(0x50); // VCOM AND DATA INTERVAL SETTING
    epd_data(0x77);

    epd_cmd(0x60); // TCON_SETTING
    epd_data(0x22);

    epd_cmd(0x65); // FLASH CONTROL
    epd_data(0x00);

    epd_cmd(0x61);  // TCON_RESOLUTION
    epd_data(EPD_WIDTH >> 8); // source 640
    epd_data(EPD_WIDTH & 0xff);
    epd_data(EPD_HEIGHT >> 8); // gate 384
    epd_data(EPD_HEIGHT & 0xff);

    epd_cmd(0xe5); // FLASH MODE
    epd_data(0x03);

    epd_cmd(0x04); // POWER ON
    epd_busy();
}

const epd_func_t *epd_7in5_rwb4_640x384(void)
{
    static const epd_func_t func = {
        .init = &epd_func_init,
        .wait = &epd_busy,
        .update = &epd_func_update,
    };
    return &func;
}
