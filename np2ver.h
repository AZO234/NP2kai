/**
 * @file	np2ver.h
 * @brief	The version
 */

#if defined(SUPPORT_IA32_HAXM)
#define	NP2VER_CORE			"HAXM 0.86"
#else
#define	NP2VER_CORE			"0.86"
#endif
#define	NP2VER_GIT			NP2KAI_GIT_TAG " " NP2KAI_GIT_HASH

