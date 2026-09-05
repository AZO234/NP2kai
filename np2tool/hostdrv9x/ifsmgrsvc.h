#ifndef NP2_HOSTDRV9X_IFSMGRSVC_H
#define NP2_HOSTDRV9X_IFSMGRSVC_H

/*
 * C declarations for the four IFSMgr services used by HOSTDRV9X.
 * The ordinals are generated from the supplied Win98 DDK IFSMGR.INC table.
 */
#define IFSMgr_DEVICE_ID 0x0040

enum {
    __IFSMgr_Get_Version = (IFSMgr_DEVICE_ID << 16) + 0,
    __IFSMgr_RegisterNet = (IFSMgr_DEVICE_ID << 16) + 2,
    __IFSMgr_InitUseAdd = (IFSMgr_DEVICE_ID << 16) + 44,
    __IFSMgr_DeregisterFSD = (IFSMgr_DEVICE_ID << 16) + 117
};

#define H9X_IFSMGRVERSION 0x0022
#define H9X_NET_ID 0x00250000UL

static __inline unsigned long h9x_IFSMgr_GetVersion(void)
{
    unsigned long result;
    VxDCall(IFSMgr_Get_Version);
    __asm mov result, eax
    return result;
}

static __inline int h9x_IFSMgr_RegisterNet(pIFSFunc entry, unsigned long netid)
{
     int result;
     __asm push netid
     __asm push H9X_IFSMGRVERSION
     __asm push entry
     VxDCall(IFSMgr_RegisterNet);
    __asm add esp, 12
     __asm mov result, eax
     return result;
}

static __inline int h9x_IFSMgr_InitUseAdd(void *info, int provider)
{
    int result;
    __asm push provider
    __asm push info
    VxDCall(IFSMgr_InitUseAdd);
    __asm add esp, 8
    __asm mov result, eax
    return result;
}

static __inline int h9x_IFSMgr_DeregisterNet(int provider, int force)
{
    int result;
    __asm push H9X_IFSMGRVERSION
    __asm push force
    __asm push provider
    __asm push FSTYPE_NET_FSD
    VxDCall(IFSMgr_DeregisterFSD);
    __asm add esp, 16
    __asm mov result, eax
    return result;
}

#endif
