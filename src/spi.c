#include "Lpspi_Ip.h" /* Lpspi_Ip_Init */
#include "Lpspi_Ip_Sa_PBcfg.h" /* Lpspi_Ip_PhyUnitConfig_...*/
#include "Platform_Types.h" /* uint */

#include "Siul2_Port_Ip_Cfg.h"
#include "Siul2_Dio_Ip.h"

#include "spi.h"
#include "pit.h"


#define RX_PERIOD_MS 1000
#define TX_PERIOD_MS 2000


struct spi_cfg {
	sint16 rx_timer_id;
	sint16 tx_timer_id;
	uint8  spi_rx_buf[8];
	uint8  spi_rx_aux;
};


static struct spi_cfg spi;

static uint8 spi_write(uint8 b);

/*
 * @brief Initialize SPI module
 * @param None
 * @return None
 */
void spi_init(void)
{
	uint8  i;
	sint16 id;

	Lpspi_Ip_Init(&Lpspi_Ip_PhyUnitConfig_SpiPhyUnit_0_Instance_0);

	id = pit_add_timer(pit_ms_to_ticks(RX_PERIOD_MS));
	if (id >= 0)
		spi.rx_timer_id = id;

	id = pit_add_timer(pit_ms_to_ticks(TX_PERIOD_MS));
	if (id >= 0)
		spi.tx_timer_id = id;

	for (i = 0; i < sizeof(spi.spi_rx_buf); i++) {
		spi.spi_rx_buf[i] = 0xFF;
	}

	spi.spi_rx_aux = 0xFF;
}

/*
 * @brief Main SPI module function
 * @param None
 * @return None
 */
void spi_main(void)
{
	static uint8 st = 0;
	uint8 r = 0;


	if (pit_elapsed(spi.rx_timer_id) == 1) {
		pit_restart_timer(spi.rx_timer_id);
	}

	if (pit_elapsed(spi.tx_timer_id) == 1) {
		pit_restart_timer(spi.tx_timer_id);
		if (st == 0) {
			Siul2_Dio_Ip_WritePin(pin_prueba_PORT, pin_prueba_PIN, 0U);
			st = 1;
		}
		else if (st == 1) {
			Siul2_Dio_Ip_WritePin(pin_prueba_PORT, pin_prueba_PIN, 1U);
			st = 0;
		}

		r = spi_write((uint8) 0xA5);
	}

	return;
}

/*
 * @brief write a byte to SPI
 * @param uint8 b byte to write
 * @return uint8 0 if success, 1 if an error occurred
 */
uint8 spi_write(uint8 b)
{
	Lpspi_Ip_HwStatusType hwst = LPSPI_IP_IDLE;
	Lpspi_Ip_StatusType st = LPSPI_IP_STATUS_FAIL;
	uint8 ret = 0;

	st = Lpspi_Ip_SyncTransmit(
	    &Lpspi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_0,
	    &b,
	    NULL_PTR,
	    1,
	    1000);

	if (st != LPSPI_IP_STATUS_SUCCESS) {
		ret = 1;
		goto out;
	}

	do {
		hwst = Lpspi_Ip_GetStatus(
		    Lpspi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_0.Instance);
	} while (hwst != LPSPI_IP_IDLE);

	if (hwst == LPSPI_IP_FAULT) {
		ret = 1;
		goto out;
	}

out:
	return ret;
}
