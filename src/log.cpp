// log.cpp

// C++
#include <cstdio>
#include <cstdarg>

// replxx
#include <replxx.hxx>

// soggy
#include "log.hpp"

replxx::Replxx soggy_rx;

void soggy_log(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int size = vsnprintf(NULL, 0, fmt, va);
	va_end(va);
	va_start(va, fmt);
	std::unique_ptr<char[]> buf(new char[size + 1]);
	vsnprintf(buf.get(), size + 1, fmt, va);
	va_end(va);
	soggy_rx.print("%s\n", buf.get());
}
