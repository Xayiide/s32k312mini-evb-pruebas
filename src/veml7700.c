#include "Platform_Types.h" /* uint */
#include "Lpi2c_Ip.h" /* Lpi2c_* */

#include "veml7700.h"
#include "i2c.h"
#include "pit.h"


#define MASTER_WRITE  0
#define MASTER_READ   1
#define TIMEOUT_US    100000U /* tiempo de espera en microsegundos 0.1 ms */
#define VEML7700_ADDR 0x10
#define ALS_COUNT_LO  100
#define ALS_COUNT_HI  45000
#define MAX_NUM_DEV   5
#define RD_PERIOD_MS  1000
#define NULL_ADDR     0xFF

enum dev_state {
	DEV_READY_ST,
	DEV_WAIT_IT_ST,
};

enum cmd_code {
	CMD_ALS_CONFIG   = 0x0,
	CMD_ALS_HIGH_THR = 0x1,
	CMD_ALS_LOW_THR  = 0x2,
	CMD_PWR_SAVE     = 0x3,
	CMD_ALS_DATA     = 0x4,
	CMD_WHITE_DATA   = 0x5,
	CMD_INT_STATUS   = 0x6,
	CMD_ID_REGISTER  = 0x7,
};

#define ALS_GAIN_NUM_OPT 4
enum als_gain {
	ALS_GAIN_2   = 0x1,
	ALS_GAIN_1   = 0x0,
	ALS_GAIN_1_4 = 0x3,
	ALS_GAIN_1_8 = 0x2,
};
static enum als_gain gain_values[ALS_GAIN_NUM_OPT] = {
	ALS_GAIN_2,
	ALS_GAIN_1,
	ALS_GAIN_1_4,
	ALS_GAIN_1_8,
};

#define ALS_IT_NUM_OPT 6
enum als_it {
	ALS_IT_25MS  = 0xC,
	ALS_IT_50MS  = 0x8,
	ALS_IT_100MS = 0x0,
	ALS_IT_200MS = 0x1,
	ALS_IT_400MS = 0x2,
	ALS_IT_800MS = 0x3,
};
static enum als_it it_values[ALS_IT_NUM_OPT] = {
	ALS_IT_800MS,
	ALS_IT_400MS,
	ALS_IT_200MS,
	ALS_IT_100MS,
	ALS_IT_50MS,
	ALS_IT_25MS,
};

#define ALS_PERS_NUM_OPT 4
enum als_pers {
	ALS_PERS_1 = 0x0,
	ALS_PERS_2 = 0x1,
	ALS_PERS_4 = 0x2,
	ALS_PERS_8 = 0x3,
};

#define PWR_SAV_NUM_OPT 4
enum pwr_mode {
	PWR_SAV_MODE_1 = 0x0,
	PWR_SAV_MODE_2 = 0x1,
	PWR_SAV_MODE_3 = 0x2,
	PWR_SAV_MODE_4 = 0x3,
};

enum int_st {
	INT_DISABLED = 0x0,
	INT_ENABLED  = 0x1,
};

struct veml7700_params {
	enum als_gain gain;
	enum als_it   it;
	enum als_pers pers;
	enum int_st   int_status;
	enum pwr_mode pwr_mode;
	float         res;
	uint32        max_lux;
};

struct veml7700_dev {
	struct veml7700_params params;
	uint8                  addr;
	enum dev_state         st;
	sint16                 it_timer_id;
};

struct veml7700_cfg {
	struct veml7700_dev devs[MAX_NUM_DEV];
	uint8  ndevs;
	sint16 rd_timer_id;
};

static struct veml7700_cfg veml7700;

static uint8  autorange(struct veml7700_dev *dev, uint16 als_count);
static sint8  gain_to_idx(enum als_gain gain);
static sint8  it_to_idx(enum als_it it);
static uint16 it_to_ms(enum als_it it);
static void   set_and_send_config(struct veml7700_dev *dev);
static void   get_default_config(struct veml7700_dev *dev);
static sint32 get_max_illumination(enum als_gain gain, enum als_it it_ms);
static double get_resolution(enum als_gain gain, enum als_it it_ms);
static void   read_reg(struct veml7700_dev *dev, enum cmd_code reg, uint16 *v);
static void   write_reg(struct veml7700_dev *dev, enum cmd_code reg, uint16 v);



/* Funciones públicas */

void veml7700_init(void)
{
	uint8  i;
	struct veml7700_dev *dev;

	veml7700.ndevs = 0;
	for (i = 0; i < MAX_NUM_DEV; i++) {
		dev = &veml7700.devs[i];
		dev->addr        = NULL_ADDR;
		dev->st          = DEV_READY_ST;
		dev->it_timer_id = pit_add_timer(pit_ms_to_ticks(RD_PERIOD_MS));
		get_default_config(&(veml7700.devs[i]));
	}
}

void veml7700_add_dev(uint8 addr)
{
	if (veml7700.ndevs >= MAX_NUM_DEV) {
		return;
	}
	else {
		veml7700.devs[veml7700.ndevs].addr = addr;
		set_and_send_config(&veml7700.devs[veml7700.ndevs]);
		veml7700.ndevs++;
	}

	/* NOTA: En la hoja de datos dice que al activar el sensor, hay
	 * que esperar al menos 2.5 ms antes de tomar la primera medida.
	 * Mientras se tenga RD_PERIOD_MS a 1000, se espera un segundo,
	 * pero si se baja a 1 ó 2, entonces hay que implementar lógica
	 * para esperar los 2.5 ms antes de medir. */
}

void veml7700_main(void)
{
	uint8 i;
	uint16 als_count, white_count;
	double lx, wh;
	struct veml7700_dev *dev;
	uint8  cfg_changed;
	uint32 it_ms, remaining_ms;

	for (i = 0; i < veml7700.ndevs; i++) {
		dev = &veml7700.devs[i];
		if (dev->addr == NULL_ADDR || dev->it_timer_id == -1)
			continue;

		if (pit_elapsed(dev->it_timer_id) == 0)
			continue;

		/* Cuando se cambia una configuración, hay que esperar su IT para que
		 * la lectura sea válida */
		switch(dev->st) {
		case DEV_READY_ST:
			read_reg(dev, CMD_ALS_DATA, &als_count);
			cfg_changed = autorange(dev, als_count);
			if (cfg_changed && 0) {
				/* Si ha cambiado la config:
				 * 1. Configuramos el sensor con la nueva
				 * 2. Establecemos el tempo. a esperar IT_TIME + reiniciar tempo.
				 * 3. Pasamos a estado WAIT_IT */
				set_and_send_config(dev);
				pit_set_limit(dev->it_timer_id,
				              pit_ms_to_ticks(it_to_ms(dev->params.it)));
				pit_restart_timer(dev->it_timer_id);
				dev->st = DEV_WAIT_IT_ST;
			}
			else {
				/* No ha cambiado al config: als_count es válido */
				read_reg(dev, CMD_WHITE_DATA, &white_count);

				lx = als_count   * dev->params.res;
				wh = white_count * dev->params.res;
				/* TODO: Hacer algo con lx y wh */

				pit_set_limit(dev->it_timer_id, pit_ms_to_ticks(RD_PERIOD_MS));
				pit_restart_timer(dev->it_timer_id);
				/* No cambiamos estado porque no hemos cambiado la
				 * configuración y no necesitamos esperar IT_TIME */
			}
			break;
		case DEV_WAIT_IT_ST:
			/* Estamos esperando a ver si ha pasado ya IT_TIME y podemos
			 * consumir la lectura */
			read_reg(dev, CMD_ALS_DATA, &als_count);
			read_reg(dev, CMD_WHITE_DATA, &white_count);

			lx = als_count   * dev->params.res;
			wh = white_count * dev->params.res;

			/* Como ya ha pasado el IT_TIME, vuelvo a esperar el tiempo normal
			 * configurado, corrigiendo la desviación acumulada:
			 * Si he esperado 100MS de IT, y ahora espero 1000 de tiempo
			 * configurado, demoraré 1100 ms en leer en lugar de 1000. Esa
			 * desviación se acumula con cada nueva configuración.
			 * Pero no puedo hacer simplemente
			 * pit_ms_to_ticks(RD_PERIOD_MS - it_to_ms(dev->params.it)) porque
			 * si RD_PERIOD_MS es pequeño, la operación desbordaría. Así que
			 * calculo los milisegundos restantes */
			it_ms = it_to_ms(dev->params.it);
			remaining_ms = (it_ms < RD_PERIOD_MS) ? (RD_PERIOD_MS - it_ms) : 0;
			pit_set_limit(dev->it_timer_id, pit_ms_to_ticks(remaining_ms));
			pit_restart_timer(dev->it_timer_id);
			dev->st = DEV_READY_ST;
			break;
		default:
			break;
		}
	}
}


/* Funciones estáticas */

uint8 autorange(struct veml7700_dev *dev, uint16 als_count)
{
	enum als_gain old_gain = dev->params.gain;
	enum als_it old_it = dev->params.it;
	uint8 gain_idx = gain_to_idx(dev->params.gain);
	uint8 it_idx   = it_to_idx(dev->params.it);
	//uint8 changed  = 0;

	/* Comprobar si la lectura es baja para aumentar la sensibilidad */
	if (als_count <= ALS_COUNT_LO) {
		/* Incrementar ganancia si no está al máximo */
		if (dev->params.gain != ALS_GAIN_2) {
			dev->params.gain = gain_values[gain_idx - 1];
			//changed = 1;
		}
		else {
			/* Ganancia está al máximo. Incrementamos IT si no está al máximo */
			if (dev->params.it != ALS_IT_800MS) {
				dev->params.it = it_values[it_idx - 1];
				//changed = 1;
			}
		}
	}
	/* Comprobar si la lectura está demasiado alta para bajar sensibilidad */
	else if (als_count >= ALS_COUNT_HI) {
		/* Reducimos IT si no está al mínimo */
		if (dev->params.it != ALS_IT_25MS) {
			dev->params.it = it_values[it_idx + 1];
			//changed = 1;
		}
		else {
			/* It está al mínimo. Reducimos ganancia si no está al mínimo */
			if (dev->params.gain != ALS_GAIN_1_8) {
				dev->params.gain = gain_values[gain_idx + 1];
				//changed = 1;
			}
		}
	}

	return (dev->params.gain != old_gain) || (dev->params.it != old_it);
}

sint8 gain_to_idx(enum als_gain gain)
{
	sint8 idx = -1;
	uint8 i;

	for (i = 0; i < ALS_GAIN_NUM_OPT; i++) {
		if (gain_values[i] == gain) {
			idx = i;
			break;
		}
	}

	return idx;
}

sint8 it_to_idx(enum als_it it)
{
	sint8 idx = -1;
	uint8 i;

	for (i = 0; i < ALS_IT_NUM_OPT; i++) {
		if (it_values[i] == it) {
			idx = i;
			break;
		}
	}

	return idx;
}

uint16 it_to_ms(enum als_it it)
{
	switch(it) {
	case ALS_IT_25MS:  return 25;
	case ALS_IT_50MS:  return 50;
	case ALS_IT_100MS: return 100;
	case ALS_IT_200MS: return 200;
	case ALS_IT_400MS: return 400;
	case ALS_IT_800MS: return 800;
	default:           return 800; /* Si it no cuadra: caso más conservador */
	}
}

void set_and_send_config(struct veml7700_dev *dev)
{
	uint16 reg_data = 0;

	reg_data = (
		(dev->params.gain       << 11) |
		(dev->params.it         <<  6) |
		(dev->params.pers       <<  4) |
		(dev->params.int_status <<  1)
	);

	dev->params.max_lux = get_max_illumination(dev->params.gain, dev->params.it);
	dev->params.res     = get_resolution(dev->params.gain, dev->params.it);

	write_reg(dev, CMD_ALS_CONFIG, reg_data);
}

void get_default_config(struct veml7700_dev *dev)
{
	dev->params.gain       = ALS_GAIN_1_8;
	dev->params.it         = ALS_IT_100MS;
	dev->params.pers       = ALS_PERS_1;
	dev->params.int_status = INT_DISABLED;
	dev->params.pwr_mode   = PWR_SAV_MODE_1;
	dev->params.res        = get_resolution(dev->params.gain, dev->params.it);
	dev->params.max_lux    = get_max_illumination(dev->params.gain, dev->params.it);
}

sint32 get_max_illumination(enum als_gain gain, enum als_it it_ms) {
	switch (it_ms) {
	case ALS_IT_800MS:
		switch (gain) {
		case ALS_GAIN_2:   return 275;
		case ALS_GAIN_1:   return 550;
		case ALS_GAIN_1_4: return 2202;
		case ALS_GAIN_1_8: return 4404;
		default:           return -1;
		}
		break;
	case ALS_IT_400MS:
		switch (gain) {
		case ALS_GAIN_2:   return 550;
		case ALS_GAIN_1:   return 1101;
		case ALS_GAIN_1_4: return 4404;
		case ALS_GAIN_1_8: return 8808;
		default:           return -1;
		}
		break;
	case ALS_IT_200MS:
		switch (gain) {
		case ALS_GAIN_2:   return 1101;
		case ALS_GAIN_1:   return 2202;
		case ALS_GAIN_1_4: return 8808;
		case ALS_GAIN_1_8: return 17616;
		default:           return -1;
		}
		break;
	case ALS_IT_100MS:
		switch (gain) {
		case ALS_GAIN_2:   return 2202;
		case ALS_GAIN_1:   return 4404;
		case ALS_GAIN_1_4: return 17616;
		case ALS_GAIN_1_8: return 35232;
		default:           return -1;
		}
		break;
	case ALS_IT_50MS:
		switch (gain) {
		case ALS_GAIN_2:   return 4404;
		case ALS_GAIN_1:   return 8808;
		case ALS_GAIN_1_4: return 35232;
		case ALS_GAIN_1_8: return 70463;
		default:           return -1;
		}
		break;
	case ALS_IT_25MS:
		switch (gain) {
		case ALS_GAIN_2:   return 8808;
		case ALS_GAIN_1:   return 17616;
		case ALS_GAIN_1_4: return 70463;
		case ALS_GAIN_1_8: return 140926;
		default:           return -1;
		}
		break;
	default:
		return -1;
	}
}

double get_resolution(enum als_gain gain, enum als_it it_ms)
{
	switch (it_ms) {
	case ALS_IT_800MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0042;
		case ALS_GAIN_1:   return 0.0084;
		case ALS_GAIN_1_4: return 0.0336;
		case ALS_GAIN_1_8: return 0.0672;
		default:           return -1;
		}
		break;
	case ALS_IT_400MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0084;
		case ALS_GAIN_1:   return 0.0168;
		case ALS_GAIN_1_4: return 0.0672;
		case ALS_GAIN_1_8: return 0.1344;
		default:           return -1;
		}
		break;
	case ALS_IT_200MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0168;
		case ALS_GAIN_1:   return 0.0336;
		case ALS_GAIN_1_4: return 0.1344;
		case ALS_GAIN_1_8: return 0.2688;
		default:           return -1;
		}
		break;
	case ALS_IT_100MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0336;
		case ALS_GAIN_1:   return 0.0672;
		case ALS_GAIN_1_4: return 0.2688;
		case ALS_GAIN_1_8: return 0.5376;
		default:           return -1;
		}
		break;
	case ALS_IT_50MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0672;
		case ALS_GAIN_1:   return 0.1344;
		case ALS_GAIN_1_4: return 0.5376;
		case ALS_GAIN_1_8: return 1.0752;
		default:           return -1;
		}
		break;
	case ALS_IT_25MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.1344;
		case ALS_GAIN_1:   return 0.2688;
		case ALS_GAIN_1_4: return 1.0752;
		case ALS_GAIN_1_8: return 2.1504;
		default:           return -1;
		}
		break;
	default:
		return -1;
	}
}

void read_reg(struct veml7700_dev *dev, enum cmd_code reg, uint16 *v)
{
	Lpi2c_Ip_StatusType st;
	uint8               tx;
	uint8               rx[2];

	Lpi2c_Ip_MasterSetSlaveAddr(I2C_INSTANCE, dev->addr, FALSE);

	tx = (uint8) reg;
	st = Lpi2c_Ip_MasterSendDataBlocking(I2C_INSTANCE, &tx, 1U,
		FALSE, TIMEOUT_US);
	if (st != LPI2C_IP_SUCCESS_STATUS) {
		/* TODO Manejar */
	}

	st = Lpi2c_Ip_MasterReceiveDataBlocking(I2C_INSTANCE, rx, 2U,
	    TRUE, TIMEOUT_US);
	if (st != LPI2C_IP_SUCCESS_STATUS) {
		/* TODO Manejar */
	}

	/* pack result in uint16 */
	*v = (uint16) ((rx[1] << 8) | rx[0]);
}

void write_reg(struct veml7700_dev *dev, enum cmd_code reg, uint16 v)
{
	Lpi2c_Ip_StatusType st;
	uint8 tx[3];

	Lpi2c_Ip_MasterSetSlaveAddr(I2C_INSTANCE, dev->addr, FALSE);

	tx[0] = (uint8) reg;
	tx[1] = (uint8) v; /* LSB */
	tx[2] = (uint8) (v >> 8); /* MSB */

	st = Lpi2c_Ip_MasterSendDataBlocking(I2C_INSTANCE, tx, 3U, TRUE,
	    TIMEOUT_US);
	if (st != LPI2C_IP_SUCCESS_STATUS) {
		/* TODO Manejar */
	}
}

