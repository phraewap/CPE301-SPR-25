/*
 * Task3.c
 *
 * Created: 3/9/2025 8:17:53 PM
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
int main(void) {
	DDRB |= (1 << 5); // PB5 output
	PORTB |= (1 << 5); 
	PORTD = 0xFF; // enables pull-ups for PD2

	EIMSK = (1 << INT0); 
	sei (); // interrupt 
	
	while (1) {}
	return 0;
}