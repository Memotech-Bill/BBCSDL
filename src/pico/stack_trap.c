/* stack_trap.c - Catch HardFault generated by SP in protected region

Registers are saved in the report in the following order:

    report[0]   = SP
    report[1]   = R12   - From exception frame
    report[2]   = LR
    report[3]   = PC
    report[4]   = PSP
    report[5]   = R0
    report[6]   = R1
    report[7]   = R2
    report[8]   = R3
    report[9]   = R0    - Saved in exception handler
    report[10]  = R1
    report[11]  = R2
    report[12]  = R3
    report{13]  = R12
    report[14]  = LR
    report[15]  = R8
    report[16]  = R9
    report[17]  = R10
    report[18]  = R11
    report[19]  = R4
    report[20]  = R5
    report[21]  = R6
    report[22]  = R7

crash_report() is executed with the stack pointer just BELOW the memory guard region
so they should use minimum stack. It is not until after longjmp() in error () that
the stack pointer is reset above the guard region.
*/

#include <stdint.h>
#include <stdbool.h>

extern uintptr_t stk_guard;
void text (const char *psTxt);
void error (int err, const char *msg);
void install_stack_guard (void *stack_bottom);

static void hex_report (int iVal)
    {
    char sHex[9];
    sHex[8] = '\0';
    for (int i = 7; i >= 0; --i)
        {
        char c = iVal & 0x0F;
        if ( c >= 10 ) c += 'A' - 10;
        else c += '0';
        sHex[i] = c;
        iVal >>= 4;
        }
    text (sHex);
    }

void crash_report (int *pReport)
    {
    // uintptr_t save_guard = stk_guard;
    // install_stack_guard ((void *) 0);
    bool bHaveFrame = ( pReport[5] == pReport[9] ) && ( pReport[6] == pReport[10] )
        && ( pReport[7] == pReport[11] ) && ( pReport[8] == pReport[12] );
    text ("\r\nR0 = ");
    hex_report (pReport[9]);
    text ("  R8  = ");
    hex_report (pReport[15]);
    text ("\r\nR1 = ");
    hex_report (pReport[10]);
    text ("  R9  = ");
    hex_report (pReport[16]);
    text ("\r\nR2 = ");
    hex_report (pReport[11]);
    text ("  R10 = ");
    hex_report (pReport[17]);
    text ("\r\nR3 = ");
    hex_report (pReport[12]);
    text ("  R11 = ");
    hex_report (pReport[18]);
    text ("\r\nR4 = ");
    hex_report (pReport[19]);
    text ("  R12 = ");
    hex_report (pReport[13]);
    text ("\r\nR5 = ");
    hex_report (pReport[20]);
    text ("  SP  = ");
    hex_report (pReport[0]);
    text ("\r\nR6 = ");
    hex_report (pReport[21]);
    text ("  LR  = ");
    if ( bHaveFrame ) hex_report (pReport[2]);
    else hex_report (pReport[14]);
    text ("\r\nR7 = ");
    hex_report (pReport[22]);
    text ("  PC  = ");
    if ( bHaveFrame ) hex_report (pReport[3]);
    else text ("????????");
    text ("\r\nSG = ");
    hex_report (stk_guard);
    text ("  PSP = ");
    if ( bHaveFrame ) hex_report (pReport[4]);
    else text ("????????");
    // if (save_guard != 0) install_stack_guard ((void *)(save_guard - 95u));
    if (( pReport[0] >= stk_guard ) && ( pReport[0] < stk_guard + 0x200 ))
        {
        error (255, "\r\nStack overrun");
        }
    else
        {
        error (255, "\r\nHard fault");
        }
    }

void __attribute__((used,naked)) stack_trap(void)
    {
	asm volatile(
        "   push    {r0-r3}         \n\t"   // Save R0-R3 on original stack
#if PICO == 2
        "   mov     r0, #0          \n\t"   // Reset stack limit
        "   msr     msplim, r0      \n\t"
#endif
        "   mov     r0, sp          \n\t"   // Get stack pointer
        "   adr     r1, 2f          \n\t"   // Address of data block
        "   ldm     r1!, {r2}       \n\t"   // Load address of stk_guard
#if PICO == 1
        "   ldr     r2, [r2]        \n\t"   // Load value of stack_guard
        "   cmp     r2, #0          \n\t"   // Test for stack_guard set
        "   beq     1f              \n\t"   // Jump if not set
        "   mov     sp, r2          \n\t"   // Set stack pointer to bottom of guard
#endif
        "1: push    {r4-r7}         \n\t"   // Save R4-R7
        "   mov     r4, r8          \n\t"   // Copy R8-R11 to R4-R7
        "   mov     r5, r9          \n\t"
        "   mov     r6, r10         \n\t"
        "   mov     r7, r11         \n\t"
        "   push    {r4-r7}         \n\t"   // Save R8-R11
        "   mov     r4, r12         \n\t"   // Copy R12 to R4
        "   mov     r5, lr          \n\t"   // Copy LR to R5
        "   push    {r4-r5}         \n\t"   // Save them
        "   ldm     r0!, {r4-r7}    \n\t"   // Retrieve R0-R3 from original stack
        "   push    {r4-r7}         \n\t"   // Save them on the new stack
        "   ldm     r0!, {r4-r7}    \n\t"   // Retrieve R0-R3 from exception frame
        "   push    {r4-r7}         \n\t"   // Save them on the new stack
        "   ldm     r0!, {r4-r7}    \n\t"   // Retrieve R12, LR, PC & PSP from exception frame
        "   push    {r0,r4-r7}      \n\t"   // Save Original SP, R12, LR, PC & PSP to stack
        "   mov     r0, sp          \n\t"   // R0 = Location of saved data
        "   ldm     r1!, {r2-r4}    \n\t"   // Load next three words from data block
        "   mov     r5, #0          \n\t"   // Create dummy exception frame
        "   push    {r2-r3}         \n\t"   // PC = stack_report, PSP = Thread mode, Thumb
        "   push    {r5}            \n\t"   // LR = 0
        "   push    {r5}            \n\t"   // R12 = 0
        "   push    {r5}            \n\t"   // R3 = 0
        "   push    {r5}            \n\t"   // R2 = 0
        "   push    {r5}            \n\t"   // R1 = 0
        "   push    {r0}            \n\t"   // R0 = 0
        "   bx      r4              \n\t"   // Exception return: Thread mode, main stack
        "   .align  2               \n\t"
        "2: .word   stk_guard       \n\t"   // Address of stack_guard
        "   .word   crash_report    \n\t"   // Address of crash_report routine
        "   .word   0x1000000       \n\t"   // PSP = Thread mode, Thumb
        "   .word   0xFFFFFFF9      \n\t"   // Exception return: Thread mode, main stack
        );
    }
    
#if PICO == 2
// The definition "xeq=pico_xeq" is forced into the compilation of bbmain.c
// so that the xeq() call in basic() calls here instead.

#include <stddef.h>
#include "BBC.h"

VAR xeq (void);

VAR pico_xeq (void)
    {
    if (stk_guard != 0) install_stack_guard ((void *)(stk_guard - 95u));
    return xeq ();
    }
#endif
