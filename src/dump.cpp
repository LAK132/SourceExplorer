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
    return lak::err_t{
      lak::streamify<char8_t>("Failed to save image '", filename, "'")};
  }
  return lak::ok_t{};
}

se::error_t se::SaveImage(source_explorer_t &srcexp,
                          uint16_t handle,
                          const fs::path &filename,
                          const frame::item_t *frame)
{
  HANDLE_RESULT_ERR(error);

  RES_TRY(lak::image4_t image =,
          GetImage(srcexp.state, handle)
            .map_err(APPEND_TRACE("failed to save image")))
    .image(srcexp.dumpColorTrans,
           (frame && frame->palette) ? frame->palette->colors : nullptr);
  return SaveImage(image, filename);
}

lak::await_result<se::error_t> se::OpenGame(source_explorer_t &srcexp)
{
  static lak::await<se::error_t> awaiter;

  if (auto result = awaiter(LoadGame, std::ref(srcexp)); result.is_ok())
  {
    return lak::ok_t{result.unwrap().map_err(APPEND_TRACE())};
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
  if (!srcexp.state.game.imageBank)
  {
    ERROR("No Image Bank");
    return;
  }

  const size_t count = srcexp.state.game.imageBank->items.size();
  size_t index       = 0;
  for (const auto &item : srcexp.state.game.imageBank->items)
  {
    lak::image4_t image = item.image(srcexp.dumpColorTrans);
    fs::path filename =
      srcexp.images.path / (std::to_string(item.entry.handle) + ".png");
    (void)SaveImage(image, filename);
    completed = (float)((double)(index++) / (double)count);
  }
}

void se::DumpSortedImages(se::source_explorer_t &srcexp,
                          std::atomic<float> &completed)
{
  if (!srcexp.state.game.imageBank)
  {
    ERROR("No Image Bank");
    return;
  }

  if (!srcexp.state.game.frameBank)
  {
    ERROR("No Frame Bank");
    return;
  }

  if (!srcexp.state.game.objectBank)
  {
    ERROR("No Object Bank");
    return;
  }

  auto LinkImages = [](const fs::path &From, const fs::path &To)
    -> lak::error_codes<std::error_code, lak::u8string> {
    bool result = false;
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

  fs::path rootPath     = srcexp.sortedImages.path;
  fs::path unsortedPath = rootPath / "[unsorted]";
  fs::create_directories(unsortedPath);
  std::error_code err;

  size_t imageIndex       = 0;
  const size_t imageCount = srcexp.state.game.imageBank->items.size();
  for (const auto &image : srcexp.state.game.imageBank->items)
  {
    std::u16string imageName = se::to_u16string(image.entry.handle) + u".png";
    fs::path imagePath       = unsortedPath / imageName;
    (void)SaveImage(image.image(srcexp.dumpColorTrans), imagePath);
    completed = (float)((double)imageIndex++ / imageCount);
  }

  size_t frameIndex       = 0;
  const size_t frameCount = srcexp.state.game.frameBank->items.size();
  for (const auto &frame : srcexp.state.game.frameBank->items)
  {
    std::u16string frameName = HandleName(frame.name, frameIndex);
    fs::path framePath       = rootPath / frameName;
    fs::create_directories(framePath / "[unsorted]", err);
    if (err)
    {
      ERROR("File System Error: (", err.value(), ")", err.message());
      continue;
    }

    if (frame.objectInstances)
    {
      std::unordered_set<uint32_t> usedImages;
      std::unordered_set<uint16_t> usedObjects;
      for (const auto &object : frame.objectInstances->objects)
      {
        if (usedObjects.find(object.handle) != usedObjects.end()) continue;
        usedObjects.insert(object.handle);
        if (const auto *obj =
              lak::as_ptr(se::GetObject(srcexp.state, object.handle).ok());
            obj)
        {
          std::u16string objectName = HandleName(
            obj->name,
            obj->handle,
            u"[" +
              lak::to_u16string(lak::astring(GetObjectTypeString(obj->type))) +
              u"]");
          fs::path objectPath = framePath / objectName;
          fs::create_directories(objectPath, err);
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
              if (usedImages.find(imghandle) == usedImages.end())
              {
                usedImages.insert(imghandle);
                std::u16string imageName =
                  se::to_u16string(imghandle) + u".png";
                fs::path imagePath = framePath / "[unsorted]" / imageName;

                // check if 8bit image
                if (img->need_palette() && frame.palette)
                  (void)SaveImage(
                    img->image(srcexp.dumpColorTrans, frame.palette->colors),
                    imagePath);
                else if (auto res =
                           LinkImages(unsortedPath / imageName, imagePath);
                         res.is_err())
                  lak::visit(res.unwrap_err(), [](const auto &err) {
                    if constexpr (lak::is_same_v<decltype(err),
                                                 std::error_code>)
                    {
                      ERROR(
                        "Linking Failed: (", err.value(), ")", err.message());
                    }
                    else
                    {
                      ERROR("Linking Failed: ", err);
                    }
                  });
              }
              for (const auto &imgname : imgnames)
              {
                std::u16string unsortedImageName =
                  se::to_u16string(imghandle) + u".png";
                fs::path unsortedImagePath =
                  framePath / "[unsorted]" / unsortedImageName;
                std::u16string imageName = imgname + u".png";
                fs::path imagePath       = objectPath / imageName;
                if (const auto *img =
                      lak::as_ptr(GetImage(srcexp.state, imghandle).ok());
                    img)
                  if (auto res = LinkImages(unsortedImagePath, imagePath);
                      res.is_err())
                    lak::visit(res.unwrap_err(), [](const auto &err) {
                      if constexpr (lak::is_same_v<decltype(err),
                                                   std::error_code>)
                      {
                        ERROR("Linking Failed: (",
                              err.value(),
                              ")",
                              err.message());
                      }
                      else
                      {
                        ERROR("Linking Failed: ", err);
                      }
                    });
              }
            }
          }
        }
      }
    }
    completed = (float)((double)frameIndex++ / frameCount);
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
    lak::memory strm;
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
    strm.write_u32(strm.position + sizeof(uint32_t));
    out->write((const char *)strm.data(), strm.position);
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
  if (!srcexp.state.game.soundBank)
  {
    ERROR("No Sound Bank");
    return;
  }

  const size_t count = srcexp.state.game.soundBank->items.size();
  size_t index       = 0;
  for (const auto &item : srcexp.state.game.soundBank->items)
  {
    lak::memory sound = item.entry.decode();

    std::u16string name;
    sound_mode_t type;
    if (srcexp.state.oldGame)
    {
      uint16_t checksum = sound.read_u16();
      (void)checksum;
      uint32_t references = sound.read_u32();
      (void)references;
      uint32_t decompLen = sound.read_u32();
      (void)decompLen;
      type              = (sound_mode_t)sound.read_u32();
      uint32_t reserved = sound.read_u32();
      (void)reserved;
      uint32_t nameLen = sound.read_u32();

      name = lak::to_u16string(sound.read_astring_exact(nameLen));

      [[maybe_unused]] uint16_t format           = sound.read_u16();
      [[maybe_unused]] uint16_t channelCount     = sound.read_u16();
      [[maybe_unused]] uint32_t sampleRate       = sound.read_u32();
      [[maybe_unused]] uint32_t byteRate         = sound.read_u32();
      [[maybe_unused]] uint16_t blockAlign       = sound.read_u16();
      [[maybe_unused]] uint16_t bitsPerSample    = sound.read_u16();
      [[maybe_unused]] uint16_t unknown          = sound.read_u16();
      uint32_t chunkSize                         = sound.read_u32();
      [[maybe_unused]] std::vector<uint8_t> data = sound.read(chunkSize);

      lak::memory output;
      output.write_astring("RIFF", false);
      output.write_s32(data.size() - 44);
      output.write_astring("WAVEfmt ", false);
      output.write_u32(0x10);
      output.write_u16(format);
      output.write_u16(channelCount);
      output.write_u32(sampleRate);
      output.write_u32(byteRate);
      output.write_u16(blockAlign);
      output.write_u16(bitsPerSample);
      output.write_astring("data", false);
      output.write_u32(chunkSize);
      output.write(data);
      output.position = 0;
      sound           = lak::move(output);
    }
    else
    {
      lak::memory header                   = item.entry.decodeHeader();
      [[maybe_unused]] uint32_t checksum   = header.read_u32();
      [[maybe_unused]] uint32_t references = header.read_u32();
      [[maybe_unused]] uint32_t decompLen  = header.read_u32();
      type                                 = (sound_mode_t)header.read_u32();
      [[maybe_unused]] uint32_t reserved   = header.read_u32();
      uint32_t nameLen                     = header.read_u32();

      if (srcexp.state.unicode)
      {
        name = sound.read_u16string_exact(nameLen);
        DEBUG("u16string name: ", name);
      }
      else
      {
        name = lak::to_u16string(sound.read_astring_exact(nameLen));
        DEBUG("u8string name: ", name);
      }

      if (sound.peek_astring(4) == lak::astring("OggS"))
      {
        type = sound_mode_t::oggs;
      }
    }

    switch (type)
    {
      case sound_mode_t::wave: name += u".wav"; break;
      case sound_mode_t::midi: name += u".midi"; break;
      case sound_mode_t::oggs: name += u".ogg"; break;
      default: name += u".mp3"; break;
    }

    DEBUG("MP3", (size_t)item.entry.ID);

    fs::path filename = srcexp.sounds.path / name;
    std::vector<uint8_t> file(sound.cursor(), sound.end());

    if (!lak::save_file(filename, file))
    {
      ERROR("Failed To Save File '", filename, "'");
    }

    completed = (float)((double)(index++) / (double)count);
  }
}

void se::DumpMusic(source_explorer_t &srcexp, std::atomic<float> &completed)
{
  if (!srcexp.state.game.musicBank)
  {
    ERROR("No Music Bank");
    return;
  }

  const size_t count = srcexp.state.game.musicBank->items.size();
  size_t index       = 0;
  for (const auto &item : srcexp.state.game.musicBank->items)
  {
    lak::memory sound = item.entry.decode();

    std::u16string name;
    sound_mode_t type;
    if (srcexp.state.oldGame)
    {
      [[maybe_unused]] uint16_t checksum   = sound.read_u16();
      [[maybe_unused]] uint32_t references = sound.read_u32();
      [[maybe_unused]] uint32_t decompLen  = sound.read_u32();
      type                                 = (sound_mode_t)sound.read_u32();
      [[maybe_unused]] uint32_t reserved   = sound.read_u32();
      uint32_t nameLen                     = sound.read_u32();

      name = lak::to_u16string(sound.read_astring_exact(nameLen));
    }
    else
    {
      [[maybe_unused]] uint32_t checksum   = sound.read_u32();
      [[maybe_unused]] uint32_t references = sound.read_u32();
      [[maybe_unused]] uint32_t decompLen  = sound.read_u32();
      type                                 = (sound_mode_t)sound.read_u32();
      [[maybe_unused]] uint32_t reserved   = sound.read_u32();
      uint32_t nameLen                     = sound.read_u32();

      if (srcexp.state.unicode)
      {
        name = sound.read_u16string_exact(nameLen);
      }
      else
      {
        name = lak::to_u16string(sound.read_astring_exact(nameLen));
      }
    }

    switch (type)
    {
      case sound_mode_t::wave: name += u".wav"; break;
      case sound_mode_t::midi: name += u".midi"; break;
      default: name += u".mp3"; break;
    }

    fs::path filename = srcexp.music.path / name;
    std::vector<uint8_t> file(sound.cursor(), sound.end());

    if (!lak::save_file(filename, file))
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

  lak::memory strm = srcexp.state.game.shaders->entry.decode();

  uint32_t count = strm.read_u32();
  std::vector<uint32_t> offsets;
  offsets.reserve(count);

  while (count-- > 0) offsets.emplace_back(strm.read_u32());

  for (auto offset : offsets)
  {
    strm.position                         = offset;
    uint32_t nameOffset                   = strm.read_u32();
    uint32_t dataOffset                   = strm.read_u32();
    [[maybe_unused]] uint32_t paramOffset = strm.read_u32();
    [[maybe_unused]] uint32_t backTex     = strm.read_u32();

    strm.position     = offset + nameOffset;
    fs::path filename = srcexp.shaders.path / strm.read_astring();

    strm.position     = offset + dataOffset;
    lak::astring file = strm.read_astring();

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
  if (!srcexp.state.game.binaryFiles)
  {
    ERROR("No Binary Files");
    return;
  }

  lak::memory strm = srcexp.state.game.binaryFiles->entry.decode();

  const size_t count = srcexp.state.game.binaryFiles->items.size();
  size_t index       = 0;
  for (const auto &file : srcexp.state.game.binaryFiles->items)
  {
    fs::path filename = file.name;
    filename          = srcexp.binaryFiles.path / filename.filename();
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
  if (!lak::save_file(srcexp.errorLog.path, lak::debugger.str()))
  {
    ERROR("Failed To Save File '", srcexp.errorLog.path, "'");
  }
}

template<typename FUNCTOR>
void AttemptFile(se::file_state_t &FileState,
                 FUNCTOR Functor,
                 bool save = false)
{
  se::Attempt(
    FileState,
    [save](se::file_state_t &FileState) {
      if (auto result = lak::open_file_modal(FileState.path, save);
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
          FileState.valid = code == lak::file_open_error::VALID;
          return true;
        }
      }
      else
      {
        FileState.valid = false;
        return true;
      }
    },
    Functor);
}

template<typename FUNCTOR>
void AttemptFolder(se::file_state_t &FileState, FUNCTOR Functor)
{
  auto load = [](se::file_state_t &FileState) {
    std::error_code ec;
    if (auto result = lak::open_folder_modal(FileState.path); result.is_ok())
    {
      if (auto code = result.unwrap();
          code == lak::file_open_error::INCOMPLETE)
      {
        // Not finished.
        return false;
      }
      else
      {
        FileState.valid = code == lak::file_open_error::VALID;
        return true;
      }
    }
    else
    {
      FileState.valid = false;
      return true;
    }
  };
  se::Attempt(FileState, load, Functor);
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
    else
    {
      result.unwrap().EXPECT("failed to open game");

      srcexp.loaded = true;
      if (srcexp.babyMode)
      {
        // Autotragically dump everything

        fs::path dumpDir =
          srcexp.exe.path.parent_path() / srcexp.exe.path.stem();

        std::error_code er;
        if (srcexp.state.game.imageBank)
        {
          srcexp.sortedImages.path = dumpDir / "images";
          if (fs::create_directories(srcexp.sortedImages.path, er); er)
          {
            ERROR("Failed To Dump Images");
            ERROR("File System Error: (", er.value(), ")", er.message());
          }
          else
          {
            srcexp.sortedImages.attempt = true;
            srcexp.sortedImages.valid   = true;
          }
        }

        if (srcexp.state.game.icon)
        {
          srcexp.appicon.path = dumpDir / "icon";
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

        if (srcexp.state.game.soundBank)
        {
          srcexp.sounds.path = dumpDir / "sounds";
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

        if (srcexp.state.game.musicBank)
        {
          srcexp.music.path = dumpDir / "music";
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
          srcexp.shaders.path = dumpDir / "shaders";
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

        if (srcexp.state.game.binaryFiles)
        {
          srcexp.binaryFiles.path = dumpDir / "binary_files";
          if (fs::create_directories(srcexp.binaryFiles.path, er); er)
          {
            ERROR("Failed To Dump Binary Files");
            ERROR("File System Error: ", er.message());
          }
          else
          {
            srcexp.binaryFiles.attempt = true;
            srcexp.binaryFiles.valid   = true;
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
  AttemptFolder(srcexp.sortedImages, [&srcexp] {
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
  AttemptFolder(srcexp.binaryFiles, [&srcexp] {
    return DumpStuff(srcexp, "Dump Binary Files", &DumpBinaryFiles);
  });
}

void se::AttemptErrorLog(source_explorer_t &srcexp)
{
  AttemptFile(
    srcexp.errorLog,
    [&srcexp] { return DumpStuff(srcexp, "Save Error Log", &SaveErrorLog); },
    true);
}
