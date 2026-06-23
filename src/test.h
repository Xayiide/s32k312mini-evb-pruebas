#ifndef TEST_H_
#define TEST_H_

#include "Platform_Types.h" /* uint */

#include "adc.h"

void test_div0(void)
{
	volatile uint32 a = 1, b = 0;
	volatile uint32 result;

	result = a / b;

	__asm volatile ("BKPT #0");
}

void test_adc(void)
{
	uint16 read = 0;

	read = adc_read();
}

#endif /* TEST_H_ */
