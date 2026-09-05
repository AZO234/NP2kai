#ifndef NP2IDLE_H
#define NP2IDLE_H

/* Ring-3 interface shared by NP2IDLECTL and NP2IDLE.VXD. */
#define NP2IDLE_DEVICE_NAME      "\\\\.\\NP2IDLE0"

#define NP2IDLE_DIOC_ENABLE      1UL
#define NP2IDLE_DIOC_DISABLE     2UL
#define NP2IDLE_DIOC_GETSTATE    3UL

#define NP2IDLE_STATE_OFF        0UL
#define NP2IDLE_STATE_ON         1UL

#endif /* NP2IDLE_H */
