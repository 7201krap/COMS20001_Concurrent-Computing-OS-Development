/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 *
 */

#include "hilevel.h"

extern void     main_console();
extern uint32_t tos_user;

pcb_t procTab[ MAX_PROCS ]; // MAX_PROCS == 20

pcb_t* executing = NULL;    // None of the procTab[] is executing at the beginning
pcb_t* get_pcb ( pid_t pid );
pcb_t* get_child_pcb();

void print_fork_message();
void print_dispatch_message(uint8_t prev_pid, uint8_t next_pid);
void print_timer_handling_interrupt();
void print_yield_message();
void print_exit_message();
void print_execute_message();
void print_kill_message();
void print_nice_message();

int emptied_pcb_id();

// dispatch function perfoms a context switch
// dispatch functions properties:
//  1. suspends execution of the previous process
//  2. resumes  execution of the next process
void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {
  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) ); // preserve execution context of P_{prev}
    prev_pid = '0' + prev->pid;
  }

  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) ); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }

  print_dispatch_message(prev_pid, next_pid);

  executing = next; // update current so it points at the executing user process

  return;
}

// Decides which process should resume execution
// Context-swtiches are executed in this function
void schedule( ctx_t* ctx ) {

  pcb_t* next = executing;

  int MAX_priority = 0;

  for (int i = 0; i < MAX_PROCS; i++) {
    if (procTab[i].status != STATUS_TERMINATED) {

      // procTab[i]'s priority is decided by using initial setted priority value and its age
      // 'base_priority' and 'age' can be found in hilevel.h. These two properties have been included
      // because of this scheduling function
      int priority = procTab[i].base_priority + procTab[i].age;
      // !!IMPORTANT!! ** variable 'priority' gets higher prority as number grows **

      // 'next process table (procTab)' is decided by priority that defined just above
      // If the procTab[i] that we are interested has higher priority then MAX_priority, set that procTab[i] for the 'next process table (procTab)'
      if (MAX_priority <= priority) {
        MAX_priority = priority;
        next = &procTab[i];
      }
    }
  }

  // 1. If procTab[i] is     executed, execute 'if statement'   therefore reset  age to 0.
  // 2. If procTab[i] is not executed, execute 'else statement' therefore adding age to the procTab[i]
  for (int i = 0; i < MAX_PROCS; i++) {
    if (procTab[i].status != STATUS_TERMINATED) {
      if (next->pid == procTab[i].pid) {
        procTab[i].age = 0;
      } else {
        procTab[i].age += 1;
      }
    }
  }

  // Context switch
  dispatch(ctx, executing, next);

  if (executing->status != STATUS_TERMINATED) {
    executing->status = STATUS_READY;
  }

  next->status = STATUS_EXECUTING;

  return;
}

// Initialisation of hilevel_handler_rst
void hilevel_handler_rst( ctx_t* ctx ) {

  /*
    <Strategy in 'hilevel_handler_rst'>

  *   1. Set up timer
        1-1. TIMER0 raises a peridoic interrupt for each timer tick
        1-2. GICC0  handles interrupt. The selected interrupts are forwarded to the processor
             via the IRA interrupt signal

  *   2. Set all of the process table status to STATUS_INVALID therefore not representing an active process

  *   3. Set up 'console'
        3-1. set procTab[ 0 ].ctx.pc as ( uint32_t )( &main_console )
        3-2. base_priority and age were already added to each of the procTab property.
        3-3. Set up following procTab[i] (1 ≤ i < MAX_PROCS)

  *   4. Check priority of the processes and dispatch highest prioritised procTab[i]
  */

  int counter = 0;
  int max     = 0;

  // 1
  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor

  // 2
  for( int i = 0; i < MAX_PROCS; i++ ) {
    procTab[ i ].status = STATUS_INVALID;
  }

  // 3
  memset( &procTab[ 0 ], 0, sizeof( pcb_t ) );
  procTab[ 0 ].pid      = 0;
  procTab[ 0 ].status   = STATUS_CREATED;
  procTab[ 0 ].tos      = ( uint32_t )( &tos_user );
  procTab[ 0 ].ctx.cpsr = 0x50;
  procTab[ 0 ].ctx.pc   = ( uint32_t )( &main_console );
  procTab[ 0 ].ctx.sp   = procTab[ 0 ].tos;
  procTab[ 0 ].base_priority = 1;
  procTab[ 0 ].age = 0;


  for (int i = 1; i < MAX_PROCS; i++) {
    memset( &procTab[ i ], 0, sizeof( pcb_t ) );
    procTab[ i ].pid      = i;
    procTab[ i ].status   = STATUS_TERMINATED;
    procTab[ i ].tos      = ( uint32_t )( &tos_user ) - (i * 0x00001000);
    procTab[ i ].ctx.cpsr = 0x50;
    procTab[ i ].ctx.sp   = procTab[ i ].tos;
    procTab[ i ].base_priority = 1;
    procTab[ i ].age = 0;
  }

  // 4
  for (int i = 0; i < MAX_PROCS; i++) {
    int priority = procTab[i].base_priority + procTab[i].age;
    if (max < priority) {
      counter = i;
      max = priority;
    }
  }

  dispatch( ctx, NULL, &procTab[ counter ] );

  int_enable_irq();

  return;

}

void hilevel_handler_irq( ctx_t* ctx ) {

  // read the interrupt identifier so we know the source.
  uint32_t id = GICC0->IAR;

  // handle the interrupt, then clear (or reset) the source.
  if( id == GIC_SOURCE_TIMER0 ) {

    print_timer_handling_interrupt();

    schedule(ctx);

    TIMER0->Timer1IntClr = 0x01;
  }

  // write the interrupt identifier to signal we're done.
  GICC0->EOIR = id;

  return;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {
  switch( id ) {

    // 0x00 == yield
    case 0x00 : {
      print_yield_message();
      schedule( ctx );
      break;
    }

    // 0x01 == write
    case 0x01 : {
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }

      ctx->gpr[ 0 ] = n;
      break;
    }

    // 0x03 == fork
    case 0x03 : {
      print_fork_message();

      /*
        <Strategy in fork>

       * 1. Find unusing process control block(pcb) by using emptied_pcb_id() and designate child pcb.

       * 2. memset: clean up its content like 'section 3' in 'hilevel_handler_rst'.

       * 3. memcpy: copy context from parent to child.

       * 4. memcpy: copy attributes from parent to child.

       * 5. Calculate offset for the stack pointer(sp) of child process table.
            This offset is used for the following process table setting for child.

       * 6. Set return

       */

      int id = emptied_pcb_id();
      pcb_t* child = get_child_pcb();

      memset( child, 0, sizeof( pcb_t ) );
      memcpy( &child->ctx, ctx, sizeof( ctx_t ) );

      uint32_t PARENT = executing->tos - PROCESSOR_SIZE;
      uint32_t CHILD  = child->tos     - PROCESSOR_SIZE;
      memcpy( ( void* ) CHILD, ( void* ) PARENT, PROCESSOR_SIZE );

      uint32_t offset = (uint32_t)( executing->tos - ctx->sp );
      procTab[ id ].pid              = id;
      procTab[ id ].status           = STATUS_CREATED;
      procTab[ id ].tos              = ( uint32_t )( &tos_user ) - (id * PROCESSOR_SIZE);
      procTab[ id ].ctx.cpsr         = 0x50;
      procTab[ id ].ctx.sp           = procTab[ id ].tos - offset;
      procTab[ id ].base_priority    = 1;
      procTab[ id ].age              = 0;

      ctx->gpr[ 0 ] = procTab[ id ].pid;
      procTab[ id ].ctx.gpr[ 0 ] = 0;

      break;
    }

    // 0x04 == exit
    case 0x04 : {
      print_exit_message();

      memset(executing, 0, sizeof( pcb_t ));

      executing->status = STATUS_TERMINATED;

      schedule(ctx);

      break;
    }

    // 0x05 == exec
    // execute processes
    // ex) execute P3, execute P4, execute P5, and execute DP
    case 0x05 : {
      print_execute_message();

      // set return
      ctx->pc = ctx->gpr[0];
      ctx->sp = executing->tos;

      break;
    }

    // 0x06 == kill
    case 0x06 : {
      print_kill_message();

      pcb_t* flag = get_pcb( ( pid_t )ctx->gpr[0] );
      if (flag != NULL) {
        memset( flag, 0, sizeof( pcb_t ));
        flag->status = STATUS_TERMINATED;
      }

      break;
    }

    // 0x07 == nice
    case 0x07 :{
      print_nice_message();

      int pid = ( pid_t )ctx->gpr[0];
      int base_priority = ctx->gpr[1];
      if(pid >= 0 && pid < MAX_PROCS && base_priority >= 0 && base_priority <= MAX_PROCS){
         procTab[pid].base_priority = base_priority;
         schedule(ctx);
       }
      break;
    }

    default : { // Unknown input occurred
      break;
    }
  }


  return;
}

/******************************************************************************/

pcb_t* get_pcb ( pid_t pid ) {

  for (int i = 0; i < MAX_PROCS; i++) {
    if (procTab[ i ].pid == pid) {
      return &procTab[ i ];
    }
  }
  return NULL;
}

pcb_t* get_child_pcb() {
  for (int i = 0; i < MAX_PROCS; i++) {
    if (procTab[ i ].status == STATUS_TERMINATED) {
      return &procTab[ i ];
    }
  }
  return NULL;
}

int emptied_pcb_id() {
  for( int i = 0; i < MAX_PROCS; i++ ) {
    if( procTab[ i ].status == STATUS_TERMINATED ) {
      return i;
    }
  }
  return -1;
}

void print_fork_message(){
  PL011_putc( UART0, '[', true);
  PL011_putc( UART0, 'F', true);
  PL011_putc( UART0, 'O', true);
  PL011_putc( UART0, 'R', true);
  PL011_putc( UART0, 'K', true);
  PL011_putc( UART0, ']', true);
  PL011_putc( UART0, '\n',     true );
}

void print_dispatch_message(uint8_t prev_pid, uint8_t next_pid){
  PL011_putc( UART0, '[',      true );
  PL011_putc( UART0, prev_pid, true );
  PL011_putc( UART0, '-',      true );
  PL011_putc( UART0, '>',      true );
  PL011_putc( UART0, next_pid, true );
  PL011_putc( UART0, ']',      true );
  PL011_putc( UART0, '\n',     true );
}

void print_timer_handling_interrupt(){
  PL011_putc( UART0, '[', true );
  PL011_putc( UART0, 'T', true );
  PL011_putc( UART0, 'I', true );
  PL011_putc( UART0, 'M', true );
  PL011_putc( UART0, 'E', true );
  PL011_putc( UART0, 'R', true );
  PL011_putc( UART0, ']', true );
  PL011_putc( UART0, '\n', true );
}

void print_yield_message(){
  PL011_putc( UART0, '[', true);
  PL011_putc( UART0, 'Y', true);
  PL011_putc( UART0, 'I', true);
  PL011_putc( UART0, 'E', true);
  PL011_putc( UART0, 'L', true);
  PL011_putc( UART0, 'D', true);
  PL011_putc( UART0, ']', true);
}

void print_exit_message(){
  PL011_putc( UART0, '[', true);
  PL011_putc( UART0, 'E', true);
  PL011_putc( UART0, 'X', true);
  PL011_putc( UART0, 'I', true);
  PL011_putc( UART0, 'T', true);
  PL011_putc( UART0, ']', true);
}

void print_execute_message(){
  PL011_putc( UART0, '[', true);
  PL011_putc( UART0, 'E', true);
  PL011_putc( UART0, 'X', true);
  PL011_putc( UART0, 'E', true);
  PL011_putc( UART0, 'C', true);
  PL011_putc( UART0, 'U', true);
  PL011_putc( UART0, 'T', true);
  PL011_putc( UART0, 'E', true);
  PL011_putc( UART0, ']', true);
}

void print_kill_message(){
  PL011_putc( UART0, '[', true);
  PL011_putc( UART0, 'K', true);
  PL011_putc( UART0, 'I', true);
  PL011_putc( UART0, 'L', true);
  PL011_putc( UART0, 'L', true);
  PL011_putc( UART0, ']', true);
}

void print_nice_message(){
  PL011_putc( UART0, '[', true );
  PL011_putc( UART0, 'N', true );
  PL011_putc( UART0, 'I', true );
  PL011_putc( UART0, 'C', true );
  PL011_putc( UART0, 'E', true );
  PL011_putc( UART0, ']', true );
}
/******************************************************************************/
