

#define F_CPU 16000000UL // clk

// libs
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// usart globals
#define BAUDRATE 9600
#define BAUD_PRE (((F_CPU / (BAUDRATE * 16UL))) - 1)

float cat;            // store values read from ADC
char dog[5];         // itoa output

void ADC_init(void){
	ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));    // 16Mhz/128 = 125Khz 
	ADMUX |= (1<<REFS0);                // voltage Ref = 5v
	ADCSRA |= (1<<ADEN);                // ADC on
	ADCSRA |= (1<<ADSC);                // for conversion
}

uint16_t read_adc(uint8_t channel){
	ADMUX &= 0xF0;                    // clears old channel
	ADMUX |= channel;                
	ADCSRA |= (1<<ADSC);             // new conversion
	while(ADCSRA & (1<<ADSC));        
	return ADCW;                    // returns the ADC value 
}

void USART_init(void){
	
	UBRR0H = (uint8_t)(BAUD_PRE >> 8); // set the upper bits
	UBRR0L = (uint8_t)(BAUD_PRE); // set the lower bits
	UCSR0B = (1 << RXEN0)|(1 << TXEN0); 
	UCSR0C = (3 << UCSZ00); 
}

void USART_send(unsigned char data){
	while(!(UCSR0A & (1 << UDRE0)));  // waits for USART reg to be empty 
	UDR0 = data;                    
}

void USART_putstring(char* StringPtr){
	while(*StringPtr != 0x00){       
		USART_send(*StringPtr);     
		StringPtr++;                
	}
}

void timer_init(void) {
	
	TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);
	OCR1A = 2499;
	TIMSK1 |= (1 << OCIE1A);
}
ISR(TIMER1_COMPA_vect) {
	cat = read_adc(0);        
	itoa(cat, dog, 10);       
	_delay_ms(50);
	USART_putstring(dog);      
	USART_send('\n');                
}


int main(void){
	
	ADC_init();        // adc setup
	USART_init();     // usart setup
	timer_init();	 // timer setup
	
	sei();
	
	while(1){
		// left empty
	}       
	return 0;
}
