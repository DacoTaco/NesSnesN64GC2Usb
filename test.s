	.file	"test.c"
__SREG__ = 0x3f
__SP_H__ = 0x3e
__SP_L__ = 0x3d
__CCP__  = 0x34
__tmp_reg__ = 0
__zero_reg__ = 1
	.text
.global	tetteuh
	.type	tetteuh, @function
tetteuh:
	push r29
	push r28
	push __tmp_reg__
	in r28,__SP_L__
	in r29,__SP_H__
/* prologue: function */
/* frame size = 1 */
	std Y+1,r24
	ldd r24,Y+1
	cpi r24,lo8(1)
	brne .L2
/* #APP */
 ;  7 "test.c" 1
	nop			

 ;  0 "" 2
/* #NOAPP */
	rjmp .L4
.L2:
	ldd r24,Y+1
/* #APP */
 ;  13 "test.c" 1
	nop			
nop			
cpse r24,1
rjmp .+2
cbi 0x18,4
rjmp .+1
cbi 0x18,5

 ;  0 "" 2
/* #NOAPP */
.L4:
/* epilogue start */
	pop __tmp_reg__
	pop r28
	pop r29
	ret
	.size	tetteuh, .-tetteuh
