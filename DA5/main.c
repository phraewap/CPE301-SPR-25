/*
 * DA5.c
 *
 * Created: 5/10/2025 12:11:37 AM
 * Author : Minnie
 */ 

#define F_CPU 16000000

#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>

#define BAUDRATE 57600
#define UBRR_VALUE (((F_CPU / (BAUDRATE * 16UL))) - 1)
#define ADCIndex 16

uint16_t adc_value;
uint8_t speedAfter = 0;
volatile uint8_t motorSpeed;
volatile uint16_t tick_count = 0;
volatile uint16_t tps = 0; // ticks per second
char dataConversion[ADCIndex];
char buff[6];
double rpm;

void UART_init( void )
{
	UBRR0H = (uint8_t)(UBRR_VALUE >> 8); //Enable baud rate to 57600
	UBRR0L = (uint8_t)UBRR_VALUE; //Enable baud rate to 57600
	UCSR0C = (1 << UCSZ01) |  (1 << UCSZ00); //Data is 8 bit size, and there is 1 stop bit
	UCSR0B = (1 << TXEN0) | (1 << RXEN0); //Enable transmitter and receiver
}


//This function transmits the data, each char is entered into UDR0 register and transmitted to the serial monitor
void UART_Transmit(char data){
	while (!(UCSR0A & (1 << UDRE0)));  // Cycles through each char until UDRE0 is 1
	UDR0 = data;
}

//Sends data to UART_Transmit
void USART_text_to_string(char *data) {
	while (*data != '\0') { //Cycles through each char in data
		UART_Transmit(*data); //Sends
		data++;
	}
	// UART_Transmit('\n');  //Print a new line for formatting
}

void ADC_init(void){
	ADCSRA |= ((1 << ADPS2) | (1 << ADPS1));	// Prescalar = 64
	ADMUX |= (1<<REFS0);	// Voltage reference from Avcc (5v)
	ADCSRA |= (1<<ADEN);	// Turn on ADC
}

void initT1(void){
	DDRB &= ~(1 << DDB0); 	// Set PB0 as input
	TCCR1A = 0; 	// Normal
	TCCR1B = (1 << ICES1) | (1 << CS10); // No prescaler, sample rising edge
	TIMSK1 = (1 << ICIE1); // Enable Input Capture interrupt
	TCNT1 = 0; // Initialize Timer1 counter
	sei(); // Enable global interrupts
}

//Creates a 1 sec delay
void initT3(void){
	// Set CTC mode (WGM32 = 1, WGM33 = 0)
	TCCR3A = 0;				// Normal port operation, OC3A disconnected
	TCCR3B = (1 << WGM32);	// CTC mode
	TCCR3B |= (1 << CS32) | (1 << CS30);  // Prescaler = 1024

	// Calculate compare match value for 1 second
	OCR3A = 15624;	// Compare match after 1 second

	// Enable Timer3 Compare Match A interrupt
	TIMSK3 |= (1 << OCIE3A);
}


void initT0(void){
	DDRD |= (1 << PD6);
	TCNT0 = 0;  //Set timer0 count zero
	TCCR0A |= (1 << WGM00) | (1 << WGM01) | (1 << COM0A1); //Enables fast PWM, non inverting
	TCCR0B = (1 << CS01); //Set Fast PWM with Fosc/64 Timer0 clock
}

// function to receive the information from the user
unsigned char UART_Receive(void){
	while(!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

// get line function to grab the necessary elements to update the motor speed
void UART_getLine(char* buf, uint8_t n){
	uint8_t bufIdx = 0;
	char c;
	
	do{
		c = UART_Receive();
		buf[bufIdx++] = c;
	}
	while((bufIdx < n) && (c != '\r'));
	
	buf[bufIdx] = 0;
}

void updateTicks(void) {
	tps = tick_count;
	tick_count = 0;
}

uint16_t read_adc(uint8_t channel){
	ADCSRA |= (1 << ADSC);

	while (ADCSRA & (1 << ADSC));
	return ADC;
}

ISR(TIMER1_CAPT_vect) {
	// Each time an edge is captured, increment the count
	tick_count++;
}


ISR(TIMER3_COMPA_vect) {
	char print[10]; // new holder for the rpm when not being updated with getline
	updateTicks(); // updating the tick count
	USART_text_to_string("RPM: ");
	double rpm = tps / 2.33; // calculating the rpm
	
	itoa(rpm, print, 10); 	//Convert to an ascii string
	USART_text_to_string(print); 	// print in terminal
	UART_Transmit('\n');
}

void initT4(void) {
	// Set CTC mode (WGM42 = 1, WGM43 = 0)
	TCCR4A = 0;                    // Normal port operation
	TCCR4B = (1 << WGM42);         // CTC mode, TOP = OCR4A
	TCCR4B |= (1 << CS41) | (1 << CS40);  // Prescaler = 64
	OCR4A = 12499;                 // Compare match value for 50ms
	// Enable Timer4 Compare Match A interrupt
	TIMSK4 |= (1 << OCIE4A);
}


ISR(TIMER4_COMPA_vect) {
	if(speedAfter == 0) {
		adc_value = read_adc(0);
		uint8_t pwm = (adc_value * 255UL) / 1023;

		// Set motor speed
		OCR0A = pwm;
	}
}


int main(void)
{
	UART_init(); // initialize the UART
	ADC_init(); // initialize the ADC
	initT0(); // initialize timer0 for PWM
	initT1(); // initialize timer1 for ICP capture
	initT3();  // initialize timer3 for 1 s clock
	initT4();
	DDRD |= (1 << PD4) | (1 << PD5); // set pd4 and pd5 as inputs

	PORTD |= (1 << PD4); // AIN1 set
	PORTD &= ~(1 << PD5); // AIN2 set
	sei();
	
	while (1) // infinite loop to take in a user input and print RPM
	{
		UART_getLine(buff, 5); // waiting for user input
		motorSpeed = atoi(buff); // storing the user input
		if (motorSpeed >= 0 && motorSpeed <= 130){
			speedAfter = motorSpeed;	// overwriting the old speed
			OCR0A = 2.045 * speedAfter; // number adjusted to match the speed of the motor
		}
	}

}


