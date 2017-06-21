
#ifdef __cplusplus
extern "C" {
#endif

void newdisk_fdd(const OEMCHAR *fname, REG8 type, const OEMCHAR *label);

void newdisk_thd(const OEMCHAR *fname, UINT hddsize);
void newdisk_nhd_ex_CHS(const OEMCHAR *fname, UINT32 C, UINT16 H, UINT16 S, UINT16 SS, int *progress, int *cancel);
void newdisk_nhd_ex(const OEMCHAR *fname, UINT hddsize, int *progress, int *cancel);
void newdisk_nhd(const OEMCHAR *fname, UINT hddsize);
void newdisk_hdi(const OEMCHAR *fname, UINT hddtype);
void newdisk_vhd(const OEMCHAR *fname, UINT hddsize);
#ifdef SUPPORT_VPCVHD
void newdisk_vpcvhd_ex_CHS(const OEMCHAR *fname, UINT32 C, UINT16 H, UINT16 S, UINT16 SS, int dynamic, int *progress, int *cancel);
void newdisk_vpcvhd_ex(const OEMCHAR *fname, UINT hddsize, int dynamic, int *progress, int *cancel);
void newdisk_vpcvhd(const OEMCHAR *fname, UINT hddsize);
#endif

#ifdef __cplusplus
}
#endif

