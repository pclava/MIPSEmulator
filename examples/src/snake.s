# SNAKE GAME by Paul Clavaud
# Depends on snake_excp.s and display.s
# Must be linked with those dependencies and __start.asm

# Example compilation:
# ./MIPSAssembler snake.s snake_excp.s display.s
# ./MIPSLinker -o snake.out -ls snake.o snake_excp.o display.o

.define SEGMENTS_START  $s0
.define FPS_DELAY       $s1
.define SEGMENTS_SIZE   $s3
.define HEADX           0($fp)
.define HEADY           1($fp)
.define TAILX           2($fp)
.define TAILY           3($fp)
.define DIRECTION       4($fp)
.define FOODX           6($fp)
.define FOODY           7($fp)
.define SPEED           8($fp)
.define PLUSX           0x0001
.define MINUSX          0x00FF
.define PLUSY           0x0100
.define MINUSY          0xFF00

.define FRAME_COUNTER   0xFFFFFB00
.define KEY_STATUS      0xFFFFFB04
.define KEY_DATA        0xFFFFFB05

.data
directions: .half PLUSX MINUSX PLUSY MINUSY
scorestr: .asciiz "You scored!\n"
diestr: .asciiz "You died!\n"
.text
.globl main
main:
RESET:
        li      SEGMENTS_SIZE, 32000
        li      FPS_DELAY, 5
        # Initialize Segments
        li      $v0, _SYSSBRK
        move    $a0, SEGMENTS_SIZE
        syscall                                 # Allocate space for 16,000 halfwords
        move    SEGMENTS_START, $v0
        # Initialize random number generator
        li      $a0, 58494
        li      $v0, _SYSSEED
        syscall
        # Initialize stack
        addiu   $sp, $sp, -12
        move    $fp, $sp
        # Set HEAD
        li      $t0, 80
        sb      $t0, HEADX
        li      $t0, 50
        sb      $t0, HEADY
        # Set TAIL
        sb      $t0, TAILY
        li      $t0, 79
        sb      $t0, TAILX
        # Set DIRECTION
        li      $t0, PLUSX
        sh      $t0, DIRECTION
        # Set FOOD
        li      $a0, 85
        sb      $a0, FOODX
        li      $a0, 50
        sb      $a0, FOODY
        # Set SPEED
        li      $t0, 1
        sb      $t0, SPEED
        # Set everything to -1
        li      $t0, 0                          # Counter
        li      $t1, -1
INIT_SEGMENTS:
        beq     $t0, SEGMENTS_SIZE, INIT_CONT   # End if i == 32,000
        addu    $t3, $t0, SEGMENTS_START        # Add start + i
        sb      $t1, 0($t3)                     # start[i] = -1
        addiu   $t0, $t0, 1                     # Increment i
        j       INIT_SEGMENTS                   # Loop
INIT_CONT:
        # Draw head
        li      $a0, 80
        li      $a1, 50
        move    $a2, $0
        jal     WRITE_COORD
        li      $a2, 0x0f
        jal     DRAW_PIXEL
        # Draw tail
        li      $a0, 79
        li      $a1, 50
        li      $a2, 0x3250                     # Coordinates of head
        jal     WRITE_COORD
        li      $a2, 0x1B
        jal     DRAW_PIXEL
        # Draw food
        lbu     $a0, FOODX
        lbu     $a1, FOODY
        li      $a2, 0x28
        jal     DRAW_PIXEL
MAIN_LOOP:
        # LOGIC
        jal     LOGIC

        # INPUT
        jal     INPUT

        # DELAY
        jal     WAIT_FOR_FRAME

        # LOOP
        j       MAIN_LOOP

# Loop indefinitely if the player died
EXIT:
        la      $a0, diestr
        li      $v0, _SYSPRINTSTR
        syscall
DIELOOP:
        j       DIELOOP

# Perform game logic
LOGIC:
        push    $ra

        # Get new coordinates of head
        lbu     $t0, HEADX
        lhu     $t1, DIRECTION
        addu    $a0, $t0, $t1       # Add direction to X
        andi    $a0, $a0, 0xFF      # Clear top bits
        lbu     $t2, HEADY
        sll     $t2, $t2, 8
        andi    $t1, $t1, 0xFF00    # Extract Y direction
        addu    $t2, $t1, $t2       # Add direction to Y
        andi    $t2, $t2, 0xFF00    # Clear other bits
        addu    $a0, $a0, $t2       # Combine X and Y
        jal     PUSH

        # Check wall collision
        lbu     $a0, HEADX
        blt     $a0, $0, EXIT
        li      $t0, 160
        bge     $a0, $t0, EXIT
        lbu     $a1, HEADY
        blt     $a1, $0, EXIT
        li      $t0, 100
        bge     $a1, $t0, EXIT

        # Check food collision
        lbu     $t0, FOODX
        lbu     $t1, FOODY
        bne     $a0, $t0, NO_FOOD
        bne     $a1, $t1, NO_FOOD
FOOD:
        # If the player scored, print a message, and increase speed by lowering fps delay
        li      $v0, _SYSPRINTSTR
        la      $a0, scorestr
        syscall
        # Move the food
        jal     NEW_FOOD
        j       LOGIC_CONT
NO_FOOD:
        # If the player did not score, pop the tail
        jal     POP

        # Check segment collision
        lbu     $a0, HEADX
        lbu     $a1, HEADY
        jal     READ_COORD
        li      $t0, 0xFFFF
        beqz    $v0, LOGIC_CONT
        bne     $v0, $t0, EXIT
LOGIC_CONT:
        # Write new head to segments
        lbu     $a0, HEADX
        lbu     $a1, HEADY
        move    $a2, $0
        jal     WRITE_COORD
        pop     $ra
        jr      $ra

# Move the tail up once, draw over old tail
POP:
        addiu   $sp, $sp, -8
        sw      $ra, 0($sp)
        # Get coordinates of old tail
        lbu     $a0, TAILX
        lbu     $a1, TAILY
        # Get coordinates of next segment
        jal     READ_COORD
        sh      $v0, 4($sp)
        # Reset segment
        li      $a2, -1
        jal     WRITE_COORD
        lhu     $v0, 4($sp)
        # Draw over
        move    $a2, $0
        jal     DRAW_PIXEL
        # Update tail variable
        sb      $v0, TAILX
        srl     $v0, $v0, 8
        sb      $v0, TAILY
        # Return
        lw      $ra, 0($sp)
        addiu   $sp, $sp, 8
        jr      $ra

# Add new head at coordinates in $a0
PUSH:
        addiu   $sp, $sp, -8
        sw      $ra, 0($sp)
        sw      $a0, 4($sp)
        # Make old head point to new head
        move    $a2, $a0
        lbu     $a0, HEADX
        lbu     $a1, HEADY
        jal     WRITE_COORD
        # Put 0 in new head ($a0)
        lw      $a0, 4($sp)
        srl     $a1, $a0, 8
        andi    $a0, $a0, 0xFF
        # Update head
        sb      $a0, HEADX
        sb      $a1, HEADY
        # Draw new head
        li      $a2, 0xF
        jal     DRAW_PIXEL
        # Return
        lw      $ra, 0($sp)
        addiu   $sp, $sp, 8
        jr      $ra

# Keep generating a new food until it's not inside a segment
NEW_FOOD:
        push    $ra
NEW_FOOD_LOOP:
        # Get new coordinates
        li      $v0, _SYSRANDRANGE
        li      $a0, 159
        syscall
        move    $t0, $a0
        li      $v0, _SYSRANDRANGE
        li      $a0, 99
        syscall
        move    $a1, $a0
        # Get contents at segments[coordinate]
        move    $a0, $t0
        jal     READ_COORD
        # Try again if not -1
        li      $t0, 0xFFFF
        bne     $v0, $t0, NEW_FOOD_LOOP
        # Save new position
        sb      $a0, FOODX
        sb      $a1, FOODY
        li      $a2, 0x28
        jal     DRAW_PIXEL
        pop     $ra
        jr      $ra

# Read user input
INPUT:
        push    $ra
        jal     GET_KEY
        beq     $v0, $0, RET_INPUT
        move    $a0, $v0
        # Branch if less than 79 or greater than 82 (not an arrow key)
        li      $t1, 82
        bgt     $a0, $t1, RET_INPUT
        li      $t1, 79
        blt     $a0, $t1, RET_INPUT
        # Subtract 79 to get index
        sub     $a0, $a0, $t1           # i=0,1,2,3
        sll     $a0, $a0, 1             # Multiply index by 2
        la      $t0, directions         # get directions
        addu    $t1, $t0, $a0           # get directions+i
        lhu     $t2, 0($t1)             # get directions[i]
        # Save new direction
        lhu     $t3, DIRECTION          # check current direction
        li      $t0, 0x100
        addu    $t1, $t2, $t3           # add new and current direction
        beq     $t1, $t0, RET_INPUT     # branch if x-directions were opposite
        sll     $t0, $t0, 8
        beq     $t1, $t0, RET_INPUT     # branch if y-directions were opposite
        sh      $t2, DIRECTION          # write to DIRECTION
RET_INPUT:
        pop     $ra
        jr      $ra

# Check KEY_STATUS, and read KEY_DATA if ready
GET_KEY:
        li      $t0, KEY_STATUS         # Load status address
        lb      $v0, 0($t0)             # Read status
        beqz    $v0, GET_KEY_RET        # Return if zero
        sb      $0, 0($t0)              # Reset status
        lb      $v0, 1($t0)             # Read data
GET_KEY_RET:
        jr      $ra

# Poll frame counter, until it reaches FPS_DELAY
WAIT_FOR_FRAME:
        push    $ra
WAIT_LOOP:
        jal     GET_FRAME
        blt     $v0, FPS_DELAY, WAIT_LOOP
        jal     RESET_FRAME
        pop     $ra
        jr      $ra

# Reset frame counter
RESET_FRAME:
        li      $t0, FRAME_COUNTER
        sw      $0, 0($t0)
        jr      $ra

# Get current frame counter
GET_FRAME:
        li      $t0, FRAME_COUNTER
        lw      $v0, 0($t0)
        jr      $ra

# Gets the offset in bytes of the given coordinate
GET_OFFSET:
        # Get 160y
        sll     $t0, $a1, 2
        addu    $t0, $t0, $a1
        sll     $t0, $t0, 5
        # Add x
        addu    $t0, $t0, $a0
        # Multiply by 2 to get offset
        sll     $v0, $t0, 1
        jr      $ra

# Draws a 2x2 pixel with the given game address and color
DRAW_PIXEL:
        push    $ra
        sll     $a0, $a0, 1
        sll     $a1, $a1, 1
        jal     __drawPixel     # Draw (x,y)
        addiu   $a0, $a0, 1
        jal     __drawPixel     # Draw (x+1,y)
        addiu   $a1, $a1, 1
        jal     __drawPixel     # Draw (x+1,y+1)
        addiu   $a0, $a0, -1
        jal     __drawPixel     # Draw (x,y+1)
        pop     $ra
        jr      $ra

# Write value to the coordinate
WRITE_COORD:
        push    $ra
        # Get offset
        jal     GET_OFFSET
        # Add to base address
        addu    $v0, $v0, $s0
        # Store value
        sh      $a2, 0($v0)
        # Return
        pop     $ra
        jr      $ra

# Gets the value at the coordinate
READ_COORD:
        push    $ra
        jal     GET_OFFSET
        addu    $v0, $v0, $s0
        lhu     $v0, 0($v0)
        pop     $ra
        jr      $ra