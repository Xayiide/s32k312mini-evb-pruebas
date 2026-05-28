#include "Siul2_Port_Ip_Cfg.h" /* BUTTONSWx_PORT + PIN */
#include "Siul2_Dio_Ip.h"      /* Siul2_Dio_Ip_ReadPin */

#include "button.h"
#include "led.h"


static uint8 bsw2_last_st = 0;
static uint8 bsw3_last_st = 0;

static uint8 bsw2_st = 0;
static uint8 bsw3_st = 0;


static void handle_bsw2(void);
static void handle_bsw3(void);

void button_init(void)
{
	bsw2_last_st = Siul2_Dio_Ip_ReadPin(BUTTONSW2_PORT, BUTTONSW2_PIN);
	bsw3_last_st = Siul2_Dio_Ip_ReadPin(BUTTONSW3_PORT, BUTTONSW3_PIN);
}

void button_main(void)
{
	uint8 sw2_pressed = 0, sw3_pressed = 0;

	bsw2_st = Siul2_Dio_Ip_ReadPin(BUTTONSW2_PORT, BUTTONSW2_PIN);
	bsw3_st = Siul2_Dio_Ip_ReadPin(BUTTONSW3_PORT, BUTTONSW3_PIN);

	if (bsw2_last_st == 1 && bsw2_st == 0) {
		sw2_pressed = 1;
	}

	if (bsw3_last_st == 1 && bsw3_st == 0) {
		sw3_pressed = 1;
	}


	if (sw2_pressed == 1) {
		/* TODO: debounce */
		handle_bsw2();
	}

	if (sw3_pressed == 1) {
		/* TODO: debounce */
		handle_bsw3();
	}

	bsw2_last_st = bsw2_st;
	bsw3_last_st = bsw3_st;

}



void handle_bsw2(void)
{
	static uint8 st = 0;

	if (st == 0) {
		led_set_r(LED_ON);
		st = 1;
	}
	else {
		led_set_r(LED_OFF);
		st = 0;
	}
}

void handle_bsw3(void)
{

}
