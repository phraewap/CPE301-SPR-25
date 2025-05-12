/*
 * DA7.c
 *
 * Created: 5/10/2025 8:39:13 PM
 * Author : Minnie
 */ 

#define F_CPU 16000000 

#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>

#define USART_BAUDRATE 9600
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)
#define SCL_CLK 100000L
#define BMI160_ADDR       0x69
#define BMI160_CHIP_ID    0x00
#define BMI160_CMD_REG    0x7E
#define BMI160_ACC_DATA   0x12
#define BMI160_GYR_DATA   0x0C

#include "SSD1306.h"
#include "i2c.h"

#define ACCELEROMETER_SENSITIVITY 16384.0
#define GYROSCOPE_SENSITIVITY 16.4
#define dt 0.01  //10ms sample rate

float Acc_x,Acc_y,Acc_z,Gyro_x,Gyro_y,Gyro_z;
int16_t Xa, Ya, Za, Xg, Yg, Zg;
float FXa, FYa, FZa, FXg, FYg, FZg;
float roll, yaw, pitch;

void UART_init( void )
{
UBRR0H = (uint8_t)(UBRR_VALUE >> 8); //Enable baud rate to 9600
UBRR0L = (uint8_t)UBRR_VALUE; //Enable baud rate to 9600
UCSR0C = (1 << UCSZ01) |  (1 << UCSZ00); //Data is 8 bit size, and there is 1 stop bit 
UCSR0B = (1 << TXEN0); //Enable transmitter
}


//This function transmits the data, each char is entered into UDR0 register and tramsitted to the /* serial monitor
void UART_Transmit(char data){
    while (!(UCSR0A & (1 << UDRE0)));  // Cycles through each char until UDRE0 is 1
    	UDR0 = data;
}

//Sends data to UART_Transmit
void USART_tx_string(const char *data) {
    while (*data != '\0') { //Cycles through each char in data
        UART_Transmit(*data); //Sends
        data++;
    }
}


void i2c_init() {
	TWSR0 = 0x00;
	TWBR0 = 0x48;
	TWCR0 = (1 << TWEN);
}

void i2c_start() {
	TWCR0 = (1<<TWSTA)|(1<<TWEN)|(1<<TWINT);
	while (!(TWCR0 & (1<<TWINT)));
}

void i2c_stop() {
	TWCR0 = (1<<TWSTO)|(1<<TWEN)|(1<<TWINT);
}

void i2c_write(uint8_t data) {
	TWDR0 = data;
	TWCR0 = (1<<TWEN)|(1<<TWINT);
	while (!(TWCR0 & (1<<TWINT)));
}

uint8_t i2c_read_ack() {
	TWCR0 = (1<<TWEN)|(1<<TWINT)|(1<<TWEA);
	while (!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

uint8_t i2c_read_nack() {
	TWCR0 = (1<<TWEN)|(1<<TWINT);
	while (!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

void bmi160_write(uint8_t reg, uint8_t data) {
	i2c_start();
	i2c_write((BMI160_ADDR << 1) | 0);
	i2c_write(reg);
	i2c_write(data);
	i2c_stop();
}

void bmi160_read_bytes(uint8_t reg, uint8_t *buf, uint8_t len) {
	i2c_start();
	i2c_write((BMI160_ADDR << 1) | 0);
	i2c_write(reg);
	_delay_us(10);
	i2c_start();
	i2c_write((BMI160_ADDR << 1) | 1);
	for (uint8_t i = 0; i < len; i++) {
		buf[i] = (i == len - 1) ? i2c_read_nack() : i2c_read_ack();
	}
	i2c_stop();
}

void bmi160_init() {
	// Check Chip ID
	i2c_start();
	i2c_write((BMI160_ADDR << 1) | 0);
	i2c_write(BMI160_CHIP_ID);
	i2c_start();
	i2c_write((BMI160_ADDR << 1) | 1);
	uint8_t id = i2c_read_nack();
	i2c_stop();
	if (id != 0xD1) return;

	_delay_ms(100);

	// Set accelerometer and gyroscope to normal mode
	bmi160_write(BMI160_CMD_REG, 0x11); // Accel Normal
	_delay_ms(50);
	bmi160_write(BMI160_CMD_REG, 0x15); // Gyro Normal
	_delay_ms(50);
}



void bmi160_read_accel(int16_t *x, int16_t *y, int16_t *z) {
	uint8_t data[6];
	bmi160_read_bytes(BMI160_ACC_DATA, data, 6);
	*x = (int16_t)((data[1] << 8) | data[0]);
	*y = (int16_t)((data[3] << 8) | data[2]);
	*z = (int16_t)((data[5] << 8) | data[4]);
}

void bmi160_read_gyro(int16_t *x, int16_t *y, int16_t *z) {
	uint8_t data[6];
	bmi160_read_bytes(BMI160_GYR_DATA, data, 6);
	*x = (int16_t)((data[1] << 8) | data[0]);
	*y = (int16_t)((data[3] << 8) | data[2]);
	*z = (int16_t)((data[5] << 8) | data[4]);
}


void ComplementaryFilter()
{
	float pitchAcc, rollAcc;
	// Integrate the gyroscope data -> int(angularSpeed) = angle
	pitch += ((float)Xg / GYROSCOPE_SENSITIVITY) * dt; // Angle around the X-axis
	roll -= ((float)Yg / GYROSCOPE_SENSITIVITY) * dt; // Angle around the Y-axis
	yaw += ((float)Zg / GYROSCOPE_SENSITIVITY) * dt;
	// Compensate for drift with accelerometer data if !bullshit
	// Sensitivity = -2 to 2 G at 16Bit -> 2G = 16384.0 && 2000dps = 16.4
	int forceMagnitudeApprox = abs(Xa) + abs(Ya) + abs(Za);
	if (forceMagnitudeApprox > 8192 && forceMagnitudeApprox < 32768)
	{
		// Turning around the X axis results in a vector on the Y-axis
		pitchAcc = atan2f((float)Ya, (float)Za) * 180 / M_PI;
		pitch = pitch * 0.98 + pitchAcc * 0.02;
		// Turning around the Y axis results in a vector on the X-axis
		rollAcc = atan2f((float)Xa, (float)Za) * 180 / M_PI;
		roll = roll * 0.98 + rollAcc * 0.02;
	}
}


int main(void) {
	i2c_init();
	UART_init();
	_delay_ms(1000);
	bmi160_init();
	USART_tx_string("BMI160 initialized successfully.");
	USART_tx_string("\n");
	USART_tx_string("\r");
	
	OLED_Init();
	OLED_Clear();

	char buffer[64];

	while (1) {
		
		bmi160_read_accel(&Xa, &Ya, &Za);
		bmi160_read_gyro(&Xg, &Yg, &Zg);

		FXa = Xa/16384.0;
		FYa = Ya/16384.0;
		FZa = Za/16384.0;
		
		FXg = Xg/16.4;
		FYg = Yg/16.4;
		FZg = Zg/16.4;
		
	    snprintf(buffer, sizeof(buffer), "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f", FXa, FYa, FZa, FXg, FYg, FZg);
		
		USART_tx_string(buffer);
		USART_tx_string("\n");
		USART_tx_string("\r");
		OLED_SetCursor(0, 0);
		OLED_Printf(buffer);
		_delay_ms(500);
		
		OLED_Clear();
		ComplementaryFilter();
		
		snprintf(buffer, sizeof(buffer), "%.2f,%.2f,%.2f", pitch, roll, yaw);
		USART_tx_string(buffer);
		USART_tx_string("\n");
		USART_tx_string("\r");
		OLED_SetCursor(0, 0);
		OLED_Printf(buffer);
		_delay_ms(500);
		OLED_Clear();
	}
}