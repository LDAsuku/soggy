// log.cpp

// POSIX
#include <cstdio>
#include <cstdarg>

// readline
#include <readline/readline.h>

// soggy
#include "log.hpp"

void soggy_log(const char *fmt, ...) {
	bool use_rl = RL_ISSTATE(RL_STATE_INITIALIZED);
	if (use_rl) {
		rl_clear_visible_line();
	}
	va_list v;
	va_start(v, fmt);
	vprintf(fmt, v);
	va_end(v);
	putchar('\n');
	rl_on_new_line();
	if (use_rl) {
		rl_on_new_line();
		if (!rl_done) {
			rl_redisplay();
		}
	}
}
