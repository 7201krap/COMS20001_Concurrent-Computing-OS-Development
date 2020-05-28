/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "console.h"

void puts( char* x, int n ) {
  for( int i = 0; i < n; i++ ) {
    PL011_putc( UART1, x[ i ], true );
  }
}

void gets( char* x, int n ) {
  for( int i = 0; i < n; i++ ) {
    x[ i ] = PL011_getc( UART1, true );

    if( x[ i ] == '\x0A' ) {
      x[ i ] = '\x00'; break;
    }
  }
}

extern void main_P3();
extern void main_P4();
extern void main_P5();
extern void main_DP();

void* load( char* x ) {
  if     ( 0 == strcmp( x, "P3" ) ) {
    return &main_P3;
  }
  else if( 0 == strcmp( x, "P4" ) ) {
    return &main_P4;
  }
  else if( 0 == strcmp( x, "P5" ) ) {
    return &main_P5;
  }
  else if( 0 == strcmp( x, "DP" ) ) {
    return &main_DP;
  }

  return NULL;
}

/* The behaviour of a console process can be summarised as an infinite
 * loop over three main steps, namely
 *
 * 1. write a command prompt then read a command,
 * 2. tokenize command, then
 * 3. execute command.
 *
 * As is, the console only recognises the following commands:
 *
 * a. execute <program name>
 *
 *    This command will use fork to create a new process; the parent
 *    (i.e., the console) will continue as normal, whereas the child
 *    uses exec to replace the process image and thereby execute a
 *    different (named) program.  For example,
 *
 *    execute P3
 *
 *    would execute the user program named P3.
 *
 * b. terminate <process ID>
 *
 *    This command uses kill to send a terminate or SIG_TERM signal
 *    to a specific process (identified via the PID provided); this
 *    acts to forcibly terminate the process, vs. say that process
 *    using exit to terminate itself.  For example,
 *
 *    terminate 3
 *
 *    would terminate the process whose PID is 3.
 *
 * c. nice(<process ID>, <base_priority>)
 *
 *    This command uses nice to set base_priority. This forces to change
 *    procTab[i].base_priority therefore priority of procTab[i] could be
 *    modified dynamically. nice command is implemented in void main_console().
 *
 *    nice(i, j) -> procTab[i].base_priority = j
 *
 *    For example, nice(1, 5) would set base_priority of procTab[1] to 5
 *
 *    To check whether nice works or not,
 *      1. comment nice(1, 1) and uncomment nice(1, 5) in void main_P3()
 *      2. open console
 *      3. type 'execute P3' in the console
 *      4. check P3 runs 5 times in a row such as,

        [TIMER]
        [0->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->0]

        [TIMER]
        [0->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->1]
        P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3P3[TIMER]
        [1->0]

        [TIMER]
 */

void main_console() {
  while( 1 ) {
    char cmd[ MAX_CMD_CHARS ];

    // step 1: write command prompt, then read command.

    puts( "console$ ", 7 ); gets( cmd, MAX_CMD_CHARS );

    // step 2: tokenize command.

    int cmd_argc = 0; char* cmd_argv[ MAX_CMD_ARGS ];

    for( char* t = strtok( cmd, " " ); t != NULL; t = strtok( NULL, " " ) ) {
      cmd_argv[ cmd_argc++ ] = t;
    }

    // step 3: execute command.

    if     ( 0 == strcmp( cmd_argv[ 0 ], "execute"   ) ) {
      void* addr = load( cmd_argv[ 1 ] );

      if( addr != NULL ) {
        if( 0 == fork() ) {
          exec( addr );
        }
      }
      else {
        puts( "unknown program\n", 16 );
      }
    }

    else if ( 0 == strcmp( cmd_argv[ 0 ], "terminate" ) ) {
      kill( atoi( cmd_argv[ 1 ] ), SIG_TERM );
    }

    else if (0 == strcmp( cmd_argv[ 0 ], "nice" )){
        int pid = atoi(strtok( NULL, " " ));
        int priority = atoi(strtok( NULL, " " ));
        nice(pid, priority);
    }

    else {
      puts( "unknown command\n", 16 );
    }
  }

  exit( EXIT_SUCCESS );
}
