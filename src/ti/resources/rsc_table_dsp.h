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
 *  ======== rsc_table_dsp.h ========
 *
 *  Include this table in each base image, which is read from remoteproc on
 *  host side.
 *
 *  These values are currently very OMAP4 specific!
 *
 */

#ifndef _RSC_TABLE_DSP_H_
#define _RSC_TABLE_DSP_H_

//#define DSP

#include <ti/resources/rsc_types.h>

/* Ducati Memory Map: */
#define L4_44XX_BASE            0x4a000000

#define L4_PERIPHERAL_L4CFG     (L4_44XX_BASE)
#define DSP_PERIPHERAL_L4CFG    0x4A000000

#define L4_PERIPHERAL_L4PER     0x48000000
#define DSP_PERIPHERAL_L4PER    0x48000000

#define L4_PERIPHERAL_L4EMU     0x54000000
#define DSP_PERIPHERAL_L4EMU    0x54000000

#define L3_TILER_MODE_0_1       0x60000000
#define DSP_TILER_MODE_0_1      0x60000000

#define L3_TILER_MODE_2         0x70000000
#define DSP_TILER_MODE_2        0x70000000

#define L3_TILER_MODE_3         0x78000000
#define DSP_TILER_MODE_3        0x78000000

#define DSP_MEM_TEXT            0x20000000
#define DSP_MEM_DATA            0x90000000
#define DSP_MEM_HEAP            0x90100000
#define DSP_MEM_IPC_DATA        0x9F000000

#define DSP_MEM_IOBUFS          0x80000000
#define PHYS_MEM_IOBUFS         0xBA300000

#define DSP_MEM_IPC_VRING       0xA0000000
#define PHYS_MEM_IPC_VRING      0xA8800000

#define RPMSG_VRING0_DA         0xA0000000
#define RPMSG_VRING1_DA         0xA0004000

#define CONSOLE_VRING0_DA       0xA0008000
#define CONSOLE_VRING1_DA       0xA000C000

#define BUFS0_DA                0xA0040000
#define BUFS1_DA                0xA0080000

/*
 * sizes of the virtqueues (expressed in number of buffers supported,
 * and must be power of 2)
 */
#define RPMSG_VQ0_SIZE                256
#define RPMSG_VQ1_SIZE                256

#define CONSOLE_VQ0_SIZE                256
#define CONSOLE_VQ1_SIZE                256

/* Size constants must match those used on host: include/asm-generic/sizes.h */
#define SZ_64K                          0x00010000
#define SZ_128K                         0x00020000
#define SZ_256K                         0x00040000
#define SZ_512K                         0x00080000
#define SZ_1M                           0x00100000
#define SZ_2M                           0x00200000
#define SZ_4M                           0x00400000
#define SZ_8M                           0x00800000
#define SZ_16M                          0x01000000
#define SZ_32M                          0x02000000
#define SZ_64M                          0x04000000
#define SZ_128M                         0x08000000
#define SZ_256M                         0x10000000
#define SZ_512M                         0x20000000

#define DSP_MEM_TEXT_SIZE	(SZ_512K)
#define DSP_MEM_DATA_SIZE	(SZ_512K)
#define DSP_MEM_HEAP_SIZE	(SZ_2M + SZ_128K)

#define DSP_MEM_IOBUFS_SIZE	(SZ_1M * 90)
#define DSP_MEM_IPC_DATA_SIZE	(SZ_1M)
#define DSP_MEM_IPC_VRING_SIZE	(SZ_1M)


/* flip up bits whose indices represent features we support */
#define RPMSG_DSP_C0_FEATURES         1
#define TESLA2DUCATI_RPMSG_FEATURES (1 << VIRTIO_RPMSG_F_NS || 1 << VIRTIO_RING_F_SYMMETRIC)

//extern Char *xdc_runtime_SysMin_Module_State_0_outbuf__A;
#define TRACEBUFADDR (UInt32)&ti_trace_SysMin_Module_State_0_outbuf__A

#pragma DATA_SECTION(resources, ".resource_table")
#pragma DATA_ALIGN(resources, 4096)

struct resource_table resources = {
	1, /* we're the first version that implements this */
	14, /* number of entries in the table */
	0, 0, /* reserved, must be zero */
	/* offsets to entries */
	{
		offsetof(struct resource_table, rpmsg_vdev),
		offsetof(struct resource_table, console_vdev),
		offsetof(struct resource_table, data_cout),
		offsetof(struct resource_table, heap_cout),
		offsetof(struct resource_table, text_cout),
		offsetof(struct resource_table, ipc_data),
		offsetof(struct resource_table, trace),
		offsetof(struct resource_table, devmem0),
		offsetof(struct resource_table, devmem1),
		offsetof(struct resource_table, devmem2),
		offsetof(struct resource_table, devmem3),
		offsetof(struct resource_table, devmem4),
		offsetof(struct resource_table, devmem5),
		offsetof(struct resource_table, devmem6),
	},

	/* rpmsg vdev entry */
	{
		TYPE_VDEV, VIRTIO_ID_RPMSG, 0,
		RPMSG_DSP_C0_FEATURES, 0, 0, 0, 2, { 0, 0 },
		/* no config data */
	},
	/* the two vrings */
	{ RPMSG_VRING0_DA, 4096, RPMSG_VQ0_SIZE, 1, 0 },
	{ RPMSG_VRING1_DA, 4096, RPMSG_VQ1_SIZE, 2, 0 },

	/* console vdev entry */
	{
		TYPE_VDEV, VIRTIO_ID_CONSOLE, 3,
		0, 0, 0, 0, 2, { 0, 0 },
		/* no config data */
	},
	/* the two vrings */
	{ CONSOLE_VRING0_DA, 4096, CONSOLE_VQ0_SIZE, 4, 0 },
	{ CONSOLE_VRING1_DA, 4096, CONSOLE_VQ1_SIZE, 5, 0 },

	{
		TYPE_CARVEOUT,
		DSP_MEM_DATA, 0, DSP_MEM_DATA_SIZE, 0, 0, "DSP_MEM_DATA",
	},

	{
		TYPE_CARVEOUT,
		DSP_MEM_HEAP, 0, DSP_MEM_HEAP_SIZE, 0, 0, "DSP_MEM_HEAP",
	},

	{
		TYPE_CARVEOUT,
		DSP_MEM_TEXT, 0, DSP_MEM_TEXT_SIZE, 0, 0, "DSP_MEM_TEXT",
	},

	{
		TYPE_CARVEOUT,
		DSP_MEM_IPC_DATA, 0, DSP_MEM_IPC_DATA_SIZE,
		0, 0, "DSP_IPC_DATA",
	},

	{
		TYPE_TRACE, TRACEBUFADDR, 0x8000, 0, "trace:dsp",
	},

	{
		TYPE_DEVMEM, DSP_MEM_IPC_VRING, PHYS_MEM_IPC_VRING,
		DSP_MEM_IPC_VRING_SIZE, 0, 0, "DSP_MEM_IPC",
	},

	{
		TYPE_DEVMEM,
		DSP_TILER_MODE_0_1, L3_TILER_MODE_0_1,
		SZ_256M, 0, 0, "DSP_TILER_MODE_0_1",
	},

	{
		TYPE_DEVMEM,
		DSP_TILER_MODE_2, L3_TILER_MODE_2,
		SZ_128M, 0, 0, "DSP_TILER_MODE_2",
	},

	{
		TYPE_DEVMEM,
		DSP_TILER_MODE_3, L3_TILER_MODE_3,
		SZ_128M, 0, 0, "DSP_TILER_MODE_3",
	},

	{
		TYPE_DEVMEM,
		DSP_PERIPHERAL_L4CFG, L4_PERIPHERAL_L4CFG,
		SZ_16M, 0, 0, "DSP_PERIPHERAL_L4CFG",
	},

	{
		TYPE_DEVMEM,
		DSP_PERIPHERAL_L4PER, L4_PERIPHERAL_L4PER,
		SZ_16M, 0, 0, "DSP_PERIPHERAL_L4PER",
	},

	{
		TYPE_DEVMEM,
		DSP_MEM_IOBUFS, PHYS_MEM_IOBUFS,
		DSP_MEM_IOBUFS_SIZE, 0, 0, "DSP_MEM_IOBUFS",
	},

};

#endif /* _RSC_TABLE_DSP_H_ */
