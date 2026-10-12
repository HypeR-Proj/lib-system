/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

.section .text.hyper_native_call6, "ax", @progbits
.p2align 2
.global hyper_native_call6
.type hyper_native_call6, @function
hyper_native_call6:
    /* LP64D returns the 24-byte structure through the hidden a0 pointer.
     * Native traps preserve t0/t1 and all registers except result a0-a2. */
    mv t0, a0
    mv t1, a1
    mv a0, a2
    mv a1, a3
    mv a2, a4
    mv a3, a5
    mv a4, a6
    mv a5, a7
    mv a7, t1
    ecall
    sd a0, 0(t0)
    sd a1, 8(t0)
    sd a2, 16(t0)
    mv a0, t0
    ret
.size hyper_native_call6, . - hyper_native_call6
.section .note.GNU-stack, "", @progbits
