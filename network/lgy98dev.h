/*
 * QEMU NE2000 emulation
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
	
#define NE2000_PMEM_SIZE    (32*1024)
#define NE2000_PMEM_START   (16*1024)
#define NE2000_PMEM_END     (NE2000_PMEM_SIZE+NE2000_PMEM_START)
#define NE2000_MEM_SIZE     NE2000_PMEM_END
	
typedef void IOReadHandler(void *opaque, const UINT8 *buf, int size);
typedef int IOCanRWHandler(void *opaque);
//typedef ssize_t (IOReadvHandler)(void *, const struct iovec *, int);
typedef void (NetCleanup) (struct tagVLANClientState *);
typedef void (LinkStatusChanged)(struct tagVLANClientState *);

struct tagVLANState {
    int id;
    struct tagVLANClientState *first_client;
    struct tagVLANState *next;
    unsigned int nb_guest_devs, nb_host_devs;
};

struct tagVLANClientState{
    IOReadHandler *fd_read;
    //LGY98_IOReadvHandler *fd_readv;
    /* Packets may still be sent if this returns zero.  It's used to
       rate-limit the slirp code.  */
    IOCanRWHandler *fd_can_read;
    NetCleanup *cleanup;
    LinkStatusChanged *link_status_changed;
    int link_down;
    void *opaque;
    struct tagVLANClientState *next;
    struct tagVLANState *vlan;
    char *model;
    char *name;
    char info_str[256];
};

typedef struct tagVLANState VLANState;
typedef struct tagVLANClientState VLANClientState;

typedef struct {
	UINT16	base;
	//REG8	macaddr[6];
	REG8	irq;
	
    REG8 cmd;
    UINT32 start;
    UINT32 stop;
    REG8 boundary;
    REG8 tsr; //
    REG8 tpsr; //
    UINT16 tcnt; //
    UINT16 rcnt; //
    UINT32 rsar; //
    REG8 rsr;
    REG8 rxcr;
    REG8 isr;
    REG8 dcfg;
    REG8 imr;
    REG8 phys[6]; /* mac address */
    REG8 curpag;
    REG8 mult[8]; /* multicast mask array */
    //qemu_irq irq;
    int isa_io_base;
    //PCIDevice *pci_dev;
    REG8 macaddr[6];
    REG8 mem[NE2000_MEM_SIZE];
	/*
	REG8	cmd;
	REG8	isr;
	REG8	tpsr;
	UINT16	rcnt;
	UINT16	tcnt;

	VLANClientState *vc;*/
} LGY98;

extern LGY98	lgy98;

#ifdef __cplusplus
}
#endif

