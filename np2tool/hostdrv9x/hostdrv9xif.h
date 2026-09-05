#ifndef NP2_HOSTDRV9X_IF_H
#define NP2_HOSTDRV9X_IF_H

/* This ABI must match generic/hostdrv9xdef.h. */
#define H9X_IO_ADDR        0x07e4
#define H9X_IO_CMD         0x07e6
#define H9X_IO_DIAG_TAG    0x07e0
#define H9X_IO_DIAG_TEXT   0x07e1
#define H9X_IO_DIAG_DATA   0x07e2
#define H9X_PROBE_ADDR     0x39
#define H9X_PROBE_CMD      0x21
#define H9X_CALL_SIGNATURE 0x43583948UL /* "H9XC" */
#define H9X_CALL_VERSION   0x00010000UL
#define H9X_COMMAND        "H9X100"


#define H9X_CONTROL_BASE          0x80000000UL
#define H9X_CTL_QUERY_DOS         0x80000001UL
#define H9X_CTL_SUSPEND_DOS       0x80000002UL
#define H9X_CTL_RESUME_DOS        0x80000003UL
#define H9X_CTL_SET_CONFIG        0x80000004UL
#define H9X_CONTROL_VERSION       0x00010000UL
#define H9X_CONTROL_DOS_MOUNTED   0x00000001UL
#define H9X_CONTROL_DOS_SUSPENDED 0x00000002UL
#define H9X_CONTROL_REAL_CAPACITY 0x00000004UL
#define H9X_CONTROL_DOS_CDS_HIDDEN 0x00000008UL
#define H9X_CONTROL_WIN95_COMPAT    0x00000010UL
#define H9X_INVALID_DRIVE         0xffffffffUL
#define H9X_FAKE_TOTAL_MB_DEFAULT 2048UL
#define H9X_FAKE_FREE_MB_DEFAULT  1024UL

#pragma pack(push, 1)
typedef struct _H9X_CALL {
    unsigned long signature;
    unsigned long version;
    unsigned long function;
    unsigned long ioreq;
} H9X_CALL;

typedef struct _H9X_CONTROL {
    unsigned long size;
    unsigned long version;
    unsigned long flags;
    unsigned long drive;
    unsigned long fakeTotalMB;
    unsigned long fakeFreeMB;
    unsigned long result;
} H9X_CONTROL;
#pragma pack(pop)

#endif
