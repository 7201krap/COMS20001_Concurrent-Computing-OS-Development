/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __HILEVEL_H
#define __HILEVEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"

#include "lolevel.h"
#include     "int.h"

#define MAX_PROCS 20
#define PROCESSOR_SIZE 0x00001000

typedef int pid_t;

typedef enum {
  STATUS_INVALID,

  STATUS_CREATED,
  STATUS_TERMINATED,

  STATUS_READY,
  STATUS_EXECUTING,
  STATUS_WAITING
} status_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef struct {
  pid_t     pid;
  status_t  status;
  uint32_t  tos;
  ctx_t     ctx;

  int base_priority;
  int age;

} pcb_t;

#endif
