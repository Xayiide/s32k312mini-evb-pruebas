#include "Pwm.h" /* Pwm_SetDutyCycle */
#include "Pwm_Cfg.h" /* EMIOS_1_CH_12 */
#include "Platform_Types.h" /* uint */

#include "led.h"
#include "pit.h"

#define RED_CH   EMIOS_1_CH_12
#define GREEN_CH EMIOS_1_CH_13
#define BLUE_CH  EMIOS_1_CH_14

#define DIM_OFF  0x8000
#define DIM_ON   0x7F00

#define GREEN_LED_COUNTER_MS 1000
static sint16 green_led_counter_id;

/*
 * @brief Initialize LED module
 * @param None
 * @return None
 */
void led_init(void)
{
	sint16 id;

	Pwm_SetDutyCycle(RED_CH, 0x8000);
	Pwm_SetDutyCycle(GREEN_CH, 0x8000);
	Pwm_SetDutyCycle(BLUE_CH, 0x8000);

	id = pit_add_timer(pit_ms_to_ticks(GREEN_LED_COUNTER_MS));
	if (id >= 0)
		green_led_counter_id = id;

}

/*
 * @brief Main LED function
 * @param None
 * @return None
 */
void led_main(void)
{
	static uint16 dim = DIM_ON;
	if (pit_elapsed(green_led_counter_id) == 1) {
		if (dim == DIM_ON)
			dim = DIM_OFF;
		else
			dim = DIM_ON;

		Pwm_SetDutyCycle(GREEN_CH, dim);
		pit_restart_timer(green_led_counter_id);
	}
#if 0
	static uint16 dim = 0x8000;
	static uint8 rgb_green_dir = 0;
	if (green_led_counter == 0) {
		if (dim >= 0x7F00)
			rgb_green_dir = 1;

		if (dim <= 0x7000)
			rgb_green_dir = 0;

		if (rgb_green_dir == 0) {
			dim += 0x100;
		} else {
			dim -= 0x100;
		}

		Pwm_SetDutyCycle(EMIOS_1_CH_13, dim);
		green_led_counter = GREEN_LED_COUNTER;
	}
#endif
}

/*
 * @brief Set red LED status
 * @param st: status to be set
 * @return None
 */
void led_set_r(uint8 st)
{
	if (st == LED_ON)
		Pwm_SetDutyCycle(RED_CH, DIM_ON);
	else
		Pwm_SetDutyCycle(RED_CH, DIM_OFF);
}

/*
 * @brief Set green LED status
 * @param st: status to be set
 * @return None
 */
void led_set_g(uint8 st)
{
	if (st == LED_ON)
		Pwm_SetDutyCycle(GREEN_CH, DIM_ON);
	else
		Pwm_SetDutyCycle(GREEN_CH, DIM_OFF);
}

/*
 * @brief Set blue LED status
 * @param st: status to be set
 * @return None
 */
void led_set_b(uint8 st)
{
	if (st == LED_ON)
		Pwm_SetDutyCycle(BLUE_CH, DIM_ON);
	else
		Pwm_SetDutyCycle(BLUE_CH, DIM_OFF);
}
