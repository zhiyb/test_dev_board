#include "epd_test.h"

#define EPD_WIDTH       122
#define EPD_HEIGHT      250
#define EPD_IMAGE_SIZE  (EPD_HEIGHT * ((EPD_WIDTH + 7) / 8))

#define CMD_DRIVER_OUTPUT_CONTROL   0x01
#define CMD_GATE_VOLTAGE_CONTROL    0x03
#define CMD_SOURCE_VOLTAGE_CONTROL  0x04
#define CMD_SOFT_START_CONTROL      0x0c
#define CMD_DATA_ENTRY_MODE_SETTING 0x11
#define CMD_SW_RESET                0x12
#define CMD_TEMP_SENSOR_CONTROL     0x18
#define CMD_TEMP_READ               0x1b
#define CMD_MASTER_ACTIVATION       0x20
#define CMD_DISPLAY_UPDATE_CONTROL1 0x21
#define CMD_DISPLAY_UPDATE_CONTROL2 0x22
#define CMD_WRITE_RAM_BW            0x24
#define CMD_WRITE_RAM_RED           0x26
#define CMD_ACVCOM_SETTING          0x2b
#define CMD_OTP_READ                0x2d
#define CMD_USER_ID_READ            0x2e
#define CMD_STATUS                  0x2f
#define CMD_BORDER_WAVEFORM_CONTROL 0x3c
#define CMD_SET_RAM_X_ADDRESS       0x44
#define CMD_SET_RAM_Y_ADDRESS       0x45
#define CMD_SET_RAM_X_COUNTER       0x4e
#define CMD_SET_RAM_Y_COUNTER       0x4f
#define CMD_ANALOG_BLOCK_CONTROL    0x74
#define CMD_DIGITAL_BLOCK_CONTROL   0x7e

static void epd_reset(void)
{
    spi_master_enable(0);
    gpio_set_rst(1);
    systick_delay(100);
    gpio_set_rst(0);
    systick_delay(2);
    gpio_set_rst(1);
    sleep_ms(100);
}

static void epd_busy(void)
{
    do
        sleep_ms(10);
    while (gpio_get_busy());
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

static void epd_window(uint32_t Xstart, uint32_t Ystart, uint32_t Xend, uint32_t Yend)
{
    epd_cmd(CMD_SET_RAM_X_ADDRESS);
    epd_data((Xstart>>3) & 0xFF);
    epd_data((Xend>>3) & 0xFF);

    epd_cmd(CMD_SET_RAM_Y_ADDRESS);
    epd_data(Ystart & 0xFF);
    epd_data((Ystart >> 8) & 0xFF);
    epd_data(Yend & 0xFF);
    epd_data((Yend >> 8) & 0xFF);
}

static void epd_cursor(uint32_t Xstart, uint32_t Ystart)
{
    epd_cmd(CMD_SET_RAM_X_COUNTER);
    epd_data(Xstart & 0xFF);

    epd_cmd(CMD_SET_RAM_Y_COUNTER);
    epd_data(Ystart & 0xFF);
    epd_data((Ystart >> 8) & 0xFF);
}

static void epd_update(void)
{
    epd_cmd(CMD_MASTER_ACTIVATION);
    epd_busy();
}

static void epd_update_start(void)
{
    epd_cmd(CMD_MASTER_ACTIVATION);
}

static void epd_display_start(const uint8_t *bw, const uint8_t *red)
{
    static const uint32_t w = (EPD_WIDTH + 7) / 8, h = EPD_HEIGHT;

    // BW\RED 0     1
    // 0      BLACK RED
    // 1      WHITE RED

    epd_cmd(CMD_WRITE_RAM_BW);
    for (uint32_t j = 0; j < h; j++)
        for (uint32_t i = 0; i < w; i++)
            epd_data(bw[i + j * w]);

    epd_cmd(CMD_WRITE_RAM_RED);
    for (uint32_t j = 0; j < h; j++)
        for (uint32_t i = 0; i < w; i++)
            epd_data(~red[i + j * w]);

    epd_update_start();
}

static void epd_func_init(void)
{
    epd_reset();

    epd_busy();
    epd_cmd(CMD_SW_RESET);
    epd_busy();

    epd_cmd(CMD_ANALOG_BLOCK_CONTROL);
    epd_data(0x54);
    epd_cmd(CMD_DIGITAL_BLOCK_CONTROL);
    epd_data(0x3b);
    epd_busy();

    epd_cmd(CMD_DRIVER_OUTPUT_CONTROL);
    epd_data(0xf9);
    epd_data(0x00);
    epd_data(0x00);

    epd_cmd(CMD_DATA_ENTRY_MODE_SETTING);
    epd_data(0x03);

    epd_cmd(CMD_BORDER_WAVEFORM_CONTROL);
    epd_data(0x05);

    epd_cmd(0x2c);
    epd_data(0x55);

    // Use the internal temperature sensor
    epd_cmd(CMD_TEMP_SENSOR_CONTROL);
    epd_data(0x80);

    // Load display mode 1 LUT
    epd_cmd(CMD_DISPLAY_UPDATE_CONTROL2);
    epd_data(0xb1);
    epd_cmd(0x20);
    epd_busy();

    // Display mode 1
    epd_cmd(CMD_DISPLAY_UPDATE_CONTROL2);
    epd_data(0xc7);

    epd_window(0, 0, EPD_WIDTH-1, EPD_HEIGHT-1);
    epd_cursor(0, 0);
    epd_busy();
}

static void epd_func_update(const uint8_t *pimg)
{
    const uint8_t *img_bw  = pimg;
    const uint8_t *img_red = pimg + EPD_IMAGE_SIZE;
    epd_display_start(img_bw, img_red);
}

const epd_func_t *epd_func_2in13(void)
{
    static const epd_func_t func = {
        .init = &epd_func_init,
        .wait = &epd_busy,
        .update = &epd_func_update,
    };
    return &func;
}

void epd_test_2in13(void)
{
    static const uint8_t bw[] = {
#include "test_patterns/tp2_128x250_k.h"
    };
    static const uint8_t red[] = {
#include "test_patterns/tp2_128x250_r.h"
    };

    epd_display_start(bw, red);
    epd_busy();
}
