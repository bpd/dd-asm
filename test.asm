; 
; Intel Syntax:
; mov dst src
;
; 
;

 idle:
 nop

.org x10

 jmp test
 jmpp myjmp
 jmpz test
 jmpn test
 jmppz idle
 jmppz test
 jmpz test
 jmp x01

.org x20
 and r1 r2 r3
 or r1 r2 r3
 not r4 r5
 rsh r5 r6
 nadd r6 r7
 lsh r7 r0
 sar r2 r3

.org x90

 test:
 mov r0 [r1]   ; a comment
 mov [r0] r1
 
 mov r1 A
 mov [r2] r1
 
 mov r1 B
 mov [r2] r1
 
 mov r3 C
 
 ; move the constant in operand C of the instruction
 ; to the memory location pointed to by r3
 mov [r3] C
 
 mov r4 0

.org x30
 mov r0 [r1]
 add r0 r1 r2
 sub r2 r3 r4
 
 myjmp:
 mul r3 r4 r5
 div r5 r6 r7


 


