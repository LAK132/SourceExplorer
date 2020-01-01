#include "debug.h"

#include "explorer.h"

namespace lak
{
  void debugger_t::std_out(const std::string &line_info, const std::string &str)
  {
    stream << scoped_indenter::str() << line_info << str;
    if (::SourceExplorer::debugConsole && !::SourceExplorer::errorOnlyConsole)
    {
      std::cout << scoped_indenter::str();
      if (::SourceExplorer::developerConsole)
      {
        std::cout << line_info;
      }
      std::cout << str << std::flush;
    }
  }

  void debugger_t::std_err(const std::string &line_info, const std::string &str)
  {
    stream << scoped_indenter::str() << line_info << str;
    std::cerr << scoped_indenter::str() << line_info << str << std::flush;
  }

  void debugger_t::clear()
  {
    stream.clear();
  }

  std::string debugger_t::str()
  {
    return stream.str();
  }

  std::filesystem::path debugger_t::save()
  {
    return save(crash_path);
  }

  std::filesystem::path debugger_t::save(const std::filesystem::path &path)
  {
    if (!path.string().empty()) lak::save_file(path, str());

    std::error_code ec;
    if (auto absolute = std::filesystem::absolute(path, ec); ec)
      return path;
    else
      return absolute;
  }

  scoped_indenter::scoped_indenter()
  {
    ++debug_indent;
  }

  scoped_indenter::~scoped_indenter()
  {
    --debug_indent;
  }

  std::string scoped_indenter::str()
  {
    std::string s;
    for (size_t i = debug_indent; i --> 0;) s += "|   ";
    return s;
  }

  std::wstring scoped_indenter::wstr()
  {
    std::wstring s;
    for (size_t i = debug_indent; i --> 0;) s += L"|   ";
    return s;
  }

  size_t debug_indent;

  debugger_t debugger;
}