/*
MIT License

Copyright (c) 2019 LAK132

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef LAK_DEBUG_H
#define LAK_DEBUG_H

#include <strconv/strconv.hpp>

#include <cstdlib>
#include <iostream>
#include <sstream>

#define TO_STRING(x) [&]{ std::stringstream s; s << x; return s.str(); }()
#define WTO_STRING(x) [&]{ std::wstringstream s; s << x; return lak::strconv_ascii(s.str()); }()

namespace lak
{
  struct debugger_t
  {
    std::stringstream stream;

    void std_out(const std::string &line_info, const std::string &str);

    void std_err(const std::string &line_info, const std::string &str);

    void clear();

    std::string str();
  };

  extern debugger_t debugger;
}

#ifdef DEBUG_LINE_FILE
#undef DEBUG_LINE_FILE
#endif

#ifdef _WIN32
#define DEBUG_LINE_FILE "(" << __FILE__ << ":" << std::dec << __LINE__ << ")"
#else
#define DEBUG_LINE_FILE "\x1B[2m(" << __FILE__ << ":" << std::dec << __LINE__ << ")\x1B[0m"
#endif

#ifdef WDEBUG_LINE_FILE
#undef WDEBUG_LINE_FILE
#endif

#ifdef _WIN32
#define WDEBUG_LINE_FILE L"(" << __FILE__ << L":" << std::dec << __LINE__ << L")"
#else
#define WDEBUG_LINE_FILE L"\x1B[2m(" << __FILE__ << L":" << std::dec << __LINE__ << L")\x1B[0m"
#endif

#ifdef DEBUG
#undef DEBUG
#endif

#ifdef NOLOG
#define DEBUG(x)
#else
#define DEBUG(x) lak::debugger.std_out(TO_STRING("DEBUG" << DEBUG_LINE_FILE << ": "), TO_STRING(std::hex << x << "\n"));
#endif

#ifdef WDEBUG
#undef WDEBUG
#endif

#ifdef NOLOG
#define WDEBUG(x)
#else
#define WDEBUG(x) lak::debugger.std_out(WTO_STRING(L"DEBUG" << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
#endif

#ifdef WARNING
#undef WARNING
#endif

#ifdef NOLOG
#define WARNING(x)
#else
#ifdef _WIN32
#define WARNING(x) lak::debugger.std_err(TO_STRING("WARNING" << DEBUG_LINE_FILE << ": "), TO_STRING(std::hex << x << "\n"));
#else
#define WARNING(x) lak::debugger.std_err(TO_STRING("\x1B[33m\x1B[1m" "WARNING" "\x1B[0m\x1B[33m" << DEBUG_LINE_FILE << ": "), TO_STRING(std::hex << x << "\n"));
#endif
#endif

#ifdef WWARNING
#undef WWARNING
#endif

#ifdef NOLOG
#define WWARNING(x)
#else
#ifdef _WIN32
#define WWARNING(x) lak::debugger.std_err(WTO_STRING(L"WARNING" << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
#else
#define WWARNING(x) lak::debugger.std_err(WTO_STRING(L"\x1B[33m\x1B[1m" L"WARNING" L"\x1B[0m\x1B[33m" << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
#endif
#endif

#ifdef ERROR
#undef ERROR
#endif

#ifdef NOLOG
#define ERROR(x)
#else
#ifdef _WIN32
#define ERROR(x) lak::debugger.std_err(TO_STRING("ERROR" << DEBUG_LINE_FILE << ": "), TO_STRING(std::hex << x << "\n"));
#else
#define ERROR(x) lak::debugger.std_err(TO_STRING("\x1B[91m\x1B[1m" "ERROR" "\x1B[0m\x1B[91m" << DEBUG_LINE_FILE << ": "), TO_STRING(std::hex << x << "\n"));
#endif
#endif

#ifdef WERROR
#undef WERROR
#endif

#ifdef NOLOG
#define WERROR(x)
#else
#ifdef _WIN32
#define WERROR(x) lak::debugger.std_err(WTO_STRING(L"ERROR" << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
#else
#define WERROR(x) lak::debugger.std_err(WTO_STRING(L"\x1B[91m\x1B[1m" L"ERROR" L"\x1B[0m\x1B[91m" << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
#endif
#endif

#ifdef FATAL
#undef FATAL
#endif

#ifdef _WIN32
#define FATAL(x) { lak::debugger.std_err(TO_STRING("FATAL" << DEBUG_LINE_FILE << ": "), TO_STRING(std::hex << x << "\n")); std::abort(); }
#else
#define FATAL(x) { lak::debugger.std_err(TO_STRING("\x1B[91m\x1B[1m" "FATAL" "\x1B[0m\x1B[91m" << DEBUG_LINE_FILE << ": "), TO_STRING(std::hex << x << "\n")); std::abort(); }
#endif

#define STRINGIFY_EX(x) #x
#define STRINGIFY(x) STRINGIFY_EX(x)

#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT(x) { if (!(x)) { FATAL("Assertion '" STRINGIFY(x) "' failed") } }

#ifdef ASSERTF
#undef ASSERTF
#endif

#define ASSERTF(x, str) { if (!(x)) { FATAL("Assertion failed: " str) } }

#endif