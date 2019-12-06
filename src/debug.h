/*
MIT License

Copyright (c) 2019 Lucas "LAK132" Kleiss

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

#ifdef NDEBUG
#define TRY try
#define CATCH(X) catch(X)
#else
#define TRY
#define CATCH(X) try {} catch(X)
#endif

#define STRINGIFY_EX(x) #x
#define STRINGIFY(x) STRINGIFY_EX(x)

#define LAK_ESC "\x1B"
#define LAK_CSI LAK_ESC "["
#define LAK_SGR(x) LAK_CSI STRINGIFY(x) "m"
#define LAK_SGR_RESET LAK_SGR(0)
#define LAK_BOLD LAK_SGR(1)
#define LAK_FAINT LAK_SGR(2)
#define LAK_ITALIC LAK_SGR(3)
#define LAK_YELLOW LAK_SGR(33)
#define LAK_BRIGHT_RED LAK_SGR(91)

#ifdef NOLOG
# define CHECKPOINT()
# define DEBUG(x)
# define WDEBUG(x)
#else
# ifdef _WIN32
#   define  DEBUG_LINE_FILE  "(" << __FILE__ <<  ":" << std::dec << __LINE__ <<  ")"
#   define WDEBUG_LINE_FILE L"(" << __FILE__ << L":" << std::dec << __LINE__ << L")"
# else
#   define  DEBUG_LINE_FILE     LAK_FAINT "(" << __FILE__ <<  ":" << std::dec << __LINE__ <<  ")" LAK_SGR_RESET
#   define WDEBUG_LINE_FILE L"" LAK_FAINT "(" << __FILE__ << L":" << std::dec << __LINE__ << L")" LAK_SGR_RESET
# endif
# define CHECKPOINT() std::cerr << "CHECKPOINT" << DEBUG_LINE_FILE << "\n";
# define  DEBUG(x) lak::debugger.std_out( TO_STRING( "DEBUG" <<  DEBUG_LINE_FILE <<  ": "),  TO_STRING(std::hex << x <<  "\n"));
# define WDEBUG(x) lak::debugger.std_out(WTO_STRING(L"DEBUG" << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
#endif

#define ABORT() { std::cerr << "Aborted. Press enter to continue..."; getchar(); std::abort(); }

#if defined(NOLOG)
# define  WARNING(x)
# define WWARNING(x)
# define  ERROR(x)
# define WERROR(x)
# define ABORTF(x) ABORT()
# define FATAL(x)
#elif defined(_WIN32)
// Windows needs to seriously fuck off
# undef ERROR
# define  WARNING(x) lak::debugger.std_err( TO_STRING( "WARNING" <<  DEBUG_LINE_FILE <<  ": "),  TO_STRING(std::hex << x <<  "\n"));
# define WWARNING(x) lak::debugger.std_err(WTO_STRING(L"WARNING" << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
# define  ERROR(x)   lak::debugger.std_err( TO_STRING( "ERROR"   <<  DEBUG_LINE_FILE <<  ": "),  TO_STRING(std::hex << x <<  "\n"));
# define WERROR(x)   lak::debugger.std_err(WTO_STRING(L"ERROR"   << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
# define ABORTF(x) { std::cerr << X << "\n"; ABORT() }
# define FATAL(x)  { lak::debugger.std_err( TO_STRING( "FATAL"   <<  DEBUG_LINE_FILE <<  ": "),  TO_STRING(std::hex << x << "\n")); ABORT(); }
#else
# define  WARNING(x) lak::debugger.std_err( TO_STRING(    LAK_YELLOW     LAK_BOLD "WARNING" LAK_SGR_RESET LAK_YELLOW     <<  DEBUG_LINE_FILE <<  ": "),  TO_STRING(std::hex << x <<  "\n"));
# define WWARNING(x) lak::debugger.std_err(WTO_STRING(L"" LAK_YELLOW     LAK_BOLD "WARNING" LAK_SGR_RESET LAK_YELLOW     << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
# define  ERROR(x)   lak::debugger.std_err( TO_STRING(    LAK_BRIGHT_RED LAK_BOLD "ERROR"   LAK_SGR_RESET LAK_BRIGHT_RED <<  DEBUG_LINE_FILE <<  ": "),  TO_STRING(std::hex << x <<  "\n"));
# define WERROR(x)   lak::debugger.std_err(WTO_STRING(L"" LAK_BRIGHT_RED LAK_BOLD "ERROR"   LAK_SGR_RESET LAK_BRIGHT_RED << WDEBUG_LINE_FILE << L": "), WTO_STRING(std::hex << x << L"\n"));
# define ABORTF(x) { std::cerr << X << "\n"; ABORT(); }
# define FATAL(x)  { lak::debugger.std_err(TO_STRING(     LAK_BRIGHT_RED LAK_BOLD "FATAL"   LAK_SGR_RESET LAK_BRIGHT_RED <<  DEBUG_LINE_FILE <<  ": "),  TO_STRING(std::hex << x << "\n")); ABORT(); }
#endif

#define ASSERT(x)       { if (!(x)) { FATAL("Assertion '" STRINGIFY(x) "' failed"); } }
#define ASSERTF(x, str) { if (!(x)) { FATAL("Assertion '" STRINGIFY(x) "' failed: " str); } }
#define NOISY_ABORT()   { ABORTF(lak::debugger.stream.str()); }

#endif