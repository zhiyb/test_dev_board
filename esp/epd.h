#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "epd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*init)(void);
    void (*update)(const uint8_t *pimg, uint32_t ofs, uint32_t len);
    void (*wait)(void);
} epd_func_t;

const epd_func_t *epd_2in13_rwb_122x250(void);
const epd_func_t *epd_4in2_rwb_400x300(void);
const epd_func_t *epd_5in65_7c_600x448(void);
const epd_func_t *epd_7in5_rwb4_640x384(void);

void epd_init(void);
void epd_deinit(void);

#ifdef __cplusplus
}
#endif
