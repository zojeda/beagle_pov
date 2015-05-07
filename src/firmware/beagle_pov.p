.origin 0

.entrypoint START

#include "pru.hp"

#define PIN_OUT r30.t7


#define T0H 40
#define T1H 80
#define T0L 85
#define T1L 45
#define T0Latch 5000

#define MAX_TAB 58

.macro Delay
  .mparam cycles //us*100

  MOV r0, cycles

  DELAY:
    SUB r0, r0, 1
    QBNE DELAY, r0, 0
.endm

.macro SendHI
  SET PIN_OUT
  Delay T1H
  CLR PIN_OUT
  Delay T1L
.endm

.macro SendLOW
  SET PIN_OUT
  Delay T0H
  CLR PIN_OUT
  Delay T0L
.endm

.macro Latch
  CLR PIN_OUT
  Delay T0Latch
.endm

  
.macro SendColor
  .mparam color=0x000000 //GBR color
	  MOV r1, 23
	  MOV r2, color
  DOUT:
	  QBBS TO_HI, r2, r1
	  SendLOW
	  JMP BACK
  TO_HI:
	  SendHI
  BACK:
	  QBEQ END, r1, 0
	  SUB r1, r1, 1
	  JMP DOUT

  END:
.endm

.macro ShowGBR
	.mparam position
        MOV r5, 0

TABBING:
	QBLE ENDTABBING, r5, position
	ADD r5, r5, 1
	SendColor 0x000000
	JMP TABBING
	

ENDTABBING:
	SendColor 0xFF0000
	SendColor 0x00FF00
	SendColor 0x0000FF

	ADD r5, r5, 3       //just send 3 colors
CLEARING:
	ADD r5, r5, 1
	SendColor 0x000000
	QBNE CLEARING, r5, MAX_TAB+3

	Latch
	Delay T0Latch*50

.endm

START:
        // Clear SYSCFG[STANDBY_INIT] to enable OCP master port:
        lbco r0, REG_SYSCFG, 4, 4  // These three instructions are required
        clr r0, r0, 4              // to initialize the PRU
        sbco r0, REG_SYSCFG, 4, 4  //


        MOV r3, 10
SEC:
        MOV r4, 0
ASC:
	ADD r4, r4, 1
	ShowGBR r4
        QBNE ASC, r4, MAX_TAB-1
DES:
	SUB r4, r4, 1
	ShowGBR r4
        QBNE DES, r4, 0

	
	SUB r3, r3, 1
        QBNE SEC, r3, 0

	
	

#ifdef AM33XX

    // Send notification to Host for program completion
    MOV R31.b0, PRU0_ARM_INTERRUPT+16

#else

    MOV R31.b0, PRU0_ARM_INTERRUPT

#endif

    HALT
