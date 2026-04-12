# Draws a bird

.text
.globl main
main:
addiu   $sp, $sp, -20
# drawLine(224,60,240,80)
li	$a0, 224
li	$a1, 60
li	$a2, 240
li	$a3, 80
li	$t0, 11
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(160,140,75,40)
li	$a0, 160
li	$a1, 140
li	$a2, 75
li	$a3, 40
li	$t0, 6
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(240,80,260,80)
li	$a0, 240
li	$a1, 80
li	$a2, 260
li	$a3, 80
li	$t0, 1
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(195,80,130,80)
li	$a0, 195
li	$a1, 80
li	$a2, 130
li	$a3, 80
li	$t0, 4
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(75,40,75,20)
li	$a0, 75
li	$a1, 40
li	$a2, 75
li	$a3, 20
li	$t0, 9
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(75,20,95,20)
li	$a0, 75
li	$a1, 20
li	$a2, 95
li	$a3, 20
li	$t0, 2
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(95,20,130,80)
li	$a0, 95
li	$a1, 20
li	$a2, 130
li	$a3, 80
li	$t0, 5
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(160,140,165,170)
li	$a0, 160
li	$a1, 140
li	$a2, 165
li	$a3, 170
li	$t0, 11
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(149,127,154,157)
li	$a0, 149
li	$a1, 127
li	$a2, 154
li	$a3, 157
li	$t0, 7
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(145,60,170,80)
li	$a0, 145
li	$a1, 60
li	$a2, 170
li	$a3, 80
li	$t0, 4
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(145,60,147,80)
li	$a0, 145
li	$a1, 60
li	$a2, 147
li	$a3, 80
li	$t0, 15
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(160,140,240,80)
li	$a0, 160
li	$a1, 140
li	$a2, 240
li	$a3, 80
li	$t0, 7
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(224,60,195,80)
li	$a0, 224
li	$a1, 60
li	$a2, 195
li	$a3, 80
li	$t0, 5
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(170,115,110,130)
li	$a0, 170
li	$a1, 115
li	$a2, 110
li	$a3, 130
li	$t0, 3
sw	$t0, 16($sp)
jal	__drawLine
# drawLine(110,130,140,100)
li	$a0, 110
li	$a1, 130
li	$a2, 140
li	$a3, 100
li	$t0, 7
sw	$t0, 16($sp)
jal	__drawLine

# drawPixel(220, 80, 0x37)
li  $a0, 220
li  $a1, 80
li  $a2, 0x37
jal __drawPixel

loop:
j   loop