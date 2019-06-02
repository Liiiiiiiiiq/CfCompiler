DATAS SEGMENT
	__STR3 DB "HELLO",'$'
	__STR4 DB "TEST",'$'
	__STR2 DB "TEST1",'$'
	__STR1 DB "TEST2",'$'
	__STR0 DB "Test",'$'
	a DB 0
	b DB 5 DUP(0)
	c DW 0
	d DW 6 DUP(0)
	_is_num DB 0
	_show_string DW 0
	_show_num DB 0
	_C10 DB 10
	_is_enter DB 0
DATAS ENDS
CODES SEGMENT
	ASSUME CS:CODES,DS:DATAS
START:
	MOV AX,DATAS
	MOV DS, AX
	CALL gogo
	MOV _is_enter,1
	MOV _is_num,0
	LEA AX,__STR4
	MOV _show_string,AX
	CALL printf
	MOV AH,4CH
	INT 21H
gogo PROC
	MOV AL,3
	MOV a,AL
	MOV AL,3
	MOV [b+2],AL
	MOV AL,a
	MOV BX,0
	MOV BL,a
	MOV SI,BX
	MOV b[SI],AL
	MOV AL,a
	MOV [b+3],AL
	MOV AL,4
	MOV BX,0
	MOV BL,a
	MOV SI,BX
	MOV b[SI],AL
	LEA AX,__STR0
	MOV c,AX
	MOV AX,c
	MOV BX,0
	MOV BL,a
	MOV SI,BX
	ADD SI,SI
	MOV d[SI],AX
	LEA AX,__STR1
	MOV [d+10],AX
	LEA AX,__STR2
	MOV BX,0
	MOV BL,a
	MOV SI,BX
	ADD SI,SI
	MOV d[SI],AX
	LEA AX,__STR3
	MOV [d+0],AX
	MOV AX,[d+0]
	MOV BX,0
	MOV BL,a
	MOV SI,BX
	ADD SI,SI
	MOV d[SI],AX
	RET
gogo ENDP
printf PROC
	CMP _is_num, 0
	JZ _T3
	MOV AL, _show_num
	MOV CX, 3
_T1: 
	MOV AH, 0
	DIV _C10
	PUSH AX
	LOOP _T1
	MOV CX, 3
_T2 : 
	POP DX
	XCHG DH, DL
	OR DL, 30H
	MOV AH, 2
	INT 21H
	LOOP _T2
	JMP _T4
_T3:
	MOV DX, _show_string
	MOV AH, 9
	INT 21H
_T4 : 
	CMP _is_enter, 0
	JZ _T5
	MOV DL, 10
	MOV AH, 2
	INT 21H
_T5 : 
	ret
printf ENDP

CODES ENDS
	END START