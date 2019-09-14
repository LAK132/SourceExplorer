/*
MIT License

Copyright (c) 2019 Lucas Kleiss (LAK132)

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

#include "dump.h"

#include <strconv/tostring.hpp>

#ifdef _WIN32
#define STBI_MSC_SECURE_CRT
#endif
#include <stb_image_write.h>

#include <unordered_set>

namespace se = SourceExplorer;

bool se::SaveImage(const lak::image4_t &image,
                   const fs::path &filename)
{
    if (stbi_write_png(filename.u8string().c_str(),
                       (int)image.size().x,
                       (int)image.size().y,
                       4,
                       &(image[0].r),
                       (int)(image.size().x * 4)) != 1)
    {
        ERROR("Failed To Save Image '" << filename << "'");
        return true;
    }
    return false;
}

bool se::SaveImage(source_explorer_t &srcexp,
                   uint16_t handle,
                   const fs::path &filename,
                   const frame::item_t *frame)
{
    auto *object = GetImage(srcexp.state, handle);
    if (!object)
    {
        ERROR("Failed To Save Image: Bad Handle '0x" << (int)handle << "'");
        return true;
    }

    lak::image4_t image = object->image(srcexp.dumpColorTrans,
                                        (frame && frame->palette)
                                          ? frame->palette->colors
                                          : nullptr);
    return SaveImage(image, filename);
}

bool se::OpenGame(source_explorer_t &srcexp)
{
    static std::tuple<se::source_explorer_t&> data = {srcexp};

    static std::thread *thread = nullptr;
    static std::atomic<bool> finished = false;
    static bool popupOpen = false;

    if (lak::AwaitPopup("Open Game",
                        popupOpen,
                        thread,
                        finished,
                        &LoadGame,
                        data))
    {
        ImGui::Text("Loading, please wait...");
        ImGui::Checkbox("Print to debug console?", &se::debugConsole);
        if (se::debugConsole)
        {
            ImGui::Checkbox("Only errors?", &se::errorOnlyConsole);
            ImGui::Checkbox("Developer mode?", &se::developerConsole);
        }
        ImGui::ProgressBar(srcexp.state.completed);
        if (popupOpen)
            ImGui::EndPopup();
    }
    else
    {
        srcexp.state.completed = 0.0f;
        return true;
    }
    return false;
}

bool se::DumpStuff(source_explorer_t &srcexp,
                   const char *str_id,
                   dump_function_t *func)
{
  static std::atomic<float> completed = 0.0f;
  static dump_data_t data = {srcexp, completed};

  static std::thread *thread = nullptr;
  static std::atomic<bool> finished = false;
  static bool popupOpen = false;

  if (lak::AwaitPopup(str_id, popupOpen, thread, finished, func, data))
  {
    ImGui::Text("Dumping, please wait...");
    ImGui::Checkbox("Print to debug console?", &debugConsole);
    if (debugConsole)
    {
      ImGui::Checkbox("Only errors?", &errorOnlyConsole);
      ImGui::Checkbox("Developer mode?", &developerConsole);
    }
    ImGui::ProgressBar(completed);
    if (popupOpen)
      ImGui::EndPopup();
  }
  else
  {
    return true;
  }
  return false;
}

void se::DumpImages(source_explorer_t &srcexp,
                    std::atomic<float> &completed)
{
  if (!srcexp.state.game.imageBank)
  {
    ERROR("No Image Bank");
    return;
  }

  const size_t count = srcexp.state.game.imageBank->items.size();
  size_t index = 0;
  for (const auto &item : srcexp.state.game.imageBank->items)
  {
    lak::image4_t image = item.image(srcexp.dumpColorTrans);
    fs::path filename =
      srcexp.images.path / (std::to_string(item.entry.handle) + ".png");
    SaveImage(image, filename);
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
  {
    bool result = false;
    std::error_code ec;
    if (fs::exists(To, ec))
    {
      ERROR("File " << To << " Already Exists");
      result = true;
    }
    else if (!ec)
    {
      if (fs::exists(From, ec))
      {
        fs::create_directories(To.parent_path(), ec);
        if (!ec)
          fs::create_hard_link(From, To, ec);
      }
      else if (!ec)
      {
        ERROR("File " << From << " Does Not Exist");
        result = true;
      }
    }

    if (ec)
    {
      ERROR("File System Error: " << ec.message());
      result = true;
    }

    if (result)
    {
      ERROR("Failed To Link Image " << From << " To " << To);
    }

    return result;
  };

  using std::string_literals::operator""s;

  auto HandleName = [](const std::unique_ptr<string_chunk_t> &name,
                       auto handle,
                       std::u16string extra = u"")
  {
    std::u32string str;
    if (extra.size() > 0) str += lak::strconv_u32(extra + u" ");
    if (name) str += lak::strconv_u32(name->value);
    std::u16string result;
    for (auto &c : str)
      if (c == U' ' ||
          c == U'(' ||
          c == U')' ||
          c == U'[' ||
          c == U']' ||
          c == U'+' ||
          c == U'-' ||
          c == U'=' ||
          c == U'_' ||
          (c >= U'0' && c <= U'9') ||
          (c >= U'a' && c <= U'z') ||
          (c >= U'A' && c <= U'Z') ||
          c > 127)
        result += lak::strconv_u16(std::u32string() + c);
    return u"["s + lak::to_u16string(handle) +
              (result.empty() ? u"]" : u"] ") + result;
  };

  fs::path rootPath = srcexp.sortedImages.path;
  fs::path unsortedPath = rootPath / "[unsorted]";
  fs::create_directories(unsortedPath);
  std::error_code err;

  size_t imageIndex = 0;
  const size_t imageCount = srcexp.state.game.imageBank->items.size();
  for (const auto &image : srcexp.state.game.imageBank->items)
  {
    std::u16string imageName = lak::to_u16string(image.entry.handle) + u".png";
    fs::path imagePath = unsortedPath / imageName;
    SaveImage(image.image(srcexp.dumpColorTrans), imagePath);
    completed = (float)((double)imageIndex++ / imageCount);
  }

  size_t frameIndex = 0;
  const size_t frameCount = srcexp.state.game.frameBank->items.size();
  for (const auto &frame : srcexp.state.game.frameBank->items)
  {
    std::u16string frameName = HandleName(frame.name, frameIndex);
    fs::path framePath = rootPath / frameName;
    fs::create_directories(framePath / "[unsorted]", err);
    if (err)
    {
      ERROR("File System Error: " << err.message());
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
        if (const auto *obj = GetObject(srcexp.state, object.handle); obj)
        {
          std::u16string objectName =
            HandleName(obj->name,
                       obj->handle,
                       u"[" + lak::strconv_u16(
                                std::string(
                                  GetObjectTypeString(obj->type))) + u"]");
          fs::path objectPath = framePath / objectName;
          fs::create_directories(objectPath, err);
          if (err)
          {
            ERROR("File System Error: " << err.message());
            continue;
          }

          for (auto [imghandle, imgnames] : obj->image_handles())
          {
            if (imghandle == 0xFFFF) continue;
            if (const auto *img = GetImage(srcexp.state, imghandle); img)
            {
              if (usedImages.find(imghandle) == usedImages.end())
              {
                usedImages.insert(imghandle);
                std::u16string imageName =
                  lak::to_u16string(imghandle) + u".png";
                fs::path imagePath = framePath / "[unsorted]" / imageName;

                // check if 8bit image
                if (img->need_palette() && frame.palette)
                  SaveImage(img->image(srcexp.dumpColorTrans,
                                       frame.palette->colors), imagePath);
                else if (LinkImages(unsortedPath / imageName, imagePath))
                  ERROR("Linking Failed");
              }
              for (const auto &imgname : imgnames)
              {
                std::u16string unsortedImageName =
                  lak::to_u16string(imghandle) + u".png";
                fs::path unsortedImagePath =
                  framePath / "[unsorted]" / unsortedImageName;
                std::u16string imageName = imgname + u".png";
                fs::path imagePath = objectPath / imageName;
                if (const auto *img = GetImage(srcexp.state, imghandle); img)
                  if (LinkImages(unsortedImagePath, imagePath))
                    ERROR("Linking Failed");
              }
            }
          }
        }
      }
    }
    completed = (float)((double)frameIndex++ / frameCount);
  }
}

void se::DumpAppIcon(source_explorer_t &srcexp,
                     std::atomic<float> &completed)
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
  if (!file.is_open())
    return;

  stbi_write_func *func = [](void *context, void *png, int len)
  {
    auto [file, bitmap] =
      *static_cast<std::tuple<std::ofstream*, lak::image4_t*>*>(context);
    lak::memory strm;
    strm.reserve(0x16);
    strm.write_u16(0); // reserved
    strm.write_u16(1); // .ICO
    strm.write_u16(1); // 1 image
    strm.write_u8 (bitmap->size().x);
    strm.write_u8 (bitmap->size().y);
    strm.write_u8 (0); // no palette
    strm.write_u8 (0); // reserved
    strm.write_u16(1); // color plane
    strm.write_u16(8 * 4); // bits per pixel
    strm.write_u32(len);
    strm.write_u32(strm.position + sizeof(uint32_t));

    file->write((const char *)strm.data(), strm.position);
    file->write((const char *)png, len);
  };

  auto context = std::tuple<std::ofstream*, lak::image4_t*>(&file, &bitmap);
  stbi_write_png_to_func(func,
                         &context,
                         (int)bitmap.size().x,
                         (int)bitmap.size().y,
                         4,
                         bitmap.data(),
                         (int)(bitmap.size().x * 4));

  file.close();
}

void se::DumpSounds(source_explorer_t &srcexp,
                    std::atomic<float> &completed)
{
  if (!srcexp.state.game.soundBank)
  {
    ERROR("No Sound Bank");
    return;
  }

  const size_t count = srcexp.state.game.soundBank->items.size();
  size_t index = 0;
  for (const auto &item : srcexp.state.game.soundBank->items)
  {
    lak::memory sound = item.entry.decode();

    std::u16string name;
    sound_mode_t type;
    if (srcexp.state.oldGame)
    {
      uint16_t checksum   = sound.read_u16(); (void)checksum;
      uint32_t references = sound.read_u32(); (void)references;
      uint32_t decompLen  = sound.read_u32(); (void)decompLen;
      type                = (sound_mode_t)sound.read_u32();
      uint32_t reserved   = sound.read_u32(); (void)reserved;
      uint32_t nameLen    = sound.read_u32();

      name = lak::strconv<char16_t>(sound.read_string_exact(nameLen));

      [[maybe_unused]] uint16_t format            = sound.read_u16();
      [[maybe_unused]] uint16_t channelCount      = sound.read_u16();
      [[maybe_unused]] uint32_t sampleRate        = sound.read_u32();
      [[maybe_unused]] uint32_t byteRate          = sound.read_u32();
      [[maybe_unused]] uint16_t blockAlign        = sound.read_u16();
      [[maybe_unused]] uint16_t bitsPerSample     = sound.read_u16();
      [[maybe_unused]] uint16_t unknown           = sound.read_u16();
                       uint32_t chunkSize         = sound.read_u32();
      [[maybe_unused]] std::vector<uint8_t> data  = sound.read(chunkSize);

      lak::memory output;
      output.write_string("RIFF", false);
      output.write_s32(data.size() - 44);
      output.write_string("WAVEfmt ", false);
      output.write_u32(0x10);
      output.write_u16(format);
      output.write_u16(channelCount);
      output.write_u32(sampleRate);
      output.write_u32(byteRate);
      output.write_u16(blockAlign);
      output.write_u16(bitsPerSample);
      output.write_string("data", false);
      output.write_u32(chunkSize);
      output.write(data);
      output.position = 0;
      sound = std::move(output);
    }
    else
    {
                       lak::memory header   = item.entry.decodeHeader();
      [[maybe_unused]] uint32_t checksum    = header.read_u32();
      [[maybe_unused]] uint32_t references  = header.read_u32();
      [[maybe_unused]] uint32_t decompLen   = header.read_u32();
                                type        = (sound_mode_t)header.read_u32();
      [[maybe_unused]] uint32_t reserved    = header.read_u32();
                       uint32_t nameLen     = header.read_u32();

      if (srcexp.state.unicode)
      {
        name = sound.read_u16string_exact(nameLen);
        WDEBUG("u16string name: " << lak::strconv_wide(name));
      }
      else
      {
        name = lak::strconv_u16(sound.read_string_exact(nameLen));
        WDEBUG("u8string name: " << lak::strconv_wide(name));
      }

      if (sound.peek_string(4) == std::string("OggS"))
      {
        type = sound_mode_t::OGGS;
      }
    }

    switch(type)
    {
      case sound_mode_t::WAVE: name += u".wav"; break;
      case sound_mode_t::MIDI: name += u".midi"; break;
      case sound_mode_t::OGGS: name += u".ogg"; break;
      default: name += u".mp3"; break;
    }

    DEBUG("MP3" << (size_t)item.entry.ID);

    fs::path filename = srcexp.sounds.path / name;
    std::vector<uint8_t> file(sound.cursor(), sound.end());

    if (!lak::SaveFile(filename, file))
    {
      ERROR("Failed To Save File '" << filename << "'");
    }

    completed = (float)((double)(index++) / (double)count);
  }
}

void se::DumpMusic(source_explorer_t &srcexp,
                   std::atomic<float> &completed)
{
  if (!srcexp.state.game.musicBank)
  {
    ERROR("No Music Bank");
    return;
  }

  const size_t count = srcexp.state.game.musicBank->items.size();
  size_t index = 0;
  for (const auto &item : srcexp.state.game.musicBank->items)
  {
    lak::memory sound = item.entry.decode();

    std::u16string name;
    sound_mode_t type;
    if (srcexp.state.oldGame)
    {
      [[maybe_unused]] uint16_t checksum    = sound.read_u16();
      [[maybe_unused]] uint32_t references  = sound.read_u32();
      [[maybe_unused]] uint32_t decompLen   = sound.read_u32();
                                type        = (sound_mode_t)sound.read_u32();
      [[maybe_unused]] uint32_t reserved    = sound.read_u32();
                       uint32_t nameLen     = sound.read_u32();

      name = lak::strconv_u16(sound.read_string_exact(nameLen));
    }
    else
    {
      [[maybe_unused]] uint32_t checksum    = sound.read_u32();
      [[maybe_unused]] uint32_t references  = sound.read_u32();
      [[maybe_unused]] uint32_t decompLen   = sound.read_u32();
                                type        = (sound_mode_t)sound.read_u32();
      [[maybe_unused]] uint32_t reserved    = sound.read_u32();
                       uint32_t nameLen     = sound.read_u32();

      if (srcexp.state.unicode)
      {
        name = sound.read_u16string_exact(nameLen);
      }
      else
      {
        name = lak::strconv_u16(sound.read_string_exact(nameLen));
      }
    }

    switch(type)
    {
      case sound_mode_t::WAVE: name += u".wav"; break;
      case sound_mode_t::MIDI: name += u".midi"; break;
      default: name += u".mp3"; break;
    }

    fs::path filename = srcexp.music.path / name;
    std::vector<uint8_t> file(sound.cursor(), sound.end());

    if (!lak::SaveFile(filename, file))
    {
      ERROR("Failed To Save File '" << filename << "'");
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

  while (count --> 0)
    offsets.emplace_back(strm.read_u32());

  for (auto offset : offsets)
  {
    strm.position = offset;
    uint32_t nameOffset                     = strm.read_u32();
    uint32_t dataOffset                     = strm.read_u32();
    [[maybe_unused]] uint32_t paramOffset   = strm.read_u32();
    [[maybe_unused]] uint32_t backTex       = strm.read_u32();

    strm.position = offset + nameOffset;
    fs::path filename = srcexp.shaders.path / strm.read_string();

    strm.position = offset + dataOffset;
    std::string file = strm.read_string();

    DEBUG(filename);
    if (!lak::SaveFile(filename, std::vector<uint8_t>(file.begin(),
                                                      file.end())))
    {
      ERROR("Failed To Save File '" << filename << "'");
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
  size_t index = 0;
  for (const auto &file : srcexp.state.game.binaryFiles->items)
  {
    fs::path filename = file.name;
    filename = srcexp.binaryFiles.path / filename.filename();
    DEBUG(filename);
    if (!lak::SaveFile(filename, file.data._data))
    {
      ERROR("Failed To Save File '" << filename << "'");
    }
    completed = (float)((double)index++ / (double)count);
  }
}

void se::AttemptExe(source_explorer_t &srcexp)
{
    srcexp.loaded = false;
    if (!srcexp.exe.valid)
    {
        if (lak::OpenFile(srcexp.exe.path, srcexp.exe.valid))
        {
            if (!srcexp.exe.valid)
            {
                // User cancelled
                srcexp.exe.attempt = false;
            }
        }
    }
    else if (OpenGame(srcexp))
    {
        srcexp.loaded = true;
        srcexp.exe.valid = false;
        srcexp.exe.attempt = false;

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
                    ERROR("File System Error: " << er.message());
                }
                else
                {
                    srcexp.sortedImages.attempt = true;
                    srcexp.sortedImages.valid = true;
                }
            }

            if (srcexp.state.game.icon)
            {
                srcexp.appicon.path = dumpDir / "icon";
                if (fs::create_directories(srcexp.appicon.path, er); er)
                {
                    ERROR("Failed To Dump Icon");
                    ERROR("File System Error: " << er.message());
                }
                else
                {
                    srcexp.appicon.attempt = true;
                    srcexp.appicon.valid = true;
                }
            }

            if (srcexp.state.game.soundBank)
            {
                srcexp.sounds.path = dumpDir / "sounds";
                if (fs::create_directories(srcexp.sounds.path, er); er)
                {
                    ERROR("Failed To Dump Audio");
                    ERROR("File System Error: " << er.message());
                }
                else
                {
                    srcexp.sounds.attempt = true;
                    srcexp.sounds.valid = true;
                }
            }

            if (srcexp.state.game.musicBank)
            {
                srcexp.music.path = dumpDir / "music";
                if (fs::create_directories(srcexp.sounds.path, er); er)
                {
                    ERROR("Failed To Dump Audio");
                    ERROR("File System Error: " << er.message());
                }
                else
                {
                    srcexp.music.attempt = true;
                    srcexp.music.valid = true;
                }
            }

            if (srcexp.state.game.shaders)
            {
                srcexp.shaders.path = dumpDir / "shaders";
                if (fs::create_directories(srcexp.shaders.path, er); er)
                {
                    ERROR("Failed To Dump Shaders");
                    ERROR("File System Error: " << er.message());
                }
                else
                {
                    srcexp.shaders.attempt = true;
                    srcexp.shaders.valid = true;
                }
            }

            if (srcexp.state.game.binaryFiles)
            {
                srcexp.binaryFiles.path = dumpDir / "binary_files";
                if (fs::create_directories(srcexp.binaryFiles.path, er); er)
                {
                    ERROR("Failed To Dump Binary Files");
                    ERROR("File System Error: " << er.message());
                }
                else
                {
                    srcexp.binaryFiles.attempt = true;
                    srcexp.binaryFiles.valid = true;
                }
            }
        }
    }
}

template <typename FUNCTOR>
void Attempt(se::file_state_t &FileState, FUNCTOR Functor)
{
    if (!FileState.valid)
    {
        if (lak::OpenFolder(FileState.path, FileState.valid))
        {
            if (!FileState.valid)
            {
                // User cancelled
                FileState.attempt = false;
            }
        }
    }
    else if (Functor())
    {
        FileState.valid = false;
        FileState.attempt = false;
    }
}

void se::AttemptImages(source_explorer_t &srcexp)
{
    Attempt(srcexp.images, [&srcexp]
    {
        return DumpStuff(srcexp, "Dump Images", &DumpImages);
    });
}

void se::AttemptSortedImages(source_explorer_t &srcexp)
{
    Attempt(srcexp.sortedImages, [&srcexp]
    {
        return DumpStuff(srcexp, "Dump Sorted Images", &DumpSortedImages);
    });
}

void se::AttemptAppIcon(source_explorer_t &srcexp)
{
    Attempt(srcexp.appicon, [&srcexp]
    {
        return DumpStuff(srcexp, "Dump App Icon", &DumpAppIcon);
    });
}

void se::AttemptSounds(source_explorer_t &srcexp)
{
    Attempt(srcexp.sounds, [&srcexp]
    {
        return DumpStuff(srcexp, "Dump Sounds", &DumpSounds);
    });
}

void se::AttemptMusic(source_explorer_t &srcexp)
{
    Attempt(srcexp.music, [&srcexp]
    {
        return DumpStuff(srcexp, "Dump Music", &DumpMusic);
    });
}

void se::AttemptShaders(source_explorer_t &srcexp)
{
    Attempt(srcexp.shaders, [&srcexp]
    {
        return DumpStuff(srcexp, "Dump Shaders", &DumpShaders);
    });
}

void se::AttemptBinaryFiles(source_explorer_t &srcexp)
{
    Attempt(srcexp.binaryFiles, [&srcexp]
    {
        return DumpStuff(srcexp, "Dump Binary Files", &DumpBinaryFiles);
    });
}
