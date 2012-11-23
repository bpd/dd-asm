; 
; Intel Syntax:
; mov dst src
;
; 
;

 idle:
 nop

.org x10
 ; M[A] <- B
 mov r0 A
 mov r1 B
 mov [r0] r1
 jmp idle

.org x20
 ; M[A] <- M[B]
 mov r0 A
 mov r1 B
 mov r1 [r1]
 mov [r0] r1
 jmp idle

.org x30
 ; M[A] <- M[B] + M[C]
 mov r0 A
 mov r1 B
 mov r2 C
 mov r1 [r1]
 mov r2 [r2]
 add r1 r1 r2
 mov [r0] r1
 jmp idle

.org x40
 ; M[A] <- M[B] - M[C]
 mov r0 A
 mov r1 B
 mov r2 C
 mov r1 [r1]
 mov r2 [r2]
 sub r1 r1 r2
 mov [r0] r1
 jmp idle

.org x50
 ; M[A] <- M[B] * M[C]
 mov r0 A
 mov r1 B
 mov r2 C
 mov r1 [r1]
 mov r2 [r2]
 mul r1 r1 r2
 mov [r0] r1
 jmp idle

.org x60
 ; M[A] <- M[B] >> 1
 mov r0 A
 mov r1 B
 mov r1 [r1]
 rsh r1 r1
 mov [r0] r1
 jmp idle

.org x70
 ; M[A] <- NOT(M[B])
 mov r0 A
 mov r1 B
 mov r1 [r1]
 not r1 r1
 mov [r0] r1
 jmp idle

.org x80
 ; M[A] <- M[B] AND M[C]
 mov r0 A
 mov r1 B
 mov r2 C
 mov r1 [r1]
 mov r2 [r2]
 and r1 r1 r2
 mov [r0] r1
 jmp idle

.org x90
 ; IF(M[C]=0) THEN M[A] <- M[B]
 mov r2 C
 mov r2 [r2]
 mov r2 r2
 jmppn idle ; if not zero, we're done
 mov r0 A
 mov r1 B
 mov r1 [r1]
 mov [r0] r1
 jmp idle

.org xA0
 ; IF(M[C] = M[B]) THEN (M[A] <- 0) ELSE (M[A] <- 1)
 mov r0 A
 mov r1 B
 mov r2 C
 mov r1 [r1]
 mov r2 [r2]
 ; 010   negate one, then and... if they're equal, the result will be zero
 ; 101 
 
 not r1 r1
 and r1 r1 r2
 jmppn nomatch
 ; M[C] = M[B], so do M[A] <- 0
 mov [r0] 0
 jmp idle
 
 nomatch:
 ; M[C] != M[B], so do M[A] <- 1
 mov r1 1 ; we can only load a constant 0, so transfer 1 through r1
 mov [r0] r1
 jmp idle
 
.org xB0
 mov r0 0
 mov r1 1

 mov r0 r0
 jmpp positive
 mov [r0] r0
 jmp idle
 
 positive:
 mov [r0] r1
 jmp idle


