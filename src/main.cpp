// Copyright (c) Mathias Kaerlev 2012, Lucas Kleiss 2019

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

#define IMGUI_DEFINE_MATH_OPERATORS
#include "main.h"

se::source_explorer_t SrcExp;

#ifndef MAXDIRLEN
#define MAXDIRLEN 512
#endif

using dump_data_t = std::tuple<
    se::source_explorer_t &,
    std::atomic<float> &
>;

using dump_function_t = void (
    se::source_explorer_t &,
    std::atomic<float> &
);

bool OpenGame()
{
    static std::tuple<se::source_explorer_t&> data = {SrcExp};

    static std::thread *thread = nullptr;
    static std::atomic<bool> finished = false;
    static bool popupOpen = false;

    if (lak::AwaitPopup("Open Game", popupOpen, thread, finished, &se::LoadGame, data))
    {
        ImGui::Text("Loading, please wait...");
        ImGui::Checkbox("Print to debug console?", &se::debugConsole);
        if (se::debugConsole)
        {
            ImGui::Checkbox("Only errors?", &se::errorOnlyConsole);
            ImGui::Checkbox("Developer mode?", &se::developerConsole);
        }
        ImGui::ProgressBar(SrcExp.state.completed);
        if (popupOpen)
            ImGui::EndPopup();
    }
    else
    {
        SrcExp.state.completed = 0.0f;
        return true;
    }
    return false;
}

bool DumpStuff(const char *str_id, dump_function_t *func)
{
    static std::atomic<float> completed = 0.0f;
    static dump_data_t data = {SrcExp, completed};

    static std::thread *thread = nullptr;
    static std::atomic<bool> finished = false;
    static bool popupOpen = false;

    if (lak::AwaitPopup(str_id, popupOpen, thread, finished, func, data))
    {
        ImGui::Text("Dumping, please wait...");
        ImGui::Checkbox("Print to debug console?", &se::debugConsole);
        if (se::debugConsole)
        {
            ImGui::Checkbox("Only errors?", &se::errorOnlyConsole);
            ImGui::Checkbox("Developer mode?", &se::developerConsole);
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

bool SaveImage(
    const lak::image4_t &image,
    const fs::path &filename
)
{
    if (stbi_write_png(filename.u8string().c_str(),
        (int)image.size().x, (int)image.size().y, 4,
        &(image[0].r), (int)(image.size().x * 4)
    ) != 1)
    {
        ERROR("Failed To Save Image '" << filename << "'");
        return true;
    }
    return false;
}

bool SaveImage(
    se::source_explorer_t &srcexp,
    uint16_t handle,
    const fs::path &filename,
    const se::frame::item_t *frame
)
{
    auto *object = se::GetImage(srcexp.state, handle);
    if (!object)
    {
        ERROR("Failed To Save Image: Bad Handle '0x" << (int)handle << "'");
        return true;
    }

    lak::image4_t image = object->image(srcexp.dumpColorTrans, (frame && frame->palette) ? frame->palette->colors : nullptr);
    return SaveImage(image, filename);
}

void DumpImages(se::source_explorer_t &srcexp, std::atomic<float> &completed)
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
        fs::path filename = srcexp.images.path / (std::to_string(item.entry.handle) + ".png");
        SaveImage(image, filename);
        completed = (float)((double)(index++) / (double)count);
    }
}

void DumpSortedImages(se::source_explorer_t &srcexp, std::atomic<float> &completed)
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

    auto HandleName = [](const std::unique_ptr<se::string_chunk_t> &name, auto handle)
    {
        return (name && !name->value.empty() ? name->value + u" [" : u"["s) + lak::to_u16string(handle) + u"]";
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
                if (const auto *obj = se::GetObject(srcexp.state, object.handle); obj)
                {
                    std::u16string objectName = HandleName(obj->name, obj->handle) +
                        u"[" + lak::strconv_u16(std::string(se::GetObjectTypeString(obj->type))) + u"]";
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
                        if (const auto *img = se::GetImage(srcexp.state, imghandle); img)
                        {
                            if (usedImages.find(imghandle) == usedImages.end())
                            {
                                usedImages.insert(imghandle);
                                std::u16string imageName = lak::to_u16string(imghandle) + u".png";
                                fs::path imagePath = framePath / "[unsorted]" / imageName;

                                // check if 8bit image
                                if (img->need_palette() && frame.palette)
                                    SaveImage(img->image(srcexp.dumpColorTrans, frame.palette->colors), imagePath);
                                else
                                    if (LinkImages(unsortedPath / imageName, imagePath)) ERROR("Linking Failed");
                            }
                            for (const auto &imgname : imgnames)
                            {
                                std::u16string unsortedImageName = lak::to_u16string(imghandle) + u".png";
                                fs::path unsortedImagePath = framePath / "[unsorted]" / unsortedImageName;
                                std::u16string imageName = imgname + u".png";
                                fs::path imagePath = objectPath / imageName;
                                if (const auto *img = se::GetImage(srcexp.state, imghandle); img)
                                    if (LinkImages(unsortedImagePath, imagePath)) ERROR("Linking Failed");
                            }
                        }
                    }
                }
            }
        }
        completed = (float)((double)frameIndex++ / frameCount);
    }
}

///
/// loop()
/// Called every loop
///
void Update(float FrameTime)
{
    bool mainOpen = true;

    static ImGuiStyle &style = ImGui::GetStyle();
    static ImGuiIO &io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImVec2 oldWindowPadding = style.WindowPadding;
    style.WindowPadding = ImVec2(0.0f, 0.0f);
    if (ImGui::Begin(APP_NAME, &mainOpen,
        ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_MenuBar|
        ImGuiWindowFlags_NoSavedSettings |ImGuiWindowFlags_NoTitleBar |ImGuiWindowFlags_NoMove)
    )
    {
        style.WindowPadding = oldWindowPadding;
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                SrcExp.exe.attempt          |= ImGui::MenuItem("Open...", nullptr);
                SrcExp.sortedImages.attempt |= ImGui::MenuItem("Dump Sorted Images...", nullptr);
                SrcExp.images.attempt       |= ImGui::MenuItem("Dump Images...", nullptr);
                SrcExp.sounds.attempt       |= ImGui::MenuItem("Dump Sounds...", nullptr);
                SrcExp.music.attempt        |= ImGui::MenuItem("Dump Music...", nullptr);
                SrcExp.shaders.attempt      |= ImGui::MenuItem("Dump Shaders...", nullptr);
                SrcExp.binaryFiles.attempt  |= ImGui::MenuItem("Dump Binary Files...", nullptr);
                SrcExp.appicon.attempt      |= ImGui::MenuItem("Dump App Icon...", nullptr);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Credits"))
            {
                ImGui::Text("Frame rate %f", 1.0f / FrameTime);
                credits();
                ImGui::EndMenu();
            }
            ImGui::Checkbox("Enable color transparency?", &SrcExp.dumpColorTrans);
            ImGui::Checkbox("Force compat mode?", &se::forceCompat);
            ImGui::Checkbox("Print to debug console? (May cause SE to run slower)", &se::debugConsole);
            if (se::debugConsole)
            {
                ImGui::Checkbox("Only errors?", &se::errorOnlyConsole);
                ImGui::Checkbox("Developer mode?", &se::developerConsole);
            }
            ImGui::EndMenuBar();
        }

        ImVec2 contentSize = ImGui::GetWindowContentRegionMax();
        contentSize.x = ImGui::GetWindowContentRegionWidth();

        static float leftSize = contentSize.x / 2;
        static float rightSize = contentSize.x / 2;

        lak::HoriSplitter(leftSize, rightSize, contentSize.x);
        ImGui::BeginChild("Left", ImVec2(leftSize, -1), true, ImGuiWindowFlags_NoSavedSettings);
        {
            if (SrcExp.loaded)
            {
                if (SrcExp.state.game.title)
                    ImGui::Text("Title: %s", lak::strconv<char>(SrcExp.state.game.title->value).c_str());
                if (SrcExp.state.game.author)
                    ImGui::Text("Author: %s", lak::strconv<char>(SrcExp.state.game.author->value).c_str());
                if (SrcExp.state.game.copyright)
                    ImGui::Text("Copyright: %s", lak::strconv<char>(SrcExp.state.game.copyright->value).c_str());
                if (SrcExp.state.game.outputPath)
                    ImGui::Text("Output: %s", lak::strconv<char>(SrcExp.state.game.outputPath->value).c_str());
                if (SrcExp.state.game.projectPath)
                    ImGui::Text("Project: %s", lak::strconv<char>(SrcExp.state.game.projectPath->value).c_str());

                ImGui::Separator();

                ImGui::Text("New Game: %s", SrcExp.state.oldGame ? "No" : "Yes");
                ImGui::Text("Unicode Game: %s", SrcExp.state.unicode ? "Yes" : "No");
                ImGui::Text("Compat Game: %s", SrcExp.state.compat ? "Yes" : "No");
                ImGui::Text("Product Build: %zu", (size_t)SrcExp.state.productBuild);
                ImGui::Text("Product Version: %zu", (size_t)SrcExp.state.productVersion);
                ImGui::Text("Runtime Version: %zu", (size_t)SrcExp.state.runtimeVersion);
                ImGui::Text("Runtime Sub-Version: %zu", (size_t)SrcExp.state.runtimeSubVersion);

                ImGui::Separator();

                SrcExp.state.game.view(SrcExp);
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("Right", ImVec2(rightSize, -1), true, ImGuiWindowFlags_NoSavedSettings);
        {
            if (SrcExp.loaded)
            {
                static int selected = 0;
                ImGui::RadioButton("Memory", &selected, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Image", &selected, 1);
                ImGui::SameLine();
                ImGui::RadioButton("Audio", &selected, 2);
                static bool crypto = false;
                ImGui::Checkbox("Crypto", &crypto);
                ImGui::Separator();

                struct audio_data_t
                {
                    std::u8string name;
                    std::u16string u16name;
                    se::sound_mode_t type = (se::sound_mode_t)0;
                    uint32_t checksum = 0;
                    uint32_t references = 0;
                    uint32_t decompLen = 0;
                    uint32_t reserved = 0;
                    uint32_t nameLen = 0;
                    uint16_t format = 0;
                    uint16_t channelCount = 0;
                    uint32_t sampleRate = 0;
                    uint32_t byteRate = 0;
                    uint16_t blockAlign = 0;
                    uint16_t bitsPerSample = 0;
                    uint16_t unknown = 0;
                    uint32_t chunkSize = 0;
                    std::vector<uint8_t> data;
                };

                static bool memUpdate = false;
                static bool imageUpdate = false;
                static bool audioUpdate = false;
                if (crypto)
                {
                    int magic_char = se::_magic_char;
                    if (ImGui::InputInt("Magic Char", &magic_char))
                    {
                        se::_magic_char = magic_char;
                        se::GetEncryptionKey(SrcExp.state);
                        memUpdate = imageUpdate = audioUpdate = true;
                    }
                    if (ImGui::Button("Generate Crypto Key"))
                    {
                        se::GetEncryptionKey(SrcExp.state);
                        memUpdate = imageUpdate = audioUpdate = true;
                    }
                    ImGui::Separator();
                }

                if (selected == 0)
                {
                    static const se::basic_entry_t *last = nullptr;
                    memUpdate |= last != SrcExp.view;
                    static int dataMode = 0;
                    static bool raw = false;

                    memUpdate |= ImGui::RadioButton("EXE", &dataMode, 0);
                    ImGui::SameLine();
                    memUpdate |= ImGui::RadioButton("Header", &dataMode, 1);
                    ImGui::SameLine();
                    memUpdate |= ImGui::RadioButton("Data", &dataMode, 2);
                    ImGui::SameLine();
                    memUpdate |= ImGui::Checkbox("Raw", &raw);
                    ImGui::SameLine();
                    memUpdate |= ImGui::RadioButton("Magic Key", &dataMode, 3);
                    ImGui::Separator();

                    if (dataMode == 0) // EXE
                    {
                        SrcExp.editor.DrawContents(SrcExp.state.file.data(), SrcExp.state.file.size());
                        if (memUpdate && SrcExp.view != nullptr)
                            SrcExp.editor.GotoAddrAndHighlight(SrcExp.view->position, SrcExp.view->end);
                    }
                    else if (dataMode == 1) // Header
                    {
                        if (memUpdate && SrcExp.view != nullptr)
                            SrcExp.buffer = raw
                                ? SrcExp.view->header.data._data
                                : SrcExp.view->decodeHeader()._data;

                        SrcExp.editor.DrawContents(&(SrcExp.buffer[0]), SrcExp.buffer.size());
                        if (memUpdate)
                            SrcExp.editor.GotoAddrAndHighlight(0, 0);
                    }
                    else if (dataMode == 2) // Data
                    {
                        if (memUpdate && SrcExp.view != nullptr)
                            SrcExp.buffer = raw
                                ? SrcExp.view->data.data._data
                                : SrcExp.view->decode()._data;

                        SrcExp.editor.DrawContents(&(SrcExp.buffer[0]), SrcExp.buffer.size());
                        if (memUpdate)
                            SrcExp.editor.GotoAddrAndHighlight(0, 0);
                    }
                    else if (dataMode == 3) // _magic_key
                    {
                        SrcExp.editor.DrawContents(&(se::_magic_key[0]), se::_magic_key.size());
                        if (memUpdate)
                            SrcExp.editor.GotoAddrAndHighlight(0, 0);
                    }
                    else dataMode = 0;

                    last = SrcExp.view;
                    memUpdate = false;
                }
                else if (selected == 1)
                {
                    static float scale = 1.0f;
                    if (SrcExp.image.valid())
                    {
                        ImGui::DragFloat("Scale", &scale, 0.1, 0.1f, 10.0f);
                        ImGui::Separator();
                        se::ViewImage(SrcExp, scale);
                    }
                    imageUpdate = false;
                }
                else if (selected == 2)
                {
                    static const se::basic_entry_t *last = nullptr;
                    audioUpdate |= last != SrcExp.view;

                    static audio_data_t audioData;
                    if (audioUpdate && SrcExp.view != nullptr)
                    {
                        lak::memory audio = SrcExp.view->decode();
                        audioData = audio_data_t{};
                        if (SrcExp.state.oldGame)
                        {
                            audioData.checksum      = audio.read_u16();
                            audioData.references    = audio.read_u32();
                            audioData.decompLen     = audio.read_u32();
                            audioData.type          = (se::sound_mode_t)audio.read_u32();
                            audioData.reserved      = audio.read_u32();
                            audioData.nameLen       = audio.read_u32();

                            audioData.name          = audio.read_u8string_exact(audioData.nameLen);

                            if (audioData.type == se::sound_mode_t::WAVE)
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
                                header_ptr = &header_temp;
                            }
                            else header_ptr = &audio;
                            lak::memory &header = *header_ptr;

                            audioData.checksum      = header.read_u32();
                            audioData.references    = header.read_u32();
                            audioData.decompLen     = header.read_u32();
                            audioData.type          = (se::sound_mode_t)audio.read_u32();;
                            audioData.reserved      = header.read_u32();
                            audioData.nameLen       = header.read_u32();

                            if (SrcExp.state.unicode)
                            {
                                audioData.name = lak::strconv<char8_t>(audio.read_u16string_exact(audioData.nameLen));
                            }
                            else
                                audioData.name = audio.read_u8string_exact(audioData.nameLen);

                            if (audio.peek_string(4) == std::string("OggS"))
                                audioData.type = se::sound_mode_t::OGGS;

                            if (audioData.type == se::sound_mode_t::WAVE)
                            {
                                audio.position += 4; // "RIFF"
                                uint32_t size = audio.read_s32() + 4;
                                audio.position += 8; // "WAVEfmt "
                                // audio.position += 4; // 0x00000010
                                // 16, 18 or 40
                                uint32_t chunkSize      = audio.read_u32();
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
                                    [[maybe_unused]] uint16_t extensionSize  = audio.read_u16(); // 18
                                    DEBUG("Extension Size 0x" << extensionSize);
                                }
                                if (chunkSize >= 40)
                                {
                                    [[maybe_unused]] uint16_t validPerSample = audio.read_u16(); // 20
                                    DEBUG("Valid Bits Per Sample 0x" << validPerSample);
                                    [[maybe_unused]] uint16_t channelMask    = audio.read_u32(); // 24
                                    DEBUG("Channel Mask 0x" << channelMask);
                                    // SubFormat // 40
                                }
                                audio.position = pos;
                                audio.position += 4; // "data"
                                audioData.chunkSize     = audio.read_u32();
                                audioData.data          = audio.read(size);
                            }
                        }
                    }

                    static size_t audioSize = 0;
                    static bool playing = false;
                    static SDL_AudioSpec audioSpec;
                    static SDL_AudioDeviceID audioDevice = 0;
                    [[maybe_unused]] static SDL_AudioSpec audioSpecGot;
                    if (!playing && ImGui::Button("Play"))
                    {
                        SDL_AudioSpec spec;
                        spec.freq = audioData.sampleRate;
                        // spec.freq = audioData.byteRate;
                        switch (audioData.format)
                        {
                            case 0x0001: spec.format = AUDIO_S16; break;
                            case 0x0003: spec.format = AUDIO_F32; break;
                            case 0x0006: spec.format = AUDIO_S8; /*8bit A-law*/ break;
                            case 0x0007: spec.format = AUDIO_S8; /*abit mu-law*/ break;
                            case 0xFFFE: /*subformat*/ break;
                            default: break;
                        }
                        spec.channels = audioData.channelCount;
                        spec.samples = 2048;
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
                            audioDevice = SDL_OpenAudioDevice(nullptr, false, &audioSpec, &audioSpecGot, 0);

                        audioSize = audioData.data.size();
                        SDL_QueueAudio(audioDevice, audioData.data.data(), audioSize);
                        SDL_PauseAudioDevice(audioDevice, 0);
                        playing = true;
                    }
                    if (playing && (ImGui::Button("Stop") || (SDL_GetQueuedAudioSize(audioDevice) == 0)))
                    {
                        SDL_PauseAudioDevice(audioDevice, 1);
                        SDL_ClearQueuedAudio(audioDevice);
                        audioSize = 0;
                        playing = false;
                    }
                    if (audioSize > 0)
                        ImGui::ProgressBar(1.0 - (SDL_GetQueuedAudioSize(audioDevice) / (double)audioSize));
                    else
                        ImGui::ProgressBar(0);

                    ImGui::Text("Name: %s", audioData.name.c_str());
                    ImGui::Text("Type: "); ImGui::SameLine();
                    switch (audioData.type)
                    {
                        case se::sound_mode_t::WAVE: ImGui::Text("WAV"); break;
                        case se::sound_mode_t::MIDI: ImGui::Text("MIDI"); break;
                        case se::sound_mode_t::OGGS: ImGui::Text("OGG"); break;
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

                    last = SrcExp.view;
                    audioUpdate = false;
                }
                else selected = 0;
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }

    if (SrcExp.exe.attempt)
    {
        SrcExp.loaded = false;
        if (!SrcExp.exe.valid)
        {
            if (lak::OpenFile(SrcExp.exe.path, SrcExp.exe.valid))
            {
                if (!SrcExp.exe.valid)
                {
                    // User cancelled
                    SrcExp.exe.attempt = false;
                }
            }
        }
        else if (OpenGame())
        {
            SrcExp.loaded = true;
            SrcExp.exe.valid = false;
            SrcExp.exe.attempt = false;
        }
    }

    if (SrcExp.images.attempt)
    {
        if (!SrcExp.images.valid)
        {
            if (lak::OpenFolder(SrcExp.images.path, SrcExp.images.valid))
            {
                if (!SrcExp.images.valid)
                {
                    // User cancelled
                    SrcExp.images.attempt = false;
                }
            }
        }
        else if (DumpStuff("Dump Image", &DumpImages))
        {
            SrcExp.images.valid = false;
            SrcExp.images.attempt = false;
        }
    }

    if (SrcExp.sortedImages.attempt)
    {
        if (!SrcExp.sortedImages.valid)
        {
            if (lak::OpenFolder(SrcExp.sortedImages.path, SrcExp.sortedImages.valid))
            {
                if (!SrcExp.sortedImages.valid)
                {
                    // User cancelled
                    SrcExp.sortedImages.attempt = false;
                }
            }
        }
        else if (DumpStuff("Sorted Image Dump", &DumpSortedImages))
        {
            SrcExp.sortedImages.valid = false;
            SrcExp.sortedImages.attempt = false;
        }
    }

    if (SrcExp.appicon.attempt)
    {
        if (!SrcExp.appicon.valid)
        {
            if (lak::OpenFolder(SrcExp.appicon.path, SrcExp.appicon.valid))
            {
                if (!SrcExp.appicon.valid)
                {
                    // User cancelled
                    SrcExp.appicon.attempt = false;
                }
            }
        }
        else if (DumpStuff("Dump App Icon",
            [](se::source_explorer_t &srcexp, std::atomic<float> &completed)
            {
                if (!srcexp.state.game.icon)
                {
                    ERROR("No Icon");
                    return;
                }

                lak::image4_t &bitmap = srcexp.state.game.icon->bitmap;

                fs::path filename = srcexp.appicon.path / "favicon.ico";
                std::ofstream file(filename, std::ios::binary | std::ios::out | std::ios::ate);
                if (!file.is_open())
                    return;

                int len;
                unsigned char *png = stbi_write_png_to_mem(&(bitmap[0].r), (int)(bitmap.size().x * 4),
                    (int)bitmap.size().x, (int)bitmap.size().y, 4, &len);

                lak::memory strm;
                strm.reserve(0x16);
                strm.write_u16(0); // reserved
                strm.write_u16(1); // .ICO
                strm.write_u16(1); // 1 image
                strm.write_u8 (bitmap.size().x);
                strm.write_u8 (bitmap.size().y);
                strm.write_u8 (0); // no palette
                strm.write_u8 (0); // reserved
                strm.write_u16(1); // color plane
                strm.write_u16(8 * 4); // bits per pixel
                strm.write_u32(len);
                strm.write_u32(strm.position + sizeof(uint32_t));

                file.write((const char *)strm.data(), strm.position);
                file.write((const char *)png, len);

                STBIW_FREE(png);

                file.close();
            }
        ))
        {
            SrcExp.appicon.valid = false;
            SrcExp.appicon.attempt = false;
        }
    }

    if (SrcExp.sounds.attempt)
    {
        if (!SrcExp.sounds.valid)
        {
            if (lak::OpenFolder(SrcExp.sounds.path, SrcExp.sounds.valid))
            {
                if (!SrcExp.sounds.valid)
                {
                    // User cancelled
                    SrcExp.sounds.attempt = false;
                }
            }
        }
        else if (DumpStuff("Dump Sounds",
            [](se::source_explorer_t &srcexp, std::atomic<float> &completed)
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
                    se::sound_mode_t type;
                    if (srcexp.state.oldGame)
                    {
                        uint16_t checksum   = sound.read_u16(); (void)checksum;
                        uint32_t references = sound.read_u32(); (void)references;
                        uint32_t decompLen  = sound.read_u32(); (void)decompLen;
                        type                = (se::sound_mode_t)sound.read_u32();
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
                        uint32_t chunkSize                          = sound.read_u32();
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
                        lak::memory header                      = item.entry.decodeHeader();
                        [[maybe_unused]] uint32_t checksum      = header.read_u32();
                        [[maybe_unused]] uint32_t references    = header.read_u32();
                        [[maybe_unused]] uint32_t decompLen     = header.read_u32();
                        type                                    = (se::sound_mode_t)header.read_u32();
                        [[maybe_unused]] uint32_t reserved      = header.read_u32();
                        uint32_t nameLen                        = header.read_u32();

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
                            type = se::sound_mode_t::OGGS;
                        }
                    }

                    switch(type)
                    {
                        case se::sound_mode_t::WAVE: name += u".wav"; break;
                        case se::sound_mode_t::MIDI: name += u".midi"; break;
                        case se::sound_mode_t::OGGS: name += u".ogg"; break;
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
        ))
        {
            SrcExp.sounds.valid = false;
            SrcExp.sounds.attempt = false;
        }
    }

    if (SrcExp.music.attempt)
    {
        if (!SrcExp.music.valid)
        {
            if (lak::OpenFolder(SrcExp.music.path, SrcExp.music.valid))
            {
                if (!SrcExp.music.valid)
                {
                    // User cancelled
                    SrcExp.music.attempt = false;
                }
            }
        }
        else if (DumpStuff("Dump Music",
            [](se::source_explorer_t &srcexp, std::atomic<float> &completed)
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
                    se::sound_mode_t type;
                    if (srcexp.state.oldGame)
                    {
                        [[maybe_unused]] uint16_t checksum      = sound.read_u16();
                        [[maybe_unused]] uint32_t references    = sound.read_u32();
                        [[maybe_unused]] uint32_t decompLen     = sound.read_u32();
                        type                                    = (se::sound_mode_t)sound.read_u32();
                        [[maybe_unused]] uint32_t reserved      = sound.read_u32();
                        uint32_t nameLen                        = sound.read_u32();

                        name = lak::strconv_u16(sound.read_string_exact(nameLen));
                    }
                    else
                    {
                        [[maybe_unused]] uint32_t checksum      = sound.read_u32();
                        [[maybe_unused]] uint32_t references    = sound.read_u32();
                        [[maybe_unused]] uint32_t decompLen     = sound.read_u32();
                        type                                    = (se::sound_mode_t)sound.read_u32();
                        [[maybe_unused]] uint32_t reserved      = sound.read_u32();
                        uint32_t nameLen                        = sound.read_u32();

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
                        case se::sound_mode_t::WAVE: name += u".wav"; break;
                        case se::sound_mode_t::MIDI: name += u".midi"; break;
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
        ))
        {
            SrcExp.music.valid = false;
            SrcExp.music.attempt = false;
        }
    }

    if (SrcExp.shaders.attempt)
    {
        if (!SrcExp.shaders.valid)
        {
            if (lak::OpenFolder(SrcExp.shaders.path, SrcExp.shaders.valid))
            {
                if (!SrcExp.shaders.valid)
                {
                    // User cancelled
                    SrcExp.shaders.attempt = false;
                }
            }
        }
        else if (DumpStuff("Dump Shaders",
            [](se::source_explorer_t &srcexp, std::atomic<float> &completed)
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
                    if (!lak::SaveFile(filename, std::vector<uint8_t>(file.begin(), file.end())))
                    {
                        ERROR("Failed To Save File '" << filename << "'");
                    }

                    completed = (float)((double)count++ / (double)offsets.size());
                }
            }
        ))
        {
            SrcExp.shaders.valid = false;
            SrcExp.shaders.attempt = false;
        }
    }

    if (SrcExp.binaryFiles.attempt)
    {
        if (!SrcExp.binaryFiles.valid)
        {
            if (lak::OpenFolder(SrcExp.binaryFiles.path, SrcExp.binaryFiles.valid))
            {
                if (!SrcExp.binaryFiles.valid)
                {
                    // User cancelled
                    SrcExp.binaryFiles.attempt = false;
                }
            }
        }
        else if (DumpStuff("Dump Binary Files",
            [](se::source_explorer_t &srcexp, std::atomic<float> &completed)
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
        ))
        {
            SrcExp.binaryFiles.valid = false;
            SrcExp.binaryFiles.attempt = false;
        }
    }
}

// #include "tcc/libtcc.h"

int main()
{
    #if 0
    {
        TCCState *state = tcc_new();
        tcc_set_options(state, "-nostdlib -nostdinc");
        tcc_set_output_type(state, TCC_OUTPUT_MEMORY);

        tcc_compile_string(state, R"?(
int print(const char *__restrict__ __format, ...);
int tccmain()
{
    print("%d blaze it!\n", 420);
    *(char*)0 = 0;
    return 0;
}
)?");

        tcc_add_symbol(state, "print", (void*)&std::printf);
        tcc_relocate(state, TCC_RELOCATE_AUTO);

        typedef int (*tcc_main_t)(int, char**);
        tcc_main_t tcc_main = (tcc_main_t)tcc_get_symbol(state, "tccmain");

        // [[maybe_unused]] int result = tcc_main(0, 0);

        if (tcc_main) try
        {
            std::thread thread(tcc_main, 0, nullptr);
            thread.detach();
            std::this_thread::sleep_for(std::chrono::seconds(4));
            // if (thread.joinable())
            //     thread.join();
        }
        catch (std::exception &e)
        {
            ERROR(e.what());
        }

        tcc_delete(state);
    }
    #endif

    // const bool opengl = false;
    const bool opengl = true;
    lak::window_t window;
    if (opengl)
    {
        lak::InitGL(window, "Source Explorer", {1280, 720}, true);
    }
    else
    {
        lak::InitSR(window, "Source Explorer", {1280, 720}, true);
        SDL_GetWindowSurface(window.window);
    }

    if (SDL_Init(SDL_INIT_AUDIO))
        ERROR("Failed to initialise SDL audio");

    [[maybe_unused]] uint16_t targetFrameFreq = 59; // FPS
    [[maybe_unused]] float targetFrameTime = 1.0f / (float) targetFrameFreq; // SPF

    [[maybe_unused]] const uint64_t perfFreq = SDL_GetPerformanceFrequency();
    [[maybe_unused]] uint64_t perfCount = SDL_GetPerformanceCounter();
    [[maybe_unused]] float frameTime = targetFrameTime; // start non-zero

    if (opengl)
    {
        glViewport(0, 0, window.size.x, window.size.y);
        glClearColor(0.0f, 0.3125f, 0.3125f, 1.0f);
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        // SDL_Rect rect;
        // rect.x = 0; rect.y = 0;
        // rect.w = window.size.x; rect.h = window.size.y;
        // SDL_RenderSetViewport(window.srContext, &rect);
    }

    ImGui::ImplContext context = ImGui::ImplCreateContext(
        opengl ? ImGui::GraphicsMode::OPENGL : ImGui::GraphicsMode::SOFTWARE);
    ImGui::ImplInit();
    ImGui::ImplInitContext(context, window);

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 0;

    tinf_init();

    for (bool running = true; running;)
    {
        /* --- BEGIN EVENTS --- */

        for (SDL_Event event; SDL_PollEvent(&event); ImGui::ImplProcessEvent(context, event))
        {
            switch (event.type)
            {
                case SDL_QUIT: {
                    running = false;
                } break;

                /* --- Window events --- */
                case SDL_WINDOWEVENT: switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        window.size.x = event.window.data1;
                        window.size.y = event.window.data2;
                        if (opengl)
                            glViewport(0, 0, window.size.x, window.size.y);
                    } break;
                } break;

                /* --- Drag and drop events --- */

                case SDL_DROPFILE: {
                    if (event.drop.file != nullptr)
                    {
                        SrcExp.exe.path = event.drop.file;
                        SrcExp.exe.valid = true;
                        SrcExp.exe.attempt = true;
                        SDL_free(event.drop.file);
                    }
                } break;
            }
        }

        // --- END EVENTS ---

        ImGui::ImplNewFrame(context, window.window, frameTime);

        // --- BEGIN UPDATE ---

        Update(frameTime);

        // --- END UPDATE ---

        if (opengl)
        {
            glViewport(0, 0, window.size.x, window.size.y);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        else
        {
            // SDL_SetRenderDrawColor(window.srContext, 0x00, 0x80, 0x80, 0xFF);
            // SDL_RenderClear(window.srContext);
        }

        // --- BEGIN DRAW ---

        // --- END DRAW ---

        ImGui::ImplRender(context);

        if (opengl)
        {
            SDL_GL_SwapWindow(window.window);
        }
        else
        {
            // SDL_RenderPresent(window.srContext);
            context->srContext->window = window.window;
            assert(SDL_UpdateWindowSurface(window.window) == 0);
        }

        // --- TIMING ---

        float frameTimeError = frameTime - targetFrameTime;
        if (frameTimeError < 0.0f)
            frameTimeError = 0.0f;
        else if (frameTimeError > 0.0f)
            frameTimeError = std::fmod(frameTimeError, targetFrameTime);
            // frameTimeError = lak::mod(frameTimeError, targetFrameTime);
        const uint64_t prevPerfCount = perfCount;
        do {
            perfCount = SDL_GetPerformanceCounter();
            frameTime = (float)(frameTimeError + ((double)(perfCount - prevPerfCount) / (double)perfFreq));
            std::this_thread::yield();
        } while (frameTime < targetFrameTime);
    }

    ImGui::ImplShutdownContext(context);

    if (opengl)
        lak::ShutdownGL(window);
    else
        lak::ShutdownSR(window);

    return(0);
}

#include "lak.cpp"
#include "imgui_impl_lak.cpp"
#include "tinflate.cpp"

#include "explorer.cpp"

#include <examples/imgui_impl_softraster.cpp>

extern "C" {
#include "tinflate.c"
}

#include <memory/memory.cpp>