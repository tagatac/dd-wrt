/* Generated by ./xlat/gen.sh from ./xlat/fib_rule_flags.in; do not edit. */

#include "gcc_compat.h"
#include "static_assert.h"

#if defined(FIB_RULE_PERMANENT) || (defined(HAVE_DECL_FIB_RULE_PERMANENT) && HAVE_DECL_FIB_RULE_PERMANENT)
DIAG_PUSH_IGNORE_TAUTOLOGICAL_COMPARE
static_assert((FIB_RULE_PERMANENT) == (0x00000001), "FIB_RULE_PERMANENT != 0x00000001");
DIAG_POP_IGNORE_TAUTOLOGICAL_COMPARE
#else
# define FIB_RULE_PERMANENT 0x00000001
#endif
#if defined(FIB_RULE_INVERT) || (defined(HAVE_DECL_FIB_RULE_INVERT) && HAVE_DECL_FIB_RULE_INVERT)
DIAG_PUSH_IGNORE_TAUTOLOGICAL_COMPARE
static_assert((FIB_RULE_INVERT) == (0x00000002), "FIB_RULE_INVERT != 0x00000002");
DIAG_POP_IGNORE_TAUTOLOGICAL_COMPARE
#else
# define FIB_RULE_INVERT 0x00000002
#endif
#if defined(FIB_RULE_UNRESOLVED) || (defined(HAVE_DECL_FIB_RULE_UNRESOLVED) && HAVE_DECL_FIB_RULE_UNRESOLVED)
DIAG_PUSH_IGNORE_TAUTOLOGICAL_COMPARE
static_assert((FIB_RULE_UNRESOLVED) == (0x00000004), "FIB_RULE_UNRESOLVED != 0x00000004");
DIAG_POP_IGNORE_TAUTOLOGICAL_COMPARE
#else
# define FIB_RULE_UNRESOLVED 0x00000004
#endif
#if defined(FIB_RULE_IIF_DETACHED) || (defined(HAVE_DECL_FIB_RULE_IIF_DETACHED) && HAVE_DECL_FIB_RULE_IIF_DETACHED)
DIAG_PUSH_IGNORE_TAUTOLOGICAL_COMPARE
static_assert((FIB_RULE_IIF_DETACHED) == (0x00000008), "FIB_RULE_IIF_DETACHED != 0x00000008");
DIAG_POP_IGNORE_TAUTOLOGICAL_COMPARE
#else
# define FIB_RULE_IIF_DETACHED 0x00000008
#endif
#if defined(FIB_RULE_OIF_DETACHED) || (defined(HAVE_DECL_FIB_RULE_OIF_DETACHED) && HAVE_DECL_FIB_RULE_OIF_DETACHED)
DIAG_PUSH_IGNORE_TAUTOLOGICAL_COMPARE
static_assert((FIB_RULE_OIF_DETACHED) == (0x00000010), "FIB_RULE_OIF_DETACHED != 0x00000010");
DIAG_POP_IGNORE_TAUTOLOGICAL_COMPARE
#else
# define FIB_RULE_OIF_DETACHED 0x00000010
#endif
#if defined(FIB_RULE_FIND_SADDR) || (defined(HAVE_DECL_FIB_RULE_FIND_SADDR) && HAVE_DECL_FIB_RULE_FIND_SADDR)
DIAG_PUSH_IGNORE_TAUTOLOGICAL_COMPARE
static_assert((FIB_RULE_FIND_SADDR) == (0x00010000), "FIB_RULE_FIND_SADDR != 0x00010000");
DIAG_POP_IGNORE_TAUTOLOGICAL_COMPARE
#else
# define FIB_RULE_FIND_SADDR 0x00010000
#endif

#ifndef XLAT_MACROS_ONLY

# ifdef IN_MPERS

#  error static const struct xlat fib_rule_flags in mpers mode

# else

static const struct xlat_data fib_rule_flags_xdata[] = {
 XLAT(FIB_RULE_PERMANENT),
 XLAT(FIB_RULE_INVERT),
 XLAT(FIB_RULE_UNRESOLVED),
 XLAT(FIB_RULE_IIF_DETACHED),
 XLAT(FIB_RULE_OIF_DETACHED),
 XLAT(FIB_RULE_FIND_SADDR),
};
static
const struct xlat fib_rule_flags[1] = { {
 .data = fib_rule_flags_xdata,
 .size = ARRAY_SIZE(fib_rule_flags_xdata),
 .type = XT_NORMAL,
} };

# endif /* !IN_MPERS */

#endif /* !XLAT_MACROS_ONLY */
