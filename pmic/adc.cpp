#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "adc.h"
#include "eeprom.h"

// Take multiple measurements to wait until voltages are stable
// Bandgap init requires 70us + capacitor charging
static const uint8_t adc_samples = 32;

static enum {AdcIdle = 0, AdcTemp, AdcVbg} adc_state;
static uint16_t adc_val[2];
static uint8_t adc_count;

static inline void adc_start_temp(void)
{
    // Start temperature measurement
    // Internal 1.1V reference, right adjusted, channel 8
    ADMUX = _BV(REFS1) | _BV(REFS0) | 8;
    // Max resolution requires ADC clock between 50 kHz and 200 kHz
    // Start ADC, interrupt, prescaler = DIV128
    ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADIF) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
}

static inline void adc_start_vcc(void)
{
    // Start AVcc measurement
    // AVcc reference, right adjusted, channel 1.1V VBG
    ADMUX = _BV(REFS0) | 0b1110;
    // Max resolution requires ADC clock between 50 kHz and 200 kHz
    // Start ADC, interrupt, prescaler = DIV128
    ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADIF) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
}

static inline void adc_cal(void)
{
    // Convert from calibrated VBG to Vcc
    adc_val[AdcChVcc] = (uint32_t)eeprom_read_word(&eeprom_data->adc_vbg) * 1024 / adc_val[AdcChVcc];
}

static void adc_next(void)
{
    int16_t delta;
    switch (adc_state) {
    case AdcIdle:
        adc_state = AdcTemp;
        adc_count = adc_samples;
        adc_start_temp();
        break;
    case AdcTemp:
        adc_val[AdcChTemp] = ADCW;
        adc_count -= 1;
        if (adc_count != 0) {
            adc_start_temp();
        } else {
            adc_state = AdcVbg;
            adc_count = adc_samples;
            adc_start_vcc();
        }
        break;
    case AdcVbg:
        adc_val[AdcChVcc] = ADCW;
        adc_count -= 1;
        if (adc_count != 0) {
            adc_start_vcc();
        } else {
            adc_cal();
            adc_state = AdcIdle;
        }
        break;
    }

    extern bool act(bool reset);
    act(true);
}

bool adc_busy(void)
{
    return adc_state != AdcIdle;
}

void adc_start(void)
{
    if (!adc_busy())
        adc_next();
}

uint16_t adc_result(uint8_t ch)
{
    return adc_val[ch];
}

ISR(ADC_vect)
{
    adc_next();
}
