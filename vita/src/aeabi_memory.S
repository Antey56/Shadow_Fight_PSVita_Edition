/* ARM RTABI memory helpers: integer registers only, no libc/VFP calls.
 * 4/8 aliases accept aligned pointers and arbitrary byte counts. */
.syntax unified
.thumb
.text
.align 2
.global sf_aeabi_memcpy
.type sf_aeabi_memcpy,%function
.thumb_func
sf_aeabi_memcpy:
.Lcopy:
    cbz r2, 2f
1:  ldrb r3, [r1], #1
    strb r3, [r0], #1
    subs r2, r2, #1
    bne 1b
2:  bx lr
.size sf_aeabi_memcpy, .-sf_aeabi_memcpy
.global sf_aeabi_memmove
.type sf_aeabi_memmove,%function
.thumb_func
sf_aeabi_memmove:
    cmp r0, r1
    bls .Lcopy
    cbz r2, 2f
    add r0, r0, r2
    add r1, r1, r2
1:  ldrb r3, [r1, #-1]!
    strb r3, [r0, #-1]!
    subs r2, r2, #1
    bne 1b
2:  bx lr
.size sf_aeabi_memmove, .-sf_aeabi_memmove
.global sf_aeabi_memset
.type sf_aeabi_memset,%function
.thumb_func
sf_aeabi_memset:
.Lset:
    cbz r1, 2f
1:  strb r2, [r0], #1
    subs r1, r1, #1
    bne 1b
2:  bx lr
.size sf_aeabi_memset, .-sf_aeabi_memset
.global sf_aeabi_memclr
.type sf_aeabi_memclr,%function
.thumb_func
sf_aeabi_memclr:
    movs r2, #0
    b .Lset
.size sf_aeabi_memclr, .-sf_aeabi_memclr
.macro ALIAS name, base
.global \name
.type \name,%function
.thumb_set \name, \base
.endm
ALIAS sf_aeabi_memcpy4, sf_aeabi_memcpy
ALIAS sf_aeabi_memcpy8, sf_aeabi_memcpy
ALIAS sf_aeabi_memmove4, sf_aeabi_memmove
ALIAS sf_aeabi_memmove8, sf_aeabi_memmove
ALIAS sf_aeabi_memset4, sf_aeabi_memset
ALIAS sf_aeabi_memset8, sf_aeabi_memset
ALIAS sf_aeabi_memclr4, sf_aeabi_memclr
ALIAS sf_aeabi_memclr8, sf_aeabi_memclr
