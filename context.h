/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#ifndef _IPTS_CONTEXT_H_
#define _IPTS_CONTEXT_H_

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mei_cl_bus.h>
#include <linux/types.h>

#include "protocol.h"

struct ipts_buffer_info {
	u8 *address;
	dma_addr_t dma_address;
};

struct ipts_context {
	struct mei_cl_device *cldev;
	struct device *dev;

	struct ipts_get_device_info_rsp device_info;

	struct ipts_buffer_info data[IPTS_BUFFERS];
	struct ipts_buffer_info doorbell;

	struct ipts_buffer_info feedback[IPTS_BUFFERS];
	struct ipts_buffer_info workqueue;
	struct ipts_buffer_info host2me;

	u8 ready;
};

#endif /* _IPTS_CONTEXT_H_ */

