#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "stm32f4xx_hal.h"

#include "key.h"

#define D_KEY_PORT	GPIOA
#define D_KEY_PIN		GPIO_PIN_6 

uint8_t getkey(void)
{
	static uint32_t t_prev = 0;
	static uint8_t k_prev = 1;
	uint32_t t_curr;
	uint8_t k_curr;
	uint8_t ret = 0;
	
	t_curr = HAL_GetTick();
	
	if ((t_curr - t_prev) < 150) return 0;
	t_prev = t_curr;

	k_curr = (uint8_t)HAL_GPIO_ReadPin(D_KEY_PORT, D_KEY_PIN);

	if (k_prev == 0 && k_curr == 1) ret = 1;
  else ret = 0;
	
	k_prev = k_curr;
	
	return ret;
}
