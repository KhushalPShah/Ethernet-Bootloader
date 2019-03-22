#include <LPC17xx.h>
#include "delay.h"

 extern volatile unsigned long timer_tick;

void delay_ms(unsigned int time_ms)
{
	timer_tick = time_ms;
	while(timer_tick);
}
