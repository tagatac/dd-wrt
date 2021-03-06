/* Generated by ./xlat/gen.sh from ./xlat/mtd_flags_options.in; do not edit. */

#include "gcc_compat.h"
#include "static_assert.h"


#ifndef XLAT_MACROS_ONLY

# ifdef IN_MPERS

extern const struct xlat mtd_flags_options[];

# else

static const struct xlat_data mtd_flags_options_xdata[] = {
#if defined(MTD_WRITEABLE) || (defined(HAVE_DECL_MTD_WRITEABLE) && HAVE_DECL_MTD_WRITEABLE)
  XLAT(MTD_WRITEABLE),
#endif
#if defined(MTD_BIT_WRITEABLE) || (defined(HAVE_DECL_MTD_BIT_WRITEABLE) && HAVE_DECL_MTD_BIT_WRITEABLE)
  XLAT(MTD_BIT_WRITEABLE),
#endif
#if defined(MTD_NO_ERASE) || (defined(HAVE_DECL_MTD_NO_ERASE) && HAVE_DECL_MTD_NO_ERASE)
  XLAT(MTD_NO_ERASE),
#endif
#if defined(MTD_POWERUP_LOCK) || (defined(HAVE_DECL_MTD_POWERUP_LOCK) && HAVE_DECL_MTD_POWERUP_LOCK)
  XLAT(MTD_POWERUP_LOCK),
#endif
};
#  if !(defined HAVE_M32_MPERS || defined HAVE_MX32_MPERS)
static
#  endif
const struct xlat mtd_flags_options[1] = { {
 .data = mtd_flags_options_xdata,
 .size = ARRAY_SIZE(mtd_flags_options_xdata),
 .type = XT_NORMAL,
} };

# endif /* !IN_MPERS */

#endif /* !XLAT_MACROS_ONLY */
