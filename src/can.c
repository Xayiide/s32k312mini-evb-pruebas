#include "FlexCAN_Ip.h"
#include "FlexCAN_Ip_Sa_PBcfg.h" /* INST_FLEXCAN_0, FlexCAN_State0, _Config0 */
#include "FlexCAN_Ip_Irq.h"
#include "FlexCAN_Ip_Types.h" /* Flexcan_Ip_MsgBuffType, _DataInfoType */
#include "IntCtrl_Ip.h" /* IntCtrl_Ip_InstallHandler */
#include "Platform_Types.h" /* uint */
#include "S32K312_COMMON.h" /* FlexCAN0_1_IRQn */

#include "can.h"
#include "pit.h"
#include "sys.h"

extern void CAN0_ORED_0_31_MB_IRQHandler(void);

#define CAN_MAX_DATA_LEN 8
#define CAN_BUFFER_SIZE  16
#define CAN_MAX_ID       0x7FF

struct can_msg {
	uint32 id;
	uint8  data[CAN_MAX_DATA_LEN];
	uint8  len;
	uint8  processed;
};

struct can_msg_buf {
	struct can_msg buf[CAN_BUFFER_SIZE];
	uint8          idx_wr;
	uint8          idx_rd;
};

struct can_cfg {
	struct can_msg_buf rx;
	struct can_msg_buf tx;
	sint16 timer_id;
	uint32 timer_ms;
	Flexcan_Ip_MsgBuffType msg;
};


static struct can_cfg can;


static void  insert_into_tx_buf(uint32 id, uint8 len, uint8 *data);
static void  insert_into_rx_buf(Flexcan_Ip_MsgBuffType *data);
static void  process_rx_buffer(void);
static void  process_rx_msg(struct can_msg *msg);
static void  process_tx_buffer(void);
static void  process_tx_msg(struct can_msg *msg);
static uint8 tx_done(void);


void Can0_Rx_Callback(uint8 instance,
		Flexcan_Ip_EventType eventType,
		uint32 buffIdx,
		const Flexcan_Ip_StateType *flexcanState)
{
	(void) buffIdx;
	(void) flexcanState;

	if (instance == INST_FLEXCAN_0) {
		switch (eventType) {
		case FLEXCAN_EVENT_RX_COMPLETE:
			insert_into_rx_buf(&can.msg);
			FlexCAN_Ip_Receive(INST_FLEXCAN_0, 1U, &can.msg, FALSE);
			break;
		default:
			break;
		}
	}
}


/*
 * @brief Initilize CAN module
 * @param None
 * @return None
 */
void can_init(void)
{
	uint8 i;
	sint16 id;

	/* Initialize driver */
	Flexcan_Ip_DataInfoType can_rx_info = {
			.msg_id_type = FLEXCAN_MSG_ID_STD,
			.data_length = 8u,
			.is_polling  = FALSE,
			.is_remote   = FALSE
	};

	FlexCAN_Ip_Init(INST_FLEXCAN_0, &FlexCAN_State0, &FlexCAN_Config0);
	FlexCAN_Ip_SetStartMode(INST_FLEXCAN_0);

	/* Configura todos los mensajes a sus buzones correspondientes */
	FlexCAN_Ip_ConfigRxMb(
			INST_FLEXCAN_0,
			1U,
			&can_rx_info,
			0x111
	);

	FlexCAN_Ip_ConfigRxMb(
			INST_FLEXCAN_0,
			1U,
			&can_rx_info,
			0x222
	);

	/* Install IRQ handlers */
	IntCtrl_Ip_InstallHandler(FlexCAN0_1_IRQn,
			CAN0_ORED_0_31_MB_IRQHandler, NULL_PTR);
	IntCtrl_Ip_EnableIrq(FlexCAN0_1_IRQn);

	/* Initialize CAN RX and TX buffers */
	can.rx.idx_rd = 0;
	can.rx.idx_wr = 0;
	can.tx.idx_rd = 0;
	can.tx.idx_rd = 0;

	for (i = 0; i < CAN_BUFFER_SIZE; i++) {
		sys_memset(can.rx.buf[i].data, 0xFF, CAN_MAX_DATA_LEN);
		sys_memset(can.tx.buf[i].data, 0xFF, CAN_MAX_DATA_LEN);

		can.rx.buf[i].id        = CAN_MAX_ID;
		can.rx.buf[i].len       = CAN_MAX_DATA_LEN;
		can.rx.buf[i].processed = TRUE;

		can.tx.buf[i].id        = CAN_MAX_ID;
		can.tx.buf[i].len       = CAN_MAX_DATA_LEN;
		can.tx.buf[i].processed = TRUE;
	}

	/* Hacer un bucle para recorrer todas las MB */
	FlexCAN_Ip_Receive(INST_FLEXCAN_0, 0U, &can.msg, FALSE);

	/* Initialize timers */
	id = pit_add_timer(pit_ms_to_ticks(can.timer_ms));
	if (id >= 0)
		can.timer_id = id;
}

/*
 * @brief Main CAN module function
 * @param None
 * @return None
 */
void can_main(void)
{
	if (pit_elapsed(can.timer_id) == 1) {
		pit_restart_timer(can.timer_id);
		process_tx_buffer();
		process_rx_buffer();
	}
}

/*
 * @brief Store a CAN msg into the TX buffer to send it
 * @param id: CAN ID of the message to send
 * @param len: length of the data to send
 * @param data: pointer to the byte array to send
 * @return None
 */
void can_send_msg(uint32 id, uint8 len, uint8 *data)
{
	if ((data != NULL) &&
	    (len <= CAN_MAX_DATA_LEN) &&
	    (id <= CAN_MAX_ID)) {
		insert_into_tx_buf(id, len, data);
	}
}







void insert_into_tx_buf(uint32 id, uint8 len, uint8 *data)
{
	struct can_msg *msg = &can.tx.buf[can.tx.idx_wr];

	if (msg->processed == FALSE) {
		return;
	}

	if (data != NULL) {
		msg->id = id;
		msg->len = (len >= CAN_MAX_DATA_LEN) ? CAN_MAX_DATA_LEN : len;
		msg->processed = FALSE;
		sys_memcpy(msg->data, data, msg->len);

		++(can.tx.idx_wr);
		if (can.tx.idx_wr >= CAN_BUFFER_SIZE)
			can.tx.idx_wr = 0;
	}
}

void insert_into_rx_buf(Flexcan_Ip_MsgBuffType *data)
{
	struct can_msg *msg = &can.rx.buf[can.rx.idx_wr];

	if (msg->processed == FALSE) {
		return;
	}

	if (data != NULL) {
		msg->id = data->msgId;
		msg->len = (data->dataLen <= CAN_MAX_DATA_LEN) ?
				data->dataLen : CAN_MAX_DATA_LEN;
		msg->processed = FALSE;
		sys_memcpy(msg->data, data->data, msg->len);

		++(can.rx.idx_wr);
		if (can.rx.idx_wr >= CAN_BUFFER_SIZE)
			can.rx.idx_wr = 0;
	}
}

void process_rx_buffer(void)
{
	struct can_msg *msg;

	if ((can.rx.idx_rd != can.rx.idx_wr) ||
	    (can.rx.buf[can.rx.idx_rd].processed == FALSE)) {
		/* Point to the first unprocessed message and process it */
		msg = &can.rx.buf[can.rx.idx_rd];
		if (msg != NULL && msg->processed == FALSE) {
			process_rx_msg(msg);
		}

		++(can.rx.idx_rd);
		if (can.rx.idx_rd >= CAN_BUFFER_SIZE)
			can.rx.idx_rd = 0;
	}
}

void process_rx_msg(struct can_msg *msg)
{
	if (msg == NULL)
		return;

	switch(msg->id) {
	case 0x111:
		break;
	case 0x222:
		break;
	default:
		break;
	}

	msg->processed = TRUE;
}

void process_tx_buffer(void)
{
	struct can_msg *msg;

	if ((can.tx.idx_rd != can.tx.idx_wr) && (tx_done() == 1)) {
		/* Point to the first unprocessed message and process it */
		msg = &can.tx.buf[can.tx.idx_rd];
		if (msg != NULL && msg->processed == FALSE) {
			process_tx_msg(msg);
		}

		++(can.tx.idx_rd);
		if (can.tx.idx_rd >= CAN_BUFFER_SIZE)
			can.tx.idx_rd = 0;
	}
}

void process_tx_msg(struct can_msg *msg)
{
	Flexcan_Ip_DataInfoType can_tx_info = {
			.msg_id_type = FLEXCAN_MSG_ID_STD,
			.data_length = CAN_MAX_DATA_LEN,
			.is_polling  = FALSE,
			.is_remote   = FALSE
	};

	if (msg == NULL)
		return;

	can_tx_info.data_length = msg->len;
	FlexCAN_Ip_Send(INST_FLEXCAN_0,
	                0U,
	                &can_tx_info,
	                msg->id,
	                (uint8 *) msg->data);
	msg->processed = TRUE;

}

uint8 tx_done(void)
{
	uint8 ret = 0;
	Flexcan_Ip_StatusType st = FLEXCAN_STATUS_BUSY;

	st = FlexCAN_Ip_GetTransferStatus(INST_FLEXCAN_0, 0U);

	switch (st) {
	case FLEXCAN_STATUS_SUCCESS:
	case FLEXCAN_STATUS_ERROR:
	case FLEXCAN_STATUS_TIMEOUT:
	case FLEXCAN_STATUS_BUFF_OUT_OF_RANGE:
	case FLEXCAN_STATUS_NO_TRANSFER_IN_PROGRESS:
		ret = 1;
		break;
	case FLEXCAN_STATUS_BUSY:
	default:
		ret = 0;
		break;
	}

	return ret;
}
