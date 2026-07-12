#ifndef VEML7700_H_
#define VEML7700_H_

#include "Platform_Types.h" /* uint */

void veml7700_init(void);
void veml7700_add_dev(uint8 addr);
void veml7700_main(void);

#endif /* VEML7700_H_ */
