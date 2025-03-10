/*
 * Task2plus3.c
 *
 * Created: 3/9/2025 9:25:37 PM
 * Author : Minnie
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void delaySubr() {
	
	// 12 * 125 ms = 1.6
	for (int i = 0; i < 12; i++) {
		_delay_ms(125);
	}
}

ISR(INT0_vect) {
	PORTB = ~(1 << 5); // LED on
	
	// 150ms * 20 = 3000ms = 3s
	for (int i = 0; i < 20; i++) {
		delaySubr();
	}
	
	PORTB = (1 << 5); // off after delay
	EIFR |= (1 << INTF0); // clear interrupt flag to reset
}

int main(void)
{
   	DDRB |= (1 << 5); // PB5 as  output
   	PORTB |= (1 << 5);
   	DDRC &= (0 << 1);
   	PORTC |= (1 << 1); // enables pull ups
   	
	EIMSK = (1 << INT0);
	sei (); // interrupt
	
   	while(1) {
	   	if (!(PINC & (1 << 1))) { // push button on low
		   	PORTB &= ~(1 << 5); // LED on
		   	delaySubr();
	   	}
	   	PORTB |= (1 << 5); // LED off
   	}
}

