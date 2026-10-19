/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

.section .text.hyper_native_invoke6, "ax", @progbits
.p2align 2
.global hyper_native_invoke6
.type hyper_native_invoke6, @function
hyper_native_invoke6:
    /* LP64D returns the 24-byte structure through the hidden a0 pointer.
     * The eighth explicit argument is therefore at 0(sp). Native traps
     * preserve t0-t2 and all registers except result a0-a2. */
    mv t0, a0
    mv t1, a1
    mv t2, a2
    mv a0, a3
    mv a1, a4
    mv a2, a5
    mv a3, a6
    mv a4, a7
    ld a5, 0(sp)
    mv a6, t2
    mv a7, t1
    ecall
    sd a0, 0(t0)
    sd a1, 8(t0)
    sd a2, 16(t0)
    mv a0, t0
    ret
.size hyper_native_invoke6, . - hyper_native_invoke6
.section .note.GNU-stack, "", @progbits
