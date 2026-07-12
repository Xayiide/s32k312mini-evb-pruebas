#include "Lpi2c_Ip.h" /* Lpi2c_Ip_MasterInit */
#include "Lpi2c_Ip_Sa_PBcfg.h" /* I2c_Lpi2cMaster_HwChannel1_Channel0 */
#include "Platform_Types.h" /* uint */

#include "i2c.h"
#include "pit.h"

#define RX_PERIOD_MS   10U
#define TX_PERIOD_MS   10U
#define RX_BUFFER_SIZE 32U
#define TX_BUFFER_SIZE 32U

struct i2c_config {
	sint16 tx_timer_id;
	sint16 rx_timer_id;
	uint8  tx_buffer[TX_BUFFER_SIZE];
	uint8  rx_buffer[RX_BUFFER_SIZE];
};

static struct i2c_config i2c;

/*
 * @brief Initialize I2C module
 * @param None
 * @return None
 */
void i2c_init(void)
{
	uint8  i;
	sint16 id;

	Lpi2c_Ip_MasterInit(I2C_INSTANCE, &I2c_Lpi2cMaster_HwChannel1_Channel0);

	id = pit_add_timer(pit_ms_to_ticks(RX_PERIOD_MS));
	if (id >= 0)
		i2c.rx_timer_id = id;

	id = pit_add_timer(pit_ms_to_ticks(TX_PERIOD_MS));
	if (id >= 0)
		i2c.tx_timer_id = id;

	for (i = 0; i < TX_BUFFER_SIZE; i++) {
		i2c.tx_buffer[i] = 0;
	}

	for (i = 0; i < RX_BUFFER_SIZE; i++) {
		i2c.rx_buffer[i] = 0;
	}

}

/*
 * @brief Main I2C module function
 * @param None
 * @return None
 */
void i2c_main(void)
{

	if (pit_elapsed(i2c.tx_timer_id) == 1) {
		pit_restart_timer(i2c.tx_timer_id);
	}

	if (pit_elapsed(i2c.rx_timer_id) == 1) {
		pit_restart_timer(i2c.rx_timer_id);
	}
}
