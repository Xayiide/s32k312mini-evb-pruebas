
/* Including necessary configuration files. */
#include "Clock_Ip.h" /* Clock_Ip_Init */
#include "Clock_Ip_PBcfg.h" /* Mcu_aClockConfigPB[0] */
#include "Siul2_Port_Ip.h" /* Siul2_Port_Ip_Init, NUM_OF_CONFIGURED_PINS */
#include "Siul2_Port_Ip_Cfg.h" /* NUM_OF_CONFIGURED_PINS_... */
#include "OsIf.h" /* OsIf_Init */
#include "Mcu.h" /* Mcu_Init */
#include "Mcu_PBcfg.h" /* Mcu_Config */
#include "CDD_Mcl.h" /* Mcl_Init */
#include "CDD_Mcl_Cfg.h" /* Mcl_Config */
#include "Pwm.h" /* Pwm_Init */
#include "Pwm_PBcfg.h" /* Pwm_Config */
#include "Platform_Types.h" /* uint */
#include "Power_Ip.h" /* Power_Ip_GetResetReason */

#include <stddef.h> /* NULL */

#include "pit.h"
#include "led.h"
#include "excpt.h"
#include "spi.h"
#include "button.h"
#include "uart.h"


/*!
  \brief The main function for the project.
  \details The startup initialization sequence is the following:
 * - startup asm routine
 * - main()
*/
int main(void)
{
	/* Leer causa del reset antes de que nada lo borre */
	//volatile uint32_t reset_cause = S32_SCB->AIRCR;
	//volatile Power_Ip_ResetType rc = Power_Ip_GetResetReason();
	//__asm volatile ("BKPT #0");

	Clock_Ip_Init(&Mcu_aClockConfigPB[0]);
	OsIf_Init(NULL);

	Mcu_Init(&Mcu_Config);
	Mcl_Init(&Mcl_Config);
	Pwm_Init(&Pwm_Config);
	/* Initialize all pins using the Siul2_Port driver */
	Siul2_Port_Ip_Init(
			NUM_OF_CONFIGURED_PINS_PortContainer_0_BOARD_InitPeripherals,
			g_pin_mux_InitConfigArr_PortContainer_0_BOARD_InitPeripherals);


	/* Inicializa los módulos */
	pit_init();
	led_init();
	spi_init();
	button_init();
	uart_init();
	excpt_ena_all();

	//test_div0();

	for(;;)
	{
		led_main();
		spi_main();
		button_main();
		uart_main();
	}

}

