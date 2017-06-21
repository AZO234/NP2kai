#ifndef	NP2_X11_OEMTEXT_H__
#define	NP2_X11_OEMTEXT_H__

#include "codecnv/codecnv.h"

#if defined(OSLANG_UTF8)
#define	oemtext_sjistooem	codecnv_sjistoutf8
#define	oemtext_oemtosjis	codecnv_utf8tosjis
#elif defined(OSLANG_EUC)
#define	oemtext_sjistooem	codecnv_sjistoeuc
#define	oemtext_oemtosjis	codecnv_euctosjis
#endif

#ifdef __cplusplus
#include <string>
namespace std
{
	typedef string oemstring;
}
#endif  /* __cplusplus */

#endif	/* NP2_X11_OEMTEXT_H__ */
