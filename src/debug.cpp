#include "debug.h"

#include "explorer.h"

namespace lak
{
  void debugger_t::std_out(const std::string &line_info, const std::string &str)
  {
    stream << line_info << str;
    if (::SourceExplorer::debugConsole && !::SourceExplorer::errorOnlyConsole)
    {
      if (::SourceExplorer::developerConsole)
      {
        std::cout << line_info;
      }
      std::cout << str << std::flush;
    }
  }

  void debugger_t::std_err(const std::string &line_info, const std::string &str)
  {
    stream << line_info << str;
    std::cerr << line_info << str << std::flush;
  }

  void debugger_t::clear()
  {
    stream.clear();
  }

  std::string debugger_t::str()
  {
    return stream.str();
  }

  fs::path debugger_t::save()
  {
    return save(crash_path);
  }

  fs::path debugger_t::save(const fs::path &path)
  {
    lak::save_file(path, str());
    std::error_code ec;
    if (auto absolute = fs::absolute(path, ec); ec)
      return path;
    else
      return absolute;
  }

  debugger_t debugger;
}