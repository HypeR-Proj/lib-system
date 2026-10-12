/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

.text
.p2align 2
.global __hyper_runtime_switch_stack
.hidden __hyper_runtime_switch_stack
.type __hyper_runtime_switch_stack, %function
__hyper_runtime_switch_stack:
    mov sp, x0
    mov x29, xzr
    mov x30, xzr
    br x1
.size __hyper_runtime_switch_stack, . - __hyper_runtime_switch_stack
.section .note.GNU-stack, "", %progbits
