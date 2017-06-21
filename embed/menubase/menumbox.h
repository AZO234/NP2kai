
enum {
	MBOX_OK				= 0x0000,
	MBOX_OKCANCEL		= 0x0001,
	MBOX_ABORT			= 0x0002,
	MBOX_YESNOCAN		= 0x0003,
	MBOX_YESNO			= 0x0004,
	MBOX_RETRY			= 0x0005,
	MBOX_ICONSTOP		= 0x0010,
	MBOX_ICONQUESTION	= 0x0020,
	MBOX_ICONEXCLAME	= 0x0030,
	MBOX_ICONINFO		= 0x0040
};

#ifdef __cplusplus
extern "C" {
#endif

int menumbox(const OEMCHAR *string, const OEMCHAR *title, UINT type);

#ifdef __cplusplus
}
#endif

