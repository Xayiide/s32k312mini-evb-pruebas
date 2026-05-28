#ifndef PIT_H_
#define PIT_H_

#include "Platform_Types.h" /* uint */

#define PIT_MAX_PERIOD 0xFFFFFFFF

void   pit_init(void);
sint16 pit_add_timer(uint32 period);
void   pit_restart_timer(sint16 id);
uint8  pit_elapsed(sint16 id);
void   pit_set_count(sint16 id, uint32 count);
void   pit_set_limit(sint16 id, uint32 limit);
uint32 pit_ms_to_ticks(uint32 ms);

#endif /* PIT_H_ */
