#ifndef _LED_H
#define _LED_H

int led_init(void);
int led_tx_on(void);
int led_tx_off(void);
int led_rx_on(void);
int led_rx_off(void);
void led_hang(unsigned long delay);

#endif
