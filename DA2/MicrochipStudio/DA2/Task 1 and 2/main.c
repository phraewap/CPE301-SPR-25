/*
 * Task 1 and 2.c
 *
 * Created: 3/7/2025 7:33:25 PM
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

int main(void)
{
	DDRB |= (1 << 5); // PB5 as  output
	PORTB |= (1 << 5);
	DDRC &= (0 << 1);
	PORTC |= (1 << 1); // enables pull ups
	
	while(1) {
		if (!(PINC & (1 << 1))) { // push button on low
			PORTB &= ~(1 << 5); // LED on
			delaySubr();
		}
		PORTB |= (1 << 5); // LED off
	}
}
