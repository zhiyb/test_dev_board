#include "sht.h"
#include "dev.h"
#include "i2c.h"
#include "timer.h"
#include "led.h"

#define I2C_ADDR    0x44

#define CMD_MEASURE_HIGH_PRECISION      0xfd
#define CMD_MEASURE_MEDIUM_PRECISION    0xf6
#define CMD_MEASURE_LOW_PRECISION       0xe0
#define CMD_SERIAL_NUMBER               0x89

static enum : uint8_t {
    StateIdle,
    StatePowerOn,
    StateMeasure,
} state;

static uint8_t retry;

void sht_trigger_update(void)
{
    dev_pwr_req(DevSHT, true);
}

void sht_powered_on(void)
{
    // Power on wait time 1ms
    state = StatePowerOn;
    timer1_restart_ms(1);
}

void sht_timer_irq(void)
{
    uint8_t cmd = CMD_MEASURE_HIGH_PRECISION;
    uint8_t buf[6];

    switch (state) {
    case StatePowerOn:
        // Power on wait complete
        i2c_master_init_resync();
        // Measure T & RH with high precision
        if (!i2c_master_write(I2C_ADDR, &cmd, 1)) {
            // Sensor didn't ACK, shutdown
            break;
        }
        // Sensor ACK'ed, wait for measurement
        state = StateMeasure;
        retry = 5;
        timer1_restart_ms(8);
        return;

    case StateMeasure:
        // Measure wait complete
        if (!i2c_master_read(I2C_ADDR, &buf[0], 6)) {
            if (retry--) {
                // Sensor didn't ACK, give it more time
                timer1_restart_ms(1);
                return;
            }
        }
        // Measurement complete
        state = StateIdle;
        break;
    }

    dev_pwr_req(DevSHT, false);
}
