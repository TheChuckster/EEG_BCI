//---------------------------------------------------------------
// From http://www.control.lth.se/%7Eanton/tinyrealtime/
// written by Dan Henriksson, Anton Cervin
// dan@control.lth.se, anton@control.lth.se
//
// Modified for the Mega644 by Bruce Land Jan 2009
// -- Changed register names to mega644 regs
// -- Changed clock rate to 20 MHz
// -- Added one byte to stack init section in trtCreateTask
//---------------------------------------------------------------

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "trtSettings.h"

#define TERMINATED 0
#define READYQ 1
#define TIMEQ 2
#define WAIT_OFFSET 2

#define SECONDS2TICKS(T) ((uint32_t)((T)*TICKSPERSECOND))

#define lo8(X) ((uint8_t)((uint16_t)(X)))
#define hi8(X) ((uint8_t)((uint16_t)(X) >> 8))


/******************* KERNEL DATA STRUCTURES ************************/

struct task {
  uint8_t spl;       // Stack pointer lo8
  uint8_t sph;       // Stack pointer hi8
  uint32_t release;
  uint32_t deadline;
  uint8_t state;     // 0=terminated, 1=readyQ, 2=timeQ
                     // 3=waiting for Sem1, 4=waiting for Sem2, etc.
};

struct kernel {
  uint8_t nbrOfTasks; // number of tasks created so far
  uint8_t running;
  struct task tasks[MAXNBRTASKS+1]; // +1 for the idle task
  uint8_t semaphores[MAXNBRSEMAPHORES]; // counters for semaphores
  uint8_t *memptr; // pointer to free memory
  uint32_t cycles;  // number of major cycles since system start
  uint32_t nextHit; // next kernel wake-up time
} kernel;


/******************* CLOCK INTERRUPT HANDLER ************************/

/**
 * Clock interrupt handler executing the kernel
 */
ISR(TIMER1_COMPA_vect) {

  uint8_t running, oldrunning;
  struct task *t;
  uint8_t i;
  uint32_t now;
  uint32_t nextHit;
  int32_t timeleft;
	
  TIMSK1 = 0 ; //&= ~(1<<OCIE1A); // turn off output compare 1A ISR
  //PORTC = ~PORTC ;
  nextHit = 0x7FFFFFFF;
  oldrunning = kernel.running;
  running = 0;

  if (TIFR1 & (1<<TOV1)) {
    ++kernel.cycles;
    TIFR1 |= (1<<TOV1) ;
  }

  // Read clock

  now = (kernel.cycles << 16) + TCNT1;

  // Release tasks from TimeQ and determine new running task

  for (i=1; i <= kernel.nbrOfTasks; i++) {
    t = &kernel.tasks[i];
    if (t->state == TIMEQ) {
      if (t->release <= now) {
	t->state = READYQ;
      } else if (t->release < nextHit) {
	nextHit = t->release;
      }
    }
    if (t->state == READYQ) {
      if (t->deadline < kernel.tasks[running].deadline) {
	running = i;
      }
    }
  }

  if (running != oldrunning) { // perform context switch?

    // store old context
    t = &kernel.tasks[oldrunning];
    t->spl = SPL;
    t->sph = SPH;

    // load new context
    t = &kernel.tasks[running];
    SPH = t->sph;
    SPL = t->spl;

    kernel.running = running;

  }

  kernel.nextHit = nextHit;  

  now = (kernel.cycles << 16) + TCNT1;
  timeleft = (int32_t)nextHit - (int32_t)now;
  if (timeleft < 4) {
    timeleft = 4;
  }

  if ((unsigned long)TCNT1 + timeleft < 65536) {
    OCR1A = TCNT1 + timeleft;
  } else if (TCNT1 < 65536 - 4) {
    OCR1A = 0x0000;
  } else {
    OCR1A = 4;
  }

  TIMSK1 = (1<<OCIE1A);
}


/********************************** API ************************************/

void trtInitKernel(int idlestack) {

  /* Set up timer 1 */
  TCNT1 = 0x0000;        /* reset counter 1 */
  TCCR1A = 0x00;         /* normal operation */
  TCCR1B = PRESCALEBITS; /* prescaler = 1024 */
  TIMSK1 = (1<<OCIE1A);  // turn on compare match ISR

  kernel.memptr = (void*)(RAMEND - idlestack);
  kernel.nbrOfTasks = 0;
  kernel.running = 0;

  kernel.cycles = 0x0000;
  kernel.nextHit = 0x7FFFFFFF;

  // Initialize idle task (task 0)
  kernel.tasks[0].deadline = 0x7FFFFFFF;
  kernel.tasks[0].release = 0x00000000;

  sei(); /* set enabled interrupts */
}


void trtCreateTask(void (*fun)(void*), uint16_t stacksize, uint32_t release, uint32_t deadline, void *args) {

  uint8_t *sp;
  struct task *t;
  int i;

  cli(); // turn off interrupts

  ++kernel.nbrOfTasks;

  sp = kernel.memptr;
  kernel.memptr -= stacksize;  // decrease free mem ptr

  // initialize stack
  *sp-- = lo8(fun);       // store PC(lo)
  *sp-- = hi8(fun);       // store PC(hi)
  for (i=0; i<25; i++)    //WAS -- for (i=0; i<24; i++)
    *sp-- = 0x00;         // store SREG,r0-r1,r3-r23

  // Save args in r24-25 (input arguments stored in these registers)
  *sp-- = lo8(args);
  *sp-- = hi8(args);

  for (i=0; i<6; i++)
    *sp-- = 0x00;         // store r26-r31

  t = &kernel.tasks[kernel.nbrOfTasks];

  t->release = release;
  t->deadline = deadline;
  t->state = TIMEQ;

  t->spl = lo8(sp);       // store stack pointer
  t->sph = hi8(sp);
  
  // call interrupt handler to schedule
  TIMER1_COMPA_vect();

}

void trtCreateSemaphore(uint8_t semnbr, uint8_t initVal) {

  cli(); // turn off interrupts

  kernel.semaphores[semnbr-1] = initVal;
  
  sei(); // set enabled interrupts;
}

void trtWait(uint8_t semnbr) {

  struct task *t;
  uint8_t *s;

  t = &kernel.tasks[kernel.running];

  cli(); // disable interrupts

  s = &kernel.semaphores[semnbr-1];
  if ((*s) > 0) {
    (*s)--;
  } else {

    t->state = semnbr + WAIT_OFFSET; // waiting for Sem#semnbr
    // call interrupt handler to schedule
	TIMER1_COMPA_vect();
  }

  sei(); // reenable interrupts
}

void trtSignal(uint8_t semnbr) {

  uint8_t i;
  struct task *t;
  uint32_t minDeadline = 0xFFFFFFFF;
  uint8_t taskToReadyQ = 0;

  cli(); // disable interrupts

  for (i=1; i <= kernel.nbrOfTasks; i++) {
    t = &kernel.tasks[i];
    if (t->state == (semnbr + WAIT_OFFSET)) {
      if (t->deadline <= minDeadline) {
	taskToReadyQ = i;
	minDeadline = t->deadline;
      }
    }
  }

  if (taskToReadyQ == 0) {
    kernel.semaphores[semnbr-1]++;
  } else {
    kernel.tasks[taskToReadyQ].state = READYQ; // make task ready
    // call interrupt handler to schedule
	TIMER1_COMPA_vect();
  }

  sei(); // reenable interrupts
}

uint32_t trtCurrentTime(void) {

  return (((uint32_t)kernel.cycles << 16) + (uint32_t)TCNT1);
}


void trtSleepUntil(uint32_t release, uint32_t deadline) {

  struct task *t;

  t = &kernel.tasks[kernel.running];

  cli(); // turn off interrupts

  t->state = TIMEQ;
  t->release = release;
  t->deadline = deadline;
  
  // call interrupt handler to schedule
  TIMER1_COMPA_vect();
}


uint32_t trtGetRelease(void) {
  return kernel.tasks[kernel.running].release;
}

uint32_t trtGetDeadline(void) {
  return kernel.tasks[kernel.running].deadline;
}

void trtTerminate(void) {

  cli();

  kernel.tasks[kernel.running].state = TERMINATED;

  // call interrupt handler to schedule
  TIMER1_COMPA_vect();
}

// --- added by bruce land --------------
uint8_t trtAccept(uint8_t semnbr) {

  //struct task *t;
  uint8_t *s;
  uint8_t temp ;
  //t = &kernel.tasks[kernel.running];

  cli(); // disable interrupts

  s = &kernel.semaphores[semnbr-1];
  temp = *s ;
  if ((*s) > 0) {
    (*s)--;
  } 
  sei(); // reenable interrupts
  return temp ;
}
