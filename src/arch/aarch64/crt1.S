/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

.section .text._start, "ax", %progbits
.p2align 2
.global _start
.type _start, %function
_start:
    mov     x0, sp
    mov     x29, xzr
    mov     x30, xzr
    b       __hyper_crt_start
.size _start, . - _start

.section .note.GNU-stack, "", %progbits
