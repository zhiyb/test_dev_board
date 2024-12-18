#pragma once

// Timer0: Key debouncing and LED activity light
void timer0_init(void);
void timer0_restart(void);
bool timer0_enabled(void);
