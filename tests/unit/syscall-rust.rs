// SPDX-FileCopyrightText: 2026 roolrz
// SPDX-License-Identifier: Apache-2.0

//! Runs the actual raw Rust SDK against the C capture boundary. Some wrappers
//! call C veneers; others encode call6 directly. Both must obey the same ABI.
use hyper_sys::{self as sys, CallResult, abi::*};

unsafe extern "C" {
    fn expect_call(number: u64, arguments: *const u64, status: HyperNativeStatus);
    fn check_consumed();
}

fn result(actual: CallResult, status: HyperNativeStatus) {
    assert_eq!(actual.status, status);
    assert_eq!(actual.value0, 0xfedc_ba98_7654_3210);
    assert_eq!(actual.value1, 0x89ab_cdef_0123_4567);
    // SAFETY: capture has no pointer arguments or side effects outside the fixture.
    unsafe { check_consumed() };
}

fn transport(status: HyperNativeStatus) {
    let handle = 0x8123_4567_89ab_cdef;
    let other = 0x9234_5678_9abc_def0;
    let offset = 0xa345_6789_abcd_ef01;
    let deadline = 0xb456_789a_bcde_f012;
    let options = 0xf123_4567_u32;
    let mut bytes = [0_u8; 17];
    let destination = [0_u8; 23];
    let mut info = core::mem::MaybeUninit::<HyperNativeHandleInfo>::uninit();
    let mut slots = [HyperNativeCapabilityReceiveSlot {
        handle: 0,
        rights: 0,
        expected_kind: 0,
        flags: 0,
    }; 2];
    macro_rules! expect {
        ($number:ident, $arguments:expr) => {
            let arguments: [u64; 6] = $arguments;
            expect_call($number, arguments.as_ptr(), status)
        };
    }
    // SAFETY: this executable links only the capture syscall, never a real
    // kernel transport. It treats handles/scalars as opaque values and does not
    // dereference payload pointers. All pointer objects outlive each call.
    unsafe {
        expect!(
            HYPER_NATIVE_SYS_FILESYSTEM_MOUNT_RAMFS,
            [handle, bytes.as_ptr() as u64, bytes.len() as u64, 0, 0, 0]
        );
        result(
            sys::filesystem_mount_ramfs(handle, bytes.as_ptr(), bytes.len()),
            status,
        );
        expect!(HYPER_NATIVE_SYS_FILE_PROTECT, [handle, 0, 0, 0, 0, 0]);
        result(sys::file_protect(handle), status);
        let mapping = HyperNativePrivateMapping {
            source_offset: 0,
            source_length: 0,
            address: 0,
            size: 0,
            data_offset: 0,
            permissions: 0,
            mode: 0,
        };
        expect!(
            HYPER_NATIVE_SYS_VMAR_MAP_FILE,
            [
                handle,
                other,
                offset,
                &mapping as *const _ as u64,
                core::mem::size_of_val(&mapping) as u64,
                0
            ]
        );
        assert_eq!(
            sys::vmar_map_file(
                handle,
                other,
                offset,
                &mapping,
                core::mem::size_of_val(&mapping)
            ),
            status
        );
        check_consumed();
        let request = HyperNativeFilesystemRootSwitch {
            new_root: 0,
            new_root_length: 0,
            put_old: 0,
            put_old_length: 0,
            moves: 0,
            move_count: 0,
        };
        expect!(
            HYPER_NATIVE_SYS_FILESYSTEM_SWITCH_ROOT,
            [handle, &request as *const _ as u64, 0, 0, 0, 0]
        );
        result(sys::filesystem_switch_root(handle, &request), status);
        let affinity_words = [3_u64, 1_u64 << 63];
        expect!(
            HYPER_NATIVE_SYS_VIRTUAL_CPU_SET_AFFINITY,
            [handle, affinity_words.as_ptr().addr() as u64, 2, 0, 0, 0]
        );
        assert_eq!(
            sys::virtual_cpu_set_affinity(handle, affinity_words.as_ptr(), 2),
            status
        );
        check_consumed();
        expect!(HYPER_NATIVE_SYS_ABI_QUERY, [0; 6]);
        result(sys::abi_query(), status);
        expect!(HYPER_NATIVE_SYS_CLOCK_GET_MONOTONIC, [0; 6]);
        result(sys::clock_get_monotonic(), status);
        expect!(HYPER_NATIVE_SYS_HANDLE_CLOSE, [handle, 0, 0, 0, 0, 0]);
        assert_eq!(sys::handle_close(handle), status);
        check_consumed();
        expect!(
            HYPER_NATIVE_SYS_HANDLE_DUPLICATE,
            [handle, other, 0, 0, 0, 0]
        );
        result(sys::handle_duplicate(handle, other), status);
        expect!(
            HYPER_NATIVE_SYS_HANDLE_GET_INFO,
            [
                handle,
                info.as_mut_ptr() as u64,
                size_of::<HyperNativeHandleInfo>() as u64,
                0,
                0,
                0
            ]
        );
        result(sys::handle_get_info(handle, info.as_mut_ptr()), status);
        expect!(
            HYPER_NATIVE_SYS_OBJECT_WAIT_ONE,
            [handle, other, deadline, 0, 0, 0]
        );
        result(sys::object_wait_one(handle, other, deadline), status);
        expect!(
            HYPER_NATIVE_SYS_BYTE_CHANNEL_WRITE,
            [handle, 0, bytes.as_ptr() as u64, 17, 0, 0]
        );
        assert_eq!(
            sys::byte_channel_write(handle, bytes.as_ptr(), bytes.len()),
            status
        );
        check_consumed();
        expect!(
            HYPER_NATIVE_SYS_CAPABILITY_CHANNEL_RECEIVE,
            [
                handle,
                deadline,
                bytes.as_mut_ptr() as u64,
                17,
                slots.as_mut_ptr() as u64,
                2
            ]
        );
        result(
            sys::capability_channel_receive(
                handle,
                deadline,
                bytes.as_mut_ptr(),
                bytes.len(),
                slots.as_mut_ptr(),
                slots.len(),
            ),
            status,
        );
        expect!(
            HYPER_NATIVE_SYS_FILE_WRITE_AT,
            [handle, options as u64, offset, bytes.as_ptr() as u64, 17, 0]
        );
        result(
            sys::file_write_at(handle, options, offset, bytes.as_ptr(), bytes.len()),
            status,
        );
        expect!(
            HYPER_NATIVE_SYS_DIRECTORY_RENAME,
            [
                handle,
                bytes.as_ptr() as u64,
                17,
                other,
                destination.as_ptr() as u64,
                23
            ]
        );
        result(
            sys::directory_rename(
                handle,
                bytes.as_ptr(),
                bytes.len(),
                other,
                destination.as_ptr(),
                destination.len(),
            ),
            status,
        );
        expect!(
            HYPER_NATIVE_SYS_VMAR_MAP,
            [handle, other, offset, deadline, 4096, options as u64]
        );
        assert_eq!(
            sys::vmar_map(handle, other, offset, deadline, 4096, options as u64),
            status
        );
        check_consumed();
        expect!(
            HYPER_NATIVE_SYS_THREAD_CREATE,
            [handle, other, offset, deadline, 0, 0]
        );
        result(
            sys::thread_create(handle, other, offset, deadline, core::ptr::null(), 0),
            status,
        );
        let affinity = [3_u64, 1_u64 << 63];
        expect!(
            HYPER_NATIVE_SYS_THREAD_CREATE,
            [handle, other, offset, deadline, affinity.as_ptr() as u64, 2]
        );
        result(
            sys::thread_create(handle, other, offset, deadline, affinity.as_ptr(), 2),
            status,
        );
        let word = 1_u32;
        expect!(
            HYPER_NATIVE_SYS_ATOMIC_WAIT,
            [
                &word as *const u32 as u64,
                options as u64,
                deadline,
                0,
                0,
                0
            ]
        );
        assert_eq!(sys::atomic_wait(&word, options, deadline), status);
        check_consumed();
    }
}

fn main() {
    for status in [
        HYPER_NATIVE_STATUS_OK,
        HYPER_NATIVE_STATUS_INVALID_ARGUMENT,
        HYPER_NATIVE_STATUS_BUFFER_TOO_SMALL,
    ] {
        transport(status);
    }
}
