
// PC-9821 PCIバス

#if defined(SUPPORT_PC9821)

#if defined(SUPPORT_PCI)

#define PCI_DEVICES_MAX	32

#define PCI_PCMC_82434LX	0
#define PCI_PCMC_82441FX	1
#define PCI_PCMC_WILDCAT	2

#define PCI_GETCFGREG_B(reg, ofs)			(*((UINT8*)((reg) + (ofs))))
#define PCI_GETCFGREG_W(reg, ofs)			(*((UINT16*)((reg) + (ofs))))
#define PCI_GETCFGREG_D(reg, ofs)			(*((UINT32*)((reg) + (ofs))))

#define PCI_SETCFGREG_B(reg, ofs, value)	(PCI_GETCFGREG_B(reg, ofs) = value)
#define PCI_SETCFGREG_W(reg, ofs, value)	(PCI_GETCFGREG_W(reg, ofs) = value)
#define PCI_SETCFGREG_D(reg, ofs, value)	(PCI_GETCFGREG_D(reg, ofs) = value)

#define PCI_SETCFGREG_B_MASK(reg, ofs, value, mask)	(PCI_GETCFGREG_B(reg, ofs) = (PCI_GETCFGREG_B(reg, ofs) & mask) | (value & ~mask))
#define PCI_SETCFGREG_W_MASK(reg, ofs, value, mask)	(PCI_GETCFGREG_W(reg, ofs) = (PCI_GETCFGREG_W(reg, ofs) & mask) | (value & ~mask))
#define PCI_SETCFGREG_D_MASK(reg, ofs, value, mask)	(PCI_GETCFGREG_D(reg, ofs) = (PCI_GETCFGREG_D(reg, ofs) & mask) | (value & ~mask))

// コンフィギュレーションレジスタ変更時に呼ばれる。
typedef void (*PCIREGWCB)(UINT32 devNumber, UINT8 funcNumber, UINT8 cfgregOffset, UINT8 sizeinbytes, UINT32 value);

#pragma pack(1)
// PCI IRQ ルーティングテーブル エントリ
typedef struct {
	UINT8 busnumber;
	UINT8 devicenumber;
	UINT8 link4intA;
	UINT16 irqmap4intA;
	UINT8 link4intB;
	UINT16 irqmap4intB;
	UINT8 link4intC;
	UINT16 irqmap4intC;
	UINT8 link4intD;
	UINT16 irqmap4intD;
	UINT8 slot;
	UINT8 reserved;
} _PCIPNP_IRQTBL_ENTRY, *PCIPNP_IRQTBL_ENTRY;
#pragma pack()

// PCI IRQ ルーティングテーブル
typedef struct {
	UINT16 datacount;
	union{
		_PCIPNP_IRQTBL_ENTRY data[PCI_DEVICES_MAX];
		UINT8	data8[PCI_DEVICES_MAX * sizeof(_PCIPNP_IRQTBL_ENTRY)];
	};
} _PCIPNP_IRQTBL, *PCIPNP_IRQTBL;

// Configuration Space Header
typedef struct {
	UINT16 vendorID;
	UINT16 deviceID;
	UINT16 command;
	UINT16 status;
	UINT8 revisionID;
	UINT8 classcode[3];
	UINT8 cachelinesize;
	UINT8 latencytimer;
	UINT8 headertype;
	UINT8 BIST;
	UINT32 baseaddrregs[6];
	UINT32 cardbusCISptr;
	UINT16 subsysventorID;
	UINT16 subsysID;
	UINT32 expROMbaseaddr;
	UINT16 capsptr;
	UINT8 reserved1[3];
	UINT32 reserved2;
	UINT8 interruptline;
	UINT8 interruptpin;
	UINT8 min_gnt;
	UINT8 max_lat;
} _PCICSH, *PCICSH;

// PCIデバイス
typedef struct {
	UINT8		enable;
	PCIREGWCB	regwfn;
	UINT8		slot; // PCIスロット番号（オンボードは0）
	UINT8		skipirqtbl; // 0でない場合はルーティングテーブルに登録しない
	union{
		UINT8	cfgreg8[0x100];
		_PCICSH header; // Type 00h Configuration Space Header
	};
	// ビットを立てたところはリードオンリ
	union{
		UINT8	cfgreg8rom[0x100];
		_PCICSH headerrom; // Type 00h Configuration Space Header
	};
} _PCIDEVICE, *PCIDEVICE;

// PCI
typedef struct {
	UINT8	enable;

	UINT8	reg_cse; // CONFIGURATION SPACE ENABLE REGISTER
	UINT8	reg_trc; // TURBO-RESET CONTROL REGISTER
	UINT8	reg_fwd; // FORWARD REGISTER
	UINT8	reg_cms; // Configuration Mechanism Select
	
	UINT32	reg32_caddr; // CONFIGURATION ADDRESS REGISTER
	
	_PCIDEVICE	devices[PCI_DEVICES_MAX]; // PCIデバイス

	UINT8	membankd0;
	UINT8	membankd8;

	UINT8	biosrom[0x8000];
	UINT8	biosromtmp[0x8000];
	OEMCHAR	biosname[16];
	
    UINT8 usebios32;
	UINT32	bios32svcdir;
	UINT32	bios32entrypoint;
	_PCIPNP_IRQTBL	biosdata;
	UINT16	allirqbitmap;
	
    UINT8 unkreg[4][256];
    UINT8 unkreg_bank1;
    UINT8 unkreg_bank2;
} _PCIDEV, *PCIDEV;

#ifdef __cplusplus
extern "C" {
#endif

void IOOUTCALL pcidev_w8_0xcfc(UINT port, UINT8 value);
void IOOUTCALL pcidev_w16_0xcfc(UINT port, UINT16 value);
void IOOUTCALL pcidev_w32(UINT port, UINT32 value);
UINT8 IOOUTCALL pcidev_r8_0xcfc(UINT port);
UINT16 IOOUTCALL pcidev_r16_0xcfc(UINT port);
UINT32 IOOUTCALL pcidev_r32(UINT port);

void pcidev_basereset();
void pcidev_reset(const NP2CFG *pConfig);
void pcidev_bind(void);
void pcidev_updateRoutingTable();
void pcidev_updateBIOS32data();

#ifdef __cplusplus
}
#endif

#else
typedef struct {
	UINT32	base;

	UINT8	membankd0;
} _PCIDEV, *PCIDEV;

#ifdef __cplusplus
extern "C" {
#endif

void IOOUTCALL pcidev_w32(UINT port, UINT32 value);
UINT32 IOOUTCALL pcidev_r32(UINT port);

void pcidev_reset(const NP2CFG *pConfig);
void pcidev_bind(void);

#ifdef __cplusplus
}
#endif

#endif

#endif

