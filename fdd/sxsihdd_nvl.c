#include	"compiler.h"

#ifdef SUPPORT_NVL_IMAGES

#include	"strres.h"
#include	"dosio.h"
#include	"sysmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"sxsi.h"


typedef void *sxsihdd_nvl_1(LPCTSTR path, BOOL ro);
typedef void sxsihdd_nvl_2(void *pv);
typedef void sxsihdd_nvl_3(void *pv, UINT32 *a);
typedef BOOL sxsihdd_nvl_4(void *pv, INT64 p, UINT32 s, void *b);
typedef BOOL sxsihdd_nvl_5(void *pv, INT64 p, UINT32 s, const void *b);


typedef struct _sxsihdd_nvl
{
	HMODULE hModule;
	sxsihdd_nvl_1 *f1;
	sxsihdd_nvl_2 *f2;
	sxsihdd_nvl_3 *f3;
	sxsihdd_nvl_4 *f4;
	sxsihdd_nvl_5 *f5;
	void *pv;
} sxsihdd_nvl;


static void nvl_close(sxsihdd_nvl *p)
{
	if (p == NULL)
	{
		return;
	}

	if (p->pv != NULL)
	{
		(*p->f2)(p->pv);
	}

	if (p->hModule != NULL)
	{
		FreeLibrary(p->hModule);
	}

	_MFREE(p);
}


static sxsihdd_nvl *nvl_open(const OEMCHAR *fname)
{
	sxsihdd_nvl *p = NULL;
	HMODULE hModule = NULL;

	p = (sxsihdd_nvl*)_MALLOC(sizeof(sxsihdd_nvl), "sxsihdd_nvl_open");
	if (p == NULL)
	{
		goto sxsiope_err;
	}

	p->hModule = NULL;
	p->pv = NULL;

	p->hModule = LoadLibrary(_T("NVL.DLL"));
	if (p->hModule == NULL)
	{
		goto sxsiope_err;
	}

	p->f1 = (sxsihdd_nvl_1 *)GetProcAddress(p->hModule, MAKEINTRESOURCEA(1));
	p->f2 = (sxsihdd_nvl_2 *)GetProcAddress(p->hModule, MAKEINTRESOURCEA(2));
	p->f3 = (sxsihdd_nvl_3 *)GetProcAddress(p->hModule, MAKEINTRESOURCEA(3));
	p->f4 = (sxsihdd_nvl_4 *)GetProcAddress(p->hModule, MAKEINTRESOURCEA(4));
	p->f5 = (sxsihdd_nvl_5 *)GetProcAddress(p->hModule, MAKEINTRESOURCEA(5));

	p->pv = (*p->f1)(fname, FALSE);
	if (p->pv == NULL)
	{
		goto sxsiope_err;
	}

	return (p);

sxsiope_err:
	if (p != NULL)
	{
		nvl_close(p);
	}

	return (NULL);
}


static BRESULT hdd_reopen(SXSIDEV sxsi)
{
	sxsihdd_nvl *p = NULL;

	p = nvl_open(sxsi->fname);
	if (p == NULL)
	{
		return (FAILURE);
	}

	sxsi->hdl = (INTPTR)p;
	return (SUCCESS);
}


static REG8 hdd_read(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size)
{
	sxsihdd_nvl *p = (sxsihdd_nvl *)sxsi->hdl;

	if (sxsi_prepare(sxsi) != SUCCESS)
	{
		return (0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals))
	{
		return (0x40);
	}

	pos = pos * sxsi->size;

	while (size)
	{
		UINT rsize;

		rsize = min(size, sxsi->size);
		CPU_REMCLOCK -= rsize;

		if (!(*p->f4)(p->pv, pos, rsize, buf))
		{
			return (0xd0);
		}

		buf += rsize;
		size -= rsize;
		pos += rsize;
	}

	return (0x00);
}


static REG8 hdd_write(SXSIDEV sxsi, FILEPOS pos, const UINT8 *buf, UINT size)
{
	sxsihdd_nvl *p = (sxsihdd_nvl *)sxsi->hdl;

	if (sxsi_prepare(sxsi) != SUCCESS)
	{
		return (0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals))
	{
		return (0x40);
	}

	pos = pos * sxsi->size;

	while (size)
	{
		UINT wsize;

		wsize = min(size, sxsi->size);
		CPU_REMCLOCK -= wsize;

		if (!(*p->f5)(p->pv, pos, wsize, buf))
		{
			return (0x70);
		}

		buf += wsize;
		size -= wsize;
		pos += wsize;
	}

	return (0x00);
}


static REG8 hdd_format(SXSIDEV sxsi, FILEPOS pos)
{
	sxsihdd_nvl *p = (sxsihdd_nvl *)sxsi->hdl;
	UINT16 i;
	UINT8 work[256];

	if (sxsi_prepare(sxsi) != SUCCESS)
	{
		return (0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals))
	{
		return (0x40);
	}

	pos = pos * sxsi->size;

	FillMemory(work, sizeof(work), 0xe5);
	for (i = 0; i < sxsi->sectors; i++)
	{
		UINT size;

		size = sxsi->size;
		while (size)
		{
			UINT wsize;

			wsize = min(size, sizeof(work));
			size -= wsize;
			CPU_REMCLOCK -= wsize;

			if (!(*p->f5)(p->pv, pos, wsize, work))
			{
				return (0x70);
			}

			pos += wsize;
		}
	}

	return (0x00);
}


static void hdd_close(SXSIDEV sxsi)
{
	sxsihdd_nvl *p = (sxsihdd_nvl *)sxsi->hdl;

	nvl_close(p);
}


static UINT8 gethddtype(SXSIDEV sxsi)
{
	const SASIHDD *sasi;
	UINT i;

	if (sxsi->size == 256)
	{
		sasi = sasihdd;
		for (i = 0; i < NELEMENTS(sasihdd); i++, sasi++)
		{
			if ((sxsi->sectors == sasi->sectors) &&
				(sxsi->surfaces == sasi->surfaces) &&
				(sxsi->cylinders == sasi->cylinders))
			{
				return ((UINT8)i);
			}
		}
	}
	return (SXSIMEDIA_INVSASI + 7);
}


BRESULT sxsihdd_nvl_open(SXSIDEV sxsi, const OEMCHAR *fname)
{
	sxsihdd_nvl *p = NULL;
	UINT32 a[4];

	p = nvl_open(fname);
	if (p == NULL)
	{
		goto sxsiope_err;
	}

	(*p->f3)(p->pv, a);

	sxsi->reopen = hdd_reopen;
	sxsi->read = hdd_read;
	sxsi->write = hdd_write;
	sxsi->format = hdd_format;
	sxsi->close = hdd_close;

	sxsi->hdl = (INTPTR)p;
	sxsi->totals = a[0];
	sxsi->cylinders = (UINT16)(a[0] / (a[2] * a[1]));
	sxsi->size = (UINT16)a[3];
	sxsi->sectors = (UINT8)a[2];
	sxsi->surfaces = (UINT8)a[1];
	sxsi->headersize = 0;
	sxsi->mediatype = gethddtype(sxsi);

	return (SUCCESS);

sxsiope_err:
	if (p != NULL)
	{
		nvl_close(p);
	}

	return (FAILURE);
}

#endif