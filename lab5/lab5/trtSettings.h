
//trtSettings.h
// include before trtkernel

#define MAXNBRTASKS 4
#define MAXNBRSEMAPHORES 7 // 3 sem (1-3) are needed for uart
#define MAXNBRMUTEX 1

#define PRESCALER 1024 // the actual value for timer1 prescalar
#define PRESCALEBITS 5 // the bits to be set in TCCR1b 

#define F_CPU 16000000UL // clock frequency in Hz
#define TICKSPERSECOND F_CPU / PRESCALER

/* UART baud rate */
#define UART_BAUD  57600

#define SEM_RX_ISR_SIGNAL 1
#define SEM_STRING_DONE 2 // user hit <enter>