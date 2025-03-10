;
; Task1 and 2.asm
;
; Created: 3/9/2025 8:07:25 PM
; Author : Minnie
;


; Replace with your application code
.org 0x00
	LDI R16, 0xFF 
	OUT DDRB, R16 
	OUT PORTB, R16 
L1:
	SBIC PINC, 1;
	RJMP LOOP
	CBI PORTB, 5
	LDI R16, 10
	CALL delaySubr
	SBI PORTB, 5
	RJMP L1
	
delaySubr:
	LDI R17, 13
	LDI R19, 216
L2:
	DEC R19
	BRNE L2
	DEC R18
	BRNE L2
	DEC R17
	BRNE L2
	DEC R16
	BRNE delaySubr
	RET