/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/launch.h>
#include <hyper/syscall.h>

/* A kernel commit starts the bootstrap thread, not the application. The
 * runtime acknowledges only after mapping, relocation and stack setup. Peer
 * closure covers faults before that point; read queued data before treating
 * closure as failure, since a short-lived successful child can already exit. */
hyper_native_status_t hyper_launch_wait(hyper_native_handle_t process,
					hyper_native_handle_t channel)
{
	hyper_native_status_t status;
	for (;;) {
		hyper_launch_result_t reply = {0};
		hyper_call_result_t read = hyper_byte_channel_read(channel, &reply, sizeof(reply));
		if (read.status == 0) {
			status = read.value0 == sizeof(reply) && reply.status <= 0
					 ? reply.status
					 : HYPER_NATIVE_STATUS_BAD_STATE;
			break;
		}
		if (read.status != HYPER_NATIVE_STATUS_WOULD_BLOCK) {
			status = read.status;
			break;
		}
		hyper_call_result_t wait =
			hyper_object_wait_one(channel,
					      HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_READABLE |
						      HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_PEER_CLOSED,
					      HYPER_NATIVE_DEADLINE_INFINITE);
		if (wait.status) {
			status = wait.status;
			break;
		}
	}
	if (status) {
		(void)hyper_process_request_stop(process);
		(void)hyper_object_wait_one(process, HYPER_NATIVE_SIGNAL_PROCESS_TERMINATED,
					    HYPER_NATIVE_DEADLINE_INFINITE);
	}
	return status;
}
