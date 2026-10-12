<!--
SPDX-FileCopyrightText: 2026 roolrz
SPDX-License-Identifier: Apache-2.0
-->

# Native integration test application

This freestanding application exercises only public HypeR ABI and Lib
interfaces. The top-level SDK check consumes it as the stable application
input for assembled SDK and compiler integration checks.

The application is deliberately separate from `tests/unit`: it is not a unit
test for library implementation details. `make sdk-check` compiles it through
the assembled SDK for the selected AArch64 or RV64 target, then links both
dynamic and static PIE executables and validates their ELF contracts. These
checks establish the installed header, runtime and linker boundary; Native
QEMU tests separately exercise application execution.
