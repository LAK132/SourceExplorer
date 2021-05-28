// This is here to stop the #define ERROR clash caused by wingdi
#include <GL/gl3w.h>

#include <lak/bitset.hpp>

#include "main.cpp"

#include "dump.cpp"
#include "explorer.cpp"
#include "lisk_impl.cpp"

#include "imgui_impl_lak.cpp"
#include "imgui_utils.cpp"

#include <lak/src/opengl/shader.cpp>
#include <lak/src/opengl/texture.cpp>

#include <lak/src/debug.cpp>
#include <lak/src/events.cpp>
#include <lak/src/file.cpp>
#include <lak/src/memmanip.cpp>
#include <lak/src/platform.cpp>
#include <lak/src/profile.cpp>
#include <lak/src/strconv.cpp>
#include <lak/src/test.cpp>
#include <lak/src/tinflate.cpp>
#include <lak/src/tokeniser.cpp>
#include <lak/src/unicode.cpp>
#include <lak/src/window.cpp>

#ifndef NO_TESTS
// #define LAK_TEST_MAIN lak_test_main
// #include <lak/src/tests/test_main.cpp>
#  include <lak/src/tests/array.cpp>
#  include <lak/src/tests/bitflag.cpp>
#  include <lak/src/tests/bitset.cpp>
#  include <lak/src/tests/test.cpp>
#  include <lak/src/tests/trie.cpp>
#  include <lak/src/tests/type_pack.cpp>
#endif

#include <examples/imgui_impl_softraster.cpp>

#include <lisk/src/atom.cpp>
#include <lisk/src/callable.cpp>
#include <lisk/src/environment.cpp>
#include <lisk/src/eval.cpp>
#include <lisk/src/expression.cpp>
#include <lisk/src/functor.cpp>
#include <lisk/src/lambda.cpp>
#include <lisk/src/lisk.cpp>
#include <lisk/src/number.cpp>
#include <lisk/src/pointer.cpp>
#include <lisk/src/string.cpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

extern "C"
{
#include <GL/gl3w.c>
}

// Fuck you Micro$oft
#include <locale>
#if _MSC_VER >= 1900 && _MSC_VER <= 1926
std::locale::id std::codecvt<char16_t, char, _Mbstatet>::id;
std::locale::id std::codecvt<char32_t, char, _Mbstatet>::id;
#endif
