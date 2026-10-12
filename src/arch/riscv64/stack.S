/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

.text
.p2align 2
.global __hyper_runtime_switch_stack
.hidden __hyper_runtime_switch_stack
.type __hyper_runtime_switch_stack, @function
__hyper_runtime_switch_stack:
    mv sp, a0
    li s0, 0
    li ra, 0
    jr a1
.size __hyper_runtime_switch_stack, . - __hyper_runtime_switch_stack
.section .note.GNU-stack, "", @progbits
