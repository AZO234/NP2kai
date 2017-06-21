/*
 * Copyright (c) 2003 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"

#include "cpu.h"
#include "pccore.h"
#include "iocore.h"
#include "cpumem.h"

static void CPUCALL check_io(UINT port, UINT len);

static void CPUCALL
check_io(UINT port, UINT len) 
{
	UINT off;
	UINT16 map;
	UINT16 mask;

	if (CPU_STAT_IOLIMIT == 0) {
		VERBOSE(("check_io: CPU_STAT_IOLIMIT == 0 (port = %04x, len = %d)", port, len));
		EXCEPTION(GP_EXCEPTION, 0);
	}

	if ((port + len) / 8 >= CPU_STAT_IOLIMIT) {
		VERBOSE(("check_io: out of range: CPU_STAT_IOLIMIT(%08x) (port = %04x, len = %d)", CPU_STAT_IOLIMIT, port, len));
		EXCEPTION(GP_EXCEPTION, 0);
	}

	off = port / 8;
	mask = ((1 << len) - 1) << (port % 8);
	map = cpu_kmemoryread_w(CPU_STAT_IOADDR + off);
	if (map & mask) {
		VERBOSE(("check_io: (bitmap(0x%04x) & bit(0x%04x)) != 0 (CPU_STAT_IOADDR=0x%08x, offset=0x%04x, port = 0x%04x, len = %d)", map, mask, CPU_STAT_IOADDR, off, port, len));
		EXCEPTION(GP_EXCEPTION, 0);
	}
}

UINT8 IOINPCALL
cpu_in(UINT port)
{

	if (CPU_STAT_PM && (CPU_STAT_VM86 || (CPU_STAT_CPL > CPU_STAT_IOPL))) {
		check_io(port, 1);
	}
	return iocore_inp8(port);
}

UINT16 IOINPCALL
cpu_in_w(UINT port)
{

	if (CPU_STAT_PM && (CPU_STAT_VM86 || (CPU_STAT_CPL > CPU_STAT_IOPL))) {
		check_io(port, 2);
	}
	return iocore_inp16(port);
}

UINT32 IOINPCALL
cpu_in_d(UINT port)
{

	if (CPU_STAT_PM && (CPU_STAT_VM86 || (CPU_STAT_CPL > CPU_STAT_IOPL))) {
		check_io(port, 4);
	}
	return iocore_inp32(port);
}

void IOOUTCALL
cpu_out(UINT port, UINT8 data)
{

	if (CPU_STAT_PM && (CPU_STAT_VM86 || (CPU_STAT_CPL > CPU_STAT_IOPL))) {
		check_io(port, 1);
	}
	iocore_out8(port, data);
}

void IOOUTCALL
cpu_out_w(UINT port, UINT16 data)
{

	if (CPU_STAT_PM && (CPU_STAT_VM86 || (CPU_STAT_CPL > CPU_STAT_IOPL))) {
		check_io(port, 2);
	}
	iocore_out16(port, data);
}

void IOOUTCALL
cpu_out_d(UINT port, UINT32 data)
{

	if (CPU_STAT_PM && (CPU_STAT_VM86 || (CPU_STAT_CPL > CPU_STAT_IOPL))) {
		check_io(port, 4);
	}
	iocore_out32(port, data);
}
