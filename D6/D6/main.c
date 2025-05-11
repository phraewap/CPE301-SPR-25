/*
 * D6.c
 *
 * Created: 5/10/2025 12:35:42 AM
 * Author : Minnie
 */ 

//some parts was from the professor's github

#define F_CPU 16000000UL

// libraries 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// declaring globals 
#define BAUDRATE 9600
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)

#define Trigger_pin PD2
#define Echo_pin PD5

#define SHIFT_REGISTER DDRB
#define SHIFT_PORT PORTB
#define DATA (1<<PB3)
#define LATCH (1<<PB2)
#define CLOCK (1<<PB5)

#define PRECISION 0
#define MINWIDTH 1
#define MAX 535
#define MIN 115
#define STEPSIZE ((float)(MAX - MIN)/105.0)
#define STEPDELAY 100
#define WAITDELAY 500

const double ANGLE_STEP = 180.0 / 105.0;
int timerOverflow = 0;

const uint8_t SEGMENT_MAP[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99,
0x92, 0x82, 0xF8, 0x80, 0x90};

const uint8_t SEGMENT_SELECT[] = {0xF1, 0xF2, 0xF4, 0xF8};

volatile int displayVal = 0;
volatile uint8_t currentDigit = 0;


// for usart 
void USART_init(void) {
	UBRR0H = (uint8_t)(BAUD_PRESCALLER>>8); //Load baud rate high byte
	UBRR0L = (uint8_t)(BAUD_PRESCALLER);    //Load baud rate low byte
	UCSR0B = (1<<TXEN0);                    //Enable transmitter
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);       //8-bit data, no parity, 1 stop bit
}

void USART_send(unsigned char data){
	while(!(UCSR0A & (1<<UDRE0))); //Wait until buffer empty
	UDR0 = data;                   //Send byte
}

void USART_putstring(char* StringPtr){
	while(*StringPtr != 0x00){      //Loop until null terminator
		USART_send(*StringPtr);
		StringPtr++;
	}
}

// for spi
void init_IO(void) {
	SHIFT_REGISTER |= (DATA | LATCH | CLOCK); //Set SPI pins as outputs
	SHIFT_PORT &= ~(DATA | LATCH | CLOCK);    //Set SPI pins low
}

void init_SPI(void) {
	SPCR0 = (1 << SPE) | (1 << MSTR); //Enable SPI, Master mode
}

void spi_send(unsigned char byte) {
	SPDR0 = byte;                     //Load byte into SPI data register
	while (!(SPSR0 & (1 << SPIF)));   //Wait for transmission complete
}

// for ultra sonic
//us = ultr asonic
void usSetup() {
	DDRD |= (1 << Trigger_pin);       //Trigger pin as output
	DDRD &= ~(1 << Echo_pin);         //Echo pin as input
	PORTD &= ~(1 << Echo_pin);        //Ensure pull-down
	TIMSK1 |= (1 << TOIE1);           //Enable Timer1 overflow interrupt
	TCCR1A = 0x00;                    //Normal mode
	sei();                            //Enable global interrupts
}

// for spi
double measureDistance() {
	TCCR1B |= (1 << CS10);            //Start Timer1 with no prescaler
	long tickCount = 0;
	double distResult = 0;
	uint32_t timeoutLimit = 30000;    //Timeout to prevent infinite wait

	PORTD |= (1 << Trigger_pin);      //Send 10us trigger pulse
	_delay_us(10);
	PORTD &= ~(1 << Trigger_pin);

	TCNT1 = 0;                        //Reset timer
	TCCR1B = (1 << ICES1) | (1 << CS10);  //Capture rising edge
	TIFR1 = (1 << ICF1) | (1 << TOV1);    //Clear input capture & overflow flags

	while (!(PIND & (1 << Echo_pin)) && timeoutLimit--);  //Wait for echo high
	if (!timeoutLimit) return -1;       //Timeout ? no echo

	TCNT1 = 0;
	TCCR1B = (1 << CS10);               //Switch to falling edge capture
	TIFR1 = (1 << ICF1) | (1 << TOV1);
	timerOverflow = 0;

	timeoutLimit = 30000;               //Reset timeout
	while ((PIND & (1 << Echo_pin)) && timeoutLimit--);  //Wait for echo low
	if (!timeoutLimit) return -1;       //Timeout ? stuck high

	tickCount = ICR1 + (65535L * timerOverflow);  //Compute total ticks
	distResult = (double)tickCount / (58.0 * 16.0);  //Convert to cm

	TCCR1B &= ~(1 << CS10);             //Stop timer
	return distResult;
}

// for 7 seg display
void updateDisplay() { //updates 7seg
	uint8_t digit = (displayVal / (int)pow(10, currentDigit)) % 10; //Extract digit
	SHIFT_PORT &= ~LATCH; //Latch LOW
	spi_send(SEGMENT_MAP[digit]); //Send digit segment
	spi_send(SEGMENT_SELECT[currentDigit]); //Select digit
	SHIFT_PORT |= LATCH; //Latch HIGH
	SHIFT_PORT &= ~LATCH; //Latch LOW
	currentDigit = (currentDigit + 1) % 4;  //Move to next digit
}

ISR(TIMER1_OVF_vect) {
	timerOverflow++;
}

int main() {
	
	// function declarations
	USART_init();
	usSetup();
	init_IO();
	init_SPI();


	DDRD |= (1 << PD0);      //PWM output for servo

	//timer3 for Fast PWM, prescaler 64
	TCCR3A |= (1 << COM3A1) | (1 << WGM31);
	TCCR3B |= (1 << WGM33) | (1 << WGM32) | (1 << CS31) | (1 << CS30);
	ICR3 = 4999;             //20ms PWM 
	OCR3A = MIN;             //moves servo to 0 deg
	_delay_ms(WAITDELAY);

	while (1) {
		char angleStr[20], distStr[20];
		double angleVal, distVal;
		int stepIdx = 0;
		int minRecorded = 1000;  //min distance

		// scan clockwise
		for (int pos = MIN; pos < MAX; pos += STEPSIZE) {
			OCR3A = pos;                      
			angleVal = stepIdx * ANGLE_STEP;  //compute angle
			dtostrf(angleVal, MINWIDTH, PRECISION, angleStr);
			USART_putstring(angleStr);
			USART_send(',');
			USART_send(' ');
			stepIdx++;

			distVal = measureDistance();      //measuring distance
			dtostrf(distVal, 4, 2, distStr);
			USART_putstring(distStr);
			USART_send('\n');

			// checks if distance is in range
			if (distVal > 2 && distVal < 400) {
				displayVal = (int)distVal;
				if (displayVal < minRecorded) minRecorded = displayVal;
				} else if (distVal == -1) {
				displayVal = 8888;             //Error code
				} else {
				displayVal = 0;                //Invalid reading
			}

			// refresh display during downtime
			for (uint16_t refresh = 0; refresh < (STEPDELAY / 2); refresh++) {
				updateDisplay();
				_delay_ms(2);
			}
		}

		_delay_ms(WAITDELAY);

		// cc scan
		for (int pos = MAX; pos > MIN; pos -= STEPSIZE) {
			OCR3A = pos;
			angleVal = stepIdx * ANGLE_STEP;
			dtostrf(angleVal, MINWIDTH, PRECISION, angleStr);
			USART_putstring(angleStr);
			USART_send(',');
			USART_send(' ');
			stepIdx--;

			distVal = measureDistance();
			dtostrf(distVal, 4, 2, distStr);
			USART_putstring(distStr);
			USART_send('\n');

			displayVal = minRecorded;   // min distance from last sweep

			// refresh display during down time
			for (uint16_t refresh = 0; refresh < (STEPDELAY / 2); refresh++) {
				updateDisplay();
				_delay_ms(2);
			}
		}

		_delay_ms(WAITDELAY);
	}
	return 0;
}

