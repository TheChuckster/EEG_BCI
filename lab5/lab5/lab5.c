#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include "uart.h"

// UART file descriptor
// putchar and getchar are in uart.c
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

// timeout values for each task
#define t1 100

volatile unsigned char time1; // timeout counter
unsigned char led; // light states

volatile int Ain, AinLow; // raw A to D number

// timer 0 compare ISR
ISR (TIMER0_COMPA_vect)
{
	if (time1 > 0) --time1;
	
	if (time1 == 0)
	{
		time1 = t1;

		// toggle the LED and the square wave test output
		led ^= 1;
		PORTD = (led << PORTD2) | (led << PORTD3);
	}
}

void pwm_init()
{
	// Set timer/counter 1 to use 10-bit PWM mode.
	// The counter counts from zero to 1023 and then back down
	// again. Each time the counter value equals the value
	// of OCR1(A), the output pin is toggled.
	// The counter speed is set in TCCR1B, to clk / 256 = 28800Hz.
	// Effective frequency is then clk / 256 / 2046 = 14 Hz

	OCR1A = 512;
	TCCR1A = (1 << COM1A1) | (1 << WGM11) | (1 << WGM10);
	TCCR1B = (1 << CS12);
}

int main()
{
	// init the A to D converter 
	//channel zero/ right adj /ext Aref
	//!!!DO NOT CONNECT Aref jumper!!!!
	ADMUX = (1<<REFS0); 
	//enable ADC and set prescaler to 1/127*16MHz=125,000
	//and set int enable
	ADCSRA = (1<<ADEN) + 7 ; 

	// set up the LED port
  	DDRD = (1 << PORTD2) | (1 << PORTD3); // PORT D.2 is an ouput

  	// set up timer 0 for 1 mSec timebase
  	TIMSK0 = 1 << OCIE0A; // turn on timer 0 cmp match ISR
  	OCR0A = 249; // set the compare register to 250 time ticks
  	TCCR0B = 3; // set prescalar to divide by 64
  	TCCR0A = 1 << WGM01; // turn on clear-on-match
  	
  	// set up timer for PWM
  	pwm_init();

  	led = 0x00; // init the LED status

  	time1 = t1; // init the task timer
   
	// init the UART -- uart_init() is in uart.c
	uart_init();
	stdout = stdin = stderr = &uart_str;
	fprintf(stdout,"\n\rStarting ADC ISR demo...\n\r"); 

	sei();

	// measure and display loop
	while (1)
	{    
		// start another conversion
		ADCSRA |= (1<<ADSC);

		AinLow = (int)ADCL;
		Ain = (int)ADCH*256; 
		Ain = Ain + AinLow;

		// program ONLY gets here after ADC ISR is done
		fprintf(stdout, "%d\n\r", Ain);
	}
}
