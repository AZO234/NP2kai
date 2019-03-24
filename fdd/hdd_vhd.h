/*
    a minimal support for VirtualPC VHD images
    (fixed disk image only, for now...)
    
    author: lpproj
    license: same as xnp2 (under the 2-clause BSD)
*/
#ifndef HDD_VPCVHD_H
//#include "compiler.h"
//#include "strres.h"
//#include "dosio.h"
//#include "sysmng.h"
//#include "cpucore.h"
//#include "pccore.h"
//#include "sxsi.h"
//#include "hdd_vpc.h"
#endif

#define LOADBEWORD LOADMOTOROLAWORD
#define LOADBEDWORD LOADMOTOROLADWORD
#define LOADBEQWORD LOADMOTOROLAQWORD
#define STOREBEWORD STOREMOTOROLAWORD
#define STOREBEDWORD STOREMOTOROLADWORD
#define STOREBEQWORD STOREMOTOROLAQWORD

static const UINT8 CookieVPCVHDFooter[8] = { 'c','o','n','e','c','t','i','x' };
static const UINT8 CookieVPCVHDDDH[8] = { 'c','x','s','p','a','r','s','e' };
static const UINT8 CookieVPCQEMUCreator[4] = { 'q','e','m','u' };

/* load/store with "network order" (big endian) */

#if 0
static UINT16 LOADBEWORD(const void *p)
{
  const UINT8 *b = (UINT8 *)p;
  return( ((UINT16)(b[0])<<8) | b[1] );
}
static UINT32 LOADBEDWORD(const void *p)
{
  const UINT8 *b = (UINT8 *)p;
  return( ((UINT32)(b[0])<<24) | ((UINT32)(b[1])<<16) | ((UINT32)(b[2])<<8) | b[3] );
}
static UINT64 LOADBEQWORD(const void *p)
{
  const UINT8 *b = (UINT8 *)p;
  return( ((UINT64)(b[0])<<56) | ((UINT64)(b[1])<<48) | ((UINT64)(b[2])<<40) | ((UINT64)(b[3])<<32) | ((UINT32)(b[4])<<24) | ((UINT32)(b[5])<<16) | ((UINT32)(b[6])<<8) | b[7] );
}

static UINT16 STOREBEWORD(void *p, UINT16 v)
{
  UINT8 *b = (UINT8 *)p;
  b[0] = (UINT8)(v>>8);
  b[1] = (UINT8)v;
  return(v);
}
static UINT32 STOREBEDWORD(void *p, UINT32 v)
{
  UINT8 *b = (UINT8 *)p;
  b[0] = (UINT8)(v>>24);
  b[1] = (UINT8)(v>>16);
  b[2] = (UINT8)(v>>8);
  b[3] = (UINT8)v;
  return(v);
}
static UINT64 STOREBEQWORD(void *p, UINT64 v)
{
  UINT8 *b = (UINT8 *)p;
  b[0] = (UINT8)(v>>56);
  b[1] = (UINT8)(v>>48);
  b[2] = (UINT8)(v>>40);
  b[3] = (UINT8)(v>>32);
  b[4] = (UINT8)(v>>24);
  b[5] = (UINT8)(v>>16);
  b[6] = (UINT8)(v>>8);
  b[7] = (UINT8)v;
  return(v);
}
#endif

/* checksum adapted from http://superuser.com/a/434816 */
UINT32 vpc_calc_checksum(UINT8* buf, size_t size)
{
    UINT32 res = 0;
    size_t i;

    for (i = 0; i < size; i++)
        res += buf[i];

    return ~res;
}

BRESULT sxsihdd_vpcvhd_mount(SXSIDEV sxsi, FILEH fh)
{
	VPCVHDFOOTER footer;
	VPCVHD_FPOS vhdlen, readlen;
	size_t footerlen;

	UINT32 disktype, formatversion, checksum, mychecksum;
	UINT32 surfaces, cylinders, sectors;
	UINT64 totals;

	footerlen = sizeof(footer);		/* 512 */
	ZeroMemory(&footer, footerlen);
	vhdlen = file_seek(fh, 0, FSEEK_END);
	if (vhdlen == (VPCVHD_FPOS)-1 || (vhdlen >= 0 && vhdlen < (VPCVHD_FPOS)footerlen))
		return(FAILURE);
	file_seek(fh, (FILEPOS)(vhdlen - footerlen), FSEEK_SET);
	readlen = file_read(fh, &footer, sizeof(footer.Cookie));
	if (memcmp(footer.Cookie, CookieVPCVHDFooter, sizeof(footer.Cookie)) != 0) {
		CopyMemory(&(footer.Cookie[0]), &(footer.Cookie[1]), sizeof(footer.Cookie)-1);
		file_read(fh, &(footer.Cookie[sizeof(footer.Cookie)-1]), 1);
		--footerlen;
	}
	if (memcmp(footer.Cookie, CookieVPCVHDFooter, sizeof(footer.Cookie)) != 0)
		return(FAILURE);
	readlen = file_read(fh, (UINT8 *)(&footer) + sizeof(footer.Cookie), (UINT)(footerlen - sizeof(footer.Cookie)));
	if (readlen < (VPCVHD_FPOS)(footerlen - sizeof(footer.Cookie)))
		return(FAILURE);

	// validate footer's checksum
	checksum = LOADBEDWORD(footer.CheckSum);
	STOREBEDWORD(footer.CheckSum, 0);
	mychecksum = vpc_calc_checksum((UINT8*)&footer,footerlen);
	if (mychecksum != checksum) {
		TRACEOUT(("vpc_vhd: bad checksum (file=%x, calc=%x)",checksum,mychecksum));
		return(FAILURE);
	}

	formatversion = LOADBEDWORD(footer.FileFormatVersion);
	disktype = LOADBEDWORD(footer.DiskType);
	sectors = footer.SectorsPerCylinder;
	surfaces = footer.Heads;
	cylinders = LOADBEWORD(footer.Cylinder);
	if (formatversion != 0x00010000 || disktype != VPCVHD_DISK_FIXED) {
		/* todo: support dynamic disk */
		TRACEOUT(("vpc_vhd: unsupported vhd image"));
		return(FAILURE);
	}
	totals = (UINT64)sectors * surfaces * cylinders;
	if (totals == 0)
		return(FAILURE);

	/* QEMU hack */
	if (!memcmp(footer.CreatorApplication, CookieVPCQEMUCreator, sizeof(footer.CreatorApplication)) && (totals * 512U + footerlen > (UINT64)vhdlen)) {
		--cylinders;
		totals = (UINT64)sectors * surfaces * cylinders;
	}

	if (disktype == VPCVHD_DISK_FIXED) {
		if (totals * 512U + footerlen > (UINT64)vhdlen)
			return(FALSE);
#if defined(SUPPORT_LARGE_HDD)
		sxsi->totals = totals;
#else
		sxsi->totals = (long)totals;
		if ((UINT64)(sxsi->totals) != totals) {	/* overflow? */
			sxsi->totals = 0;
			return(FALSE);
		}
#endif
		/* the simplest way: reuse built-in methods for fixed image */
		sxsi->reopen = hdd_reopen;
		sxsi->read = hdd_read;
		sxsi->write = hdd_write;
		sxsi->format = hdd_format;
		sxsi->close = hdd_close;

		sxsi->hdl = (INTPTR)fh;
		sxsi->cylinders = (UINT16)cylinders;
		sxsi->size = 512;				/* 512 only */
		sxsi->sectors = (UINT8)sectors;
		sxsi->surfaces = (UINT8)surfaces;
		sxsi->headersize = 0;			/* footer only in the fixed image */
		sxsi->mediatype = gethddtype(sxsi);
		file_seek(fh, 0, FSEEK_SET);	/* rewind the pointer, for a proof */
		TRACEOUT(("vpc_vhd: vhd fixed image mounted (c=%u h=%u s=%u %luMbytes)", (unsigned)cylinders, (unsigned)surfaces, (unsigned)sectors, (unsigned long)(totals /2U / 1024U)));
		return(SUCCESS);
	}
	return(FAILURE);
}

