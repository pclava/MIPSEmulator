# Functions for interacting with the display

.define X $a0
.define Y $a1
.define COLOR $a2

.text
.globl __drawPixel __readPixel __drawLine
.globl __drawTriangle __drawRect __drawRectFilled

# void __drawPixel(unsigned int x, unsigned int y, unsigned int color)
__drawPixel:
        # Check boundaries
        bgei    X, 320, ret_drawPixel
        bgei    Y, 200, ret_drawPixel
        blt     X, $0, ret_drawPixel
        blt     Y, $0, ret_drawPixel
        # Multiply Y by 320
        sll     $t0, Y, 2
        addu    $t0, $t0, Y
        sll     $t0, $t0, 6
        # Add X offset
        addu    $t0, $t0, X
        # Add base address
        lui     $t1, %hi(_DISPLAYADDR)
        addu    $t0, $t1, $t0
        # Write to memory
        sb      COLOR, 0($t0)
ret_drawPixel:
        jr      $ra

# unsigned int __readPixel(unsigned int x, unsigned int y)
__readPixel:
        # Check boundaries
        bgei    X, 320, ret_readPixel
        bgei    Y, 200, ret_readPixel
        blt     X, $0, ret_drawPixel
        blt     Y, $0, ret_drawPixel
        # Multiply Y by 320
        sll     $t0, Y, 2
        addu    $t0, $t0, Y
        sll     $t0, $t0, 6
        # Add X offset
        addu    $t0, $t0, X
        # Add base address
        lui     $t1, %hi(_DISPLAYADDR)
        addu    $t0, $t1, $t0
        # Read from memory
        lbu     $v0, 0($t0)
ret_readPixel:
        jr      $ra

# void drawLineLow(const unsigned int x0, const unsigned int y0, const unsigned int x1, const unsigned int y1, register const unsigned int color)
drawLineLow:
        # Initialization
        addiu   $sp, $sp, -64
        sw      $ra, 60($sp)
        sw      $fp, 56($sp)
        sw      $s3, 52($sp)
        sw      $s2, 48($sp)
        sw      $s1, 44($sp)
        sw      $s0, 40($sp)
        move    $fp, $sp
        sw      $a0, 64($fp)    # x0 = fp+64
        sw      $a1, 68($fp)    # y0 = fp+68
        sw      $a2, 72($fp)    # x1 = fp+72
        sw      $a3, 76($fp)    # y1 = fp+76
        # Calculate dx
        lw      $v1, 72($fp)
        lw      $v0, 64($fp)
        subu    $v0, $v1, $v0
        sw      $v0, 24($fp)    # dx = fp+24
        # Calculate dy
        lw      $v1, 76($fp)
        lw      $v0, 68($fp)
        subu    $v0, $v1, $v0
        sw      $v0, 28($fp)    # dy = fp+28
        # Set yi
        li      $s3, 1          # yi = s3
        # Set yi to -1 and negate dy if dy < 0
        lw      $v0, 28($fp)
        bgez    $v0, DLL_$L2
        subu    $v0, $0, $v0
        sw      $v0, 28($fp)
        li      $s3, -1
DLL_$L2:
        # Calculate D
        lw      $v0, 28($fp)
        sll     $v1, $v0, 1
        lw      $v0, 24($fp)
        subu    $s0, $v1, $v0   # D = s0
        # Initialize y
        lw      $s2, 68($fp)    # y = s2
        # Calculate m
        lw      $v1, 28($fp)
        lw      $v0, 24($fp)
        subu    $v0, $v1, $v0
        sll     $v0, $v0, 1
        sw      $v0, 32($fp)    # m = fp+32
        # Begin loop
        lw      $s1, 64($fp)    # x = s1
        j       DLL_$L3
DLL_$L6:
        # drawPixel(x, y, color)
        lw      $a2, 80($fp)
        move    $a1, $s2
        move    $a0, $s1
        jal     __drawPixel
        # Check D
        blez    $s0, DLL_$L4
        # D > 0
        addu    $s2, $s2, $s3
        lw      $v0, 32($fp)
        addu    $s0, $s0, $v0
        j       DLL_$L5
DLL_$L4:
        # D <= 0
        lw      $v0, 28($fp)
        sll     $v0, $v0, 1
        addu    $s0, $s0, $v0
DLL_$L5:
        # Increment x
        addiu   $s1, $s1, 1
DLL_$L3:
        # Iterate loop
        lw      $v0, 72($fp)
        sltu    $v0, $s1, $v0
        bne     $v0, $0, DLL_$L6
        # Restore and return
        move    $sp, $fp
        lw      $ra, 60($sp)
        lw      $fp, 56($sp)
        lw      $s3, 52($sp)
        lw      $s2, 48($sp)
        lw      $s1, 44($sp)
        lw      $s0, 40($sp)
        addiu   $sp, $sp, 64
        jr      $ra

# void drawLineHigh(const unsigned int x0,  const unsigned int y0,  const unsigned int x1,  const unsigned int y1,  register const unsigned int color)
drawLineHigh:
        addiu   $sp, $sp, -64
        sw      $ra, 60($sp)
        sw      $fp, 56($sp)
        sw      $s3, 52($sp)
        sw      $s2, 48($sp)
        sw      $s1, 44($sp)
        sw      $s0, 40($sp)
        move    $fp, $sp
        sw      $a0, 64($fp)    # x0 = fp+64
        sw      $a1, 68($fp)    # y0 = fp+68
        sw      $a2, 72($fp)    # x1 = fp+72
        sw      $a3, 76($fp)    # x2 = fp+76
        # Calculate dx
        lw      $v1, 72($fp)
        lw      $v0, 64($fp)
        subu    $v0, $v1, $v0
        sw      $v0, 24($fp)    # dx = fp+24
        # Calculate dy
        lw      $v1, 76($fp)
        lw      $v0, 68($fp)
        subu    $v0, $v1, $v0
        sw      $v0, 28($fp)    # dy = fp+28
        # Set xi
        li      $s3, 1          # xi = s3
        # Set xi to -1 if dx < 0
        lw      $v0, 24($fp)
        bgez    $v0, DLH_$L2
        subu    $v0, $0, $v0
        sw      $v0, 24($fp)
        li      $s3, -1
DLH_$L2:
        # Calculate D
        lw      $v0, 24($fp)
        sll     $v1, $v0, 1
        lw      $v0, 28($fp)
        subu    $s0, $v1, $v0   # D = s0
        # Initialize x
        lw      $s2, 64($fp)    # x = s2
        # Calculate m
        lw      $v1, 24($fp)
        lw      $v0, 28($fp)
        subu    $v0, $v1, $v0
        sll     $v0, $v0, 1
        sw      $v0, 32($fp)    # m = fp+32
        # Begin loop
        lw      $s1, 68($fp)    # y = s1
        j       DLH_$L3
DLH_$L6:
        # drawPixel(x, y, color)
        lw      $a2, 80($fp)
        move    $a1, $s1
        move    $a0, $s2
        jal     __drawPixel
        # Check D
        blez    $s0, DLH_$L4
        # D > 0
        addu    $s2, $s2, $s3
        lw      $v0, 32($fp)
        addu    $s0, $s0, $v0
        j       DLH_$L5
DLH_$L4:
        # D <= 0
        lw      $v0, 24($fp)
        sll     $v0, $v0, 1
        addu    $s0, $s0, $v0
DLH_$L5:
        # Increment y
        addiu   $s1, $s1, 1
DLH_$L3:
        # Iterate loop
        lw      $v0, 76($fp)
        sltu    $v0, $s1, $v0
        bne     $v0, $0, DLH_$L6
        # Restore and return
        move    $sp, $fp
        lw      $ra, 60($sp)
        lw      $fp, 56($sp)
        lw      $s3, 52($sp)
        lw      $s2, 48($sp)
        lw      $s1, 44($sp)
        lw      $s0, 40($sp)
        addiu   $sp, $sp, 64
        jr      $ra

# void __drawLine(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned int color);
__drawLine:
        # Initialize
        addiu   $sp, $sp, -28
        sw      $ra, 24($sp)
        sw      $fp, 20($sp)
        move    $fp, $sp
        sw      $a0, 28($fp)    # x0
        sw      $a1, 32($fp)    # y0
        sw      $a2, 36($fp)    # x1
        sw      $a3, 40($fp)    # y1
        # Get abs(y1 - y0)
        subu    $t0, $a3, $a1
        abs     $t0, $t0
        # Get abs(x1 - x0)
        subu    $t1, $a2, $a0
        abs     $t1, $t1
        # Compare
        lw      $t2, 44($fp)    # Load color and pass as argument immediately
        sw      $t2, 16($fp)
        bge     $t0, $t1, dl_high
dl_low:
        # Compare x0, x1
        ble     $a0, $a2, dl_low_swap
        # drawLineLow(x1, y1, x0, y0)
        lw      $a0, 36($fp)    # x1
        lw      $a1, 40($fp)    # y1
        lw      $a2, 28($fp)    # x0
        lw      $a3, 32($fp)    # y0
        jal     drawLineLow
        j       dl_ret
dl_low_swap:
        # drawLineLow(x0, y0, x1, y1)
        lw      $a0, 28($fp)    # x0
        lw      $a1, 32($fp)    # y0
        lw      $a2, 36($fp)    # x1
        lw      $a3, 40($fp)    # y1
        jal     drawLineLow
        j       dl_ret
dl_high:
        # Compare y0, y1
        ble     $a1, $a3, dl_high_swap
        # drawLineHigh(x1, y1, x0, y0)
        lw      $a0, 36($fp)    # x1
        lw      $a1, 40($fp)    # y1
        lw      $a2, 28($fp)    # x0
        lw      $a3, 32($fp)    # y0
        jal     drawLineHigh
        j       dl_ret
dl_high_swap:
        # drawLineHigh(x0, y0, x1, y1)
        lw      $a0, 28($fp)    # x0
        lw      $a1, 32($fp)    # y0
        lw      $a2, 36($fp)    # x1
        lw      $a3, 40($fp)    # y1
        jal     drawLineHigh
dl_ret:
        # Restore and return
        move    $sp, $fp
        lw      $fp, 20($sp)
        lw      $ra, 24($sp)
        addiu   $sp, $sp, 28
        jr      $ra

# void __drawTriangle(struct point a,  struct point b,  struct point c,  unsigned int color);
__drawTriangle:
        addiu   $sp, $sp, -40
        sw      $ra, 36($sp)
        sw      $fp, 32($sp)
        move    $fp, $sp
        sw      $a0, 40($fp)
        sw      $a1, 44($fp)
        sw      $a2, 48($fp)
        sw      $a3, 52($fp)
        lw      $v1, 40($fp)
        lw      $a0, 44($fp)
        lw      $a1, 48($fp)
        lw      $a2, 52($fp)
        lw      $v0, 64($fp)
        sw      $v0, 16($sp)
        move    $a3, $a2
        move    $a2, $a1
        move    $a1, $a0
        move    $a0, $v1
        jal     __drawLine
        lw      $v1, 48($fp)
        lw      $a0, 52($fp)
        lw      $a1, 56($fp)
        lw      $a2, 60($fp)
        lw      $v0, 64($fp)
        sw      $v0, 16($sp)
        move    $a3, $a2
        move    $a2, $a1
        move    $a1, $a0
        move    $a0, $v1
        jal     __drawLine
        lw      $v1, 40($fp)
        lw      $a0, 44($fp)
        lw      $a1, 56($fp)
        lw      $a2, 60($fp)
        lw      $v0, 64($fp)
        sw      $v0, 16($sp)
        move    $a3, $a2
        move    $a2, $a1
        move    $a1, $a0
        move    $a0, $v1
        jal     __drawLine
        move    $sp, $fp
        lw      $ra, 36($sp)
        lw      $fp, 32($sp)
        addiu   $sp, $sp, 40
        jr      $ra

# void __drawRect(struct point a,  struct point size,  unsigned int color)
__drawRect:
        addiu   $sp, $sp, -48
        sw      $ra, 44($sp)
        sw      $fp, 40($sp)
        move    $fp, $sp
        sw      $a0, 48($fp)
        sw      $a1, 52($fp)
        sw      $a2, 56($fp)
        sw      $a3, 60($fp)
        lw      $v1, 48($fp)
        lw      $v0, 56($fp)
        addu    $v0, $v1, $v0
        sw      $v0, 32($fp)
        lw      $v1, 52($fp)
        lw      $v0, 60($fp)
        addu    $v0, $v1, $v0
        sw      $v0, 36($fp)
        lw      $v1, 48($fp)
        lw      $a0, 52($fp)
        lw      $a1, 52($fp)
        lw      $v0, 64($fp)
        sw      $v0, 16($sp)
        move    $a3, $a1
        lw      $a2, 32($fp)
        move    $a1, $a0
        move    $a0, $v1
        jal     __drawLine
        lw      $v1, 52($fp)
        lw      $v0, 64($fp)
        sw      $v0, 16($sp)
        lw      $a3, 36($fp)
        lw      $a2, 32($fp)
        move    $a1, $v1
        lw      $a0, 32($fp)
        jal     __drawLine
        lw      $v1, 48($fp)
        lw      $a0, 52($fp)
        lw      $a1, 48($fp)
        lw      $v0, 64($fp)
        sw      $v0, 16($sp)
        lw      $a3, 36($fp)
        move    $a2, $a1
        move    $a1, $a0
        move    $a0, $v1
        jal     __drawLine
        lw      $v1, 48($fp)
        lw      $v0, 64($fp)
        sw      $v0, 16($sp)
        lw      $a3, 36($fp)
        lw      $a2, 32($fp)
        lw      $a1, 36($fp)
        move    $a0, $v1
        jal     __drawLine
        move    $sp, $fp
        lw      $ra, 44($sp)
        lw      $fp, 40($sp)
        addiu   $sp, $sp, 48
        jr      $ra

# void __drawRectFilled(struct point start, struct point size, unsigned int color)
__drawRectFilled:
    lw      $t2, 16($sp)        # COLOR
    # Multiply initial Y by 320
    sll     $t3, $a1, 2
    addu    $t3, $t3, $a1
    sll     $t3, $t3, 6         # t3 = 320*Y
    # Add base address
    lui     $t0, 0xffff
    addu    $t3, $t3, $t0
    # Add initial x
    addu    $t3, $t3, $a0
    # Initialize outer loop
    move    $t0, $0
DRF_$L0:
    beq     $t0, $a3, DRF_$L3
    # Initialize inner loop
    move    $t1, $0
    move    $t4, $t3
DRF_$L1:
    beq     $t1, $a2, DRF_$L2
    sb      $t2, 0($t4)
    addiu   $t1, $t1, 1
    addiu   $t4, $t4, 1
    j       DRF_$L1
DRF_$L2:
    addiu   $t0, $t0, 1
    addiu   $t3, $t3, 320       # Increment row address by 320
    j       DRF_$L0
DRF_$L3:
    jr      $ra