#include "Platform_Types.h" /* float32 */

#include "temt6000.h"
#include "adc.h"
#include "pit.h"

#define TEMT6000_PERIOD_MS 500

static const float32 VREF_volt = 3.3;
static const uint32  IRES_ohm  = 10000;

struct temt6000_config {
	sint16 rd_timer_id;
};

static struct temt6000_config temt6000;

/*
 * @brief Initialize TEMT6000 module
 * @param None
 * @return None
 */
void temt6000_init(void)
{
	sint16 id;

	id = pit_add_timer(pit_ms_to_ticks(TEMT6000_PERIOD_MS));
	if (id >= 0) {
		temt6000.rd_timer_id = id;
	}
}

/*
 * @brief Main TEMT6000 module function
 * @param None
 * @return None
 */
void temt6000_main(void)
{
	float32 r;

	if (pit_elapsed(temt6000.rd_timer_id) == 1) {
		r = temt6000_read();

		pit_restart_timer(temt6000.rd_timer_id);
	}
}

/*
 * @brief Read lux from TEMT6000 sensor
 * @param None
 * @return float32: lux reading
 */
float32 temt6000_read(void)
{
	uint16  raw = 0;
	float32 volts, amps, uamps, lux;

	raw = adc_read();

	volts = raw * VREF_volt / ((float32) 1023.0); /* Regla de tres */
	amps  = volts / IRES_ohm;
	uamps = amps * ((float32) 1000000.0);
	lux   = uamps * 2;

	return lux;
}
