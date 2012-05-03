/*
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ======== rsc_types.h ========
 *
 *  Common definition types used in the resource table.
 *
 */

#ifndef _RSC_TYPES_H_
#define _RSC_TYPES_H_

/* virtio ids: keep in sync with the linux "include/linux/virtio_ids.h" */
#define VIRTIO_ID_CONSOLE	3 /* virtio console */
#define VIRTIO_ID_RPMSG		7 /* virtio remote processor messaging */

/* Indices of rpmsg virtio features we support */
#define VIRTIO_RPMSG_F_NS	0 /* RP supports name service notifications */
#define VIRTIO_RING_F_SYMMETRIC	30 /* We support symmetric vring */

/* Resource info: Must match include/linux/remoteproc.h: */
#define TYPE_CARVEOUT    0
#define TYPE_DEVMEM      1
#define TYPE_TRACE       2
#define TYPE_VDEV        3

struct fw_rsc_carveout {
	UInt32 type;
	UInt32 da;
	UInt32 pa;
	UInt32 len;
	UInt32 flags;
	UInt32 reserved;
	char name[32];
};

struct fw_rsc_devmem {
	UInt32 type;
	UInt32 da;
	UInt32 pa;
	UInt32 len;
	UInt32 flags;
	UInt32 reserved;
	char name[32];
};

struct fw_rsc_trace {
	UInt32 type;
	UInt32 da;
	UInt32 len;
	UInt32 reserved;
	char name[32];
};

struct fw_rsc_vdev_vring {
	UInt32 da; /* device address */
	UInt32 align;
	UInt32 num;
	UInt32 notifyid;
	UInt32 reserved;
};

struct fw_rsc_vdev {
	UInt32 type;
	UInt32 id;
	UInt32 notifyid;
	UInt32 dfeatures;
	UInt32 gfeatures;
	UInt32 config_len;
	char status;
	char num_of_vrings;
	char reserved[2];
};

struct resource_table {
	UInt32 version;
	UInt32 num;
	UInt32 reserved[2];
	UInt32 offset[17];

	/* rpmsg vdev entry */
	struct fw_rsc_vdev rpmsg_vdev;
	struct fw_rsc_vdev_vring rpmsg_vring0;
	struct fw_rsc_vdev_vring rpmsg_vring1;

	/* console vdev entry */
	struct fw_rsc_vdev console_vdev;
	struct fw_rsc_vdev_vring console_vring0;
	struct fw_rsc_vdev_vring console_vring1;

	/* data carveout entry */
	struct fw_rsc_carveout data_cout;
#ifdef DSP
	/* heap carveout entry */
	struct fw_rsc_carveout heap_cout;
#endif
	/* text carveout entry */
	struct fw_rsc_carveout text_cout;

	/* ipc_data carveout entry */
	struct fw_rsc_carveout ipc_data;

	/* trace entry */
	struct fw_rsc_trace trace;

	/* devmem entry */
	struct fw_rsc_devmem devmem0;

	/* devmem entry */
	struct fw_rsc_devmem devmem1;

	/* devmem entry */
	struct fw_rsc_devmem devmem2;

	/* devmem entry */
	struct fw_rsc_devmem devmem3;

	/* devmem entry */
	struct fw_rsc_devmem devmem4;

	/* devmem entry */
	struct fw_rsc_devmem devmem5;

	/* devmem entry */
	struct fw_rsc_devmem devmem6;

	/* devmem entry */
	struct fw_rsc_devmem devmem7;

	/* devmem entry */
	struct fw_rsc_devmem devmem8;

	/* devmem entry */
	struct fw_rsc_devmem devmem9;

};

#endif /* _RSC_TYPES_H_ */
