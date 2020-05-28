/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __INT_H
#define __INT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern void int_init();
extern void int_enable_irq();
extern void int_unable_irq();
extern void int_enable_fiq();
extern void int_unable_fiq();

#endif
