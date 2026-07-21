#ifndef CAN_H_
#define CAN_H_

#include "Platform_Types.h" /* uint */

void can_init(void);
void can_main(void);
void can_send_msg(uint32 id, uint8 len, uint8 *data);


#endif /* CAN_H_ */
