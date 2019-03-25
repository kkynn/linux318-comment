/*
 * =====================================================================================
 *
 *       Filename:  wb_flush.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:
 *        Company:
 *
 * =====================================================================================
 */
#include <linux/module.h>
//#include <stdint.h>
uint32_t wbtarget;
static void rtk_wbflush(void) {
        asm volatile(".set push\n\t"
                        ".set noreorder\n\t"
                        "la     $8, wbtarget\n\t"
                        "lui    $9, 0x2000\n\t"
                        "or     $8, $8, $9\n\t"
                        "sw     $0, 0($8)\n\t"
                        "nop    \n\t"
                        "lw     $9, 0($8)\n\t"
												//"sdbbp\n\t"
                        ".set pop\n\t"
                        :
                        :
                        : "$8", "$9"
                        );
}
void (*__wbflush)(void) = rtk_wbflush;
EXPORT_SYMBOL(__wbflush);
