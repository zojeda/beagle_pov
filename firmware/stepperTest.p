.origin 0
.entrypoint START

#define PIN_OUT         r30.t5 //P9.27
#define STEPS_DELAY     0x00000000
#define STEPS_NUMBER    0x00000004 
#define STEP_DELAY      800000
#define PRU0_ARM_INTERRUPT 19
#define REG_SYSCFG      C4

.macro Delay
  .mparam cycles //us*100

  MOV r0, cycles

  DELAY:
    SUB r0, r0, 1
    QBNE DELAY, r0, 0
.endm

START:
        // Clear SYSCFG[STANDBY_INIT] to enable OCP master port:
        LBCO    r0, REG_SYSCFG, 4, 4  // These three instructions are required
        CLR     r0, r0, 4              // to initialize the PRU
        SBCO    r0, REG_SYSCFG, 4, 4  //

        MOV     r1, 0
        MOV     r0, STEPS_NUMBER
        LBBO    r1, r0, 0, 4
        MOV     r0, STEPS_DELAY
        LBBO    r2, r0, 0, 4
//            MOV     r1, 1
BLINK:
        SET     PIN_OUT
        Delay   r2      
        CLR     PIN_OUT
        Delay   r2      

        SUB     r1, r1, 1
        QBNE    BLINK, r1, 0
mov r31.b0, PRU0_ARM_INTERRUPT+16

HALT
