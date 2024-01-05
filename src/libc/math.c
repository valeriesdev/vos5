/**
 * @defgroup   LIBC libc
 */
/**
 * @defgroup   MATH math
 * @ingroup    LIBC
 *
 * @brief      This file implements mathematics functions.
 *
 * @author     Valerie Whitmire
 * @date       2023
 */
#include "libc/math.h"

uint16_t logi(uint32_t n, uint8_t base) {
	uint8_t return_value = 0;
	while(n > base) {
		n /= base;
		return_value++;
	}
	return return_value;
}