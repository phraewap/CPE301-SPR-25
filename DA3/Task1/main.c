/*
* Task1.c
*
* Created: 3/27/2025 10:05:40 PM
* Author : Minnie
*/

#include <avr/io.h>
#include <avr/interrupt.h>

// counter variables
int c3 = 0; // timer3
int c4 = 0; // timer4

ISR(TIMER3_COMPA_vect) {
	c3++;
	if (c3 >= 8000) { // 8000 x 0.25ms = 2secs
		PORTB ^= (1 << PINB4); // PB4 LED
		c3 = 0;
	}
}

ISR(TIMER4_OVF_vect) {
	c4++;
	if (c4 >= 10000) { // 0.100 ms * 10000 = 1sec
		PORTB ^= (1 << PINB3); // PB3 LED
		c4 = 0;
	}
	TCNT4 = 65336; // recall the value
}

void INIT(void){
	/* --- Timer0 --- */
	TCNT0 = 224; // ~ 0.125ms delay
	TCCR0A = 0x00; // normal
	TCCR0B |= (1 << CS01) | (1 << CS00); // pre scale = 64
	
	/* --- Timer3 (CTC mode) --- */
	TCNT3 = 0;
	OCR3A = 499; // compares with duration of 0.25ms
	TCCR3B |= (1 << CS31); // timer3 w/ pre scale = 8
	TCCR3B |= (1 << WGM32); // CTC
	TIMSK3 |= (1 << OCIE3A); // enables timer3 then compares
	
	/* --- Timer4 (Normal Mode) --- */
	TCNT4 = 65336; // 0.1ms delay
	TCCR4A = 0x00; // normal mode
	TCCR4B |= (1 << CS41) | (1 << CS40); // timer4 w/ pre scale = 8
	TIMSK4 |= (1 << TOIE4); // overflow interrupt
}

void T0_overflow(void){
	int c0 = 0;
	if (TIFR0 & (1 << TOV0)) {
		TIFR0 |= (1 << TOV0); // clear overflow
		TCNT0 = 224; // Reload for 0.125ms delay
		c0++;
		if (c0 >= 11720) { // 1.5sec (11720 x 0.125ms)
			PORTB ^= (1 << PINB5);
			c0 = 0;
		}
	}
}

int main(void) {
	// setting ports as outputs
	DDRB |= (1 << PINB5) | (1 << PINB4) | (1 << PINB3);
	PORTB &= ~((1 << PINB5) | (1 << PINB4) | (1 << PINB3));
	
	INIT();
	
	sei(); // global interrupt
	
	while (1) {
		T0_overflow();
	}
}




