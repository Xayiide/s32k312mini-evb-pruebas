#include "Platform_Types.h" /* uint */

#include "sys.h"
#include "pit.h"

static sint16 blocking_wait_timer_id;

void sys_init(void)
{
	sint16 id;

	id = pit_add_timer(PIT_MAX_PERIOD);
	if (id >= 0)
		blocking_wait_timer_id = id;
}

/*
 * @brief Blocking wait for a period of time
 * @param uint32 ticks_125us: number of 125 micro seconds ticks to wait
 * @return None
 */
void sys_blocking_wait(uint32 ticks)
{
	/* Max wait = 0xFFFF FFFF * 1.25e-4 = 536 870.9 seconds (~149 hours) */

	pit_set_limit(blocking_wait_timer_id, ticks);
	pit_restart_timer(blocking_wait_timer_id);

	while (pit_elapsed(blocking_wait_timer_id) == 0) {
	}
}

/* @brief return the binary size, obtained from calculations from the linker
 *        script symbols
 * @param None
 * @return uint32 with size of the binary
 *
 * The symbols used are:
 * - __ROM_CODE_START, which equals ORIGIN(int_pflash). This means that is the
 *    first used byte in the program flash.
 * - __ROM_ITCM_END, which equsl __tcm_code_rom_end. This includes all bytes
 *    in the .text and .data sections.
 *
 * When debugging, you can validate the return values by browsing memory and
 * going to __ROM_ITCM_END. The next address should be the first one not
 * programmed (0xFFFFFFFF).
 */
uint32 sys_get_binsize(void)
{
	uint32 binsize = 0;

	extern const uint32 __ROM_CODE_START, __ROM_ITCM_END;

	binsize = ((uint32) &__ROM_ITCM_END) -
	          ((uint32) &__ROM_CODE_START);

	return binsize;
}

/*
 * @brief fill the first n bytes of the memory area pointed to by s with the
 *        constant byte c.
 * @param s: pointer to the memory area to fill
 * @param c: byte with which to fill the memory area
 * @param n: size of the memory area to fill
 * @return pointer to s
 */
void *sys_memset(void *s, uint8 c, uint32 n)
{
    uint8 *temp;

    temp = (uint8 *) s;

    for (; n != 0; n--)
        *temp++ = c;

    return s;
}

/*
 * @brief Copy n bytes from memory area src to memory area dest. Memory areas
 *        MUST NOT overlap.
 * @param dest: pointer to the destination memory area
 * @param src: pointer to the source memory area
 * @param n: number of bytes to copy
 * @return pointer to dest
 */
void *sys_memcpy(void *dest, const void *src, uint32 n)
{
    const char *sp;
    char *dp;

    sp = (const char *) src;
    dp = (char *) dest;

    for (; n != 0; n--)
        *dp++ = *sp++;

    return dest;
}

/*
 * @brief Compare two memory regions
 * @param s1 pointer to the first region
 * @param s2 pointer to the second region
 * @param n max size to compare
 * @return integer less than, equal to, or greater than zero if the first n
 *         bytes of s1 is found, respectively, to be less than, to match, or be
 *         greater than the first n bytes of s2. For a nonzero return value, the
 *         sign is determined by the sign of the difference between the first
 *         pair of bytes that differ in s1 and s2.
 *
 * Implementation source: OpenBSD LIBC rev 1.6
 * https://cvsweb.openbsd.org/cgi-bin/cvsweb/src/lib/libc/string/memcmp.c?rev=1.6
 */
sint32 sys_memcmp(const void *s1, const void *s2, uint32 n)
{
	const uint8 *p1 = (const uint8 *) s1;
	const uint8 *p2 = (const uint8 *) s2;

	if (n != 0) {
		do {
			if (*p1++ != *p2++) {
				return ((sint32) (*--p1 - *--p2));
			}
		} while (--n != 0);
	}

	return 0;
}


/*
 * @brief Calculate the length of the string pointed to by str, excluding the
 *        terminating null byte ('\0')
 * @param str: pointer to the string
 * @return length of str, excluding the terminating null byte ('\0')
 *
 * Implementation source: OpenBSD LIBC rev 1.9
 * https://cvsweb.openbsd.org/cgi-bin/cvsweb/src/lib/libc/string/strlen.c?rev=1.9
 */
uint32 sys_strlen(const uint8 *str)
{
	const uint8 *s;

	for (s = str; *s; ++s);

	return (s - str);
}

/*
 * @brief Copy the string pointed to by src into a string pointed to by dest.
 * @param dest: pointer to the destination string
 * @param src: pointer to the source string
 * @return pointer to dest
 *
 */
uint8 *sys_strcpy(uint8 *dest, const uint8 *src)
{
	uint8 *d = dest;

	while ((*dest++ = *src++));

	return d;
}


/*
 * @brief Copy at most n characters from the string pointed to by src into
 *        the string pointed to by dest
 * @param dest: pointer to the destination string
 * @param src: pointer to the source string
 * @param n: max amount of chars to copy
 * @return pointer to dest
 */
uint8 *sys_strncpy(uint8 *dest, const uint8 *src, uint32 n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';

    return dest;
}

/*
 * @brief Construct a NULL-terminated ASCII string representation of an integer
 * @param value: signed integer to convert
 * @param result: pointer to the string representation
 * @param base: base to convert the value to. Possible bases: 2 to 16
 * @return pointer to the string representation (result)
 */
uint8 *sys_itoa(sint32 value, uint8 *result, sint32 base)
{
	uint8  *ptr  = result;
	uint8  *ptr1 = result;
	uint8   tmp_char;
	uint8   str[16] = "0123456789abcdef";
	uint8   negative = 0;
	uint32  uvalue;

	if (base < 2 || base > 16) {
		*result = '\0';
		return result;
	}

	if (value < 0 && base == 10) {
		negative = 1;
		if (value == INT32_MIN) { /* value == -2 147 483 648 */
			uvalue = (uint32) INT32_MAX + 1U; /* uvalue = 2 147 483 648 */
		}
		else {
			uvalue = (uint32) (-value);
		}
	}
	else {
		uvalue = (uint32) value;
	}

	/* Conversion */
	do {
		*ptr++ = str[uvalue % (uint32) base];
		uvalue /= (uint32) base;
	} while (uvalue != 0U);

	/* Signo negativo */
	if (negative == 1) {
		*ptr++ = '-';
	}

	/* Terminador nulo */
	*ptr-- = '\0';

	/* Dar la vuelta a la cadena */
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--   = *ptr1;
		*ptr1++  =  tmp_char;
	}

	return result;
}

/*
 * @brief Convert the uint32 v into an array of chars (uint8) out.
 * @param out: pointer to char/uint8 array to store the result
 * @param v: uint32 to convert
 * @return None
 */
void  sys_u32_to_hex8(uint8 *out, uint32 v)
{
	static const uint8 hex[] = "0123456789ABCDEF";
	int i = 0;

	for (i = 7; i >= 0; i--) {
		out[i] = hex[v & 0xF];
		v >>= 4;
	}
}
