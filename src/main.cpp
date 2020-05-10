// Copyright (c) Mathias Kaerlev 2012, LAK132 2019

// This file is part of Anaconda.

// Anaconda is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Anaconda is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Anaconda.  If not, see <http://www.gnu.org/licenses/>.

#define SDL_MAIN_HANDLED

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_utils.hpp"

#include "dump.h"
#include "lisk_impl.hpp"
#include "main.h"

#include <lak/opengl/shader.hpp>
#include <lak/opengl/state.hpp>
#include <lak/opengl/texture.hpp>

#include <lak/defer.hpp>
#include <lak/file.hpp>
#include <lak/window.hpp>

se::source_explorer_t SrcExp;
int openglMajor, openglMinor;

#ifndef MAXDIRLEN
#  define MAXDIRLEN 512
#endif

bool bytePairsMode = false;

void ViewImage(const lak::opengl::texture &texture, const float scale)
{
  ImGui::Image(
    (ImTextureID)(uintptr_t)texture.get(),
    ImVec2(scale * (float)texture.size().x, scale * (float)texture.size().y));
}

void HelpText()
{
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0, 10.0));
  ImGui::Text(
    "To open a game either drag and drop it into this window or "
    "go to [File]->[Open]\n");
  ImGui::Text(
    "If Source Explorer cannot open/throws errors while opening a file,\n"
    "please save the error log ([File]->[Save Error Log]) and share\n"
    "it at https://github.com/LAK132/SourceExplorer/issues\n");
  ImGui::Text(
    "If Source Explorer crashes before you can save the log file,\n"
    "it will attempt to save it to:\n'%s'\n",
    lak::debugger.crash_path.string().c_str());
  ImGui::PopStyleVar();
}

void MenuBar(float FrameTime)
{
  if (ImGui::BeginMenu("File"))
  {
    ImGui::Checkbox("Auto-dump Mode", &SrcExp.babyMode);
    SrcExp.exe.attempt |= ImGui::MenuItem(
      SrcExp.babyMode ? "Open And Dump..." : "Open...", nullptr);
    SrcExp.sortedImages.attempt |= ImGui::MenuItem(
      "Dump Sorted Images...", nullptr, false, !SrcExp.babyMode);
    SrcExp.images.attempt |=
      ImGui::MenuItem("Dump Images...", nullptr, false, !SrcExp.babyMode);
    SrcExp.sounds.attempt |=
      ImGui::MenuItem("Dump Sounds...", nullptr, false, !SrcExp.babyMode);
    SrcExp.music.attempt |=
      ImGui::MenuItem("Dump Music...", nullptr, false, !SrcExp.babyMode);
    SrcExp.shaders.attempt |=
      ImGui::MenuItem("Dump Shaders...", nullptr, false, !SrcExp.babyMode);
    SrcExp.binaryFiles.attempt |= ImGui::MenuItem(
      "Dump Binary Files...", nullptr, false, !SrcExp.babyMode);
    SrcExp.appicon.attempt |=
      ImGui::MenuItem("Dump App Icon...", nullptr, false, !SrcExp.babyMode);
    ImGui::Separator();
    SrcExp.errorLog.attempt |= ImGui::MenuItem("Save Error Log...");
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("About"))
  {
    ImGui::Text(APP_NAME " by LAK132");
    if (SrcExp.graphicsMode == lak::graphics_mode::OpenGL)
      ImGui::Text("OpenGL %d.%d", openglMajor, openglMinor);
    ImGui::Text("Frame rate %f", 1.0f / FrameTime);
    credits();
    ImGui::Checkbox("Byte Pairs", &bytePairsMode);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Help"))
  {
    HelpText();
    ImGui::EndMenu();
  }

  ImGui::Checkbox("Color transparency?", &SrcExp.dumpColorTrans);
  ImGui::Checkbox("Force compat mode?", &se::forceCompat);
  ImGui::Checkbox("Debug console? (May make SE slow)",
                  &lak::debugger.live_output_enabled);

  if (lak::debugger.live_output_enabled)
  {
    ImGui::Checkbox("Only errors?", &lak::debugger.live_errors_only);
    ImGui::Checkbox("Developer mode?", &lak::debugger.line_info_enabled);
  }
}

void Navigator()
{
  if (SrcExp.loaded)
  {
    if (SrcExp.state.game.title)
      ImGui::Text("Title: %s",
                  lak::strconv<char>(SrcExp.state.game.title->value).c_str());
    if (SrcExp.state.game.author)
      ImGui::Text("Author: %s",
                  lak::strconv<char>(SrcExp.state.game.author->value).c_str());
    if (SrcExp.state.game.copyright)
      ImGui::Text(
        "Copyright: %s",
        lak::strconv<char>(SrcExp.state.game.copyright->value).c_str());
    if (SrcExp.state.game.outputPath)
      ImGui::Text(
        "Output: %s",
        lak::strconv<char>(SrcExp.state.game.outputPath->value).c_str());
    if (SrcExp.state.game.projectPath)
      ImGui::Text(
        "Project: %s",
        lak::strconv<char>(SrcExp.state.game.projectPath->value).c_str());

    ImGui::Separator();

    if (SrcExp.state.recompiled)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, 0xFF8080FF);
      ImGui::Text(
        "WARNING: THIS GAME APPEARS TO HAVE BEEN RECOMPILED WITH\n"
        "AN EXTERNAL TOOL, DUMPING WILL FAIL!\nTHIS IS NOT A BUG.");
      ImGui::PopStyleColor();
    }

    ImGui::Text("New Game: %s", SrcExp.state.oldGame ? "No" : "Yes");
    ImGui::Text("Unicode Game: %s", SrcExp.state.unicode ? "Yes" : "No");
    ImGui::Text("Compat Game: %s", SrcExp.state.compat ? "Yes" : "No");
    ImGui::Text("Product Build: %zu", (size_t)SrcExp.state.productBuild);
    ImGui::Text("Product Version: %zu", (size_t)SrcExp.state.productVersion);
    ImGui::Text("Runtime Version: %zu", (size_t)SrcExp.state.runtimeVersion);
    ImGui::Text("Runtime Sub-Version: %zu",
                (size_t)SrcExp.state.runtimeSubVersion);

    ImGui::Separator();

    SrcExp.state.game.view(SrcExp);
  }
}

bool Crypto()
{
  bool updated   = false;
  int magic_char = se::_magic_char;
  if (ImGui::InputInt("Magic Char", &magic_char))
  {
    se::_magic_char = magic_char;
    se::GetEncryptionKey(SrcExp.state);
    updated = true;
  }
  if (ImGui::Button("Generate Crypto Key"))
  {
    se::GetEncryptionKey(SrcExp.state);
    updated = true;
  }
  return updated;
}

void BytePairsMemoryExplorer(const uint8_t *Data, size_t Size, bool Update)
{
  static lak::image<GLfloat> image(lak::vec2s_t(256, 256));
  static lak::opengl::texture texture(GL_TEXTURE_2D);
  static float scale             = 1.0f;
  static uint64_t from           = 0;
  static uint64_t to             = SIZE_MAX;
  static const uint8_t *old_data = Data;

  if (Data == nullptr && old_data == nullptr) return;

  if (Data != nullptr && Data != old_data)
  {
    old_data = Data;
    Update   = true;
  }

  if (Update)
  {
    if (SrcExp.view != nullptr && Data == SrcExp.state.file.data())
    {
      from = SrcExp.view->position;
      to   = SrcExp.view->end;
    }
    else
    {
      from = 0;
      to   = SIZE_MAX;
    }
  }

  Update |= !texture.get();

  {
    static bool count_mode = true;
    ImGui::Checkbox("Fixed Size", &count_mode);
    uint64_t count = to - from;

    if (ImGui::DragScalar("From", ImGuiDataType_U64, &from, 1.0f))
    {
      if (count_mode)
        to = from + count;
      else if (from > to)
        to = from;
      Update = true;
    }
    if (ImGui::DragScalar("To", ImGuiDataType_U64, &to, 1.0f))
    {
      if (from > to) from = to;
      Update = true;
    }
  }

  ImGui::Separator();

  if (from > to) from = to;
  if (from > Size) from = Size;
  if (to > Size) to = Size;

  if (Update)
  {
    image.fill(0.0f);

    const auto begin = old_data;
    const auto end   = begin + to;
    auto it          = begin + from;

    const GLfloat step = 1.0f / ((to - from) / image.contig_size());
    for (uint8_t prev = (it != end ? *it : 0); it != end; prev = *(it++))
      image[{prev, *it}] += step;

    texture.bind()
      .apply(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
      .apply(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
      .apply(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
      .apply(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
      .build(0,
             GL_RED,
             (lak::vec2<GLsizei>)image.size(),
             0,
             GL_RED,
             GL_FLOAT,
             image.data());
  }

  if (texture.get())
  {
    ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.0f);
    ImGui::Separator();
    ViewImage(texture, scale);
  }
}

void RawImageMemoryExplorer(const uint8_t *Data, size_t Size, bool Update)
{
  static bool resetOnUpdate       = true;
  static lak::vec2u64_t imageSize = {256, 256};
  static lak::vec2u64_t blockSkip = {0, 0};
  static lak::image4_t image{lak::vec2s_t(imageSize)};
  static lak::opengl::texture texture(GL_TEXTURE_2D);
  static float scale             = 1.0f;
  static uint64_t from           = 0;
  static uint64_t to             = SIZE_MAX;
  static int colourSize          = 3;
  static const uint8_t *old_data = Data;

  if (Data == nullptr && old_data == nullptr) return;

  if (Data != nullptr && Data != old_data)
  {
    old_data = Data;
    Update   = true;
  }

  if (Update)
  {
    if (resetOnUpdate)
    {
      imageSize = {256, 256};
      blockSkip = {0, 0};
      image.resize(lak::vec2s_t(imageSize));
    }

    if (SrcExp.view != nullptr && Data == SrcExp.state.file.data())
    {
      from = SrcExp.view->position;
      to   = SrcExp.view->end;
    }
    else
    {
      from = 0;
      to   = SIZE_MAX;
    }
  }

  Update |= !texture.get();

  {
    ImGui::Checkbox("Reset Configuration On Update", &resetOnUpdate);

    static bool count_mode = true;
    ImGui::Checkbox("Fixed Size", &count_mode);
    uint64_t count = to - from;

    if (ImGui::DragScalar("From", ImGuiDataType_U64, &from, 1.0f))
    {
      if (count_mode)
        to = from + count;
      else if (from > to)
        to = from;
      Update = true;
    }
    if (ImGui::DragScalar("To", ImGuiDataType_U64, &to, 1.0f))
    {
      if (from > to) from = to;
      Update = true;
    }
    const static uint64_t sizeMin = 0;
    const static uint64_t sizeMax = 10000;
    if (ImGui::DragScalarN("Image Size (Width/Height)",
                           ImGuiDataType_U64,
                           &imageSize,
                           2,
                           1.0f,
                           &sizeMin,
                           &sizeMax))
    {
      image.resize(lak::vec2s_t(imageSize));
      Update = true;
    }
    if (ImGui::DragScalarN("For Every/Skip",
                           ImGuiDataType_U64,
                           &blockSkip,
                           2,
                           0.1f,
                           &sizeMin,
                           &sizeMax))
    {
      Update = true;
    }
    if (ImGui::SliderInt("Colour Size", &colourSize, 1, 4))
    {
      Update = true;
    }
  }

  ImGui::Separator();

  if (from > to) from = to;
  if (from > Size) from = Size;
  if (to > Size) to = Size;

  if (Update)
  {
    image.fill({0, 0, 0, 255});

    const auto begin = old_data;
    const auto end   = begin + to;
    auto it          = begin + from;

    auto img          = (uint8_t *)image.data();
    const auto imgEnd = img + (image.contig_size() * sizeof(image[0]));

    for (uint64_t i = 0; img < imgEnd && it < end;)
    {
      *img = *it;
      ++it;
      ++i;
      if (blockSkip.x > 0 && (i % blockSkip.x) == 0) it += blockSkip.y;
      img += (5 - colourSize);
    }

    texture.bind()
      .apply(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
      .apply(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
      .apply(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
      .apply(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
      .build(0,
             GL_RGBA,
             (lak::vec2<GLsizei>)image.size(),
             0,
             GL_RGBA,
             GL_UNSIGNED_BYTE,
             image.data());
  }

  if (texture.get())
  {
    ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.0f);
    ImGui::Separator();
    ViewImage(texture, scale);
  }
}

void MemoryExplorer(bool &Update)
{
  static const se::basic_entry_t *last = nullptr;
  static int dataMode                  = 0;
  static int contentMode               = 0;
  static bool raw                      = true;
  Update |= last != SrcExp.view;

  Update |= ImGui::RadioButton("EXE", &dataMode, 0);
  ImGui::SameLine();
  Update |= ImGui::RadioButton("Header", &dataMode, 1);
  ImGui::SameLine();
  Update |= ImGui::RadioButton("Data", &dataMode, 2);
  ImGui::SameLine();
  Update |= ImGui::RadioButton("Magic Key", &dataMode, 3);
  ImGui::Separator();

  if (dataMode != 3)
  {
    if (dataMode != 0)
    {
      Update |= ImGui::Checkbox("Raw", &raw);
      ImGui::SameLine();
    }
    Update |= ImGui::RadioButton("Binary", &contentMode, 0);
    ImGui::SameLine();
    Update |= ImGui::RadioButton("Byte Pairs", &contentMode, 1);
    ImGui::SameLine();
    Update |= ImGui::RadioButton("Data Image", &contentMode, 2);
    ImGui::Separator();
  }

  if (dataMode == 0) // EXE
  {
    if (contentMode == 1)
    {
      BytePairsMemoryExplorer(
        SrcExp.state.file.data(), SrcExp.state.file.size(), Update);
    }
    else if (contentMode == 2)
    {
      RawImageMemoryExplorer(
        SrcExp.state.file.data(), SrcExp.state.file.size(), Update);
    }
    else
    {
      if (contentMode != 0) contentMode = 0;
      SrcExp.editor.DrawContents(SrcExp.state.file.data(),
                                 SrcExp.state.file.size());
      if (Update && SrcExp.view != nullptr)
        SrcExp.editor.GotoAddrAndHighlight(SrcExp.view->position,
                                           SrcExp.view->end);
    }
  }
  else if (dataMode == 1) // Header
  {
    if (Update && SrcExp.view != nullptr)
      SrcExp.buffer =
        raw ? SrcExp.view->header.data : SrcExp.view->decodeHeader();

    if (contentMode == 1)
    {
      BytePairsMemoryExplorer(
        SrcExp.buffer.data(), SrcExp.buffer.size(), Update);
    }
    else if (contentMode == 2)
    {
      RawImageMemoryExplorer(
        SrcExp.buffer.data(), SrcExp.buffer.size(), Update);
    }
    else
    {
      if (contentMode != 0) contentMode = 0;
      SrcExp.editor.DrawContents(SrcExp.buffer.data(), SrcExp.buffer.size());
      if (Update) SrcExp.editor.GotoAddrAndHighlight(0, 0);
    }
  }
  else if (dataMode == 2) // Data
  {
    if (Update && SrcExp.view != nullptr)
      SrcExp.buffer = raw ? SrcExp.view->data.data : SrcExp.view->decode();

    if (contentMode == 1)
    {
      BytePairsMemoryExplorer(
        SrcExp.buffer.data(), SrcExp.buffer.size(), Update);
    }
    else if (contentMode == 2)
    {
      RawImageMemoryExplorer(
        SrcExp.buffer.data(), SrcExp.buffer.size(), Update);
    }
    else
    {
      if (contentMode != 0) contentMode = 0;
      SrcExp.editor.DrawContents(SrcExp.buffer.data(), SrcExp.buffer.size());
      if (Update) SrcExp.editor.GotoAddrAndHighlight(0, 0);
    }
  }
  else if (dataMode == 3) // _magic_key
  {
    SrcExp.editor.DrawContents(&(se::_magic_key[0]), se::_magic_key.size());
    if (Update) SrcExp.editor.GotoAddrAndHighlight(0, 0);
  }
  else
    dataMode = 0;

  last   = SrcExp.view;
  Update = false;
}

void ImageExplorer(bool &Update)
{
  static float scale = 1.0f;
  ImGui::DragFloat("Scale", &scale, 0.1, 0.1f, 10.0f);
  ImGui::Separator();
  se::ViewImage(SrcExp, scale);
  Update = false;
}

void AudioExplorer(bool &Update)
{
  struct audio_data_t
  {
    std::u8string name;
    std::u16string u16name;
    se::sound_mode_t type  = (se::sound_mode_t)0;
    uint32_t checksum      = 0;
    uint32_t references    = 0;
    uint32_t decompLen     = 0;
    uint32_t reserved      = 0;
    uint32_t nameLen       = 0;
    uint16_t format        = 0;
    uint16_t channelCount  = 0;
    uint32_t sampleRate    = 0;
    uint32_t byteRate      = 0;
    uint16_t blockAlign    = 0;
    uint16_t bitsPerSample = 0;
    uint16_t unknown       = 0;
    uint32_t chunkSize     = 0;
    std::vector<uint8_t> data;
  };

  static const se::basic_entry_t *last = nullptr;
  Update |= last != SrcExp.view;

  static audio_data_t audioData;
  if (Update && SrcExp.view != nullptr)
  {
    lak::memory audio = SrcExp.view->decode();
    audioData         = audio_data_t{};
    if (SrcExp.state.oldGame)
    {
      audioData.checksum   = audio.read_u16();
      audioData.references = audio.read_u32();
      audioData.decompLen  = audio.read_u32();
      audioData.type       = (se::sound_mode_t)audio.read_u32();
      audioData.reserved   = audio.read_u32();
      audioData.nameLen    = audio.read_u32();

      audioData.name = audio.read_u8string_exact(audioData.nameLen);

      if (audioData.type == se::sound_mode_t::wave)
      {
        audioData.format        = audio.read_u16();
        audioData.channelCount  = audio.read_u16();
        audioData.sampleRate    = audio.read_u32();
        audioData.byteRate      = audio.read_u32();
        audioData.blockAlign    = audio.read_u16();
        audioData.bitsPerSample = audio.read_u16();
        audioData.unknown       = audio.read_u16();
        audioData.chunkSize     = audio.read_u32();
        audioData.data          = audio.read(audioData.chunkSize);
      }
    }
    else
    {
      lak::memory header_temp;
      lak::memory *header_ptr;
      if (SrcExp.view->header.data.size() > 0)
      {
        header_temp = SrcExp.view->decodeHeader();
        header_ptr  = &header_temp;
      }
      else
        header_ptr = &audio;
      lak::memory &header = *header_ptr;

      audioData.checksum   = header.read_u32();
      audioData.references = header.read_u32();
      audioData.decompLen  = header.read_u32();
      audioData.type       = (se::sound_mode_t)audio.read_u32();
      audioData.reserved   = header.read_u32();
      audioData.nameLen    = header.read_u32();

      if (SrcExp.state.unicode)
      {
        audioData.name =
          lak::strconv<char8_t>(audio.read_u16string_exact(audioData.nameLen));
      }
      else
        audioData.name = audio.read_u8string_exact(audioData.nameLen);

      if (audio.peek_string(4) == std::string("OggS"))
        audioData.type = se::sound_mode_t::oggs;

      if (audioData.type == se::sound_mode_t::wave)
      {
        audio.position += 4; // "RIFF"
        uint32_t size = audio.read_s32() + 4;
        audio.position += 8; // "WAVEfmt "
        // audio.position += 4; // 0x00000010
        // 16, 18 or 40
        uint32_t chunkSize = audio.read_u32();
        DEBUG("Chunk Size 0x" << chunkSize);
        const size_t pos        = audio.position + chunkSize;
        audioData.format        = audio.read_u16(); // 2
        audioData.channelCount  = audio.read_u16(); // 4
        audioData.sampleRate    = audio.read_u32(); // 8
        audioData.byteRate      = audio.read_u32(); // 12
        audioData.blockAlign    = audio.read_u16(); // 14
        audioData.bitsPerSample = audio.read_u16(); // 16
        if (chunkSize >= 18)
        {
          [[maybe_unused]] uint16_t extensionSize = audio.read_u16(); // 18
          DEBUG("Extension Size 0x" << extensionSize);
        }
        if (chunkSize >= 40)
        {
          [[maybe_unused]] uint16_t validPerSample = audio.read_u16(); // 20
          DEBUG("Valid Bits Per Sample 0x" << validPerSample);
          [[maybe_unused]] uint16_t channelMask = audio.read_u32(); // 24
          DEBUG("Channel Mask 0x" << channelMask);
          // SubFormat // 40
        }
        audio.position = pos;
        audio.position += 4; // "data"
        audioData.chunkSize = audio.read_u32();
        audioData.data      = audio.read(size);
      }
    }
  }

  static size_t audioSize = 0;
  static bool playing     = false;
  static SDL_AudioSpec audioSpec;
  static SDL_AudioDeviceID audioDevice = 0;
  static SDL_AudioSpec audioSpecGot;

  if (!playing && ImGui::Button("Play"))
  {
    SDL_AudioSpec spec;
    spec.freq = audioData.sampleRate;
    // spec.freq = audioData.byteRate;
    switch (audioData.format)
    {
      case 0x0001: spec.format = AUDIO_S16; break;
      case 0x0003: spec.format = AUDIO_F32; break;
      case 0x0006:
        spec.format = AUDIO_S8; /*8bit A-law*/
        break;
      case 0x0007:
        spec.format = AUDIO_S8; /*abit mu-law*/
        break;
      case 0xFFFE: /*subformat*/ break;
      default: break;
    }
    spec.channels = audioData.channelCount;
    spec.samples  = 2048;
    spec.callback = nullptr;

    if (std::memcmp(&audioSpec, &spec, sizeof(SDL_AudioSpec)) != 0)
    {
      std::memcpy(&audioSpec, &spec, sizeof(SDL_AudioSpec));
      if (audioDevice != 0)
      {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
      }
    }

    if (audioDevice == 0)
      audioDevice =
        SDL_OpenAudioDevice(nullptr, false, &audioSpec, &audioSpecGot, 0);

    audioSize = audioData.data.size();
    SDL_QueueAudio(audioDevice, audioData.data.data(), audioSize);
    SDL_PauseAudioDevice(audioDevice, 0);
    playing = true;
  }

  if (playing &&
      (ImGui::Button("Stop") || (SDL_GetQueuedAudioSize(audioDevice) == 0)))
  {
    SDL_PauseAudioDevice(audioDevice, 1);
    SDL_ClearQueuedAudio(audioDevice);
    audioSize = 0;
    playing   = false;
  }

  if (audioSize > 0)
    ImGui::ProgressBar(
      1.0 - (SDL_GetQueuedAudioSize(audioDevice) / (double)audioSize));
  else
    ImGui::ProgressBar(0);

  ImGui::Text("Name: %s", audioData.name.c_str());
  ImGui::Text("Type: ");
  ImGui::SameLine();
  switch (audioData.type)
  {
    case se::sound_mode_t::wave: ImGui::Text("WAV"); break;
    case se::sound_mode_t::midi: ImGui::Text("MIDI"); break;
    case se::sound_mode_t::oggs: ImGui::Text("OGG"); break;
    default: ImGui::Text("Unknown"); break;
  }
  ImGui::Text("Data Size: 0x%zX", (size_t)audioData.data.size());
  ImGui::Text("Format: 0x%zX", (size_t)audioData.format);
  ImGui::Text("Channel Count: %zu", (size_t)audioData.channelCount);
  ImGui::Text("Sample Rate: %zu", (size_t)audioData.sampleRate);
  ImGui::Text("Byte Rate: %zu", (size_t)audioData.byteRate);
  ImGui::Text("Block Align: 0x%zX", (size_t)audioData.blockAlign);
  ImGui::Text("Bits Per Sample: %zu", (size_t)audioData.bitsPerSample);
  ImGui::Text("Chunk Size: 0x%zX", (size_t)audioData.chunkSize);

  last   = SrcExp.view;
  Update = false;
}

lisk::string lisk_init_script;
lisk::string lisk_loop_script;
lisk::expression lisk_loop_expr;
lisk::string lisk_exception_message = "";
lisk::environment lisk_script_environment;
bool run_lisk_script = false;

void LiskEditor()
{
  if (!run_lisk_script)
  {
    if (ImGui::Button("Run"))
    {
      lisk_script_environment = DefaultEnvironment();

      if (const auto result =
            lisk::root_eval_string(lisk_init_script, lisk_script_environment);
          result.is_exception())
      {
        lisk_exception_message = result.as_exception().message;
      }

      if (const auto tokens = lisk::root_tokenise(lisk_loop_script);
          !tokens.empty())
      {
        auto loop = lisk::parse(tokens);
        if (loop.is_exception())
        {
          lisk_exception_message = loop.as_exception().message;
        }
        else if (loop.is_list())
        {
          run_lisk_script = true;
          lisk_loop_expr  = loop;
        }
      }
    }
  }
  else
  {
    if (ImGui::Button("Stop"))
    {
      run_lisk_script = false;
    }
  }

  if (!lisk_exception_message.empty())
    ImGui::Text("Exception thrown: %s", lisk_exception_message.c_str());

  ImGui::Text("Init:");

  lak::input_text(
    "lisk-init-editor", &lisk_init_script, ImGuiInputTextFlags_Multiline);

  ImGui::Text("Loop:");

  lak::input_text(
    "lisk-loop-editor", &lisk_loop_script, ImGuiInputTextFlags_Multiline);

  if (run_lisk_script)
  {
    auto result = lisk::eval(lisk_loop_expr, lisk_script_environment, true);
    if (result.is_exception())
    {
      run_lisk_script        = false;
      lisk_exception_message = result.as_exception().message;
    }
  }
}

void Explorer()
{
  if (SrcExp.loaded)
  {
    enum mode
    {
      MEMORY,
      IMAGE,
      AUDIO,
      LISK
    };
    static int selected = 0;
    ImGui::RadioButton("Memory", &selected, MEMORY);
    ImGui::SameLine();
    ImGui::RadioButton("Image", &selected, IMAGE);
    ImGui::SameLine();
    ImGui::RadioButton("Audio", &selected, AUDIO);
    ImGui::SameLine();
    ImGui::RadioButton("Lisk", &selected, LISK);

    static bool crypto = false;
    ImGui::Checkbox("Crypto", &crypto);
    ImGui::Separator();

    static bool memUpdate   = false;
    static bool imageUpdate = false;
    static bool audioUpdate = false;
    if (crypto)
    {
      if (Crypto()) memUpdate = imageUpdate = audioUpdate = true;
      ImGui::Separator();
    }

    switch (selected)
    {
      case MEMORY: MemoryExplorer(memUpdate); break;
      case IMAGE: ImageExplorer(imageUpdate); break;
      case AUDIO: AudioExplorer(audioUpdate); break;
      case LISK: LiskEditor(); break;
      default: selected = 0; break;
    }
  }
}

void SourceExplorerMain(float FrameTime)
{
  if (ImGui::BeginMenuBar())
  {
    MenuBar(FrameTime);
    ImGui::EndMenuBar();
  }

  if (!SrcExp.babyMode && SrcExp.loaded)
  {
    ImVec2 contentSize = ImGui::GetWindowContentRegionMax();
    contentSize.x      = ImGui::GetWindowContentRegionWidth();

    static float leftSize  = contentSize.x / 2;
    static float rightSize = contentSize.x / 2;

    lak::HoriSplitter(leftSize, rightSize, contentSize.x);

    ImGui::BeginChild(
      "Left", {leftSize, -1}, true, ImGuiWindowFlags_NoSavedSettings);
    Navigator();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild(
      "Right", {rightSize, -1}, true, ImGuiWindowFlags_NoSavedSettings);
    Explorer();
    ImGui::EndChild();
  }
  else
  {
    HelpText();
  }

  if (SrcExp.exe.attempt)
    se::AttemptExe(SrcExp);
  else if (SrcExp.images.attempt)
    se::AttemptImages(SrcExp);
  else if (SrcExp.sortedImages.attempt)
    se::AttemptSortedImages(SrcExp);
  else if (SrcExp.appicon.attempt)
    se::AttemptAppIcon(SrcExp);
  else if (SrcExp.sounds.attempt)
    se::AttemptSounds(SrcExp);
  else if (SrcExp.music.attempt)
    se::AttemptMusic(SrcExp);
  else if (SrcExp.shaders.attempt)
    se::AttemptShaders(SrcExp);
  else if (SrcExp.binaryFiles.attempt)
    se::AttemptBinaryFiles(SrcExp);
  else if (SrcExp.errorLog.attempt)
    se::AttemptErrorLog(SrcExp);
}

void SourceBytePairsMain(float FrameTime)
{
  if (ImGui::BeginMenuBar())
  {
    ImGui::Checkbox("Byte Pairs", &bytePairsMode);
    ImGui::EndMenuBar();
  }

  auto load = [](se::file_state_t &FileState) {
    lak::debugger.clear();
    std::error_code ec;
    auto code = lak::open_file_modal(FileState.path, false, ec);
    if (code == lak::file_open_error::VALID)
    {
      SrcExp.state      = se::game_t{};
      SrcExp.state.file = lak::read_file(FileState.path);
      return true;
    }
    return code != lak::file_open_error::INCOMPLETE;
  };

  auto manip = [] {
    DEBUG("File size: 0x" << SrcExp.state.file.size());
    SrcExp.loaded = true;
    return true;
  };

  if (!SrcExp.loaded || SrcExp.exe.attempt)
  {
    SrcExp.exe.attempt = true;
    se::Attempt(SrcExp.exe, load, manip);
    if (!SrcExp.exe.attempt) bytePairsMode = false; // User cancelled
  }

  if (SrcExp.loaded)
    BytePairsMemoryExplorer(
      SrcExp.state.file.data(), SrcExp.state.file.size(), false);
}

void MainScreen(float FrameTime)
{
  if (bytePairsMode)
    SourceBytePairsMain(FrameTime);
  else
    SourceExplorerMain(FrameTime);
}


#include <lak/basic_program.inl>

ImGui::ImplContext imgui_context = nullptr;

void basic_window_init(int argc, char **argv, lak::window &window)
{
  lak::debugger.crash_path = SrcExp.errorLog.path =
    fs::current_path() / "SEND-THIS-CRASH-LOG-TO-LAK132.txt";

  SrcExp.exe.path = SrcExp.images.path = SrcExp.sortedImages.path =
    SrcExp.sounds.path = SrcExp.music.path = SrcExp.shaders.path =
      SrcExp.binaryFiles.path = SrcExp.appicon.path = fs::current_path();

  bool force_software = false;
  if (argc > 1)
  {
    if (argv[1] == std::string("-nogl"))
    {
      force_software = true;
    }
    else
    {
      SrcExp.babyMode    = false;
      SrcExp.exe.path    = argv[1];
      SrcExp.exe.valid   = true;
      SrcExp.exe.attempt = true;
    }
  }

  switch (window.graphics())
  {
    case lak::graphics_mode::OpenGL:
      imgui_context = ImGui::ImplCreateContext(ImGui::GraphicsMode::OpenGL);
      openglMajor   = lak::opengl::get_uint<1>(GL_MAJOR_VERSION);
      openglMinor   = lak::opengl::get_uint<1>(GL_MINOR_VERSION);
      break;
    case lak::graphics_mode::Software:
      imgui_context = ImGui::ImplCreateContext(ImGui::GraphicsMode::Software);
      break;
  }

  SrcExp.graphicsMode = window.graphics();
  ImGui::ImplInit();
  ImGui::ImplInitContext(imgui_context, window);

  if (SDL_Init(SDL_INIT_AUDIO)) ERROR("Failed to initialise SDL audio");

  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
  ImGui::GetStyle().WindowRounding = 0;
}

void basic_window_handle_event(SDL_Event &event, lak::window &window)
{
  ImGui::ImplProcessEvent(imgui_context, event);

  switch (event.type)
  {
    case SDL_DROPFILE:
    {
      if (event.drop.file != nullptr)
      {
        SrcExp.exe.path    = event.drop.file;
        SrcExp.exe.valid   = true;
        SrcExp.exe.attempt = true;
        SDL_free(event.drop.file);
      }
    }
    break;
  }
}

void basic_window_loop(lak::window &window, uint64_t counter_delta)
{
  const float frame_time = (float)counter_delta / lak::performance_frequency();
  ImGui::ImplNewFrame(imgui_context, window.sdl_window(), frame_time);

  bool mainOpen = true;

  ImGuiStyle &style = ImGui::GetStyle();
  ImGuiIO &io       = ImGui::GetIO();

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImVec2 old_window_padding = style.WindowPadding;
  style.WindowPadding       = ImVec2(0.0f, 0.0f);
  if (ImGui::Begin(APP_NAME,
                   &mainOpen,
                   ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
  {
    style.WindowPadding = old_window_padding;
    MainScreen(frame_time);
    ImGui::End();
  }

  ImGui::ImplRender(imgui_context);
}

void basic_window_quit(lak::window &window)
{
  ImGui::ImplShutdownContext(imgui_context);
}

#include "dump.cpp"
#include "explorer.cpp"
#include "lisk_impl.cpp"

#include "imgui_impl_lak.cpp"
#include "imgui_utils.cpp"

#include <lak/src/opengl/shader.cpp>
#include <lak/src/opengl/texture.cpp>

#include <lak/src/debug.cpp>
#include <lak/src/file.cpp>
#include <lak/src/memory.cpp>
#include <lak/src/strconv.cpp>
#include <lak/src/tinflate.cpp>
#include <lak/src/window.cpp>

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
