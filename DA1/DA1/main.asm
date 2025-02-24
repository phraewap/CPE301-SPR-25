;
; DA1.asm
;
;
; Author : Phraewa Pookprajob
;


; Replace with your application code
.ORG 0
	; Initialize the stack pointer
	LDI R20,HIGH(RAMEND)
	OUT SPH,R20
	LDI R20,LOW(RAMEND)
	OUT SPL,R20
	; Initialize X-pointer for EEPROM Key retrieval
	LDI XH,HIGH(0xA00)
	LDI XL,LOW(0xA00)

	; Initialize the Y-pointer for Decrypted Message starting @ 0x200
	LDI YH,HIGH(0x200)
	LDI YL,LOW(0x200)

	; Initialize the Z-pointer for Raw message starting @ 0x400
	LDI ZH,HIGH(KEY<<1)
	LDI ZL,LOW(KEY<<1)

	LDI R16, 16
L1:	
	LPM R20,Z+
	CPI R20, 0xFF			; compare R20 w/ 0xFF
	BREQ EXITS_L1			; breaks out of loop
	CALL STORE_IN_EEPROM	
	INC XL					; inc next EEPROM 
	RJMP L1

EXITS_L1:
	RCALL LOAD_KEY

    LDI ZL, LOW(MESSAGE<<1)
    LDI ZH, HIGH(MESSAGE<<1)

    RCALL TEA_EN

    LDI YL, LOW(0x200)
    LDI YH, HIGH(0x200)
    RCALL STORE_DATA

LOAD_KEY:
	; loads from EEPROM
    RCALL LOAD_FROM_EEPROM
    MOV R20, R20        ; R20 = k[0]
    INC XL
    RCALL LOAD_FROM_EEPROM
    MOV R21, R20        ; R21 = k[1]
    INC XL
    RCALL LOAD_FROM_EEPROM
    MOV R22, R20        ; R22 = k[2]
    INC XL
    RCALL LOAD_FROM_EEPROM
    MOV R23, R20        ; R23 = k[3]
    RET

TEA_EN:
	; loads bytes of DELTA

	;LDI R24, 0

	LDI R25, 0xB9			; loads first lowest byte
	LDI R26, 0x79			; loads second lowest byte
	LDI R27, 0x37			; loads second highest byte
	LDI R29, 0x9E			; loads first highest byte

	LDI R16, 32				

TEA_LOOP:

	; y += (z << 4) + k[0]) ^ (z + sum) ^ ((z >> 5) + k[1])
	MOV R25, R19				; R25 = z

	LSL R25					; z << 4
	LSL R25	
	LSL R25
	LSL R25
	ADD R25, R20				; (z << 4) + k[0]
	MOV R3, R19				; R3 = Z
	ADD R3, R24				; R3 = z + sum
	EOR R25, R3				; (z << 4) + k[0]) ^ (z + sum)
	MOV R3, R19				; R3 = z
	ROR R3					; z >> 5
	ROR R3
	ROR R3
	ROR R3
	ADD R3, R21
	EOR R25, R3
	ADD R18, R25

	; z += (y << 4) + k[2]) ^ (y + sum) ^ ((y >> 5) + k[3])
	MOV R25, R18			; R25 = y
	LSL R25
	LSL R25
	LSL R25
	LSL R25
	ADD R25, R22
	MOV R3, R18
	ADD R3, R24
	EOR R25, R3
	MOV R3, R18
	ROR R3
	ROR R3
	ROR R3
	ROR R3
	ADD R3, R29
	EOR R25, R3
	ADD R19, R25
	
	ADD R24, R25
	ADC R24, R26
	ADD R24, R27
	ADD R24, R28

	DEC R16
	BRNE TEA_LOOP
	RET

STORE_DATA:
	LPM R18, Z+
	LPM R19, Z+
	ST X+, R18				; store z in SRAM
	ST X+, R19
	CPI R19, 0xFF
	BRNE STORE_DATA
	RET 

LOAD_FROM_EEPROM:
	SBIC EECR, EEPE
	RJMP LOAD_FROM_EEPROM
	OUT EEARH,XH
	OUT EEARL,XL
	SBI EECR,EERE			; reads EEPROM
	IN R20,EEDR				; load into R20
	RET

STORE_IN_EEPROM:
	SBIC EECR, EEPE
	RJMP STORE_IN_EEPROM
	OUT EEARH,XH
	OUT EEARL,XL
	OUT EEDR,R20
	SBI EECR,EEMPE
	SBI EECR,EEPE
	RET

; Message: "jiLSTLNdaRrxrmiElGjSeiZBNSIrXEOInKAljICoLQvnCSTuTqApIrpqhyjBNAYy"

.ORG 0x200
MESSAGE: .DB 0x6a, 0x69, 0x4c, 0x53, 0x54, 0x4c, 0x4e, 0x64, 0x61, 0x52, 0x72, 0x78, 0x72, 0x6d, 0x69, 0x45
;.DB 6c, 47, 6a, 53, 65, 69, 5a, 42, 4e, 53, 49, 72, 58, 45, 4f, 49
;.DB 6e, 4b, 41, 6c, 6a, 49, 43, 6f, 4c, 51, 76, 6e, 43, 53, 54, 75
;.DB 54, 71, 41, 70, 49, 72, 70, 71, 68, 79, 6a, 42, 4e, 41, 59, 79


; KEY: "jiLSTLNdaRrxrmiEÿÿ"
.ORG 0x300
KEY:.DB 0x59, 0x4b, 0x54, 0x46, 0x67, 0x57, 0x6e, 0x76, 0x61, 0x6c, 0x6f, 0x42, 0x66, 0x6c, 0x72, 0x72

.ESEG
.ORG 0x00
EKEY:.DB 0x59, 0x4b, 0x54, 0x46, 0x67, 0x57, 0x6e, 0x76, 0x61, 0x6c, 0x6f, 0x42, 0x66, 0x6c, 0x72, 0x72


.ORG 0x310
DELTA: .DB 0x9E, 0x37, 0x79, 0xB9