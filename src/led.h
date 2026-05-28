#ifndef LED_H_
#define LED_H_

#define LED_ON  1U
#define LED_OFF 0U

void led_init(void);
void led_main(void);
void led_set_r(uint8 st);
void led_set_g(uint8 st);
void led_set_b(uint8 st);

#endif /* LED_H_ */
