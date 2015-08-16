.origin 0
.entrypoint START

#define GPIO1 0x4804c000
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_SETDATAOUT 0x194
#define PRU0_ARM_INTERRUPT 19
#define REG_SYSCFG         C4

START:
        // Clear SYSCFG[STANDBY_INIT] to enable OCP master port:
        LBCO r0, REG_SYSCFG, 4, 4  // These three instructions are required
        CLR r0, r0, 4              // to initialize the PRU
        SBCO r0, REG_SYSCFG, 4, 4  //

        MOV r1, 100
BLINK:
        MOV r2, 1<<24
        MOV r3, GPIO1 | GPIO_SETDATAOUT
        SBBO r2, r3, 0, 4
        MOV r0, 0x00a00002
DELAY:
        SUB r0, r0, 1
        QBNE DELAY, r0, 0
        MOV r2, 1<<24
        MOV r3, GPIO1 | GPIO_CLEARDATAOUT
        SBBO r2, r3, 0, 4
        MOV r0, 0x00a00002
DELAY2:
        SUB r0, r0, 1
        QBNE DELAY2, r0, 0
        SUB r1, r1, 1
        QBNE BLINK, r1, 0
mov r31.b0, PRU0_ARM_INTERRUPT+16

HALT
