/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, UART implementation
 *
 * $Id: uart.c,v 1.1 2005/12/28 21:38:59 joerg_wunsch Exp $
 *
 * Mod for mega644 Bruce Land Jan2009
 * interrupt-driven getchar added by BL 
 * Circular buffer/ISR driven putchar added by Jeff Melville
 */

#define UART_USE_RING_BUF_TX

/* CPU frequency */
//#define F_CPU 16000000UL

// UART baud rate defined in settings
//#define UART_BAUD  9600


#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>

//jsm - add for interrupt handling
#include <avr/interrupt.h>
#include "trtSettings.h"
#include "trtuart.h"

//jsm - create the circular buffer with in/out index
#define TX_BUF_SIZE 200
static volatile unsigned int tx_in;
static volatile unsigned int tx_out;
static volatile char tx_buff [TX_BUF_SIZE];

/*
 * Initialize the UART to 9600 Bd, tx/rx, 8N1.
 */
void
trt_uart_init(void)
{
#if F_CPU < 2000000UL && defined(U2X)
  UCSR0A = _BV(U2X);             /* improve baud rate error by using 2x clk */
  UBRR0L = (F_CPU / (8UL * UART_BAUD)) - 1;
#else
  UBRR0L = (F_CPU / (16UL * UART_BAUD)) - 1;
#endif
  UCSR0B = _BV(TXEN0) | _BV(RXEN0); /* tx/rx enable */
  
  //Set up circular buffer state variables
  tx_in = 0;
  tx_out = 0;

  //enable receive ISR -- added for TRT
  UCSR0B |= (1<<RXCIE0) ;
}



ISR( USART0_UDRE_vect )
{
  if( tx_in == tx_out ){		// nothing to send
    UCSR0B &= ~(1 << UDRIE0);	// disable TX interrupt
    //return;
  }
  else {
  	UDR0 = tx_buff[tx_out];
  	tx_out++;
  	if (tx_out == TX_BUF_SIZE) tx_out = 0;
  }
}

int uart_putchar(char c, FILE *stream) {
  if (c == '\n') uart_putchar('\r', stream);
  char i = tx_in;
  i++;
  if (i == TX_BUF_SIZE) i = 0;
  tx_buff[tx_in] = c;
  while( i == tx_out);		// until at least one byte free
					// tx_out modified by interrupt !
  tx_in = i;
  UCSR0B |= (1 << UDRIE0);  // enable TX interrupt
  return 0;
 }


/*
 * Receive a character from the UART Rx.
 *
 * This features a simple line-editor that allows to delete and
 * re-edit the characters entered, until either CR or NL is entered.
 * Printable characters entered will be echoed using uart_putchar().
 *
 * Editing characters:
 *
 * . \b (BS) or \177 (DEL) delete the previous character
 * . ^u kills the entire input buffer
 * . ^w deletes the previous word
 * . ^r sends a CR, and then reprints the buffer
 * . \t will be replaced by a single space
 *
 * All other control characters will be ignored.
 *
 * The internal line buffer is RX_BUFSIZE (80) characters long, which
 * includes the terminating \n (but no terminating \0).  If the buffer
 * is full (i. e., at RX_BUFSIZE-1 characters in order to keep space for
 * the trailing \n), any further input attempts will send a \a to
 * uart_putchar() (BEL character), although line editing is still
 * allowed.
 *
 * Input errors while talking to the UART will cause an immediate
 * return of -1 (error indication).  Notably, this will be caused by a
 * framing error (e. g. serial line "break" condition), by an input
 * overrun, and by a parity error (if parity was enabled and automatic
 * parity recognition is supported by hardware).
 *
 * Successive calls to uart_getchar() will be satisfied from the
 * internal buffer until that buffer is emptied again.
 */

 // --- added for TRT ------------
uint8_t trt_rx_c;

ISR(USART0_RX_vect) {
	trt_rx_c = UDR0;
	trtSignal(SEM_RX_ISR_SIGNAL) ;
}
// --- end addition --------------

int
uart_getchar(FILE *stream)
{
  uint8_t c;
  char *cp, *cp2;
  static char b[RX_BUFSIZE];
  static char *rxp;

  if (rxp == 0)
    for (cp = b;;)
      {
	// --- trtWait added instead of loop_until wait
	trtWait(SEM_RX_ISR_SIGNAL) ; //loop_until_bit_is_set(UCSR0A, RXC0)
	if (UCSR0A & _BV(FE0))
	  return _FDEV_EOF;
	if (UCSR0A & _BV(DOR0))
	  return _FDEV_ERR;
	// -- added to take char from ISR ---
	  c = trt_rx_c ; //c = UDR0; -- CHANGED

	/* behaviour similar to Unix stty ICRNL */
	if (c == '\r')
	  c = '\n';
	if (c == '\n')
	  {
	    *cp = c;
	    uart_putchar(c, stream);
	    rxp = b;
		// --- added for TRT to signal string-end
		trtSignal(SEM_STRING_DONE); //added--to signal end of string
	    break;
	  }
	else if (c == '\t')
	  c = ' ';

	if ((c >= (uint8_t)' ' && c <= (uint8_t)'\x7e') ||
	    c >= (uint8_t)'\xa0')
	  {
	    if (cp == b + RX_BUFSIZE - 1)
	      uart_putchar('\a', stream);
	    else
	      {
		*cp++ = c;
		uart_putchar(c, stream);
	      }
	    continue;
	  }

	switch (c)
	  {
	  case 'c' & 0x1f:
	    return -1;

	  case '\b':
	  case '\x7f':
	    if (cp > b)
	      {
		uart_putchar('\b', stream);
		uart_putchar(' ', stream);
		uart_putchar('\b', stream);
		cp--;
	      }
	    break;

	  case 'r' & 0x1f:
	    uart_putchar('\r', stream);
	    for (cp2 = b; cp2 < cp; cp2++)
	      uart_putchar(*cp2, stream);
	    break;

	  case 'u' & 0x1f:
	    while (cp > b)
	      {
		uart_putchar('\b', stream);
		uart_putchar(' ', stream);
		uart_putchar('\b', stream);
		cp--;
	      }
	    break;

	  case 'w' & 0x1f:
	    while (cp > b && cp[-1] != ' ')
	      {
		uart_putchar('\b', stream);
		uart_putchar(' ', stream);
		uart_putchar('\b', stream);
		cp--;
	      }
	    break;
	  }
      }

  c = *rxp++;
  if (c == '\n')
    rxp = 0;

  return c;
}

