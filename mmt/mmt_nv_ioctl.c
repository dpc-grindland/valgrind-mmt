/*
   Copyright (C) 2006 Dave Airlie
   Copyright (C) 2007 Wladimir J. van der Laan
   Copyright (C) 2009, 2011, 2014 Marcin Slusarz <marcin.slusarz@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/
#include "mmt_nv_ioctl.h"
#include "mmt_trace.h"
#include "mmt_trace_bin.h"
#include "pub_tool_vki.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcproc.h"
#include "pub_tool_libcassert.h"
#include "nvrm_ioctl.h"
#include "nvrm_mthd.h"

#include <sys/select.h>

static fd_set nvidiactl_fds;
static fd_set nvidia0_fds;

int mmt_trace_nvidia_ioctls = False;
int mmt_trace_marks = False;
static int trace_mark_fd;
static int trace_mark_cnt = 0;

/*
 * Binary format message subtypes:
 *     a = allocate map
 *     b = bind
 *     c = create object
 *     d = destroy object
 *     e = deallocate map
 *     g = gpu map
 *     h = gpu unmap
 *     i = ioctl before
 *     j = ioctl after
 *     k = mark (mmiotrace)
 *     l = call method
 *     m = mmap
 *     o = memory dump
 *     p = create mapped object
 *     P = nouveau's GEM_PUSHBUF data
 *     r = create driver object
 *     t = create dma object
 *     v = create device object
 *     x = create context object
 *     1 = call method data
 *     4 = ioctl 4d
 *
 */
static inline unsigned long long mmt_2x4to8(UInt h, UInt l)
{
	return (((unsigned long long)h) << 32) | l;
}

void mmt_nv_ioctl_fini()
{
	if (mmt_trace_marks) {
		VG_(close)(trace_mark_fd);
	}
}

void mmt_nv_ioctl_post_clo_init(void)
{
	if (mmt_trace_marks) {
		SysRes ff;
		ff = VG_(open)("/sys/kernel/debug/tracing/trace_marker", VKI_O_WRONLY, 0777);
		if (ff._isError) {
			VG_(message) (Vg_UserMsg, "Cannot open marker file!\n");
			mmt_trace_marks = 0;
		}
		trace_mark_fd = ff._val;
	}
}

static struct mmt_mmap_data *get_nvidia_mapping(Off64T offset)
{
	struct mmt_mmap_data *region;

	region = mmt_find_region_by_fdset_offset(&nvidia0_fds, offset);
	if (region)
		return region;

	return mmt_add_region(0, 0, 0, offset, 0, 0, 0);
}


static Addr release_nvidia_mapping(Off64T offset)
{
	struct mmt_mmap_data *region;
	Addr start;

	region = mmt_find_region_by_fdset_offset(&nvidia0_fds, offset);
	if (!region)
		return 0;

	start = region->start;
	mmt_free_region(region);

	return start;
}

static Addr release_nvidia_mapping2(UWord data1, UWord data2)
{
	struct mmt_mmap_data *region;
	Addr start;

	region = mmt_find_region_by_fdset_data(&nvidia0_fds, data1, data2);
	if (!region)
		return 0;

	start = region->start;
	mmt_free_region(region);

	return start;
}

static void dumpmem(const char *s, Addr addr, UInt size)
{
	char line[4096];
	int idx = 0;
	line[0] = 0;

	UInt i;
	if (mmt_binary_output)
	{
		mmt_bin_write_1('n');
		mmt_bin_write_1('o');
		mmt_bin_write_8(addr);
	}

	if (!addr || (addr & 0xffff0000) == 0xbeef0000)
	{
		if (mmt_binary_output)
		{
			mmt_bin_write_str("");
			mmt_bin_write_buffer((UChar *)"", 0);
			mmt_bin_end();
		}
		return;
	}

	if (mmt_binary_output)
	{
		mmt_bin_write_str(s);
		mmt_bin_write_buffer((UChar *)addr, size);
		mmt_bin_end();
	}
	else
	{
        if(size%4==0){
            size = size / 4;
            for (i = 0; i < size; ++i)
            {
                if (idx + 11 >= 4095)
                    break;
                VG_(sprintf) (line + idx, "0x%08x ", ((UInt *) addr)[i]);
                idx += 11;
            }
            VG_(message) (Vg_DebugMsg, "%s%s\n", s, line);
        }
        else{
            for (i = 0; i < size; ++i)
            {
                if (idx + 5 >= 4095)
                    break;
                VG_(sprintf) (line + idx, "0x%02x ", ((char *) addr)[i]);
                idx += 5;
            }
            VG_(message) (Vg_DebugMsg, "%s%s\n", s, line);
        }
	}
}

void mmt_nv_ioctl_post_open(UWord *args, SysRes res)
{
	const char *path = (const char *)args[0];

	if (mmt_trace_nvidia_ioctls)
	{
		if (VG_(strcmp)(path, "/dev/nvidiactl") == 0)
			FD_SET(res._val, &nvidiactl_fds);
		else if (VG_(strncmp)(path, "/dev/nvidia", 11) == 0)
			FD_SET(res._val, &nvidia0_fds);
	}
}

void mmt_nv_ioctl_post_close(UWord *args)
{
	int fd = (int)args[0];

	if (mmt_trace_nvidia_ioctls)
	{
		FD_CLR(fd, &nvidiactl_fds);
		FD_CLR(fd, &nvidia0_fds);
	}
}

int mmt_nv_ioctl_post_mmap(UWord *args, SysRes res, int offset_unit)
{
	Addr start = args[0];
	unsigned long len = args[1];
//	unsigned long prot = args[2];
//	unsigned long flags = args[3];
	unsigned long fd = args[4];
	unsigned long offset = args[5];
	struct mmt_mmap_data *region;
	struct mmt_mmap_data tmp;

	if (!mmt_trace_nvidia_ioctls)
		return 0;
	if (!FD_ISSET(fd, &nvidia0_fds))
		return 0;

	region = mmt_find_region_by_fd_offset(fd, offset * offset_unit);
	if (!region)
		return 0;

	tmp = *region;

	mmt_free_region(region);

	start = res._val;
	region = mmt_add_region(fd, start, start + len, tmp.offset, tmp.id, tmp.data1, tmp.data2);

	if (mmt_binary_output)
	{
		mmt_bin_write_1('n');
		mmt_bin_write_1('m');
		mmt_bin_write_8(region->offset);
		mmt_bin_write_4(region->id);
		mmt_bin_write_8(region->start);
		mmt_bin_write_8(len);
		mmt_bin_write_8(region->data1);
		mmt_bin_write_8(region->data2);
		mmt_bin_end();
	}
	else
		VG_(message) (Vg_DebugMsg,
				"got new mmap for 0x%08lx:0x%08lx at %p, len: 0x%08lx, offset: 0x%llx, serial: %d\n",
				region->data1, region->data2, (void *)region->start, len,
				region->offset, region->id);

	return 1;
}

static struct object_type {
	UInt id;		// type id
	const char *name;		// some name
	UInt cargs;		// number of constructor args (uint32)
} object_types[] =
{
	{0x0000, "NV_CONTEXT_NEW", 0},

	{0x0004, "NV_PTIMER", 0},

	{0x0041, "NV_CONTEXT", 0},

	{0x502d, "NV50_2D", 0},
	{0x902d, "NVC0_2D", 0},

	{0x5039, "NV50_M2MF", 0},
	{0x9039, "NVC0_M2MF", 0},

	{0x9068, "NVC0_PEEPHOLE", 0},

	{0x406e, "NV40_FIFO_DMA", 6},

	{0x506f, "NV50_FIFO_IB", 6},
	{0x826f, "NV84_FIFO_IB", 6},
	{0x906f, "NVC0_FIFO_IB", 6},

	{0x5070, "NV84_DISPLAY", 4},
	{0x8270, "NV84_DISPLAY", 4},
	{0x8370, "NVA0_DISPLAY", 4},
	{0x8870, "NV98_DISPLAY", 4},
	{0x8570, "NVA3_DISPLAY", 4},

	{0x5072, NULL, 8},

	{0x7476, "NV84_VP", 0},

	{0x507a, "NV50_DISPLAY_CURSOR", 0},
	{0x827a, "NV84_DISPLAY_CURSOR", 0},
	{0x857a, "NVA3_DISPLAY_CURSOR", 0},

	{0x507b, "NV50_DISPLAY_OVERLAY", 0},
	{0x827b, "NV84_DISPLAY_OVERLAY", 0},
	{0x857b, "NVA3_DISPLAY_OVERLAY", 0},

	{0x507c, "NV50_DISPLAY_SYNC_FIFO", 8},
	{0x827c, "NV84_DISPLAY_SYNC_FIFO", 8},
	{0x837c, "NVA0_DISPLAY_SYNC_FIFO", 8},
	{0x857c, "NVA3_DISPLAY_SYNC_FIFO", 8},

	{0x507d, "NV50_DISPLAY_MASTER_FIFO", 0},
	{0x827d, "NV84_DISPLAY_MASTER_FIFO", 0},
	{0x837d, "NVA0_DISPLAY_MASTER_FIFO", 0},
	{0x887d, "NV98_DISPLAY_MASTER_FIFO", 0},
	{0x857d, "NVA3_DISPLAY_MASTER_FIFO", 0},

	{0x307e, "NV30_PEEPHOLE", 0},

	{0x507e, "NV50_DISPLAY_OVERLAY_FIFO", 8},
	{0x827e, "NV84_DISPLAY_OVERLAY_FIFO", 8},
	{0x837e, "NVA0_DISPLAY_OVERLAY_FIFO", 8},
	{0x857e, "NVA3_DISPLAY_OVERLAY_FIFO", 8},

	{0x0080, "NV_DEVICE", 1},
	{0x2080, "NV_SUBDEVICE_0", 0},
	{0x2081, "NV_SUBDEVICE_1", 0},
	{0x2082, "NV_SUBDEVICE_2", 0},
	{0x2083, "NV_SUBDEVICE_3", 0},

	{0x5097, "NV50_3D", 0},
	{0x8297, "NV84_3D", 0},
	{0x8397, "NVA0_3D", 0},
	{0x8597, "NVA3_3D", 0},
	{0x8697, "NVAF_3D", 0},
	{0x9097, "NVC0_3D", 0},

	{0x74b0, "NV84_BSP", 0},

	{0x88b1, "NV98_BSP", 0},
	{0x85b1, "NVA3_BSP", 0},
	{0x86b1, "NVAF_BSP", 0},
	{0x90b1, "NVC0_BSP", 0},

	{0x88b2, "NV98_VP", 0},
	{0x85b2, "NVA3_VP", 0},
	{0x90b2, "NVC0_VP", 0},

	{0x88b3, "NV98_PPP", 0},
	{0x85b3, "NVA3_PPP", 0},
	{0x90b3, "NVC0_PPP", 0},

	{0x88b4, "NV98_CRYPT", 0},

	{0x85b5, "NVA3_COPY", 0},
	{0x90b5, "NVC0_COPY0", 0},

	{0x50c0, "NV50_COMPUTE", 0},
	{0x85c0, "NVA3_COMPUTE", 0},
	{0x90c0, "NVC0_COMPUTE", 0},

	{0x74c1, "NV84_CRYPT", 0},

	{0x50e0, "NV50_PGRAPH", 0},
	{0x50e2, "NV50_PFIFO", 0},
};

static struct object_type *find_objtype(UInt id)
{
	int i;
	int n = sizeof(object_types) / sizeof(struct object_type);

	for (i = 0; i < n; ++i)
		if (object_types[i].id == id)
			return &object_types[i];

	return NULL;
}

void mmt_nv_dump_call(char * s, uint32_t mthd, char *in, uint32_t in_size);
void mmt_nv_dump_call(char * s, uint32_t mthd, char *in, uint32_t in_size){
    dumpmem(s, (Addr)in, in_size);
    switch(mthd){
//     case 0x10000002:{
//         // word 2 is an address
//         // what is there?
//         UInt *addr2 = (*(UInt **) (&data[4]));
//         dumpmem("in2 ", addr2[2], 0x3c);
//         break;
//     }
    case NVRM_MTHD_DEVICE_UNK00800201:{
        struct nvrm_mthd_device_unk00800201 *x = in;
        if(x->ptr){
            dumpmem("UNK00800201 ptr ", (Addr)x->ptr, x->cnt * sizeof(uint32_t));
        }
        break;
    }
    case NVRM_MTHD_DEVICE_UNK00801102:{
        struct nvrm_mthd_device_unk00801102 *x = in;
        if(x->ptr){
            dumpmem("UNK00801102 ptr ", (Addr)x->ptr, x->cnt * sizeof(uint8_t));
        }
        break;
    }
    case NVRM_MTHD_DEVICE_UNK00801401:{
        struct nvrm_mthd_device_unk00801401 *x = in;
        if(x->ptr){
            dumpmem("UNK00801401 ptr ", (Addr)x->ptr, x->cnt * sizeof(uint8_t));
        }
        break;
    }
    case NVRM_MTHD_DEVICE_UNK0080170d:{
        struct nvrm_mthd_device_unk0080170d *x = in;
        if(x->ptr1){
            dumpmem("UNK0080170d ptr1 ", (Addr)x->ptr1, x->cnt * sizeof(uint32_t));
        }
        if(x->ptr2){
            dumpmem("UNK0080170d ptr2 ", (Addr)x->ptr2, x->cnt * sizeof(uint32_t));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_UNK20801301:{
        struct nvrm_mthd_subdevice_unk20801301 *x = in;
        if(x->ptr){
            dumpmem("UNK20801301 ptr ", (Addr)x->ptr, x->cnt * sizeof(uint64_t));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_UNK20800101:{
        struct nvrm_mthd_device_unk20800101 *x = in;
        if(x->ptr){
            dumpmem("UNK20800101 ptr ", (Addr)x->ptr, x->cnt * sizeof(uint64_t));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_GET_FIFO_ENGINES:{
        struct nvrm_mthd_subdevice_get_fifo_engines *x = in;
        if(x->ptr){
            dumpmem("SUBDEVICE_GET_FIFO_ENGINES ptr ", (Addr)x->ptr, x->cnt * sizeof(uint32_t));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_GET_FIFO_CLASSES:{
        struct nvrm_mthd_subdevice_get_fifo_classes *x = in;
        if(x->ptr){
            dumpmem("SUBDEVICE_GET_FIFO_CLASSES ptr ", (Addr)x->ptr, x->cnt * sizeof(uint32_t));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_GET_FIFO_JOINABLE_ENGINES:{
        struct nvrm_mthd_subdevice_get_fifo_joinable_engines *x = in;
        if(x->res){
            dumpmem("SUBDEVICE_GET_FIFO_JOINABLE_ENGINES ptr ", (Addr)x->res, x->cnt * sizeof(uint32_t));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_GET_UUID:{
        struct nvrm_mthd_subdevice_get_uuid *x = in;
        if(x->uuid){
            dumpmem("SUBDEVICE_GET_UUID ptr ", (Addr)x->uuid, x->uuid_len * sizeof(uint8_t));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_UNK20801201:{
        struct nvrm_mthd_subdevice_unk20801201 *x = in;
        if(x->ptr){
            dumpmem("UNK20801201 ptr ", (Addr)x->ptr, x->cnt*sizeof(struct nvrm_mthd_key_value));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_BUS_GET_PARAMS:{
        struct nvrm_mthd_subdevice_bus_get_params *x = in;
        if(x->ptr){
            dumpmem("SUBDEVICE_BUS_GET_PARAMS ptr ", (Addr)x->ptr, x->cnt*sizeof(struct nvrm_mthd_key_value));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_UNK20800122:{
        struct nvrm_mthd_subdevice_unk20800122    *x   = in;
        struct nvrm_mthd_subdevice_unk20800122_tx *txs = x->ptr;
        if (mmt_binary_output)
        {
            mmt_bin_write_1('n');
            mmt_bin_write_1('1');
            mmt_bin_write_4(x->cnt);
            mmt_bin_write_8((ULong)txs);
            mmt_bin_write_buffer((UChar *)txs, x->cnt * 8 * 4);
            mmt_bin_end();
        }
        else
        {
            dumpmem("UNK20800122: ", (Addr)x, sizeof(*x));
            uint32_t k;
            for (k = 0; k < x->cnt; ++k)
                dumpmem("UNK20800122 tx: ", (Addr)&txs[k], x->cnt * sizeof(*txs));
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_UNK2080100a:{
        struct nvrm_mthd_subdevice_unk2080100a *x = in;
        if(x->ptr){
            dumpmem("UNK2080100a ptr ", (Addr)x->ptr, x->cnt*16);
        }
        break;
    }
    case NVRM_MTHD_SUBDEVICE_UNK20802002:{
        struct nvrm_mthd_subdevice_unk20802002 *x = in;
        if(x->ptr){
            dumpmem("UNK20802002 ptr ", (Addr)x->ptr, 72);
        }
        break;
    }
    case NVRM_MTHD_DEVICE_UNK20802016:{
        struct nvrm_mthd_subdevice_unk20802016 *x = in;
        if(x->ptr){
            dumpmem("UNK20802016 ptr ", (Addr)x->ptr, x->cnt*16);
        }
        break;
    }
    // TODO:
    // case NVRM_MTHD_SUBDEVICE_UNK20800512:
    // case NVRM_MTHD_SUBDEVICE_UNK20800522:
    default:{
        break;
    }
    } // switch
}

void mmt_nv_ioctl_pre(UWord *args)
{
	int fd = args[0];
	UInt id = args[1];
	UInt *data = (UInt *) args[2];
	UWord addr;
	UInt obj1, obj2, size;
	int i;

	if (!FD_ISSET(fd, &nvidiactl_fds) && !FD_ISSET(fd, &nvidia0_fds))
		return;

	if (mmt_trace_marks) {
		char buf[50];
		VG_(snprintf)(buf, 50, "VG-%d-%d-PRE\n", VG_(getpid)(), trace_mark_cnt);
		VG_(write)(trace_mark_fd, buf, VG_(strlen)(buf));
		if (mmt_binary_output)
		{
			mmt_bin_write_1('n');
			mmt_bin_write_1('k');
			mmt_bin_write_str(buf);
			mmt_bin_end();
		}
		else
			VG_(message)(Vg_DebugMsg, "MARK: %s", buf);
	}

	if ((id & 0x0000FF00) == 0x4600)
	{
		size = (id & 0x3FFF0000) >> 16;
		if (mmt_binary_output)
		{
			mmt_bin_write_1('n');
			mmt_bin_write_1('i');
			mmt_bin_write_4(fd);
			mmt_bin_write_4(id);
			mmt_bin_write_buffer((UChar *)data, size);
			mmt_bin_end();
		}
		else
		{
			char line[4096];
			int idx = 0;

			VG_(sprintf) (line, "pre_ioctl: fd:%d, id:0x%02x (full:0x%x), data: ", fd, id & 0xFF, id);
			idx = VG_(strlen(line));

			for (i = 0; i < size / 4; ++i)
			{
				if (idx + 11 >= 4095)
					break;
				VG_(sprintf) (line + idx, "0x%08x ", data[i]);
				idx += 11;
			}
			VG_(message) (Vg_DebugMsg, "%s\n", line);
		}
	}
    else if(id==0x30000001)
    {
        VG_(message) (Vg_DebugMsg, "pre_ioctl: fd:%d,           (full:0x%x)\n", fd, id);
    }
    else if(id==0x00000001)
    {
        char line[4096];
        int idx = 0;
        VG_(sprintf) (line, "pre_ioctl: fd:%d,           (full:0x%x) data: ", fd, id);
        idx = VG_(strlen(line));
        for (i = 0; i < 24 / 4; ++i)
        {
            if (idx + 11 >= 4095)
                break;
            VG_(sprintf) (line + idx, "0x%08x ", data[i]);
            idx += 11;
        }
        VG_(message) (Vg_DebugMsg, "%s\n", line);
    }
	else
		VG_(message)(Vg_DebugMsg, "pre_ioctl, fd: %d, wrong id:0x%x\n", fd, id);

	switch (id)
	{
		// 0x23
		// c1d00041 5c000001 00000080 00000000 00000000 00000000 00000000 00000000
		// c1d0004a beef0003 000000ff 00000000 04fe8af8 00000000 00000000 00000000
		case 0xc0204623:
			obj1 = data[1];

			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('v');
				mmt_bin_write_4(obj1);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg, "create device object 0x%08x\n", obj1);

			// argument can be a string (7:0, indicating the bus number), but only if
			// argument is 0xff
			dumpmem("in ", data[4], 0x3C);

			break;
			// 0x37 read stuff from video ram?
			//case 0xc0204637:
		case 0xc0204638:
			dumpmem("in ", data[4], data[6]);
			break;
#if 1
		case 0xc0204637:
		{
			//UInt *addr2;
			dumpmem("in ", data[4], data[6]);

			//addr2 = (*(UInt **) (&data[4]));
			//if(data[2]==0x14c && addr2[2])
			//    dumpmem("in2 ", addr2[2], 0x40);
			break;
		}
#endif
		case 0xc020462a:
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('l');
				mmt_bin_write_4(data[1]);
				mmt_bin_write_4(data[2]);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg, "call method 0x%08x:0x%08x\n", data[1], data[2]);
            mmt_nv_dump_call("in ",data[2],(char*)mmt_2x4to8(data[5], data[4]), mmt_2x4to8(data[7], data[6]));
			break;

		case 0xc040464d:
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('4');
				mmt_bin_write_str(*(char **) (&data[6]));
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg, "in %s\n", *(char **) (&data[6]));
			break;
		case 0xc028465e:
		{
			// Copy data from mem to GPU
#if 0
			SysRes ff;
			ff = VG_(open) ("dump5e", O_CREAT | O_WRONLY | O_TRUNC, 0777);
			if (!ff.isError)
			{
				VG_(write) (ff.res, (void *) data[6], 0x01000000);
				VG_(close) (ff.res);
			}
#endif
			break;
		}
		case 0xc0104629:
			obj1 = data[1];
			obj2 = data[2];
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('d');
				mmt_bin_write_4(obj1);
				mmt_bin_write_4(obj2);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg, "destroy object 0x%08x:0x%08x\n", obj1, obj2);
			break;
		case 0xc020462b:
		{
			struct object_type *objtype;
			const char *name = "???";
			obj1 = data[1];
			obj2 = data[2];
			addr = data[3];
			objtype = find_objtype(addr);
			if (objtype && objtype->name)
				name = objtype->name;

			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('c');
				mmt_bin_write_4(obj1);
				mmt_bin_write_4(obj2);
				mmt_bin_write_4(addr);
				mmt_bin_write_str(name);
				mmt_bin_end();
			}
			else
			{
				VG_(message) (Vg_DebugMsg,
						"create gpu object 0x%08x:0x%08x type 0x%04lx (%s)\n",
						obj1, obj2, addr, name);
			}

			if (data[4])
			{
				if (objtype)
					dumpmem("in ", mmt_2x4to8(data[5], data[4]), objtype->cargs * 4);
				else
					dumpmem("in ", mmt_2x4to8(data[5], data[4]), 0x40);
			}

			break;
		}
		case 0xc014462d:
			obj1 = data[1];
			obj2 = data[2];
			addr = data[3];
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('r');
				mmt_bin_write_4(obj1);
				mmt_bin_write_4(obj2);
				mmt_bin_write_8(addr);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg,
						"create driver object 0x%08x:0x%08x type 0x%04lx\n", obj1, obj2, addr);
			break;

	}
}

void mmt_nv_ioctl_post(UWord *args)
{
	int fd = args[0];
	UInt id = args[1];
	UInt *data = (UInt *) args[2];
	Off64T addr;
	UInt obj1, obj2, size, type;
	int i;
	struct mmt_mmap_data *region;

	if (!FD_ISSET(fd, &nvidiactl_fds) && !FD_ISSET(fd, &nvidia0_fds))
		return;

	if (mmt_trace_marks) {
		char buf[50];
		VG_(snprintf)(buf, 50, "VG-%d-%d-POST\n", VG_(getpid)(), trace_mark_cnt++);
		VG_(write)(trace_mark_fd, buf, VG_(strlen)(buf));
		if (mmt_binary_output)
		{
			mmt_bin_write_1('n');
			mmt_bin_write_1('k');
			mmt_bin_write_str(buf);
			mmt_bin_end();
		}
		else
			VG_(message)(Vg_DebugMsg, "MARK: %s", buf);
	}

	if ((id & 0x0000FF00) == 0x4600)
	{
		size = (id & 0x3FFF0000) >> 16;

		if (mmt_binary_output)
		{
			mmt_bin_write_1('n');
			mmt_bin_write_1('j');
			mmt_bin_write_4(fd);
			mmt_bin_write_4(id);
			mmt_bin_write_buffer((UChar *)data, size);
			mmt_bin_end();
		}
		else
		{
			char line[4096];
			int idx = 0;

			VG_(sprintf) (line, "post_ioctl: fd:%d, id:0x%02x (full:0x%x), data: ", fd, id & 0xFF, id);
			idx = VG_(strlen(line));

			for (i = 0; i < size / 4; ++i)
			{
				if (idx + 11 >= 4095)
					break;
				VG_(sprintf) (line + idx, "0x%08x ", data[i]);
				idx += 11;
			}
			VG_(message) (Vg_DebugMsg, "%s\n", line);
		}
	}
    else if(id==0x30000001)
    {
        VG_(message) (Vg_DebugMsg, "post_ioctl: fd:%d,           (full:0x%x)\n", fd, id);
    }
    else if(id==0x00000001)
    {
        char line[4096];
        int idx = 0;
        VG_(sprintf) (line, "post_ioctl: fd:%d,           (full:0x%x) data: ", fd, id);
        idx = VG_(strlen(line));
        for (i = 0; i < 24 / 4; ++i)
        {
            if (idx + 11 >= 4095)
                break;
            VG_(sprintf) (line + idx, "0x%08x ", data[i]);
            idx += 11;
        }
        VG_(message) (Vg_DebugMsg, "%s\n", line);
    }
	else
		VG_(message)(Vg_DebugMsg, "post_ioctl, fd: %d, wrong id:0x%x\n", fd, id);

	switch (id)
	{
		// NVIDIA
		case 0xc00c4622:		// Initialize
			obj1 = data[0];
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('x');
				mmt_bin_write_4(obj1);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg, "created context object 0x%08x\n", obj1);
			break;

		case 0xc0204623:
			dumpmem("out", data[4], 0x3C);
			break;

		case 0xc030464e:		// Allocate map for existing object
			obj1 = data[1];
			obj2 = data[2];
			addr = (((Off64T)data[9]) << 32) | data[8];

			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('a');
				mmt_bin_write_4(obj1);
				mmt_bin_write_4(obj2);
				mmt_bin_write_8(addr);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg, "allocate map 0x%08x:0x%08x 0x%08llx\n", obj1, obj2, addr);

			region = get_nvidia_mapping(addr);
			if (region)
			{
				region->data1 = obj1;
				region->data2 = obj2;
			}

			break;
		case 0xc020464f:		// Deallocate map for existing object
			obj1 = data[1];
			obj2 = data[2];
			addr = (((Off64T)data[5]) << 32) | data[4];
			/// XXX some currently mapped memory might be orphaned

			if (release_nvidia_mapping(addr))
			{
				if (mmt_binary_output)
				{
					mmt_bin_write_1('n');
					mmt_bin_write_1('e');
					mmt_bin_write_4(obj1);
					mmt_bin_write_4(obj2);
					mmt_bin_write_8(addr);
					mmt_bin_end();
				}
				else
					VG_(message) (Vg_DebugMsg, "deallocate map 0x%08x:0x%08x 0x%08llx\n", obj1, obj2, addr);
			}

			break;
		case 0xc0304627:		// 0x27 Allocate map (also create object)
			obj1 = data[1];
			obj2 = data[2];
			type = data[3];
			addr = (((Off64T)data[7]) << 32) | data[6];
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('p');
				mmt_bin_write_4(obj1);
				mmt_bin_write_4(obj2);
				mmt_bin_write_4(type);
				mmt_bin_write_8(addr);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg,
						"create mapped object 0x%08x:0x%08x type=0x%08x 0x%08llx\n",
						obj1, obj2, type, addr);
			if (addr == 0)
				break;

			region = get_nvidia_mapping(addr);
			if (region)
			{
				region->data1 = obj1;
				region->data2 = obj2;
			}
#if 0
			dumpmem("out ", data[2], 0x40);
#endif
			break;
			// 0x29 seems to destroy/deallocate
		case 0xc0104629:
			obj1 = data[1];
			obj2 = data[2];
			/// XXX some currently mapped memory might be orphaned

			{
				Addr addr1 = release_nvidia_mapping2(obj1, obj2);
				if ((void *)addr1 != NULL)
				{
					if (mmt_binary_output)
					{
						mmt_bin_write_1('n');
						mmt_bin_write_1('e');
						mmt_bin_write_4(obj1);
						mmt_bin_write_4(obj2);
						mmt_bin_write_8(addr1);
						mmt_bin_end();
					}
					else
						VG_(message) (Vg_DebugMsg, "deallocate map 0x%08x:0x%08x %p\n",
								obj1, obj2, (void *)addr1);
				}
			}
			break;
			// 0x2a read stuff from video ram?
			//   i 3 pre  2a: c1d00046 c1d00046 02000014 00000000 be88a948 00000000 00000080 00000000
		case 0xc020462a:
            mmt_nv_dump_call("out ",data[2],(char*)mmt_2x4to8(data[5], data[4]), mmt_2x4to8(data[7], data[6]));
			break;
			// 0x37 read configuration parameter
		case 0xc0204638:
			dumpmem("out", data[4], data[6]);
			break;
		case 0xc0204637:
			{
				UInt *addr2 = (*(UInt **) (&data[4]));
				dumpmem("out", data[4], data[6]);
				if (data[2] == 0x14c && addr2[2])
					/// List supported object types
					dumpmem("out2 ", addr2[2], addr2[0] * 4);
			}
			break;
		case 0xc0384657:		// map GPU address
			addr = (((Off64T)data[11]) << 32) | data[10];
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('G');
				mmt_bin_write_4(data[1]);
				mmt_bin_write_4(data[2]);
				mmt_bin_write_4(data[3]);
				mmt_bin_write_8(addr);
				mmt_bin_write_4(data[6]);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg,
						"gpu map 0x%08x:0x%08x:0x%08x, addr 0x%08llx, len 0x%08x\n",
						data[1], data[2], data[3], addr, data[6]);
			break;
		case 0xc0284658:		// unmap GPU address
			addr = (((Off64T)data[7]) << 32) | data[6];
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('H');
				mmt_bin_write_4(data[1]);
				mmt_bin_write_4(data[2]);
				mmt_bin_write_4(data[3]);
				mmt_bin_write_8(addr);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg,
						"gpu unmap 0x%08x:0x%08x:0x%08x addr 0x%08llx\n", data[1],
						data[2], data[3], addr);
			break;
		case 0xc0304654:		// create DMA object [3] is some kind of flags, [6] is an offset?
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('t');
				mmt_bin_write_4(data[1]);
				mmt_bin_write_4(data[2]);
				mmt_bin_write_4(data[5]);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg,
						"create dma object 0x%08x, type 0x%08x, parent 0x%08x\n",
						data[1], data[2], data[5]);
			break;
		case 0xc0104659:		// bind
			if (mmt_binary_output)
			{
				mmt_bin_write_1('n');
				mmt_bin_write_1('b');
				mmt_bin_write_4(data[1]);
				mmt_bin_write_4(data[2]);
				mmt_bin_end();
			}
			else
				VG_(message) (Vg_DebugMsg, "bind 0x%08x 0x%08x\n", data[1], data[2]);
			break;
		//case 0xc01c4634:
			//    dumpmem("out", data[4], 0x40);
			//    break;
			//   to c1d00046 c1d00046 02000014 00000000, from be88a948 00000000, size 00000080 00000000
			//            2b: c1d00046 5c000001 5c000009 0000506f be88a888 00000000 00000000 00000000
			//   same, but other way around?
			//   i 5 pre  37: c1d00046 5c000001 0000014c 00000000 be88a9c8 00000000 00000010 00000000

			// 0x23 create first object??
			// 0x2a method call? args/in/out depend
			// 0x2b object creation
			//      c1d00046 beef0003 beef0028 0000307e
			// 0x32 gets some value
			// 0x37 read from GPU object? seems a read, not a write
			// 0x4a memory allocation
			// 0x4d after opening /dev/nvidiaX
			// 0xd2 version id check
			// 0x22 initialize (get context)
			// 0x54 bind? 0xc0304654
			// 0x57 map to card  0xc0384657
			// 0x58 unmap from card 0xc0284658
			// 0xca ??

			// These have external pointer:
			// 0x2a (offset 0x10, size 0x18)
			// 0x2b (offset 0x10, no size specified)
			// 0x37 (offset 0x10, size 0x18)
			// 0x38 (offset 0x10, size 0x18)
	}
}

void mmt_nv_ioctl_pre_clo_init(void)
{
	FD_ZERO(&nvidiactl_fds);
	FD_ZERO(&nvidia0_fds);
}
