# Optionally inserted by the linker at the end of the program:
# - calls the 'main' function
# - delegates exception handling to the emulator

.kdata
store: .space 8
.ktext
    # Preserve CPU registers
    move    $k1, $at        # Save $at
    la      $k0, store
    sw      $v0, 0($k0)
    sw      $a0, 4($k0)

    # Extract cause
    mfc0    $k0 $13		    # Cause register
    srl     $a0 $k0 2		# Extract ExcCode Field
    andi    $a0 $a0 0x1f

    # If not an interrupt, request default handler
    bnez    $a0, __sysexcp

    # Code to handle the interrupt would go here

    # Return from interrupt (don't bump EPC)
    la      $k0, store      # Restore registers
    lw      $v0, 0($k0)
    lw      $a0, 4($k0)
    move    $at, $k1

    mtc0    $0, $13         # Clear Cause register

    mfc0    $k0, $12        # Get Status register
    ori     $k0, $k0, 1     # Enable interrupts
    mtc0    $k0, $12        # Set Status register

    eret                    # Return

    # Calls the default exception handler, which always terminates
.globl __sysexcp
__sysexcp:
    li      $v0, _SYSEXCP
    syscall


.text
.globl __start
__start:
    move    $fp, $sp        # Set frame pointer

    # main(argc, argv)
    lw      $a0, 0($sp)     # argc = sp[0]
    addiu   $a1, $sp, 4     # argv starts at sp[1]
    jal     main

    move    $a0 $v0         # Put return value in a0 (argument to syscall 17)

.globl __exit
__exit:
    li $v0 17           # Exit with the value in a0
    syscall

.globl __end
__end:
    nop