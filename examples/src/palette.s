# Repeatedly draw the color palette, and print how many frames it takes
# Depends on display.s

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

    # Update frame counter
    li      $k0, _MMIO
    addiu   $k0, $k0, 256
    lw      $v0, 0($k0)
    addiu   $v0, $v0, 1
    sw      $v0, 0($k0)

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

.text
.globl main
main:
        addiu   $sp,$sp,-64
        sw      $31,60($sp)
        sw      $fp,56($sp)
        move    $fp,$sp
        sw      $0,36($fp)
        sw      $0,40($fp)
        li      $2,12                 # 0xc
        sw      $2,44($fp)
        li      $2,12                 # 0xc
        sw      $2,48($fp)
        sw      $0,32($fp)
        j       $L2
$L4:
        lw      $3,36($fp)
        li      $2,192                  # 0xc0
        bne     $3,$2,$L3
        lw      $2,40($fp)
        addiu   $2,$2,12
        sw      $2,40($fp)
        sw      $0,36($fp)
$L3:
        lw      $2,32($fp)
        sw      $2,16($sp)
        lw      $6,44($fp)
        lw      $7,48($fp)
        lw      $4,36($fp)
        lw      $5,40($fp)
        jal     __drawRectFilled
        lw      $2,36($fp)
        addiu   $2,$2,12
        sw      $2,36($fp)
        lw      $2,32($fp)
        addiu   $2,$2,1
        sw      $2,32($fp)
$L2:
        lw      $2,32($fp)
        sltiu   $2,$2,256
        bne     $2,$0,$L4
        move    $2,$0
        move    $sp,$fp
        lw      $31,60($sp)
        lw      $fp,56($sp)
        addiu   $sp,$sp,64

        # Read, print, and reset frame counter
        li      $t0, _MMIO
        addiu   $t0, $t0, 256
        lw      $a0, 0($t0)
        li      $v0, _SYSPRINTINT
        syscall
        li      $v0, _SYSPRINTCHR
        li      $a0, "\n"
        syscall
        sw      $0, 0($t0)

        j main