#ifndef _BUZZER_H
#define _BUZZER_H

int buzzer_init(void);
int buzzer_high(void);
int buzzer_low(void);
void buzzer_hang(unsigned long hz);

#endif
