# Basic input buffer. Uses a 256-byte ring buffer to sequentially store keypresses.
# Exposes three functions for interacting with the input buffer

.kdata
KEYMAP:                                     # SDL_Scancode to ASCII mapping
.byte 0 0 0 0                               # 0 - 3
.byte "a" "b" "c" "d" "e" "f" "g" "h"       # 4 - 11
.byte "i" "j" "k" "l" "m" "n" "o" "p"       # 12 - 19
.byte "q" "r" "s" "t" "u" "v" "w" "x"       # 20 - 27
.byte "y" "z" "1" "2" "3" "4" "5" "6"       # 28 - 35
.byte "7" "8" "9" "0" "\r" 0x8 "\b" "\t"    # 36 - 43
.byte " " "-" "=" "[" "]" "\\" "\\" ";"     # 44 - 51
.byte "'" "`" "," "." "/" 0 0 0             # 52 - 59
.byte 0 0 0 0 0 0 0 0                       # 60 - 67
.byte 0 0 0 0 0 0 0 0                       # 68 - 75
.byte 0xB1 0 0 0 0 0 0 0                    # 76 - 83 (Arrow keys are 79-92: RLDU)
.byte "/" "*" "-" "+" 0x8 "1" "2" "3"       # 84 - 91    
.byte "4" "5" "6" "7" "8" "9" "0" "."       # 92 - 99
.byte "\\" 0 0 "=" 0 0 0 0                  # 100 - 107
.byte 0 0 0 0 0 0 0 0                       # 108 - 115
.byte 0 0 0 0 0 0 0 0                       # 116 - 123
.byte 0 0 0 0 0 0 0 0                       # 124 - 131   
.byte 0 0 0 0 0 0 0 0                       # 132 - 139
.byte 0 0 0 0 0 0 0 0                       # 140 - 147
.byte 0 0 0 0 0 0 0 0                       # 148 - 155
.byte 0 0 0 0 0 0 0 0                       # 156 - 163
.byte 0 0 0 0 0 0 0 0                       # 164 - 171
.byte 0 0 0 0 0 0 0 0                       # 172 - 179
.byte 0 0 0 0 0 0 0 0                       # 180 - 187
.byte 0 0 0 0 0 0 0 0                       # 188 - 195
.byte 0 0 0 0 0 0 0 0                       # 196 - 203
.byte 0 0 0 0 0 0 0 0                       # 204 - 211
.byte 0 0 0 0 0 0 0 0                       # 212 - 219
.byte 0 0 0 0 0 0 0 0                       # 220 - 227
.byte 0 0 0 0 0 0 0 0                       # 228 - 235
.byte 0 0 0 0 0 0 0 0                       # 236 - 243
.byte 0 0 0 0 0 0 0 0                       # 244 - 251
.byte 0 0 0 0                               # 252 - 255

.define MMIO_START 0xFFFFF000
.define KEY_IN 0xA00                        # Byte register in MMIO region where emulator puts scancode upon keypress
.define KEY_BUFFER 0xB00                    # 256 byte buffer
.define WRITE_PTR 0xC00                     # Offset to write
.define READ_PTR 0xC01                      # Offset to read

.ktext
# This function should be called in the main exception handler
.globl __keyboardHandler
__keyboardHandler:
    addiu   $sp, $sp, -8
    sw      $a0, 0($sp)                     # We preserve every register we use (except $k0)
    sw      $v0, 4($sp)
    # Determine interrupt
    li      $k0, 2
    beq     $a0, $k0, _KC_keydown
    li      $k0, 4
    beq     $a0, $k0, _KC_keyup
_KC_ret:
    lw      $a0, 0($sp)
    lw      $v0, 4($sp)
    addiu   $sp, $sp, 8
    jr      $ra
_KC_keydown:
    addiu   $sp, $sp, -4
    sw      $t0, 0($sp)
    # Read key
    li      $t0, MMIO_START                 # Start of MMIO region
    la      $k0, KEYMAP                     # Get address of keymap
    lbu     $a0, KEY_IN($t0)                # Read scancode
    addu    $k0, $k0, $a0                   # Use scancode as index in keymap
    lbu     $a0, 0($k0)                     # Read from keymap
    # Store key
    lbu     $k0, WRITE_PTR($t0)             # Get write offset in buffer -> $k0
    addiu   $v0, $t0, KEY_BUFFER            # Get start of key buffer
    addu    $v0, $k0, $v0                   # Add write offset to start of buffer
    sb      $a0, 0($v0)                     # Store key in key buffer
    addiu   $k0, $k0, 1                     # Increment write ptr
    sb      $k0, WRITE_PTR($t0)             # Store write ptr
    # Return
    lw      $t0, 0($sp)
    addiu   $sp, $sp, 4
    j       _KC_ret

_KC_keyup:
    j       _KC_ret

.text

# unsigned char pollKey(void); -> returns the next character in the key buffer, or 0 if there is nothing to read
.globl pollKey
pollKey:
    li      $t0, MMIO_START
    lbu     $t1, WRITE_PTR($t0)
    lbu     $t2, READ_PTR($t0)
    bne     $t1, $t2, _PK_read
    move    $v0, $0
    j       _PK_ret
_PK_read:
    addiu   $t3, $t0, KEY_BUFFER        # Get start of key buffer
    addu    $t4, $t3, $t2               # Add read offset
    lbu     $v0, 0($t4)                 # Read from buffer
    addiu   $t2, $t2, 1                 # Increment read ptr
    sb      $t2, READ_PTR($t0)          # Save read ptr
_PK_ret:
    jr      $ra

# unsigned char keysPending(void); -> returns number of unread keystrokes in buffer
.globl keysPending
keysPending:
    li      $t0, MMIO_START
    lbu     $t1, WRITE_PTR($t0)
    lbu     $t2, READ_PTR($t0)
    subu    $t3, $t1, $t2
    abs     $v0, $t3
    jr      $ra

# unsigned char getKey(); -> returns the last key in the buffer and clears the buffer, or 0 if no keys were pending
.globl getKey
getKey:
    addiu   $sp, $sp, -4
    sw      $ra, 0($sp)
    jal     keysPending
    beq     $v0, $0, _GK_ret
    li      $t0, MMIO_START
    lbu     $t5, WRITE_PTR($t0)         # We want to read character right before write pointer (last character in buffer)
    addiu   $t1, $t5, -1
    addiu   $t2, $t0, KEY_BUFFER        # Get start of key buffer
    addu    $t4, $t2, $t1               # Add offset
    lbu     $v0, 0($t4)                 # Read key
    sb      $t5, READ_PTR($t0)          # Clear buffer (set read_ptr to write_ptr)
_GK_ret:
    lw      $ra, 0($sp)
    addiu   $sp, $sp, 4
    jr      $ra