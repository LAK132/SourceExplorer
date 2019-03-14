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
        if (popupOpen)
            ImGui::EndPopup();
    }
    else
    {
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

///
/// loop()
/// Called every loop
///
void Update()
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
                SrcExp.exe.attempt |= ImGui::MenuItem("Open...", nullptr);
                SrcExp.images.attempt |= ImGui::MenuItem("Dump Images", nullptr);
                SrcExp.sounds.attempt |= ImGui::MenuItem("Dump Sounds", nullptr);
                SrcExp.music.attempt |= ImGui::MenuItem("Dump Music", nullptr);
                SrcExp.shaders.attempt |= ImGui::MenuItem("Dump Shaders", nullptr);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Credits"))
            {
                credits();
                ImGui::EndMenu();
            }
            ImGui::Checkbox("Enable Color Transparency?", &SrcExp.dumpColorTrans);
            ImGui::Checkbox("Print to debug console? (May cause SE to run slower)", &se::debugConsole);
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
                SrcExp.state.game.view(SrcExp);
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("Right", ImVec2(rightSize, -1), true, ImGuiWindowFlags_NoSavedSettings);
        {
            static int selected = 0;
            ImGui::RadioButton("Memory", &selected, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Image", &selected, 1);
            ImGui::Separator();

            if (selected == 0)
            {
                static int dataMode = 0;
                static bool raw = false;
                static const se::entry_t *last = nullptr;
                bool update = last != SrcExp.view;

                update |= ImGui::RadioButton("EXE", &dataMode, 0);
                ImGui::SameLine();
                update |= ImGui::RadioButton("Header", &dataMode, 1);
                ImGui::SameLine();
                update |= ImGui::RadioButton("Data", &dataMode, 2);
                ImGui::SameLine();
                update |= ImGui::Checkbox("Raw", &raw);
                ImGui::Separator();

                if (dataMode == 0) // EXE
                {
                    SrcExp.editor.DrawContents(&(SrcExp.state.file.memory[0]), SrcExp.state.file.size());
                    if (update && SrcExp.view != nullptr)
                        SrcExp.editor.GotoAddrAndHighlight(SrcExp.view->position, SrcExp.view->end);
                }
                else if (dataMode == 1) // Header
                {
                    if (update && SrcExp.view != nullptr)
                        SrcExp.buffer = raw
                            ? SrcExp.view->header.data.memory
                            : SrcExp.view->decodeHeader().memory;

                    SrcExp.editor.DrawContents(&(SrcExp.buffer[0]), SrcExp.buffer.size());
                    if (update)
                        SrcExp.editor.GotoAddrAndHighlight(0, 0);
                }
                else if (dataMode == 2) // Data
                {
                    if (update && SrcExp.view != nullptr)
                        SrcExp.buffer = raw
                            ? SrcExp.view->data.data.memory
                            : SrcExp.view->decode().memory;

                    SrcExp.editor.DrawContents(&(SrcExp.buffer[0]), SrcExp.buffer.size());
                    if (update)
                        SrcExp.editor.GotoAddrAndHighlight(0, 0);
                }
                else dataMode = 0;

                last = SrcExp.view;
            }
            else if (selected == 1)
            {
                if (SrcExp.image.valid())
                    ImGui::Image((ImTextureID)(intptr_t)SrcExp.image.get(),
                        ImVec2((float)SrcExp.image.size().x, (float)SrcExp.image.size().y));
            }
            else selected = 0;
        }
        ImGui::EndChild();
        ImGui::End();
    }

    if (SrcExp.exe.attempt)
    {
        if (!SrcExp.exe.valid)
        {
            if (lak::OpenFile(SrcExp.exe.path, SrcExp.exe.valid))
            {
                if (!SrcExp.exe.valid)
                {
                    // User cancelled
                    SrcExp.loaded = false;
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
        else if (DumpStuff("Dump Image",
            [](se::source_explorer_t &srcexp, std::atomic<float> &completed)
            {
                if (!srcexp.state.game.imageBank)
                {
                    DEBUG("No Image Bank");
                    return;
                }

                const size_t count = srcexp.state.game.imageBank->items.size();
                size_t index = 0;
                for (const auto &item : srcexp.state.game.imageBank->items)
                {
                    lak::memstrm_t strm = item.entry.decode();
                    const se::image_t &image = se::CreateImage(strm,
                        srcexp.dumpColorTrans, srcexp.state.oldGame);
                    fs::path filename = srcexp.images.path / (std::to_string(index) + ".png");
                    if (stbi_write_png(filename.u8string().c_str(),
                        (int)image.bitmap.size.x, (int)image.bitmap.size.y, 4,
                        &(image.bitmap.pixels[0].r), (int)(image.bitmap.size.x * 4)) != 1)
                    {
                        ERROR("Failed To Save File '" << filename << "'");
                    }
                    completed = (float)((double)(index++) / (double)count);
                }
            }
        ))
        {
            SrcExp.images.valid = false;
            SrcExp.images.attempt = false;
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
                    DEBUG("No Sound Bank");
                    return;
                }

                const size_t count = srcexp.state.game.soundBank->items.size();
                size_t index = 0;
                for (const auto &item : srcexp.state.game.soundBank->items)
                {
                    lak::memstrm_t sound = item.entry.decode();

                    std::u16string name;
                    sound_mode_t type;
                    if (srcexp.state.oldGame)
                    {
                        uint16_t checksum = sound.readInt<uint16_t>(); (void)checksum;
                        uint32_t references = sound.readInt<uint32_t>(); (void)references;
                        uint32_t decompLen = sound.readInt<uint32_t>(); (void)decompLen;
                        type = sound.readInt<sound_mode_t>(); // uint32_t
                        uint32_t reserved = sound.readInt<uint32_t>(); (void)reserved;
                        uint32_t nameLen = sound.readInt<uint32_t>();

                        name = lak::strconv<char16_t>(sound.readStringExact<char>(nameLen));

                        uint16_t format = sound.readInt<uint16_t>(); (void)format;
                        uint16_t channelCount = sound.readInt<uint16_t>(); (void)channelCount;
                        uint32_t sampleRate = sound.readInt<uint32_t>(); (void)sampleRate;
                        uint32_t byteRate = sound.readInt<uint32_t>(); (void)byteRate;
                        uint16_t blockAlign = sound.readInt<uint16_t>(); (void)blockAlign;
                        uint16_t bitsPerSample = sound.readInt<uint16_t>(); (void)bitsPerSample;
                        uint16_t unknown = sound.readInt<uint16_t>(); (void)unknown;
                        uint32_t chunkSize = sound.readInt<uint32_t>();
                        std::vector<uint8_t> data = sound.readBytes(chunkSize); (void) data;

                        lak::memstrm_t output;
                        output.writeString<char>("RIFF", false);
                        output.writeInt<int32_t>(data.size() - 44);
                        output.writeString<char>("WAVEfmt ", false);
                        output.writeInt<uint32_t>(0x10);
                        output.writeInt<uint16_t>(format);
                        output.writeInt<uint16_t>(channelCount);
                        output.writeInt<uint32_t>(sampleRate);
                        output.writeInt<uint32_t>(byteRate);
                        output.writeInt<uint16_t>(blockAlign);
                        output.writeInt<uint16_t>(bitsPerSample);
                        output.writeString<char>("data", false);
                        output.writeInt<uint32_t>(chunkSize);
                        output.writeBytes(data);
                        output.position = 0;
                        sound = std::move(output);
                    }
                    else
                    {
                        lak::memstrm_t header = item.entry.decodeHeader();
                        uint32_t checksum = header.readInt<uint32_t>(); (void)checksum;
                        uint32_t references = header.readInt<uint32_t>(); (void)references;
                        uint32_t decompLen = header.readInt<uint32_t>(); (void)decompLen;
                        type = header.readInt<sound_mode_t>(); // uint32_t
                        uint32_t reserved = header.readInt<uint32_t>(); (void)reserved;
                        uint32_t nameLen = header.readInt<uint32_t>();

                        if (srcexp.state.unicode)
                        {
                            name = sound.readStringExact<char16_t>(nameLen);
                        }
                        else
                        {
                            name = lak::strconv<char16_t>(sound.readStringExact<char>(nameLen));
                        }

                        if (sound.peekString<char>(4) == "OggS")
                        {
                            type = OGGS;
                        }
                    }

                    switch(type)
                    {
                        case WAVE: name += u".wav"; break;
                        case MIDI: name += u".midi"; break;
                        case OGGS: name += u".ogg"; break;
                        default: name += u".mp3"; break;
                    }

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
                    DEBUG("No Music Bank");
                    return;
                }

                const size_t count = srcexp.state.game.musicBank->items.size();
                size_t index = 0;
                for (const auto &item : srcexp.state.game.musicBank->items)
                {
                    lak::memstrm_t sound = item.entry.decode();

                    std::u16string name;
                    sound_mode_t type;
                    if (srcexp.state.oldGame)
                    {
                        uint16_t checksum = sound.readInt<uint16_t>(); (void)checksum;
                        uint32_t references = sound.readInt<uint32_t>(); (void)references;
                        uint32_t decompLen = sound.readInt<uint32_t>(); (void)decompLen;
                        type = sound.readInt<sound_mode_t>(); // uint32_t
                        uint32_t reserved = sound.readInt<uint32_t>(); (void)reserved;
                        uint32_t nameLen = sound.readInt<uint32_t>();

                        name = lak::strconv<char16_t>(sound.readStringExact<char>(nameLen));
                    }
                    else
                    {
                        uint32_t checksum = sound.readInt<uint32_t>(); (void)checksum;
                        uint32_t references = sound.readInt<uint32_t>(); (void)references;
                        uint32_t decompLen = sound.readInt<uint32_t>(); (void)decompLen;
                        type = sound.readInt<sound_mode_t>(); // uint32_t
                        uint32_t reserved = sound.readInt<uint32_t>(); (void)reserved;
                        uint32_t nameLen = sound.readInt<uint32_t>();

                        if (srcexp.state.unicode)
                        {
                            name = sound.readStringExact<char16_t>(nameLen);
                        }
                        else
                        {
                            name = lak::strconv<char16_t>(sound.readStringExact<char>(nameLen));
                        }
                    }

                    switch(type)
                    {
                        case WAVE: name += u".wav"; break;
                        case MIDI: name += u".midi"; break;
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
                    DEBUG("No Shaders");
                    return;
                }

                lak::memstrm_t strm = srcexp.state.game.shaders->entry.decode();

                uint32_t count = strm.readInt<uint32_t>();
                std::vector<uint32_t> offsets;
                offsets.reserve(count);

                while (count --> 0)
                    offsets.emplace_back(strm.readInt<uint32_t>());

                for (auto offset : offsets)
                {
                    strm.position = offset;
                    uint32_t nameOffset = strm.readInt<uint32_t>();
                    uint32_t dataOffset = strm.readInt<uint32_t>();
                    uint32_t paramOffset = strm.readInt<uint32_t>(); (void)paramOffset;
                    uint32_t backTex = strm.readInt<uint32_t>(); (void)backTex;

                    strm.position = offset + nameOffset;
                    fs::path filename = srcexp.shaders.path / strm.readString<char>();

                    strm.position = offset + dataOffset;
                    std::string file = strm.readString<char>();

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
}

int main()
{
    lak::glWindow_t window = lak::InitGL("Source Explorer", {640, 480}, true);

    uint16_t targetFrameFreq = 59; // FPS
    float targetFrameTime = 1.0f / (float) targetFrameFreq; // SPF

    const uint64_t perfFreq = SDL_GetPerformanceFrequency();
    uint64_t perfCount = SDL_GetPerformanceCounter();
    float frameTime = targetFrameTime; // start non-zero

    glViewport(0, 0, window.size.x, window.size.y);
    glClearColor(0.0f, 0.3125f, 0.3125f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    ImGuiContext *imguiContext = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 0;
    assert(ImGui::ImplInit(window));

    tinf_init();

    for (bool running = true; running;)
    {
        /* --- BEGIN EVENTS --- */

        for (SDL_Event event; SDL_PollEvent(&event); ImGui::ImplProcessEvent(event))
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

        ImGui::ImplNewFrame(window.window, frameTime);

        // --- BEGIN UPDATE ---

        Update();

        // --- END UPDATE ---

        glViewport(0, 0, window.size.x, window.size.y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- BEGIN DRAW ---

        // --- END DRAW ---

        ImGui::ImplRender();

        SDL_GL_SwapWindow(window.window);

        // --- TIMING ---

        float frameTimeError = frameTime - targetFrameTime;
        if (frameTimeError < 0.0f)
            frameTimeError = 0.0f;
        else if (frameTimeError > 0.0f)
            frameTimeError = lak::mod(frameTimeError, targetFrameTime);

        const uint64_t prevPerfCount = perfCount;
        do {
            perfCount = SDL_GetPerformanceCounter();
            frameTime = (float)(frameTimeError + ((double)(perfCount - prevPerfCount) / (double)perfFreq));
            std::this_thread::yield();
        } while (frameTime < targetFrameTime);
    }

    ImGui::ImplShutdown();
    ImGui::DestroyContext(imguiContext);

    lak::ShutdownGL(window);

    return(0);
}

#include "lak.cpp"
#include "imgui_impl_lak.cpp"
#include "tinflate.cpp"

#include "explorer.cpp"

#include "tinflate.c"
