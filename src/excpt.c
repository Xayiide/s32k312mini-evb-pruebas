#include "S32K312_SCB.h"    /* S32_SCB macros */
#include "Platform_Types.h" /* TRUE */

#include "excpt.h"

static void excpt_set_field(uint32 mask, uint32 value);

/*
 * @brief Enable all exceptions: memfault, busfault and usagefault
 * @param None
 * @return None
 */
void excpt_ena_all(void)
{
	uint32_t reg = S32_SCB->SHCSR;

	reg |= S32_SCB_SHCSR_MEMFAULTENA(1U) |
	       S32_SCB_SHCSR_BUSFAULTENA(1U) |
	       S32_SCB_SHCSR_USGFAULTENA(1U);

	S32_SCB->SHCSR = reg;

	/* Habilita DIV_0_TRAP y UNALIGN_TRAP */
	S32_SCB->CCR |= (uint32) (((1U) << 4) | ((1U) << 3));

}

/*
 * @brief Disable all exceptions: memfault, busfault, usagefault
 * @param None
 * @return None
 */
void excpt_dis_all(void)
{
	uint32_t reg = S32_SCB->SHCSR;

	reg &= ~(S32_SCB_SHCSR_MEMFAULTENA_MASK |
	         S32_SCB_SHCSR_BUSFAULTENA_MASK |
	         S32_SCB_SHCSR_USGFAULTENA_MASK);

	S32_SCB->SHCSR = reg;
}

/*
 * @brief Enable or disable memfault
 * @param state: status of the exception
 * @return None
 */
void excpt_set_mem(enum excpt_state state)
{
	excpt_set_field(S32_SCB_SHCSR_MEMFAULTENA_MASK,
	                S32_SCB_SHCSR_MEMFAULTENA(state));
}

/*
 * @brief Enable or disable busfault
 * @param state: status of the exception
 * @return None
 */
void excpt_set_bus(enum excpt_state state)
{
	excpt_set_field(S32_SCB_SHCSR_BUSFAULTENA_MASK,
	                S32_SCB_SHCSR_BUSFAULTENA(state));
}

/*
 * @brief Enable or disable usagefault
 * @param state: status of the exception
 * @return None
 */
void excpt_set_usg(enum excpt_state state)
{
	excpt_set_field(S32_SCB_SHCSR_USGFAULTENA_MASK,
	                S32_SCB_SHCSR_USGFAULTENA(state));
}


/*
 * @brief NMI interrupt handler
 * @param None
 * @return None
 */
void NMI_Handler(void)
{
	while(TRUE){};
}

/*
 * @brief Hardfault handler
 * @param None
 * @return None
 */
void HardFault_Handler(void)
{
	volatile uint32 hfsr  = S32_SCB->HFSR;
	volatile uint32 cfsr  = S32_SCB->CFSR;
	volatile uint32 mmfar = S32_SCB->MMFAR;
	volatile uint32 bfar  = S32_SCB->BFAR;

	(void) hfsr;
	(void) cfsr;
	(void) mmfar;
	(void) bfar;

	while(TRUE) {

	}
}

/*
 * @brief Memfault handler
 * @param None
 * @return None
 */
void MemManage_Handler(void)
{
	volatile uint32 cfsr  = S32_SCB->CFSR;
	volatile uint32 mmfar = S32_SCB->MMFAR;

	(void) cfsr;
	(void) mmfar;

	__asm volatile ("BKPT #0");

	while(TRUE) {

	}
}

/*
 * @brief Busfualt handler
 * @param None
 * @return None
 */
void BusFault_Handler(void)
{
	volatile uint32 cfsr = S32_SCB->CFSR;
	volatile uint32 bfar = S32_SCB->BFAR;

	(void) cfsr;
	(void) bfar;

	__asm volatile ("BKPT #0");

	while(TRUE) {

	}
}

/*
 * @brief Usagefault handler
 * @param None
 * @return None
 */
void UsageFault_Handler(void)
{
	volatile uint32 cfsr  = S32_SCB->CFSR;

	(void) cfsr;

	__asm volatile ("BKPT #0");

	while(TRUE) {

	}
}

/*
 * @brief Set a field in the SHCSR register
 * @param mask: uint32 for the mask
 * @param value: uint32 for the value to be set
 * @return None
 */
void excpt_set_field(uint32 mask, uint32 value)
{
	uint32 reg = S32_SCB->SHCSR;

	reg &= ~mask;
	reg |=  value;

	S32_SCB->SHCSR = reg;
}

