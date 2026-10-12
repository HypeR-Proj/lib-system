/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

.section .text._start, "ax", @progbits
.p2align 2
.global _start
.type _start, @function
_start:
    mv a0, sp
    li s0, 0
    li ra, 0
    tail __hyper_crt_start
.size _start, . - _start
.section .note.GNU-stack, "", @progbits
