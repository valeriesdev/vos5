/**
 * @defgroup   PORTS ports
 * @ingroup    CPU
 * @brief      This file implements ports.
 *
 * @author     Valerie Whitmire
 * @date       2023
 */
#include "cpu/ports.h"
#include "libc/string.h"
/**
 * Read a byte from the specified port
 */
/**
 * @brief      Reads a byte in from the port
 * @ingroup    PORTS
 * @param[in]  port  The port
 *
 * @return     The value accessed
 */
uint8_t port_byte_in (uint16_t port) {
    uint8_t result;
    asm("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

/**
 * @brief      Send a byte out to the port
 * @ingroup    PORTS
 * @param[in]  port  The port
 * @param[in]  data  The data to send
 */
void port_byte_out (uint16_t port, uint8_t data) {
    asm volatile("out %%al, %%dx" : : "a" (data), "d" (port));
}

/**
 * @brief      Reads a word in from the port
 * @ingroup    PORTS
 * @param[in]  port  The port
 *
 * @return     The value accessed
 */
uint16_t port_word_in (uint16_t port) {
    uint16_t result;
    asm("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

/**
 * @brief      Send a word out to the port
 * @ingroup    PORTS
 * @param[in]  port  The port
 * @param[in]  data  The data to send
 *
 * @return     The value accessed
 */
void port_word_out (uint16_t port, uint16_t data) {
    asm volatile("out %%ax, %%dx" : : "a" (data), "d" (port));
}
