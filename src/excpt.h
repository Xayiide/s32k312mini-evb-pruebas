#ifndef EXCPT_H_
#define EXCPT_H_

#include "Platform_Types.h" /* uint8 */

enum excpt_state {
	EXCPT_ENABLE  = 1U,
	EXCPT_DISABLE = 0U
};

void excpt_ena_all(void);
void excpt_dis_all(void);
void excpt_set_mem(enum excpt_state state);
void excpt_set_bus(enum excpt_state state);
void excpt_set_usg(enum excpt_state state);


#endif /* EXCPT_H_ */
