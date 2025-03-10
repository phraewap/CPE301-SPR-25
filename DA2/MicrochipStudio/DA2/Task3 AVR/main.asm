;
; Task3 AVR.asm
;
; Created: 3/9/2025 9:12:21 PM
; Author : Minnie
;




.ORG 0  // reset (start of program)
    RJMP MAIN
.ORG  0x02  // external interrupt 0
    RJMP ISR

MAIN:
    // set LED
    SBI        DDRB, 5       // PB5 as output (LED)
    SBI        PORTB, 5		 // turns off LED (active low)

    CBI        DDRD, 2      // PD2 as input (switch)  
    SBI        PORTD, 2		// enables pulls at PD2

    SBI        EIMSK, INT0  // enables INT0
    LDI        R20, (1 << ISC01) | (1 << ISC00) // INT0 triggers at rising edge
    STS        EICRA, R20	// global interrupt
    SEI                    

L1:
    SBI     PORTB, 5    // turn off LED
    RJMP    L1			// keeps LED off

ISR:

    CBI        PORTB, 5   // turn on LED 
    LDI        R16, 10    // load 10 
    CALL	   delaySubr
    SBI        PORTB, 5   // turn off LED
    
    LDI        R20, 1 << INTF0  // load R20 w/ INT0  
    STS        EIFR, R20		// clear INT0

    RETI                

delaySubr:
    LDI        R17, 13		
    LDI        R18, 45	
    LDI        R19, 216
L2: 
    DEC     R19	//decrement
    BRNE    L2	// branch R19 when R19 isnt 0
	// same logic throughout
    DEC     R18 
    BRNE    L2	
    DEC     R17
    BRNE    L2

    DEC     R16
    BRNE    delaySubr // branch to function if R16 isn't 0
    RET