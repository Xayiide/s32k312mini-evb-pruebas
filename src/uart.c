#include "CDD_Uart.h"       /* Uart_Init, Uart_GetStatus, etc. */
#include "Uart_Types.h"     /* Uart_StatusType, UART_RECEIVE */
#include "Compiler.h"       /* NULL_PTR */
#include "Platform_Types.h" /* uint */

#include "uart.h"
#include "pit.h"

#define INTERNAL_CHANNEL 0
#define RX_BUFFER_SIZE   64
#define PROCESS_TIMER_MS 100

static uint8  rxBuffer[RX_BUFFER_SIZE];
static sint16 timer_id;

void uart_init(void)
{
	Uart_Init(NULL_PTR);

    /* Initialize UART receive once before main loop */
    Uart_AsyncReceive(INTERNAL_CHANNEL, rxBuffer, 1);

    timer_id = pit_add_timer(pit_ms_to_ticks(PROCESS_TIMER_MS));
}

void uart_main(void)
{
	Uart_StatusType rxst = UART_STATUS_NO_ERROR;
	Uart_StatusType txst = UART_STATUS_NO_ERROR;
	uint32 brem = 0;

	if (pit_elapsed(timer_id) == 1) {
		rxst = Uart_GetStatus(INTERNAL_CHANNEL, &brem, UART_RECEIVE);
		if (rxst == UART_STATUS_NO_ERROR) {
			/* Haz eco del byte recibido */
			Uart_AsyncSend(INTERNAL_CHANNEL, rxBuffer, 1);
			do {
				txst = Uart_GetStatus(INTERNAL_CHANNEL, &brem, UART_SEND);
			} while (txst != UART_STATUS_NO_ERROR);

			Uart_AsyncReceive(INTERNAL_CHANNEL, rxBuffer, 1);
		} else if (rxst == UART_STATUS_OPERATION_ONGOING) {

		} else {
			/* Start new operation */
			Uart_AsyncReceive(INTERNAL_CHANNEL, rxBuffer, 1);
		}

		pit_restart_timer(timer_id);
	}
}
