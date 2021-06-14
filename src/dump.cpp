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

#ifdef _WIN32
#  define STBI_MSC_SECURE_CRT
#endif
#include <stb_image_write.h>

#include "dump.h"
#include "explorer.h"
#include "tostring.hpp"

#include <lak/result.hpp>
#include <lak/string_utils.hpp>
#include <lak/visit.hpp>

#include <unordered_set>

#ifdef GetObject
#  undef GetObject
#endif

namespace se = SourceExplorer;

se::error_t se::SaveImage(const lak::image4_t &image, const fs::path &filename)
{
  if (stbi_write_png(filename.u8string().c_str(),
                     (int)image.size().x,
                     (int)image.size().y,
                     4,
                     &(image[0].r),
                     (int)(image.size().x * 4)) != 1)
  {
    return lak::err_t{se::error(LINE_TRACE,
                                se::error::str_err,
                                "Failed to save image '",
                                filename,
                                "'")};
  }
  return lak::ok_t{};
}

se::error_t se::SaveImage(source_explorer_t &srcexp,
                          uint16_t handle,
                          const fs::path &filename,
                          const frame::item_t *frame)
{
  return GetImage(srcexp.state, handle)
    .MAP_SE_ERR("failed to get image item")
    .and_then([&](const auto &item) {
      return item.image(
        srcexp.dump_color_transparent,
        (frame && frame->palette) ? frame->palette->colors.data() : nullptr);
    })
    .MAP_SE_ERR("failed to read image data")
    .and_then([&](const auto &image) { return SaveImage(image, filename); });
}

lak::await_result<se::error_t> se::OpenGame(source_explorer_t &srcexp)
{
  static lak::await<se::error_t> awaiter;

  if (auto result = awaiter(LoadGame, std::ref(srcexp)); result.is_ok())
  {
    return lak::ok_t{result.unwrap().MAP_SE_ERR("OpenGame")};
  }
  else
  {
    switch (result.unwrap_err())
    {
      case lak::await_error::running:
      {
        const auto str_id = "Open Game";
        if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize))
        {
          ImGui::Text("Loading, please wait...");
          ImGui::Checkbox("Print to debug console?",
                          &lak::debugger.live_output_enabled);
          if (lak::debugger.live_output_enabled)
          {
            ImGui::Checkbox("Only errors?", &lak::debugger.live_errors_only);
            ImGui::Checkbox("Developer mode?",
                            &lak::debugger.line_info_enabled);
          }
          ImGui::ProgressBar(srcexp.state.completed);
          ImGui::ProgressBar(srcexp.state.bank_completed);
          ImGui::EndPopup();
        }
        else
        {
          ImGui::OpenPopup(str_id);
        }

        return lak::err_t{lak::await_error::running};
      }
      break;

      case lak::await_error::failed:
        return lak::err_t{lak::await_error::failed};
        break;

      default:
        ASSERT_NYI();
        return lak::err_t{lak::await_error::failed};
        break;
    }
  }
}

bool se::DumpStuff(source_explorer_t &srcexp,
                   const char *str_id,
                   dump_function_t *func)
{
  static lak::await<se::error_t> awaiter;
  static std::atomic<float> completed = 0.0f;

  auto functor = [&, func]() -> se::error_t {
    func(srcexp, completed);
    return lak::ok_t{};
  };

  if (auto result = awaiter(functor); result.is_ok())
  {
    return true;
  }
  else
  {
    switch (result.unwrap_err())
    {
      case lak::await_error::running:
      {
        if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize))
        {
          ImGui::Text("Dumping, please wait...");
          ImGui::Checkbox("Print to debug console?",
                          &lak::debugger.live_output_enabled);
          if (&lak::debugger.live_output_enabled)
          {
            ImGui::Checkbox("Only errors?", &lak::debugger.live_errors_only);
            ImGui::Checkbox("Developer mode?",
                            &lak::debugger.line_info_enabled);
          }
          ImGui::ProgressBar(completed);
          ImGui::EndPopup();
        }
        else
        {
          ImGui::OpenPopup(str_id);
        }
        return false;
      }
      break;

      case lak::await_error::failed:
      default:
      {
        ASSERT_NYI();
        return false;
      }
      break;
    }
  }
}

void se::DumpImages(source_explorer_t &srcexp, std::atomic<float> &completed)
{
  if (!srcexp.state.game.image_bank)
  {
    ERROR("No Image Bank");
    return;
  }

  const size_t count = srcexp.state.game.image_bank->items.size();
  size_t index       = 0;
  for (const auto &item : srcexp.state.game.image_bank->items)
  {
    lak::image4_t image = item.image(srcexp.dump_color_transparent).UNWRAP();
    fs::path filename =
      srcexp.images.path / (std::to_string(item.entry.handle) + ".png");
    (void)SaveImage(image, filename);
    completed = (float)((double)(index++) / (double)count);
  }
}

void se::DumpSortedImages(se::source_explorer_t &srcexp,
                          std::atomic<float> &completed)
{
  if (!srcexp.state.game.image_bank)
  {
    ERROR("No Image Bank");
    return;
  }

  if (!srcexp.state.game.frame_bank)
  {
    ERROR("No Frame Bank");
    return;
  }

  if (!srcexp.state.game.object_bank)
  {
    ERROR("No Object Bank");
    return;
  }

  auto LinkImages = [](const fs::path &From, const fs::path &To)
    -> lak::error_codes<std::error_code, lak::u8string> {
    std::error_code ec;

    if (fs::exists(To, ec))
    {
      return lak::err_t{
        lak::var_t<1>(lak::streamify<char8_t>(To, " already exists"))};
    }
    else if (!ec)
    {
      if (fs::exists(From, ec))
      {
        fs::create_directories(To.parent_path(), ec);
        if (!ec) fs::create_hard_link(From, To, ec);
      }
      else if (!ec)
      {
        return lak::err_t{
          lak::var_t<1>(lak::streamify<char8_t>(From, " does not exist"))};
      }
    }

    if (ec)
    {
      return lak::err_t{lak::var_t<0>(ec)};
    }

    return lak::ok_t{};
  };

  using namespace std::string_literals;

  auto HandleName = [](const std::unique_ptr<string_chunk_t> &name,
                       auto handle,
                       std::u16string extra = u"") {
    std::u32string str;
    if (extra.size() > 0) str += lak::to_u32string(extra + u" ");
    if (name) str += lak::to_u32string(name->value);
    std::u16string result;
    for (auto &c : str)
      if (c == U' ' || c == U'(' || c == U')' || c == U'[' || c == U']' ||
          c == U'+' || c == U'-' || c == U'=' || c == U'_' ||
          (c >= U'0' && c <= U'9') || (c >= U'a' && c <= U'z') ||
          (c >= U'A' && c <= U'Z') || c > 127)
        result += lak::to_u16string(std::u32string() + c);
    return u"["s + se::to_u16string(handle) + (result.empty() ? u"]" : u"] ") +
           result;
  };

  fs::path root_path     = srcexp.sorted_images.path;
  fs::path unsorted_path = root_path / "[unsorted]";
  fs::create_directories(unsorted_path);
  std::error_code err;

  size_t image_index       = 0;
  const size_t image_count = srcexp.state.game.image_bank->items.size();
  for (const auto &image : srcexp.state.game.image_bank->items)
  {
    std::u16string image_name = se::to_u16string(image.entry.handle) + u".png";
    fs::path image_path       = unsorted_path / image_name;
    (void)SaveImage(image.image(srcexp.dump_color_transparent).UNWRAP(),
                    image_path);
    completed = (float)((double)image_index++ / image_count);
  }

  size_t frame_index       = 0;
  const size_t frame_count = srcexp.state.game.frame_bank->items.size();
  for (const auto &frame : srcexp.state.game.frame_bank->items)
  {
    std::u16string frame_name = HandleName(frame.name, frame_index);
    fs::path frame_path       = root_path / frame_name;
    fs::create_directories(frame_path / "[unsorted]", err);
    if (err)
    {
      ERROR("File System Error: (", err.value(), ")", err.message());
      continue;
    }

    if (frame.object_instances)
    {
      std::unordered_set<uint32_t> used_images;
      std::unordered_set<uint16_t> used_objects;
      for (const auto &object : frame.object_instances->objects)
      {
        if (used_objects.find(object.handle) != used_objects.end()) continue;
        used_objects.insert(object.handle);
        if (const auto *obj =
              lak::as_ptr(se::GetObject(srcexp.state, object.handle).ok());
            obj)
        {
          std::u16string object_name = HandleName(
            obj->name,
            obj->handle,
            u"[" +
              lak::to_u16string(lak::astring(GetObjectTypeString(obj->type))) +
              u"]");
          fs::path object_path = frame_path / object_name;
          fs::create_directories(object_path, err);
          if (err)
          {
            ERROR("File System Error: (", err.value(), ")", err.message());
            continue;
          }

          for (auto [imghandle, imgnames] : obj->image_handles())
          {
            if (imghandle == 0xFFFF) continue;
            if (const auto *img =
                  lak::as_ptr(GetImage(srcexp.state, imghandle).ok());
                img)
            {
              if (used_images.find(imghandle) == used_images.end())
              {
                used_images.insert(imghandle);
                std::u16string image_name =
                  se::to_u16string(imghandle) + u".png";
                fs::path image_path = frame_path / "[unsorted]" / image_name;

                // check if 8bit image
                if (img->need_palette() && frame.palette)
                  (void)SaveImage(img
                                    ->image(srcexp.dump_color_transparent,
                                            frame.palette->colors.data())
                                    .UNWRAP(),
                                  image_path);
                else if (auto res =
                           LinkImages(unsorted_path / image_name, image_path);
                         res.is_err())
                  lak::visit(
                    res.unwrap_err(),
                    lak::overloaded{
                      [](const std::error_code &err) {
                        ERROR("Linking Failed: (",
                              err.value(),
                              ")",
                              err.message());
                      },
                      [](const auto &err) { ERROR("Linking Failed: ", err); },
                    });
              }
              for (const auto &imgname : imgnames)
              {
                std::u16string unsorted_image_name =
                  se::to_u16string(imghandle) + u".png";
                fs::path unsorted_image_path =
                  frame_path / "[unsorted]" / unsorted_image_name;
                std::u16string image_name = imgname + u".png";
                fs::path image_path       = object_path / image_name;
                if (const auto *img =
                      lak::as_ptr(GetImage(srcexp.state, imghandle).ok());
                    img)
                  if (auto res = LinkImages(unsorted_image_path, image_path);
                      res.is_err())
                    lak::visit(res.unwrap_err(),
                               lak::overloaded{
                                 [](const std::error_code &err) {
                                   ERROR("Linking Failed: (",
                                         err.value(),
                                         ")",
                                         err.message());
                                 },
                                 [](const auto &err) {
                                   ERROR("Linking Failed: ", err);
                                 },
                               });
              }
            }
          }
        }
      }
    }
    completed = (float)((double)frame_index++ / frame_count);
  }
}

void se::DumpAppIcon(source_explorer_t &srcexp, std::atomic<float> &completed)
{
  if (!srcexp.state.game.icon)
  {
    ERROR("No Icon");
    return;
  }

  lak::image4_t &bitmap = srcexp.state.game.icon->bitmap;

  fs::path filename = srcexp.appicon.path / "favicon.ico";
  std::ofstream file(filename,
                     std::ios::binary | std::ios::out | std::ios::ate);
  if (!file.is_open()) return;

  stbi_write_func *func = [](void *context, void *png, int len) {
    auto [out, image] =
      *static_cast<std::tuple<std::ofstream *, lak::image4_t *> *>(context);
    lak::binary_array_writer strm;
    strm.reserve(0x16);
    strm.write_u16(0); // reserved
    strm.write_u16(1); // .ICO
    strm.write_u16(1); // 1 image
    strm.write_u8(image->size().x);
    strm.write_u8(image->size().y);
    strm.write_u8(0);      // no palette
    strm.write_u8(0);      // reserved
    strm.write_u16(1);     // color plane
    strm.write_u16(8 * 4); // bits per pixel
    strm.write_u32(len);
    strm.write_u32(strm.size() + sizeof(uint32_t));
    auto result = strm.release();
    out->write(reinterpret_cast<const char *>(result.data()), result.size());
    out->write((const char *)png, len);
  };

  auto context = std::tuple<std::ofstream *, lak::image4_t *>(&file, &bitmap);
  stbi_write_png_to_func(func,
                         &context,
                         (int)bitmap.size().x,
                         (int)bitmap.size().y,
                         4,
                         bitmap.data(),
                         (int)(bitmap.size().x * 4));

  file.close();
}

void se::DumpSounds(source_explorer_t &srcexp, std::atomic<float> &completed)
{
  if (!srcexp.state.game.sound_bank)
  {
    ERROR("No Sound Bank");
    return;
  }

  auto arr_to_str = [](const auto &arr) {
    return lak::string<lak::remove_cvref_t<decltype(*arr.begin())>>(
      arr.begin(), arr.end());
  };

  const size_t count = srcexp.state.game.sound_bank->items.size();
  size_t index       = 0;
  for (const auto &item : srcexp.state.game.sound_bank->items)
  {
    data_reader_t sound(item.entry.decode_body().UNWRAP());
    lak::array<uint8_t> result;

    std::u16string name;
    sound_mode_t type;

    if (srcexp.state.old_game)
    {
      [[maybe_unused]] uint16_t checksum   = sound.read_u16().UNWRAP();
      [[maybe_unused]] uint32_t references = sound.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t decomp_len = sound.read_u32().UNWRAP();
      type = (sound_mode_t)sound.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t reserved = sound.read_u32().UNWRAP();
      uint32_t name_len                  = sound.read_u32().UNWRAP();

      name =
        lak::to_u16string(arr_to_str(sound.read<char>(name_len).UNWRAP()));

      [[maybe_unused]] uint16_t format          = sound.read_u16().UNWRAP();
      [[maybe_unused]] uint16_t channel_count   = sound.read_u16().UNWRAP();
      [[maybe_unused]] uint32_t sample_rate     = sound.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t byte_rate       = sound.read_u32().UNWRAP();
      [[maybe_unused]] uint16_t block_align     = sound.read_u16().UNWRAP();
      [[maybe_unused]] uint16_t bits_per_sample = sound.read_u16().UNWRAP();
      [[maybe_unused]] uint16_t unknown         = sound.read_u16().UNWRAP();
      uint32_t chunk_size                       = sound.read_u32().UNWRAP();
      [[maybe_unused]] auto data = sound.read<uint8_t>(chunk_size).UNWRAP();

      lak::binary_array_writer output;
      output.write("RIFF"_aview);
      output.write_s32(data.size() - 44);
      output.write("WAVEfmt "_aview);
      output.write_u32(0x10);
      output.write_u16(format);
      output.write_u16(channel_count);
      output.write_u32(sample_rate);
      output.write_u32(byte_rate);
      output.write_u16(block_align);
      output.write_u16(bits_per_sample);
      output.write("data"_aview);
      output.write_u32(chunk_size);
      output.write(lak::span(data));
      result = output.release();
    }
    else
    {
      data_reader_t header(item.entry.decode_head().UNWRAP());

      [[maybe_unused]] uint32_t checksum   = header.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t references = header.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t decomp_len = header.read_u32().UNWRAP();
      type = (sound_mode_t)header.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t reserved = header.read_u32().UNWRAP();
      uint32_t name_len                  = header.read_u32().UNWRAP();

      if (srcexp.state.unicode)
      {
        name = arr_to_str(sound.read<char16_t>(name_len).UNWRAP());
        DEBUG("u16string name: ", name);
      }
      else
      {
        name =
          lak::to_u16string(arr_to_str(sound.read<char>(name_len).UNWRAP()));
        DEBUG("u8string name: ", name);
      }

      if (const auto peek = sound.peek<char>(4).UNWRAP();
          lak::span(peek) == "OggS"_aview)
      {
        type = sound_mode_t::oggs;
      }
      else if (lak::span(peek) == "Exte"_aview)
      {
        type = sound_mode_t::xm;
      }

      result = lak::array<uint8_t>(sound.remaining().begin(),
                                   sound.remaining().end());
    }

    switch (type)
    {
      case sound_mode_t::wave: name += u".wav"; break;
      case sound_mode_t::midi: name += u".midi"; break;
      case sound_mode_t::oggs: name += u".ogg"; break;
      case sound_mode_t::xm: name += u".xm"; break;
      default: name += u".mp3"; break;
    }

    DEBUG("MP3", (size_t)item.entry.ID);

    fs::path filename = srcexp.sounds.path / name;

    if (!lak::save_file(filename, result))
    {
      ERROR("Failed To Save File '", filename, "'");
    }

    completed = (float)((double)(index++) / (double)count);
  }
}

void se::DumpMusic(source_explorer_t &srcexp, std::atomic<float> &completed)
{
  if (!srcexp.state.game.music_bank)
  {
    ERROR("No Music Bank");
    return;
  }

  auto arr_to_str = [](const auto &arr) {
    return lak::string<lak::remove_cvref_t<decltype(*arr.begin())>>(
      arr.begin(), arr.end());
  };

  const size_t count = srcexp.state.game.music_bank->items.size();
  size_t index       = 0;
  for (const auto &item : srcexp.state.game.music_bank->items)
  {
    data_reader_t sound(item.entry.decode_body().UNWRAP());

    std::u16string name;
    sound_mode_t type;

    if (srcexp.state.old_game)
    {
      [[maybe_unused]] uint16_t checksum   = sound.read_u16().UNWRAP();
      [[maybe_unused]] uint32_t references = sound.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t decomp_len = sound.read_u32().UNWRAP();
      type = (sound_mode_t)sound.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t reserved = sound.read_u32().UNWRAP();
      uint32_t name_len                  = sound.read_u32().UNWRAP();

      name =
        lak::to_u16string(arr_to_str(sound.read<char>(name_len).UNWRAP()));
    }
    else
    {
      [[maybe_unused]] uint32_t checksum   = sound.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t references = sound.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t decomp_len = sound.read_u32().UNWRAP();
      type = (sound_mode_t)sound.read_u32().UNWRAP();
      [[maybe_unused]] uint32_t reserved = sound.read_u32().UNWRAP();
      uint32_t name_len                  = sound.read_u32().UNWRAP();

      if (srcexp.state.unicode)
      {
        name = arr_to_str(sound.read<char16_t>(name_len).UNWRAP());
      }
      else
      {
        name =
          lak::to_u16string(arr_to_str(sound.read<char>(name_len).UNWRAP()));
      }
    }

    switch (type)
    {
      case sound_mode_t::wave: name += u".wav"; break;
      case sound_mode_t::midi: name += u".midi"; break;
      default: name += u".mp3"; break;
    }

    fs::path filename = srcexp.music.path / name;

    if (!lak::save_file(filename, sound.remaining()))
    {
      ERROR("Failed To Save File '", filename, "'");
    }

    completed = (float)((double)(index++) / (double)count);
  }
}

void se::DumpShaders(source_explorer_t &srcexp, std::atomic<float> &completed)
{
  if (!srcexp.state.game.shaders)
  {
    ERROR("No Shaders");
    return;
  }

  data_reader_t strm(srcexp.state.game.shaders->entry.decode_body().UNWRAP());

  uint32_t count = strm.read_u32().UNWRAP();
  std::vector<uint32_t> offsets;
  offsets.reserve(count);

  while (count-- > 0) offsets.push_back(strm.read_u32().UNWRAP());

  for (auto offset : offsets)
  {
    strm.seek(offset).UNWRAP();
    uint32_t name_offset                   = strm.read_u32().UNWRAP();
    uint32_t data_offset                   = strm.read_u32().UNWRAP();
    [[maybe_unused]] uint32_t param_offset = strm.read_u32().UNWRAP();
    [[maybe_unused]] uint32_t bank_tex     = strm.read_u32().UNWRAP();

    strm.seek(offset + name_offset).UNWRAP();
    fs::path filename = srcexp.shaders.path / strm.read_c_str<char>().UNWRAP();

    strm.seek(offset + data_offset).UNWRAP();
    lak::astring file = strm.read_c_str<char>().UNWRAP();

    DEBUG(filename);
    if (!lak::save_file(filename,
                        std::vector<uint8_t>(file.begin(), file.end())))
    {
      ERROR("Failed To Save File '", filename, "'");
    }

    completed = (float)((double)count++ / (double)offsets.size());
  }
}

void se::DumpBinaryFiles(source_explorer_t &srcexp,
                         std::atomic<float> &completed)
{
  if (!srcexp.state.game.binary_files)
  {
    ERROR("No Binary Files");
    return;
  }

  data_reader_t strm(
    srcexp.state.game.binary_files->entry.decode_body().UNWRAP());

  const size_t count = srcexp.state.game.binary_files->items.size();
  size_t index       = 0;
  for (const auto &file : srcexp.state.game.binary_files->items)
  {
    fs::path filename = lak::to_u16string(file.name);
    filename          = srcexp.binary_files.path / filename.filename();
    DEBUG(filename);
    if (!lak::save_file(filename, file.data))
    {
      ERROR("Failed To Save File '", filename, "'");
    }
    completed = (float)((double)index++ / (double)count);
  }
}

void se::SaveErrorLog(source_explorer_t &srcexp, std::atomic<float> &completed)
{
  if (!lak::save_file(srcexp.error_log.path, lak::debugger.str()))
  {
    ERROR("Failed To Save File '", srcexp.error_log.path, "'");
  }
}

template<typename FUNCTOR>
void AttemptFile(se::file_state_t &file_state,
                 FUNCTOR functor,
                 bool save = false)
{
  se::Attempt(
    file_state,
    [save](se::file_state_t &file_state) {
      if (auto result = lak::open_file_modal(file_state.path, save);
          result.is_ok())
      {
        if (auto code = result.unwrap();
            code == lak::file_open_error::INCOMPLETE)
        {
          // Not finished.
          return false;
        }
        else
        {
          file_state.valid = code == lak::file_open_error::VALID;
          return true;
        }
      }
      else
      {
        file_state.valid = false;
        return true;
      }
    },
    functor);
}

template<typename FUNCTOR>
void AttemptFolder(se::file_state_t &file_state, FUNCTOR functor)
{
  auto load = [](se::file_state_t &file_state) {
    std::error_code ec;
    if (auto result = lak::open_folder_modal(file_state.path); result.is_ok())
    {
      if (auto code = result.unwrap();
          code == lak::file_open_error::INCOMPLETE)
      {
        // Not finished.
        return false;
      }
      else
      {
        file_state.valid = code == lak::file_open_error::VALID;
        return true;
      }
    }
    else
    {
      file_state.valid = false;
      return true;
    }
  };
  se::Attempt(file_state, load, functor);
}

void se::AttemptExe(source_explorer_t &srcexp)
{
  lak::debugger.clear();
  srcexp.loaded = false;
  AttemptFile(srcexp.exe, [&srcexp] {
    if (auto result = OpenGame(srcexp); result.is_err())
    {
      ASSERT(result.unwrap_err() == lak::await_error::running);
      return false;
    }
    else if (result.unwrap().is_err())
    {
      result.unwrap().IF_ERR("AttemptExe failed").discard();
      // ERROR(result.unwrap()
      //         .MAP_SE_ERR("AttemptExe failed")
      //         .unwrap_err());
      srcexp.loaded = true;
      return true;
    }
    else
    {
      srcexp.loaded = true;
      if (srcexp.baby_mode)
      {
        // Autotragically dump everything

        fs::path dump_dir =
          srcexp.exe.path.parent_path() / srcexp.exe.path.stem();

        std::error_code er;
        if (srcexp.state.game.image_bank)
        {
          srcexp.sorted_images.path = dump_dir / "images";
          if (fs::create_directories(srcexp.sorted_images.path, er); er)
          {
            ERROR("Failed To Dump Images");
            ERROR("File System Error: (", er.value(), ")", er.message());
          }
          else
          {
            srcexp.sorted_images.attempt = true;
            srcexp.sorted_images.valid   = true;
          }
        }

        if (srcexp.state.game.icon)
        {
          srcexp.appicon.path = dump_dir / "icon";
          if (fs::create_directories(srcexp.appicon.path, er); er)
          {
            ERROR("Failed To Dump Icon");
            ERROR("File System Error: ", er.message());
          }
          else
          {
            srcexp.appicon.attempt = true;
            srcexp.appicon.valid   = true;
          }
        }

        if (srcexp.state.game.sound_bank)
        {
          srcexp.sounds.path = dump_dir / "sounds";
          if (fs::create_directories(srcexp.sounds.path, er); er)
          {
            ERROR("Failed To Dump Audio");
            ERROR("File System Error: ", er.message());
          }
          else
          {
            srcexp.sounds.attempt = true;
            srcexp.sounds.valid   = true;
          }
        }

        if (srcexp.state.game.music_bank)
        {
          srcexp.music.path = dump_dir / "music";
          if (fs::create_directories(srcexp.sounds.path, er); er)
          {
            ERROR("Failed To Dump Audio");
            ERROR("File System Error: ", er.message());
          }
          else
          {
            srcexp.music.attempt = true;
            srcexp.music.valid   = true;
          }
        }

        if (srcexp.state.game.shaders)
        {
          srcexp.shaders.path = dump_dir / "shaders";
          if (fs::create_directories(srcexp.shaders.path, er); er)
          {
            ERROR("Failed To Dump Shaders");
            ERROR("File System Error: ", er.message());
          }
          else
          {
            srcexp.shaders.attempt = true;
            srcexp.shaders.valid   = true;
          }
        }

        if (srcexp.state.game.binary_files)
        {
          srcexp.binary_files.path = dump_dir / "binary_files";
          if (fs::create_directories(srcexp.binary_files.path, er); er)
          {
            ERROR("Failed To Dump Binary Files");
            ERROR("File System Error: ", er.message());
          }
          else
          {
            srcexp.binary_files.attempt = true;
            srcexp.binary_files.valid   = true;
          }
        }
      }
      return true;
    }
  });
}

void se::AttemptImages(source_explorer_t &srcexp)
{
  AttemptFolder(srcexp.images, [&srcexp] {
    return DumpStuff(srcexp, "Dump Images", &DumpImages);
  });
}

void se::AttemptSortedImages(source_explorer_t &srcexp)
{
  AttemptFolder(srcexp.sorted_images, [&srcexp] {
    return DumpStuff(srcexp, "Dump Sorted Images", &DumpSortedImages);
  });
}

void se::AttemptAppIcon(source_explorer_t &srcexp)
{
  AttemptFolder(srcexp.appicon, [&srcexp] {
    return DumpStuff(srcexp, "Dump App Icon", &DumpAppIcon);
  });
}

void se::AttemptSounds(source_explorer_t &srcexp)
{
  AttemptFolder(srcexp.sounds, [&srcexp] {
    return DumpStuff(srcexp, "Dump Sounds", &DumpSounds);
  });
}

void se::AttemptMusic(source_explorer_t &srcexp)
{
  AttemptFolder(srcexp.music, [&srcexp] {
    return DumpStuff(srcexp, "Dump Music", &DumpMusic);
  });
}

void se::AttemptShaders(source_explorer_t &srcexp)
{
  AttemptFolder(srcexp.shaders, [&srcexp] {
    return DumpStuff(srcexp, "Dump Shaders", &DumpShaders);
  });
}

void se::AttemptBinaryFiles(source_explorer_t &srcexp)
{
  AttemptFolder(srcexp.binary_files, [&srcexp] {
    return DumpStuff(srcexp, "Dump Binary Files", &DumpBinaryFiles);
  });
}

void se::AttemptErrorLog(source_explorer_t &srcexp)
{
  AttemptFile(
    srcexp.error_log,
    [&srcexp] { return DumpStuff(srcexp, "Save Error Log", &SaveErrorLog); },
    true);
}
