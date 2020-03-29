#ifndef LAK_TOSTRING_HPP
#define LAK_TOSTRING_HPP

#include <lak/strconv.hpp>

namespace lak
{
  template<typename TO, typename FROM>
  std::basic_string<TO> to_string(FROM from)
  {
    return strconv<TO>(std::to_string(from));
  }

  template<typename FROM>
  std::string to_string(FROM from)
  {
    return std::to_string(from);
  }

  template<typename FROM>
  std::wstring to_wstring(FROM from)
  {
    return std::to_wstring(from);
  }

  template<typename FROM>
  std::u8string to_u8string(FROM from)
  {
    return strconv_u8(std::to_string(from));
  }

  template<typename FROM>
  std::u16string to_u16string(FROM from)
  {
    return strconv_u16(std::to_string(from));
  }

  template<typename FROM>
  std::u32string to_u32string(FROM from)
  {
    return strconv_u32(std::to_string(from));
  }
}

#endif