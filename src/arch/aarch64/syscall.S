/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

.section .text.hyper_native_invoke6, "ax", %progbits
.p2align 2
.global hyper_native_invoke6
.type hyper_native_invoke6, %function
hyper_native_invoke6:
    /* AAPCS64 passes the 24-byte result address in x8. Preserve it while x8
     * carries the target capability. The operation uses x6 independently
     * of the six business arguments in x0-x5. */
    mov     x9, x8
    mov     x8, x0
    mov     x10, x1
    mov     x0, x2
    mov     x1, x3
    mov     x2, x4
    mov     x3, x5
    mov     x4, x6
    mov     x5, x7
    mov     x6, x10
    svc     #0
    stp     x0, x1, [x9]
    str     x2, [x9, #16]
    ret
.size hyper_native_invoke6, . - hyper_native_invoke6
