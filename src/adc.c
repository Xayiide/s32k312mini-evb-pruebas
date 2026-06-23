#include "Adc_Sar_Ip.h"     /* Adc_Sar_Ip_Init, etc. */
#include "IntCtrl_Ip.h"     /* IntCtrl_Ip_InstallHandler, EnableIrq */
#include "S32K312_COMMON.h" /* ADC0_IRQn */
#include "Platform_Types.h" /* uint */
#include "Compiler.h"       /* NULL_PTR */

#include "adc.h"
#include "pit.h"

#define ADC_CHANNEL   1U
#define ADC_PERIOD_MS 500U
#define ADC_NUM_CALIB 6U

extern void Adc_Sar_0_Isr(void);

enum adc_conv_st {
	ADC_CONV_ONGOING,
	ADC_CONV_DONE,
	ADC_CONV_NONE,
};

struct adc_config {
	enum adc_conv_st conv_st;
	volatile uint16  raw_data;
	sint16           rd_timer_id;
};

static struct adc_config adc;

/*
 * @brief Callback function called when ADC read ends
 * @param None
 * @return None
 */
void adc_end_of_chain_notif(void) {
	adc.raw_data = Adc_Sar_Ip_GetConvData(ADCHWUNIT_0_INSTANCE,
		ADC_CHANNEL);
	adc.conv_st = ADC_CONV_DONE;
}

/*
 * @brief Initialize the ADC module
 * @param None
 * @return None
 */
void adc_init(void)
{
	uint8  i;
	sint16 id;

	Adc_Sar_Ip_Init(ADCHWUNIT_0_INSTANCE, &AdcHwUnit_0);

	IntCtrl_Ip_InstallHandler(ADC0_IRQn, Adc_Sar_0_Isr, NULL_PTR);
	IntCtrl_Ip_EnableIrq(ADC0_IRQn);

	/* Calibrar ADC */
	for (i = 0U; i < ADC_NUM_CALIB; i++) {
		Adc_Sar_Ip_DoCalibration(ADCHWUNIT_0_INSTANCE);
	}

	Adc_Sar_Ip_EnableNotifications(ADCHWUNIT_0_INSTANCE,
		ADC_SAR_IP_NOTIF_FLAG_NORMAL_ENDCHAIN);

	adc.conv_st  = ADC_CONV_NONE;
	adc.raw_data = 0;

	id = pit_add_timer(pit_ms_to_ticks(ADC_PERIOD_MS));
	if (id >= 0)
		adc.rd_timer_id = id;

}

/*
 * @brief Main ADC module function
 * @param None
 * @return None
 */
void adc_main(void)
{
	if (pit_elapsed(adc.rd_timer_id) == 1) {
		pit_restart_timer(adc.rd_timer_id);
		if (adc.conv_st == ADC_CONV_ONGOING) {
			goto exit;
		}
		else {
			Adc_Sar_Ip_StartConversion(ADCHWUNIT_0_INSTANCE,
				ADC_SAR_IP_CONV_CHAIN_NORMAL);
			adc.conv_st = ADC_CONV_ONGOING;
		}
	}

exit:
	return;
}

/*
 * @param Get the last ADC reading
 * @param None
 * @return uint16: last ADC raw reading
 */
uint16 adc_read(void)
{
	uint16 ret = 0;

	ret = adc.raw_data;

	return ret;
}
