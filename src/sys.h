#ifndef SYS_H_
#define SYS_H_

#include "Platform_Types.h" /* uint */

#define disable_global_interrupts() __asm("cpsid i");
#define enable_global_interrupts() __asm("cpsie i");

void    sys_init(void);
void    sys_blocking_wait(uint32 ticks);
uint32  sys_get_binsize(void);
void   *sys_memset(void *s, uint8 c, uint32 n);
void   *sys_memcpy(void *dest, const void *src, uint32 n);
sint32  sys_memcmp(const void *s1, const void *s2, uint32 n);
uint32  sys_strlen(const uint8 *str);
uint8  *sys_strcpy(uint8 *dest, const uint8 *src);
uint8  *sys_strncpy(uint8 *dest, const uint8 *src, uint32 n);
uint8  *sys_itoa(sint32 value, uint8 *result, sint32 base);
void    sys_u32tohex8(uint8 *out, uint32 value);


#endif /* SYS_H_ */
