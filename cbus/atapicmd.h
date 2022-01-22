
#if defined(SUPPORT_IDEIO)

// sense key
enum {
	ATAPI_SK_NO_SENSE = 0x00,
	ATAPI_SK_RECOVERED_ERROR = 0x01,
	ATAPI_SK_NOT_READY = 0x02,
	ATAPI_SK_MEDIUM_ERROR = 0x03,
	ATAPI_SK_HARDWARE_ERROR = 0x04,
	ATAPI_SK_ILLEGAL_REQUEST = 0x05,
	ATAPI_SK_UNIT_ATTENTIOn = 0x06,
	ATAPI_SK_DATA_PROTECT = 0x07,
	ATAPI_SK_ABORTED_COMMAND = 0x0b,
	ATAPI_SK_MISCOMPARE = 0x0e,
	ATAPI_SK_MAX
};

#define	ATAPI_SENSE_KEY_SHIFT	4
#define	ATAPI_SENSE_KEY_MASK	0xf0
#define	ATAPI_SET_SENSE_KEY(drv, sensekey) \
do { \
	(drv)->error &= ~ATAPI_SENSE_KEY_MASK; \
	(drv)->error |= (sensekey) << ATAPI_SENSE_KEY_SHIFT; \
	(drv)->sk = (sensekey); \
} while (/*CONSTCOND*/ 0)


// additional sense code (ASC) & ASC Qualifier (ASCQ)
enum {
	ATAPI_ASC_NO_ADDITIONAL_SENSE_INFORMATION = 0x0000,
	ATAPI_ASC_PARAMETER_LIST_LENGTH_ERROR = 0x001a,
	ATAPI_ASC_INVALID_COMMAND_OPERATION_CODE = 0x0020,
	ATAPI_ASC_INVALID_FIELD_IN_CDB = 0x0024,
	ATAPI_ASC_NOT_READY_TO_READY_TRANSITION = 0x0028,
	ATAPI_ASC_POWER_ON_RESET_OR_BUS_DEVICE_RESET_OCCURRED = 0x0029,
	ATAPI_ASC_SAVING_PARAMETERS_NOT_SUPPORTED = 0x0039,
	ATAPI_ASC_MEDIUM_NOT_PRESENT = 0x003a,
	ATAPI_ASC_MEDIUM_NOT_PRESENT_TRAY_CLOSED = 0x013a,
	ATAPI_ASC_MEDIUM_NOT_PRESENT_TRAY_OPEN = 0x023a,
	ATAPI_ASC_MAX
};


#ifdef __cplusplus
extern "C" {
#endif

void atapi_initialize(void);
void atapi_deinitialize(void);

void atapicmd_a0(IDEDRV drv);

void atapi_dataread(IDEDRV drv);
void atapi_dataread_asyncwait(int wait);

#ifdef __cplusplus
}
#endif

#endif	/* SUPPORT_IDEIO */

