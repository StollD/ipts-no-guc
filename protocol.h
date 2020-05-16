/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_PROTOCOL_H_
#define _IPTS_PROTOCOL_H_

#include <linux/build_bug.h>
#include <linux/types.h>

#define IPTS_WORKQUEUE_SIZE 8192
#define IPTS_WORKQUEUE_ITEM_SIZE 16

/*
 * How many data / feedback buffers IPTS uses
 */
#define IPTS_BUFFERS 16

/*
 * Helpers to avoid writing boilerplate code.
 * The response to a command code is always 0x8000000x, where x
 * is the command code itself. Instead of writing two definitions,
 * we use macros to calculate the value on the fly instead.
 */
#define IPTS_CMD(COMMAND) IPTS_EVT_##COMMAND
#define IPTS_RSP(COMMAND) (IPTS_CMD(COMMAND) + 0x80000000)

/*
 * Events that can be sent and received from the ME
 */
enum ipts_evt_code {
	IPTS_EVT_GET_DEVICE_INFO = 1,
	IPTS_EVT_SET_MODE,
	IPTS_EVT_SET_MEM_WINDOW,
	IPTS_EVT_QUIESCE_IO,
	IPTS_EVT_READY_FOR_DATA,
	IPTS_EVT_FEEDBACK,
	IPTS_EVT_CLEAR_MEM_WINDOW,
	IPTS_EVT_NOTIFY_DEV_READY,
};

/*
 * Status codes that are returned from the ME to indicate
 * whether a command was executed successfully.
 *
 * Some of theese errors are less serious than others, and need
 * to be ignored to ensure proper operation. See also the error
 * handling in receiver.c
 */
enum ipts_me_status {
	IPTS_ME_STATUS_SUCCESS = 0,
	IPTS_ME_STATUS_INVALID_PARAMS,
	IPTS_ME_STATUS_ACCESS_DENIED,
	IPTS_ME_STATUS_CMD_SIZE_ERROR,
	IPTS_ME_STATUS_NOT_READY,
	IPTS_ME_STATUS_REQUEST_OUTSTANDING,
	IPTS_ME_STATUS_NO_SENSOR_FOUND,
	IPTS_ME_STATUS_OUT_OF_MEMORY,
	IPTS_ME_STATUS_INTERNAL_ERROR,
	IPTS_ME_STATUS_SENSOR_DISABLED,
	IPTS_ME_STATUS_COMPAT_CHECK_FAIL,
	IPTS_ME_STATUS_SENSOR_EXPECTED_RESET,
	IPTS_ME_STATUS_SENSOR_UNEXPECTED_RESET,
	IPTS_ME_STATUS_RESET_FAILED,
	IPTS_ME_STATUS_TIMEOUT,
	IPTS_ME_STATUS_TEST_MODE_FAIL,
	IPTS_ME_STATUS_SENSOR_FAIL_FATAL,
	IPTS_ME_STATUS_SENSOR_FAIL_NONFATAL,
	IPTS_ME_STATUS_INVALID_DEVICE_CAPS,
	IPTS_ME_STATUS_QUIESCE_IO_IN_PROGRESS,
	IPTS_ME_STATUS_MAX
};

/*
 * The mode that IPTS will use. Singletouch mode will disable the stylus
 * and only return basic HID data. Multitouch mode will return stylus, as
 * well as heatmap data for touch input.
 *
 * This driver only supports multitouch mode.
 */
enum ipts_sensor_mode {
	IPTS_SENSOR_MODE_SINGLETOUCH = 0,
	IPTS_SENSOR_MODE_MULTITOUCH,
};

/*
 * The parameters for the SET_MODE command.
 *
 * sensor_mode needs to be either IPTS_SENSOR_MODE_MULTITOUCH or
 * IPTS_SENSOR_MODE_SINGLETOUCH.
 *
 * On newer generations of IPTS (surface gen7), this command will fail
 * if anything other than IPTS_SENSOR_MODE_MULTITOUCH is sent.
 */
struct ipts_set_mode_cmd {
	u32 sensor_mode;
	u8 reserved[12];
} __packed;

static_assert(sizeof(struct ipts_set_mode_cmd) == 16);

/*
 * The parameters for the SET_MEM_WINDOW command.
 *
 * This passes the (physical) addresses of buffers to the ME,
 * which will then fill up those buffers with touch data. It also
 * sets up the doorbell, an unsigned integer that will correspond to
 * the next touch buffer to be filled.
 *
 * Even though they are not used by the driver and stay empty, the feedback
 * buffers need to be allocated and passed too, otherwise the ME will refuse
 * to continue normally. Same applies for the host2me buffer, and the two
 * workqueue values.
 */
struct ipts_set_mem_window_cmd {
	u32 data_buffer_addr_lower[IPTS_BUFFERS];
	u32 data_buffer_addr_upper[IPTS_BUFFERS];
	u32 workqueue_addr_lower;
	u32 workqueue_addr_upper;
	u32 doorbell_addr_lower;
	u32 doorbell_addr_upper;
	u32 feedback_buffer_addr_lower[IPTS_BUFFERS];
	u32 feedback_buffer_addr_upper[IPTS_BUFFERS];
	u32 host2me_addr_lower;
	u32 host2me_addr_upper;
	u32 host2me_size;
	u8 reserved1;
	u8 workqueue_item_size;
	u16 workqueue_size;
	u8 reserved[32];
} __packed;

static_assert(sizeof(struct ipts_set_mem_window_cmd) == 320);

/*
 * The parameters for the FEEDBACK command.
 *
 * This command is sent to indicate that the data in a buffer has been
 * consumed by the host, and that the ME can overwrite the data. This
 * requires the transaction ID, which can be fetched from the touch data
 * buffer.
 */
struct ipts_feedback_cmd {
	u32 buffer;
	u32 transaction;
	u8 reserved[8];
} __packed;

static_assert(sizeof(struct ipts_feedback_cmd) == 16);

/*
 * Commands are sent from the host to the ME. They consist of a
 * command code, and optionally a set of parameters. If no parameters
 * are neccessary, those bytes should be 0.
 *
 * The ME will react to a command by sending a response, either returning
 * data that was queried, or simply indicating whether the command was
 * successfull.
 */
struct ipts_command {
	u32 code;
	union {
		struct ipts_set_mode_cmd set_mode;
		struct ipts_set_mem_window_cmd set_mem_window;
		struct ipts_feedback_cmd feedback;
	} data;
} __packed;

static_assert(sizeof(struct ipts_command) == 324);

/*
 * The data that is returned in response to the GET_DEVICE_INFO command.
 *
 * The first four parameters identify the device we're talking to.
 * data_size and feedback_size are the required sizes for the data and
 * feedback buffers.
 */
struct ipts_device_info {
	u16 vendor_id;
	u16 device_id;
	u32 hw_rev;
	u32 fw_rev;
	u32 data_size;
	u32 feedback_size;
	u8 reserved[24];
} __packed;

static_assert(sizeof(struct ipts_device_info) == 44);

/*
 * Responses are sent from the ME to the host in reaction to a command.
 * They consist of the response code (command code + 0x80000000), the
 * status (IPTS_ME_STATUS_*), and optionally, the data that was queried
 * by the host.
 */
struct ipts_response {
	u32 code;
	u32 status;
	union {
		struct ipts_device_info device_info;
		u8 reserved[80];
	} data;
} __packed;

static_assert(sizeof(struct ipts_response) == 88);

#endif /* _IPTS_PROTOCOL_H_ */
