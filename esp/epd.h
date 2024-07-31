#pragma once

typedef enum {
    EPD_TYPE_INVALID,
    EPD_TYPE_2IN13_RWB,
    EPD_TYPE_4IN2_RWB,
    EPD_TYPE_5IN65_FULL,
} epd_type_t;

static const unsigned int gpio_epd_busy = 16;
static const unsigned int gpio_epd_ncs  = 15;
static const unsigned int gpio_epd_dc   = 0;
static const unsigned int gpio_epd_rst  = 2;

void init_epd(void);
