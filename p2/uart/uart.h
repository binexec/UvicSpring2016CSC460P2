#ifndef MY_UART_H
#define MY_UART_H

/*http://www.appelsiini.net/2011/simple-usart-with-avr-libc*/

#include <avr/io.h>
#include <stdio.h>
#include <util/setbaud.h>
#include <avr/sfr_defs.h>

#ifndef F_CPU
	#define F_CPU 16000000UL
#endif

#ifndef BAUD
	#define BAUD 9600
#endif

void uart_putchar(char c, FILE *stream);
char uart_getchar(FILE *stream);
void uart_init(void);
void uart_setredir(void);

/* http://www.ermicro.com/blog/?p=325 */

#endif