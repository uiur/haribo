[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]

[FILE "naskfunc.nas"]
  GLOBAL _io_hlt, _io_cli, _io_sti, _io_stihlt, _io_in8, _io_in16, _io_in32, _io_out8, _io_out16, _io_out32, _io_load_eflags, _io_store_eflags, _load_idtr, _load_gdtr

[SECTION .text]

_io_hlt:
  HLT
  RET

_io_cli:
  CLI
  RET

_io_sti:
  STI
  RET

_io_stihlt:
  STI
  HLT
  RET

_io_in8:
  MOV EDX,[ESP+4]
  MOV EAX,0
  IN  AL,DX
  RET

_io_in16:
  MOV EDX,[ESP+4]
  MOV EAX,0
  IN  AX,DX
  RET

_io_in32:
  MOV EDX,[ESP+4]
  IN  EAX,DX
  RET

_io_out8:
  MOV EDX,[ESP+4]
  MOV AL,[ESP+8]
  OUT DX,AL
  RET

_io_out16:
  MOV EDX,[ESP+4]
  MOV EAX,[ESP+8]
  OUT DX,AX
  RET

_io_out32:
  MOV EDX,[ESP+4]
  MOV EAX,[ESP+8]
  OUT DX,EAX
  RET

_io_load_eflags:
  PUSHFD
  POP EAX
  RET

_io_store_eflags:
  MOV EAX,[ESP+4]
  PUSH EAX
  POPFD
  RET

_load_gdtr:		; void load_gdtr(int limit, int addr);
	MOV		AX,[ESP+4]		; limit
	MOV		[ESP+6],AX
	LGDT	[ESP+6]
	RET

_load_idtr:		; void load_idtr(int limit, int addr);
	MOV		AX,[ESP+4]		; limit
	MOV		[ESP+6],AX
	LIDT	[ESP+6]
	RET
