# Exception handler for snake_excp.s

.define FRAME_COUNTER 0xFFFFFB00
.define KEY_STATUS 0xFFFFFB04
.define KEY_DATA 0xFFFFFB05
.define KEY_IN  0xFFFFFA00

.ktext
    # Preserve CPU registers
    move    $k1, $at        # Save $at
    addiu   $sp, $sp, -12
    sw      $v0, 0($sp)
    sw      $a0, 4($sp)
    sw      $t0, 8($sp)

    # Extract cause
    mfc0    $k0 $13		    # Cause register
    srl     $a0 $k0 2		# Extract ExcCode Field
    andi    $a0 $a0 0x1f

    # If not an interrupt, request default handler
    bnez    $a0, __sysexcp

INT_READ:
    srl     $a0, $k0, 10    # Extract interrupt bits
    andi    $a0, $a0, 0xFF

    # Determine interrupt
    addiu   $k0, $0, 1
    beq     $a0, $k0, INT_DISPLAY   # IP2
    sll     $k0, $k0, 1
    beq     $a0, $k0, INT_KEYDOWN   # IP3

INT_RET:
    # Return from interrupt (don't bump EPC)
    lw      $v0, 0($sp)
    lw      $a0, 4($sp)
    lw      $t0, 8($sp)
    addiu   $sp, $sp, 12
    move    $at, $k1

    mtc0    $0, $13         # Clear Cause register

    mfc0    $k0, $12        # Get Status register
    ori     $k0, $k0, 1     # Enable interrupts
    mtc0    $k0, $12        # Set Status register

    eret                    # Return

INT_KEYDOWN:
    # Update KEY_STATUS
    li      $t0, KEY_STATUS
    li      $v0, 1
    sb      $v0, 0($t0)
    # Get scancode
    li      $k0, KEY_IN
    lbu     $a0, 0($k0)
    # Put in KEY_DATA
    sb      $a0, 1($t0)
    j       INT_RET
INT_DISPLAY:
    # Update frame counter
    li      $k0, FRAME_COUNTER
    lw      $v0, 0($k0)
    addiu   $v0, $v0, 1
    sw      $v0, 0($k0)
    j       INT_RET