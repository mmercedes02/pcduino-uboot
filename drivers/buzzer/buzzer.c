/*
 * 通过对GPIO进行控制来实现gpio6789的高低电平控制。
 * 此处将复杂操作包装成各种实现。
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

int buzzer_init(void)
{
	sunxi_gpio_set_cfgpin(SUNXI_GPH(11), SUNXI_GPIO_OUTPUT);
	sunxi_gpio_set_cfgpin(SUNXI_GPH(12), SUNXI_GPIO_OUTPUT);
	sunxi_gpio_set_cfgpin(SUNXI_GPH(13), SUNXI_GPIO_OUTPUT);
	sunxi_gpio_set_cfgpin(SUNXI_GPH(14), SUNXI_GPIO_OUTPUT);
	sunxi_gpio_set_pull(SUNXI_GPH(11), 1);
	sunxi_gpio_set_pull(SUNXI_GPH(12), 1);
	sunxi_gpio_set_pull(SUNXI_GPH(13), 1);
	sunxi_gpio_set_pull(SUNXI_GPH(14), 1);
	return 0;
}

int buzzer_high(void)
{
	sunxi_gpio_set_dat_bit(SUNXI_GPH(11), 1);
	sunxi_gpio_set_dat_bit(SUNXI_GPH(12), 1);
	sunxi_gpio_set_dat_bit(SUNXI_GPH(13), 1);
	sunxi_gpio_set_dat_bit(SUNXI_GPH(14), 1);
	return 0;
}

int buzzer_low(void)
{
	sunxi_gpio_set_dat_bit(SUNXI_GPH(11), 0);
	sunxi_gpio_set_dat_bit(SUNXI_GPH(12), 0);
	sunxi_gpio_set_dat_bit(SUNXI_GPH(13), 0);
	sunxi_gpio_set_dat_bit(SUNXI_GPH(14), 0);
	return 0;
}

void buzzer_hang(unsigned long hz)
{
	int delay = 1008000000 / (hz*2);
	buzzer_init();
	while (1) {
		buzzer_high();
		sdelay(delay);
		buzzer_low();
		sdelay(delay);
	};
}
