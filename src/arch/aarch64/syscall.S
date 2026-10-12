/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

.section .text.hyper_native_call6, "ax", %progbits
.p2align 2
.global hyper_native_call6
.type hyper_native_call6, %function
hyper_native_call6:
    /* AAPCS64 passes the 24-byte result address in x8. Preserve it while x8
     * carries the HypeR Native syscall number across SVC. */
    mov     x9, x8
    mov     x8, x0
    mov     x0, x1
    mov     x1, x2
    mov     x2, x3
    mov     x3, x4
    mov     x4, x5
    mov     x5, x6
    svc     #0
    stp     x0, x1, [x9]
    str     x2, [x9, #16]
    ret
.size hyper_native_call6, . - hyper_native_call6
