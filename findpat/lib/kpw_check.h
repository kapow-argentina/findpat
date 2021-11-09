#ifndef __KPW_CHECK_H__
#define __KPW_CHECK_H__

/* KPW file checker (check integrity and repair) */

#include "kpw.h"
#include "libp2zip.h"

bool kpw_check_fix(char *fn, bool fix);

#endif /*__KPW_CHECK_H__*/
