#include "Pit_Ip.h"             /* Pit_Ip_Init, Pit_Ip_*                   */
#include "Pit_Ip_SA_PBcfg.h"    /* Pit_0_ChannelConfig_PB                  */
#include "Pit_Ip_Cfg_Defines.h" /* PIT_0_ISR                               */
#include "S32K312_COMMON.h"     /* PIT0_IRQn                               */
#include "IntCtrl_Ip.h"         /* IntCtrl_Ip_InstallHandler, IntCtrl_Ip_* */
#include "Platform_Types.h"     /* uint                                    */
#include "Compiler.h"           /* NULL_PTR                                */
#include "Mcu.h"                /* Mcu_GetClockFrequency                   */

#include "pit.h"

extern ISR(PIT_0_ISR);


#define PIT0_INST 0
#define PIT0_CH0  0
#define TIMERS_ARRAY_SIZE 32

#define PIT_IRQ_HZ   8000U /* PIT interrumpe cada 125 us */
#define PIT_MS_TICKS (PIT_IRQ_HZ / 1000U)


struct pit_timer {
	volatile uint32 counter;
	uint32          limit;
};

struct pitmgr {
	struct pit_timer timers[TIMERS_ARRAY_SIZE];
	uint8            ntimers;
};


static struct pitmgr pit;


void pit_callback(uint8 channel)
{
	(void) channel;

	uint8 i;

	for (i = 0; i < pit.ntimers; i++) {
		if (pit.timers[i].counter < pit.timers[i].limit) {
			pit.timers[i].counter++;
		}
	}

	return;
}

/*
 * @brief this function initializes the PIT module
 * @param None
 * @return None
 */
void pit_init(void)
{
	uint8  i;
	uint32 clk   = Mcu_GetClockFrequency(PIT0_CLK);
	uint32 count = clk / PIT_IRQ_HZ;

	/* PIT Timer config */
	Pit_Ip_Init(PIT0_INST, &PIT_0_InitConfig_PB);
	Pit_Ip_InitChannel(PIT0_INST, &PIT_0_ChannelConfig_PB[0]);

	Pit_Ip_StartChannel(PIT0_INST, PIT0_CH0, count);
	Pit_Ip_EnableChannelInterrupt(PIT0_INST, PIT0_CH0);

	/* Install IRQ handlers */
	IntCtrl_Ip_InstallHandler(PIT0_IRQn, PIT_0_ISR, NULL_PTR);
	IntCtrl_Ip_EnableIrq(PIT0_IRQn);

	pit.ntimers = 0;
	for (i = 0; i < TIMERS_ARRAY_SIZE; i++) {
		pit.timers[i].counter = 0;
		pit.timers[i].limit   = 0;
	}
}

/*
 * @brief add a timer to the pit module
 * @param period: uint32 period in PIT ticks
 * @return sint16: id of the timer created, or -1 on error
 */
sint16 pit_add_timer(uint32 period)
{
	sint16 id;

	if (pit.ntimers < TIMERS_ARRAY_SIZE) {
		pit.timers[pit.ntimers].counter = 0;
		pit.timers[pit.ntimers].limit   = period;
		id = pit.ntimers++;
	} else {
		id = -1;
	}

	return id;
}

/*
 * @brief Restart a timer
 * @param sint16 id: id of the timer to restart
 * @return None
 */
void pit_restart_timer(sint16 id)
{
	if (id < pit.ntimers) {
		pit.timers[id].counter = 0;
	}
}

/*
 * @brief Return whether a timer is elapsed or not
 * @param sint16 id: id of the timer to check
 * @return uint8: 1 means it elapsed, 0 means it didn't
 */
uint8 pit_elapsed(sint16 id)
{
	uint8 elapsed = 0;

	if (id < pit.ntimers) {
		if (pit.timers[id].counter == pit.timers[id].limit)
			elapsed = 1;
	}

	return elapsed;
}

/*
 * @brief Set a counter to a count value
 * @param sint16 id: id of the counter to set
 * @param uint32 count: value to set the counter to
 * @return None
 */
void pit_set_count(sint16 id, uint32 count)
{
	if (id < pit.ntimers) {
		pit.timers[id].counter = count;
	}
}

/*
 * @brief Set a new limit for a timer
 * @param sint16 id: id of the counter to set
 * @param uint32 limit: new limit to set to the counter
 * @return None
 */
void pit_set_limit(sint16 id, uint32 limit)
{
	if (id < pit.ntimers) {
		pit.timers[id].limit = limit;
	}
}

/*
 * @brief Convert from milliseconds to PIT ticks
 * @param milliseconds
 * @return uint32: pit ticks
 */
uint32 pit_ms_to_ticks(uint32 ms)
{
	uint32 ticks;

	ticks = ms * PIT_MS_TICKS;

	return ticks;
}
