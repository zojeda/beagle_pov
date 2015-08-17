.origin 0

.entrypoint START

#include "pru.hp"

#define PIN_OUT r30.t5 //P(.27
#define IMG_WIDTH   60


#define T0H 40
#define T1H 80
#define T0L 85
#define T1L 45
#define T0Latch 5000


#define SHARED_RAM  0x100
#define C_SHARED_RAM  C28

#define PRU0_CTRL   0x22000
#define PRU1_CTRL   0x24000

#define CTPPR0      0x28


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


START:
    // Clear SYSCFG[STANDBY_INIT] to enable OCP master port:
    lbco r0, REG_SYSCFG, 4, 4  // These three instructions are required
    clr r0, r0, 4              // to initialize the PRU
    sbco r0, REG_SYSCFG, 4, 4  //
    
    //SETTING C28 entry to Shared RAM
    MOV     r0, SHARED_RAM
    MOV     r1, PRU0_CTRL + CTPPR0
    SBBO    r0, r1, 0, 4

BEGIN_LINE:
    MOV     r4, 0

PIXEL:
    LBCO    r3, C_SHARED_RAM, r4, 3
    SendColor r3
    ADD     r4, r4, 3
    QBNE    PIXEL, r4, IMG_WIDTH
    Latch
    JMP BEGIN_LINE

#ifdef AM33XX

    // Send notification to Host for program completion
    MOV R31.b0, PRU0_ARM_INTERRUPT+16

#else

    MOV R31.b0, PRU0_ARM_INTERRUPT

#endif

    HALT
