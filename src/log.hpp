// log.hpp

#pragma once

#include <replxx.hxx>

extern replxx::Replxx soggy_rx;

#if defined(__GNUC__)
# define PRINTF_FORMAT_STRING
# define PRINTF_VARARG_FN(fmtargnumber) __attribute__((format(printf, fmtargnumber, fmtargnumber+1)))
#elif defined(_MSC_VER)
# include <sal.h>
# define PRINTF_FORMAT_STRING _Printf_format_string_
# define PRINTF_VARARG_FN(fmtargnumber)
#else
# define PRINTF_FORMAT_STRING
# define PRINTF_VARARG_FN(fmtargnumber)
#endif

extern void soggy_log(PRINTF_FORMAT_STRING const char *fmt, ...) PRINTF_VARARG_FN(1);
