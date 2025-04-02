#pragma once

// Timer1: Key debouncing and LED activity light
void timer1_restart_ms(uint8_t ms);
void timer1_restart_debouncing(void);
bool timer1_enabled(void);
void timer1_wait_sleep(void);
