/**
 * @defgroup   CPU cpu
 */
/**
 * @defgroup   IDT idt
 * @ingroup    CPU
 * 
 * @brief      This file implements some IDT functions.
 *
 * @author     Valerie Whitmire
 * @date       2023
 */
#include <stdint.h>
#include "cpu/idt.h"

/**
 * @brief      Sets an idt gate.
 * @ingroup    IDT
 * @param[in]  n        The gate
 * @param[in]  handler  The handler
 */
void set_idt_gate(int n, uint32_t handler) {
    idt[n].low_offset = handler & 0xFFFF;
    idt[n].sel = KERNEL_CS;
    idt[n].always0 = 0;
    idt[n].flags = 0x8E; 
    idt[n].high_offset = handler & 0xFFFF0000;
}

/**
 * @brief      Loads the IDT
 * @ingroup    IDT
 */
void set_idt() {
    idt_reg.base = (uint32_t) &idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
    /* Don't make the mistake of loading &idt -- always load &idt_reg */
    asm volatile("lidtl (%0)" : : "r" (&idt_reg));
}