// This pgm just blinks D.2 for testing the protoboard.
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

// serial communication library
#include "uart.h"
// UART file descriptor
// putchar and getchar are in uart.c
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
  

//timeout values for each task
#define t1 200   
#define begin {
#define end }
 
// task subroutines
void task1(void);  	//blink at 2 or 8 Hz 
void initialize(void); //all the usual mcu stuff 
          
volatile unsigned char time1;	//timeout counter 
unsigned char led;				//light states  
unsigned int ticks ;			// running time
         
//**********************************************************
//timer 0 compare ISR
ISR (TIMER0_COMPA_vect)
begin      
  //Decrement the  time if they are not already zero
  if (time1>0)	--time1;
end  

//**********************************************************       
//Entry point and task scheduler loop
int main(void)
begin  
  	initialize();
  
  	//main task scheduler loop 
  	while(1)
  	begin     
		if (time1==0){time1=t1;	task1();}
  	end
end  
  
//**********************************************************          
//Task 1
void task1(void) 
begin 
 
  	//toggle the second bit
  	led = led ^ 1 ;
  	PORTD = (led<<PORTD2) ;

	// print time to test USART
	printf("%d\n\r", ticks++);

	// get a single character to test USART
	if (UCSR0A & (1<<RXC0))
	begin
		printf("you typed...%c\n\r", UDR0);
	end
end  
 

//********************************************************** 
//Set it all up
void initialize(void)
begin
	//set up the LED port
  	DDRD = (1<<PORTD2) ;	// PORT D.2 is an ouput
           
  	//set up timer 0 for 1 mSec timebase 
  	TIMSK0= (1<<OCIE0A);	//turn on timer 0 cmp match ISR 
  	OCR0A = 249;  		//set the compare register to 250 time ticks
  	//set prescalar to divide by 64 
  	TCCR0B= 3; 	
  	// turn on clear-on-match
  	TCCR0A= (1<<WGM01) ;
    
  	//init the LED status 
  	led=0x00; 
  
  	//init the task timer
  	time1=t1;  
    
	//init the UART -- uart_init() is in uart.c
  	uart_init();
  	stdout = stdin = stderr = &uart_str;
  	fprintf(stdout,"Starting...\n\r");
    
  	//crank up the ISRs
  	sei();
end  

   