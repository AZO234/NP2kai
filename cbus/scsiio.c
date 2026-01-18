#include	<compiler.h>

#if defined(SUPPORT_SCSI)

#include	<dosio.h>
#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<cbus/cbuscore.h>
#include	<cbus/scsiio.h>
#include	"scsiio.tbl"
#include	<cbus/scsicmd.h>
#include	"scsibios.res"
#if defined(SUPPORT_IA32_HAXM)
#include	<i386hax/haxfunc.h>
#include	<i386hax/haxcore.h>
#endif
#include <fdd/sxsi.h>


	_SCSIIO		scsiio;

static const UINT8 scsiirq[] = {0x03, 0x05, 0x06, 0x09, 0x0c, 0x0d, 3, 3};


void scsiioint(NEVENTITEM item) {

	TRACEOUT(("scsiioint"));
	if (scsiio.membank & 4) {
		pic_setirq(scsiirq[(scsiio.resent >> 3) & 7]);
		TRACEOUT(("scsi intr"));
	}
	scsiio.auxstatus = 0x80;
	(void)item;
}

static void scsiintr(REG8 status) {

	scsiio.scsistatus = status;
	nevent_set(NEVENT_SCSIIO, 4000, scsiioint, NEVENT_ABSOLUTE);
}

static void scsicmd(REG8 cmd) {

	REG8	ret;
	UINT8	id;

	id = scsiio.reg[SCSICTR_DSTID] & 7;
	switch(cmd) {
		case SCSICMD_RESET:
			scsiintr(SCSISTAT_RESET);
			break;

		case SCSICMD_NEGATE:
			ret = scsicmd_negate(id);
			scsiintr(ret);
			break;

		case SCSICMD_SEL:
			ret = scsicmd_select(id);
			if (ret & 0x80) {
				scsiintr(0x11);
				// で retはどーやって割り込みさせるの？
			}
			else {
				scsiintr(ret);
			}
			break;

		case SCSICMD_SEL_TR:
			ret = scsicmd_transfer(id, scsiio.reg + SCSICTR_CDB);
			if (ret != 0xff) {
				scsiintr(ret);
			}
			break;
	}
}



// ----

static void IOOUTCALL scsiio_occ0(UINT port, REG8 dat) {

	scsiio.port = dat;
	(void)port;
}

static void IOOUTCALL scsiio_occ2(UINT port, REG8 dat) {

	UINT8	bit;

	if (scsiio.port < 0x40) {
		TRACEOUT(("scsi ctrl write %s %.2x", scsictr[scsiio.port], dat));
	}
	if (scsiio.port <= 0x19) {
		scsiio.reg[scsiio.port] = dat;
		if (scsiio.port == SCSICTR_CMD) {
			scsicmd(dat);
		}
		scsiio.port++;
	}
	else {
		switch(scsiio.port) {
			case SCSICTR_MEMBANK:
				scsiio.membank = dat;
				if (!(dat & 0x40)) {
					CopyMemory(mem + 0xd2000, scsiio.bios[0], 0x2000);
				}
				else {
					CopyMemory(mem + 0xd2000, scsiio.bios[1], 0x2000);
				}
				break;

			case 0x3f:
				bit = 1 << (dat & 7);
				if (dat & 8) {
					scsiio.datmap |= bit;
				}
				else {
					if (scsiio.datmap & bit) {
						scsiio.datmap &= ~bit;
						if (bit == (1 << 1)) {
							scsiio.wrdatpos = 0;
						}
						else if (bit == (1 << 5)) {
							scsiio.rddatpos = 0;
						}
					}
				}
				break;
		}
	}
	(void)port;
}

static void IOOUTCALL scsiio_occ4(UINT port, REG8 dat) {

	TRACEOUT(("scsiio_occ4 %.2x", dat));
	(void)port;
	(void)dat;
}

static void IOOUTCALL scsiio_occ6(UINT port, REG8 dat) {

	scsiio.data[scsiio.wrdatpos & 0x7fff] = dat;
	scsiio.wrdatpos++;
	(void)port;
}

static REG8 IOINPCALL scsiio_icc0(UINT port) {

	REG8	ret;

	ret = scsiio.auxstatus;
	scsiio.auxstatus = 0;
	(void)port;
	return(ret);
}

static REG8 IOINPCALL scsiio_icc2(UINT port) {

	REG8	ret;

	switch(scsiio.port) {
		case SCSICTR_STATUS:
			scsiio.port++;
			return(scsiio.scsistatus);

		case SCSICTR_MEMBANK:
			return(scsiio.membank);

		case SCSICTR_MEMWND:
			return(scsiio.memwnd);

		case SCSICTR_RESENT:
			return(scsiio.resent);

		case 0x36:
			return(0);					// ２枚刺しとか…
	}
	if (scsiio.port <= 0x19) {
		ret = scsiio.reg[scsiio.port];
		TRACEOUT(("scsi ctrl read %s %.2x [%.4x:%.4x]",
							scsictr[scsiio.port], ret, CPU_CS, CPU_IP));
		scsiio.port++;
		return(ret);
	}
	(void)port;
	return(0xff);
}

static REG8 IOINPCALL scsiio_icc4(UINT port) {

	TRACEOUT(("scsiio_icc4"));
	(void)port;
	return(0x00);
}

static REG8 IOINPCALL scsiio_icc6(UINT port) {

	REG8	ret;

	ret = scsiio.data[scsiio.rddatpos & 0x7fff];
	scsiio.rddatpos++;
	(void)port;
	return(ret);
}

#if defined(SUPPORT_NP2SCSI)

_NP2STOR		np2stor;

#if 0
#undef	TRACEOUT2
static void trace_fmt_ex(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "¥n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT2(s)	trace_fmt_ex s
#else
#define	TRACEOUT2(s)
#endif	/* 1 */

static void np2stor_memread(UINT32 vaddr, void* buffer, UINT32 size)
{
	UINT32 readaddr = vaddr;
	UINT32 readsize = size;
	UINT8* readptr = (UINT8*)buffer;
	while (readsize >= 4)
	{
		*((UINT32*)readptr) = cpu_kmemoryread_d(readaddr);
		readsize -= 4;
		readptr += 4;
		readaddr += 4;
	}
	while (readsize > 0)
	{
		*readptr = cpu_kmemoryread(readaddr);
		readsize--;
		readptr++;
		readaddr++;
	}
}
static void np2stor_memwrite(UINT32 vaddr, void* buffer, UINT32 size)
{
	UINT32 writeaddr = vaddr;
	UINT32 writesize = size;
	UINT8* writeptr = (UINT8*)buffer;
	while (writesize >= 4)
	{
		cpu_kmemorywrite_d(writeaddr, *((UINT32*)writeptr));
		writesize -= 4;
		writeptr += 4;
		writeaddr += 4;
	}
	while (writesize > 0)
	{
		cpu_kmemorywrite(writeaddr, *writeptr);
		writesize--;
		writeptr++;
		writeaddr++;
	}
}

// StartIoの処理
static void np2stor_startIo()
{
	NP2STOR_INVOKEINFO invokeInfo;

	if (np2stor.maddr == 0) return;

#if defined(SUPPORT_IA32_HAXM)
	// HAXMレジスタを読み取り
	i386haxfunc_vcpu_getREGs(&np2haxstat.state);
	i386haxfunc_vcpu_getFPU(&np2haxstat.fpustate);
	np2haxstat.update_regs = np2haxstat.update_fpu = 0;
	// HAXMレジスタ→猫レジスタにコピー
	ia32hax_copyregHAXtoNP2();
#endif

	// ドライバから渡されたメモリアドレスからデータを直接読み取り
	invokeInfo.version = cpu_kmemoryread_d(np2stor.maddr);
	if (invokeInfo.version == 1)
	{
		invokeInfo.cmd = cpu_kmemoryread_d(np2stor.maddr + 4);
		if (invokeInfo.cmd == NP2STOR_INVOKECMD_DEFAULT || invokeInfo.cmd == NP2STOR_INVOKECMD_NOBUSY)
		{
			UINT8 drv = 0;
			SXSIDEV	sxsi = NULL;
			NP2_SCSI_REQUEST_BLOCK srb = { 0 };
			invokeInfo.srbAddr = cpu_kmemoryread_d(np2stor.maddr + 8);
			np2stor_memread(invokeInfo.srbAddr, &srb, sizeof(srb));
			if (srb.PathId == 0 && 0 <= srb.TargetId && srb.TargetId < 4 && srb.Lun == 0)
			{
				drv = SXSIDRV_SCSI + srb.TargetId;
				sxsi = sxsi_getptr(drv);
			}
			if ((sxsi == NULL) || (!(sxsi->flag & SXSIFLAG_READY)))
			{
				srb.SrbStatus = NP2_SRB_STATUS_NO_DEVICE;
				return;
			}

			switch (srb.Function)
			{
			case NP2_SRB_FUNCTION_EXECUTE_SCSI:
				switch (srb.Cdb[0])
				{
				case NP2_SCSIOP_TEST_UNIT_READY:
					srb.ScsiStatus = NP2_SCSISTAT_GOOD;
					srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
					srb.DataTransferLength = 0;
					break;

				case NP2_SCSIOP_INQUIRY:
				{
					UINT32 dataLength = sizeof(NP2_INQUIRYDATA);
					NP2_INQUIRYDATA inquiryData = { 0 };
					if (srb.DataTransferLength < dataLength) dataLength = srb.DataTransferLength;
					//np2stor_memread(srb.DataBuffer, &inquiryData, dataLength); // NT4では初期値は0にしておかないとだめ　なので現在のメモリのデータは読まない
					inquiryData.DeviceType = 0x00; // DIRECT_ACCESS_DEVICE
					inquiryData.RemovableMedia = FALSE;
					inquiryData.Versions = 0x04;
					inquiryData.ResponseDataFormat = 0x02;
					inquiryData.AdditionalLength = 0x1f;
					RtlCopyMemory(inquiryData.VendorId, "NP2     ", 8);
					RtlCopyMemory(inquiryData.ProductId, "FASTSTORAGE     ", 16);
					RtlCopyMemory(inquiryData.ProductRevisionLevel, "1.00", 4);
					//inquiryData->ProductId[8] = '0' + Srb->PathId;
					//inquiryData->ProductId[10] = '0' + Srb->TargetId;
					//inquiryData->ProductId[12] = '0' + Srb->Lun;
					srb.ScsiStatus = NP2_SCSISTAT_GOOD;
					srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
					srb.DataTransferLength = dataLength;
					np2stor_memwrite(srb.DataBuffer, &inquiryData, dataLength);

					break;
				}

				case NP2_SCSIOP_READ_CAPACITY:
				{
					UINT32 dataLength = sizeof(NP2_READ_CAPACITY_DATA);
					NP2_READ_CAPACITY_DATA cap = { 0 };
					UINT32 lastSector;
					if (srb.DataTransferLength < dataLength)
					{
						srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
						break;
					}
					np2stor_memread(srb.DataBuffer, &cap, dataLength);
					lastSector = sxsi->totals - 1;
					cap.LogicalBlockAddress = (((lastSector) & 0xff) << 24) | (((lastSector >> 8) & 0xff) << 16) | (((lastSector >> 16) & 0xff) << 8) | ((lastSector >> 24) & 0xff);
					cap.BytesPerBlock = (((NP2STOR_SECTOR_SIZE) & 0xff) << 24) | (((NP2STOR_SECTOR_SIZE >> 8) & 0xff) << 16) | (((NP2STOR_SECTOR_SIZE >> 16) & 0xff) << 8) | ((NP2STOR_SECTOR_SIZE >> 24) & 0xff);

					np2stor_memwrite(srb.DataBuffer, &cap, dataLength);

					srb.ScsiStatus = NP2_SCSISTAT_GOOD;
					srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
					srb.DataTransferLength = dataLength;
					break;
				}

				case NP2_SCSIOP_READ:
				case NP2_SCSIOP_WRITE:
				{
					UINT64 offset = (((UINT64)srb.Cdb[2] << 24) | ((UINT64)srb.Cdb[3] << 16) | ((UINT64)srb.Cdb[4] << 8) | (UINT64)srb.Cdb[5]);
					UINT64 lengthInBytes = (((UINT64)srb.Cdb[7] << 8) | (UINT64)srb.Cdb[8]) * NP2STOR_SECTOR_SIZE;
					UINT8* lpBuffer;

					if (offset * NP2STOR_SECTOR_SIZE + lengthInBytes > sxsi->totals * NP2STOR_SECTOR_SIZE)
					{
						srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
						break;
					}

					lpBuffer = (UINT8*)malloc(lengthInBytes);
					if (!lpBuffer)
					{
						srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
						break;
					}
					if (srb.Cdb[0] == NP2_SCSIOP_READ)
					{
						if (sxsi_read(drv, offset, lpBuffer, lengthInBytes))
						{
							srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
							break;
						}
						np2stor_memwrite(srb.DataBuffer, lpBuffer, lengthInBytes);
					}
					else
					{
						if (invokeInfo.cmd != NP2STOR_INVOKECMD_NOBUSY)
						{
							// WORKAROUND: Win2KがEDBx.LOGを大量生成してしまうので、20MB書き込む毎にBUSYを返す
							np2stor.busyflag += lengthInBytes;
							if (np2stor.busyflag > 1024 * 1024 * 20)
							{
								np2stor.busyflag = 0;
								srb.SrbStatus = NP2_SRB_STATUS_BUSY;
								break;
							}
						}
						np2stor_memread(srb.DataBuffer, lpBuffer, lengthInBytes);
						if (sxsi_write(drv, offset, lpBuffer, lengthInBytes))
						{
							srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
							break;
						}
					}

					free(lpBuffer);

					srb.ScsiStatus = NP2_SCSISTAT_GOOD;
					srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
					srb.DataTransferLength = lengthInBytes;
					break;
				}

				case NP2_SCSIOP_READ6:
				case NP2_SCSIOP_WRITE6:
				{
					UINT64 offset = (((UINT64)srb.Cdb[1] & 0x1F) << 16) | ((UINT64)srb.Cdb[2] << 8) | ((UINT64)srb.Cdb[3]);
					UINT64 lengthInBytes = srb.Cdb[4] * NP2STOR_SECTOR_SIZE;
					UINT8* lpBuffer;
					if (lengthInBytes == 0)
					{
						lengthInBytes = 256 * NP2STOR_SECTOR_SIZE;
					}

					if (offset * NP2STOR_SECTOR_SIZE + lengthInBytes > sxsi->totals * NP2STOR_SECTOR_SIZE)
					{
						srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
						break;
					}

					lpBuffer = (UINT8*)malloc(lengthInBytes);
					if (!lpBuffer)
					{
						srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
						break;
					}
					if (srb.Cdb[0] == NP2_SCSIOP_READ6)
					{
						if (sxsi_read(drv, offset, lpBuffer, lengthInBytes))
						{
							srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
							break;
						}
						np2stor_memwrite(srb.DataBuffer, lpBuffer, lengthInBytes);
					}
					else
					{
						np2stor_memread(srb.DataBuffer, lpBuffer, lengthInBytes);
						if (sxsi_write(drv, offset, lpBuffer, lengthInBytes))
						{
							srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
							break;
						}
					}

					free(lpBuffer);

					srb.ScsiStatus = NP2_SCSISTAT_GOOD;
					srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
					srb.DataTransferLength = lengthInBytes;
					break;
				}

				case NP2_SCSIOP_SEEK:
					srb.ScsiStatus = NP2_SCSISTAT_GOOD;
					srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
					srb.DataTransferLength = 0;
					break;

				case NP2_SCSIOP_VERIFY:
				{
					UINT64 lba = ((UINT64)srb.Cdb[2] << 24) | ((UINT64)srb.Cdb[3] << 16) | ((UINT64)srb.Cdb[4] << 8) | ((UINT64)srb.Cdb[5]);
					UINT64 length = ((UINT64)srb.Cdb[7] << 8) | (UINT64)srb.Cdb[8];

					if ((lba + length) > sxsi->totals)
					{
						srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
					}
					srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
					srb.DataTransferLength = 0;
					break;
				}

				case NP2_SCSIOP_MODE_SENSE:
				{
					// PC-98版WIn2000のDISK.SYSでは以下の条件を満たしたときだけMODE SENSEのCHSの結果を使用
					// ・MODE SENSEで全ページ要求したとき、Page1→Page3→Page4の順に返す 
					// ・PageLength（ページ全体のサイズ - 2）が以下の通り
					// 	　　Page1 = 6, Page2 = 22, Page3 = 18
					// ・MODE SENSEでPage3とPage4がとれない場合はIDEのBIOSでディスクCHS探しに行く
					// ・それでも見つからない場合はH:Sを255:63にされる
					if ((srb.Cdb[2] & 0x3f) == NP2_MODE_SENSE_RETURN_ALL)
					{
						UINT8 buffer[sizeof(NP2_MODE_PARAMETER_HEADER) + sizeof(NP2_MODE_PARAMETER_BLOCK) + sizeof(NP2_MODE_READ_RECOVERY_PAGE) + sizeof(NP2_MODE_FORMAT_PAGE) + sizeof(NP2_MODE_RIGID_GEOMETRY_PAGE)] = { 0 };
						UINT32 bufferLength = sizeof(buffer);
						UINT8 dbd = srb.Cdb[1] & 0x08;  // DBD
						UINT8 allocationLength = srb.Cdb[4];  // CDB byte 4 = Allocation length
						UINT8 pageCode = srb.Cdb[2] & 0x3F;  // bit6=PCF (Page Control), bit0-5 = Page Code
						NP2_MODE_PARAMETER_HEADER* header = NULL;
						NP2_MODE_PARAMETER_BLOCK* blockdesc = NULL;
						NP2_MODE_READ_RECOVERY_PAGE* recpage = NULL;
						NP2_MODE_FORMAT_PAGE* fmtpage = NULL;
						NP2_MODE_RIGID_GEOMETRY_PAGE* geopage = NULL;

						header = (NP2_MODE_PARAMETER_HEADER*)(buffer);
						header->ModeDataLength = bufferLength - 1;
						header->MediumType = 0;
						header->DeviceSpecificParameter = 0;

						if (dbd)
						{
							header->BlockDescriptorLength = 0;
							recpage = (NP2_MODE_READ_RECOVERY_PAGE*)((UINT8*)header + sizeof(NP2_MODE_PARAMETER_HEADER));
							fmtpage = (NP2_MODE_FORMAT_PAGE*)((UINT8*)recpage + sizeof(NP2_MODE_READ_RECOVERY_PAGE));
							geopage = (NP2_MODE_RIGID_GEOMETRY_PAGE*)((UINT8*)fmtpage + sizeof(NP2_MODE_FORMAT_PAGE));
							bufferLength -= sizeof(NP2_MODE_PARAMETER_BLOCK);
						}
						else
						{
							header->BlockDescriptorLength = sizeof(NP2_MODE_PARAMETER_BLOCK);
							blockdesc = (NP2_MODE_PARAMETER_BLOCK*)((UINT8*)header + sizeof(NP2_MODE_PARAMETER_HEADER));
							recpage = (NP2_MODE_READ_RECOVERY_PAGE*)((UINT8*)blockdesc + sizeof(NP2_MODE_PARAMETER_BLOCK));
							fmtpage = (NP2_MODE_FORMAT_PAGE*)((UINT8*)recpage + sizeof(NP2_MODE_READ_RECOVERY_PAGE));
							geopage = (NP2_MODE_RIGID_GEOMETRY_PAGE*)((UINT8*)fmtpage + sizeof(NP2_MODE_FORMAT_PAGE));

							blockdesc->DensityCode = 0x00; // Density Code
							blockdesc->NumberOfBlocks[0] = (sxsi->sectors >> 16) & 0xff; // Number of Blocks
							blockdesc->NumberOfBlocks[1] = (sxsi->sectors >> 8) & 0xff; // Number of Blocks
							blockdesc->NumberOfBlocks[2] = (sxsi->sectors) & 0xff; // Number of Blocks
							blockdesc->Reserved = 0x00; // Reserved
							blockdesc->BlockLength[0] = (sxsi->size >> 8) & 0xff; // Block Length
							blockdesc->BlockLength[1] = (sxsi->size >> 8) & 0xff; // Block Length
							blockdesc->BlockLength[2] = (sxsi->size) & 0xff; // Block Length
						}

						recpage->PageCode = 0x01;
						recpage->PageLength = sizeof(NP2_MODE_READ_RECOVERY_PAGE) - 2;

						fmtpage->PageCode = 0x03;
						fmtpage->PageLength = sizeof(NP2_MODE_FORMAT_PAGE) - 2;
						fmtpage->SectorsPerTrack[0] = (sxsi->sectors >> 8) & 0xff;
						fmtpage->SectorsPerTrack[1] = (sxsi->sectors) & 0xff;
						fmtpage->BytesPerPhysicalSector[0] = (sxsi->size >> 8) & 0xff;
						fmtpage->BytesPerPhysicalSector[1] = (sxsi->size) & 0xff;
						fmtpage->Interleave[1] = 1;

						geopage->PageCode = 0x04;
						geopage->PageLength = sizeof(NP2_MODE_RIGID_GEOMETRY_PAGE) - 2;
						geopage->NumberOfCylinders[0] = (sxsi->cylinders >> 16) & 0xff;
						geopage->NumberOfCylinders[1] = (sxsi->cylinders >> 8) & 0xff;
						geopage->NumberOfCylinders[2] = (sxsi->cylinders) & 0xff;
						geopage->NumberOfHeads = sxsi->surfaces;

						if (allocationLength < bufferLength)
						{
							// 格納先長さ足りないので削る
							bufferLength -= sizeof(NP2_MODE_RIGID_GEOMETRY_PAGE);
						}
						if (allocationLength < bufferLength)
						{
							// 格納先長さ足りないので削る
							bufferLength -= sizeof(NP2_MODE_FORMAT_PAGE);
						}
						if (allocationLength < bufferLength)
						{
							// 格納先長さ足りないので削る
							bufferLength -= sizeof(NP2_MODE_READ_RECOVERY_PAGE);
						}
						if (allocationLength < bufferLength)
						{
							// もう削れないのでエラー
							srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
						}
						else
						{
							// 返せる分を返す
							np2stor_memwrite(srb.DataBuffer, &buffer, bufferLength);

							srb.ScsiStatus = NP2_SCSISTAT_GOOD;
							srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
							srb.DataTransferLength = bufferLength;
						}
					}
					else
					{
						srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
					}
					break;
				}

				case NP2_SCSIOP_MEDIUM_REMOVAL:
				{
					srb.ScsiStatus = NP2_SCSISTAT_GOOD;
					srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
					srb.DataTransferLength = 0;
					break;
				}

				default:
					TRACEOUT2(("Unknown Function=0x%02x, SCSIOP=0x%02x", srb.Function, srb.Cdb[0]));
					srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
					break;
				}
				break;

			case NP2_SRB_FUNCTION_IO_CONTROL:
			{
				//NP2_SRB_IO_CONTROL ioctrl;
				//if (srb.DataBuffer == NULL && cpu_kmemoryread_d(srb.DataBuffer) < sizeof(ioctrl))
				//{
				//	srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
				//	break;
				//}
				//np2stor_memread(srb.DataBuffer, &ioctrl, sizeof(ioctrl));
				//if (ioctrl.ControlCode == IOCTL_DISK_GET_DRIVE_GEOMETRY)
				//{
				//	if (ioctrl.Length >= sizeof(DISK_GEOMETRY))
				//	{
				//		NP2_DISK_GEOMETRY geo = { 0 };
				//		np2stor_memwrite(srb.DataBuffer + sizeof(ioctrl), &geo, sizeof(geo));
				//		geo.Cylinders = sxsi->cylinders;
				//		geo.MediaType = 12; // Fixed hard disk media
				//		geo.TracksPerCylinder = sxsi->surfaces;
				//		geo.SectorsPerTrack = sxsi->sectors;
				//		geo.BytesPerSector = sxsi->size;

				//		ioctrl.ReturnCode = 0;
				//		np2stor_memwrite(srb.DataBuffer, &ioctrl, sizeof(ioctrl));
				//		srb.ScsiStatus = NP2_SCSISTAT_GOOD;
				//		srb.SrbStatus = NP2_SRB_STATUS_SUCCESS;
				//		srb.DataTransferLength = sizeof(NP2_SRB_IO_CONTROL) + sizeof(NP2_DISK_GEOMETRY);
				//	}
				//	else
				//	{
				//		srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
				//	}
				//}
				//else
				{
					srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
				}

				break;
			}

			default:
				srb.SrbStatus = NP2_SRB_STATUS_INVALID_REQUEST;
				break;
			}

			np2stor_memwrite(invokeInfo.srbAddr, &srb, sizeof(srb));
		}
	}
}

static void IOOUTCALL np2stor_o7ea(UINT port, REG8 dat)
{
	np2stor.maddr = (dat << 24) | (np2stor.maddr >> 8);
	(void)port;
}

static void IOOUTCALL np2stor_o7eb(UINT port, REG8 dat)
{
	// データ格納先の仮想メモリアドレスをI/Oポート7EAhへ書き込み、
	// 7EBhに0x98→0x01の順で書き込むと猫本体が処理を実行する。
	if (dat == NP2STOR_CMD_ACTIVATE)
	{
		np2stor.ioenable = 1;
	}
	else if (dat == NP2STOR_CMD_STARTIO && np2stor.ioenable)
	{
		np2stor_startIo();
	}
	else
	{
		np2stor.ioenable = 0;
	}
	(void)port;
}

static REG8 IOINPCALL np2stor_i7ea(UINT port)
{
	return(98);
}

static REG8 IOINPCALL np2stor_i7eb(UINT port)
{
	return(21);
}

#endif


// ----

void scsiio_reset(const NP2CFG *pConfig) {

	FILEH	fh;
	UINT	r;

	ZeroMemory(&scsiio, sizeof(scsiio));
	if (pccore.hddif & PCHDD_SCSI) {
		scsiio.memwnd = (0xd200 & 0x0e00) >> 9;
		scsiio.resent = (3 << 3) + (7 << 0);

		CPU_RAM_D000 |= (3 << 2);				// ramにする
		fh = file_open_rb_c(OEMTEXT("scsi.rom"));
		r = 0;
		if (fh != FILEH_INVALID) {
			r = file_read(fh, scsiio.bios, 0x4000);
			file_close(fh);
		}
		if (r != 0) { // if (r == 0x4000) {
			TRACEOUT(("load scsi.rom"));
		}
		else {
			ZeroMemory(mem + 0xd2000, 0x4000);
			CopyMemory(scsiio.bios, scsibios, sizeof(scsibios));
			TRACEOUT(("use simulate scsi.rom"));
		}
		CopyMemory(mem + 0xd2000, scsiio.bios[0], 0x2000);
	}

#if defined(SUPPORT_NP2SCSI)
	ZeroMemory(&np2stor, sizeof(np2stor));
	if (np2cfg.usenp2stor)
	{
		np2stor.enable = 1;
	}
#endif

	(void)pConfig;
}

void scsiio_bind(void) {

	if (pccore.hddif & PCHDD_SCSI) {
		iocore_attachout(0x0cc0, scsiio_occ0);
		iocore_attachout(0x0cc2, scsiio_occ2);
		iocore_attachout(0x0cc4, scsiio_occ4);
		iocore_attachout(0x0cc6, scsiio_occ6);
		iocore_attachinp(0x0cc0, scsiio_icc0);
		iocore_attachinp(0x0cc2, scsiio_icc2);
		iocore_attachinp(0x0cc4, scsiio_icc4);
		iocore_attachinp(0x0cc6, scsiio_icc6);

#if defined(SUPPORT_NP2SCSI)
		if (np2stor.enable)
		{
			iocore_attachout(0x07ea, np2stor_o7ea);
			iocore_attachout(0x07eb, np2stor_o7eb);
			iocore_attachinp(0x07ea, np2stor_i7ea);
			iocore_attachinp(0x07eb, np2stor_i7eb);
		}
#endif
	}
}

#endif

