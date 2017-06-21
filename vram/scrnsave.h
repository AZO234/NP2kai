/**
 * @file	scrnsave.h
 * @brief	Interface of the screen saver
 */

#pragma once

/**
 * types
 */
enum tagScrnSaveType
{
	SCRNSAVE_1BIT	= 0,
	SCRNSAVE_4BIT	= 1,
	SCRNSAVE_8BIT	= 2,
	SCRNSAVE_24BIT	= 3
};

/**
 * flags
 */
enum tagScrnSaveFlags
{
	SCRNSAVE_AUTO	= 0
};

struct tagScrnSave;
typedef struct tagScrnSave *		SCRNSAVE;


#ifdef __cplusplus
extern "C"
{
#endif

SCRNSAVE scrnsave_create(void);
void scrnsave_destroy(SCRNSAVE hdl);
int scrnsave_gettype(SCRNSAVE hdl);
BRESULT scrnsave_writebmp(SCRNSAVE hdl, const OEMCHAR *filename, UINT flag);
BRESULT scrnsave_writegif(SCRNSAVE hdl, const OEMCHAR *filename, UINT flag);

#ifdef __cplusplus
}
#endif
