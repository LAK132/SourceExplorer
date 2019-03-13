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

#ifndef MAXDIRLEN
#define MAXDIRLEN 512
#endif

#define DUMP_FUNCTION(name) void name (const fs::path&path, se::game_t&state, se::resource_entry_t&entry, se::resource_entry_t&parent)
typedef DUMP_FUNCTION(dump_function_t);

// void Dump(std::atomic<float> &completed, const fs::path &path, se::game_t &state,
//     chunk_t ID, dump_function_t *func)
// {
//     completed = 0.0f;
//     size_t count = 0;
//     for (auto it = state.game.chunks.begin(); it != state.game.chunks.end(); ++it)
//     {
//         if (it->ID == ID)
//         {
//             func(path, state, *it, state.game);
//         }
//         completed = ((float)count / (float)state.game.chunks.size()) * 100.0f;
//         ++count;
//     }
// }

// void DumpBank(std::atomic<float> &completed, const fs::path &path, se::game_t &state,
//     chunk_t ID, dump_function_t *func)
// {
//     for (auto bank = state.game.chunks.begin(); bank != state.game.chunks.end(); ++bank)
//     {
//         if (bank->ID == ID)
//         {
//             completed = 0.0f;
//             size_t count = 0;
//             for (auto it = bank->chunks.begin(); it != bank->chunks.end(); ++it)
//             {
//                 func(path, state, *it, *bank);
//                 completed = ((float)count / (float)bank->chunks.size()) * 100.0f;
//                 ++count;
//             }
//             break;
//         }
//     }
// }

// dump_function_t DumpSound;
// void DumpSound(const fs::path &path, se::game_t &state,
//     se::resource_entry_t &entry, se::resource_entry_t &parent)
DUMP_FUNCTION(DumpSound)
{
    lak::memstrm_t sndheader(entry.decodeHeader());

    uint32_t checksum = sndheader.readInt<uint32_t>(); (void)checksum;
    uint32_t references = sndheader.readInt<uint32_t>(); (void)references;
    uint32_t decompLen = sndheader.readInt<uint32_t>(); (void)decompLen;
    sound_mode_t type = sndheader.readInt<sound_mode_t>(); // uint32_t
    uint32_t reserved = sndheader.readInt<uint32_t>(); (void)reserved;
    uint32_t nameLen = sndheader.readInt<uint32_t>();

    lak::memstrm_t sound(entry.decode());

    std::u16string name = sound.readString<char16_t>(nameLen);
    switch(type)
    {
        case WAVE: name += u".wav"; break;
        case MIDI: name += u".midi"; break;
        default: name += u".mp3"; break;
    }
    WDEBUG(L"\nDumping Music '" << lak::strconv<wchar_t>(name) << L"'");

    fs::path dir = path / name;
    std::vector<uint8_t> file(sound.cursor(), sound.end());

    if (!lak::SaveFile(dir, file))
    {
        WERROR(L"Failed To Save File '" << dir << L"'");
    }
}

// dump_function_t DumpMusic;
// void DumpMusic(const fs::path &path, se::game_t &state,
//     se::resource_entry_t &entry, se::resource_entry_t &parent)
DUMP_FUNCTION(DumpMusic)
{
    lak::memstrm_t sound(entry.decode());

    uint32_t checksum = sound.readInt<uint32_t>(); (void)checksum;
    uint32_t references = sound.readInt<uint32_t>(); (void)references;
    uint32_t decompLen = sound.readInt<uint32_t>(); (void)decompLen;
    sound_mode_t type = sound.readInt<sound_mode_t>(); // uint32_t
    uint32_t reserved = sound.readInt<uint32_t>(); (void)reserved;
    uint32_t nameLen = sound.readInt<uint32_t>();

    std::u16string name = sound.readString<char16_t>(nameLen);
    switch(type)
    {
        case WAVE: name += u".wav"; break;
        case MIDI: name += u".midi"; break;
        default: name += u".mp3"; break;
    }
    WDEBUG(L"\nDumping Music '" << lak::strconv<wchar_t>(name) << L"'");

    fs::path dir = path / name;
    std::vector<uint8_t> file(sound.cursor(), sound.end());

    if (!lak::SaveFile(dir, file))
    {
        WERROR(L"Failed To Save File '" << dir << L"'");
    }
}

// dump_function_t DumpImage;
// void DumpImage(const fs::path &path, se::game_t &state,
//     se::resource_entry_t &entry, se::resource_entry_t &parent)
DUMP_FUNCTION(DumpImage)
{
    DEBUG("\nDumping Image 0x" << entry.ID);

    char strid[100];
    sprintf(strid, "%x.png", entry.ID);

    lak::memstrm_t imgstrm(entry.decode());

    const se::image_t &image = se::CreateImage(imgstrm, state.oldGame);

    fs::path dir = path / strid;
    if (1 != stbi_write_png(dir.u8string().c_str(),
        (int)image.bitmap.size.x, (int)image.bitmap.size.y, 4,
        &(image.bitmap.pixels[0].r), (int)(image.bitmap.size.x * 4)))
    {
        WERROR(L"Failed To Save File '" << L"'");
    }
}

using dump_data_t = std::tuple<
    std::atomic<float> &,
    const fs::path &,
    se::game_t &
>;

using dump_bank_function_t = void (
    std::atomic<float> &,
    const fs::path &,
    se::game_t &
);

///
/// loop()
/// Called every loop
///
void Update(se::source_explorer_t &srcexp, MemoryEditor &memEdit)
{
    static bool loaded = false;

    bool mainOpen = true;
    static bool openGame = false;
    static bool dumpImages = false;
    static bool dumpSounds = false;

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
                openGame |= ImGui::MenuItem("Open...", nullptr);
                dumpImages |= ImGui::MenuItem("Dump Images", nullptr);
                dumpSounds |= ImGui::MenuItem("Dump Sounds", nullptr);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Credits"))
            {
                credits();
                ImGui::EndMenu();
            }
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
            if (loaded)
            {
                // ViewGame(srcexp);
                srcexp.state.game.view(srcexp);
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("Right", ImVec2(rightSize, -1), true, ImGuiWindowFlags_NoSavedSettings);
        {
            // ImVec2 contentSize2 = ImGui::GetWindowContentRegionMax();
            // contentSize2.x = ImGui::GetWindowContentRegionWidth();

            // static float topSize = contentSize2.x / 2;
            // static float bottomSize = contentSize2.x / 2;

            // lak::VertSplitter(topSize, bottomSize, contentSize2.y);
            // ImGui::BeginChild("Top", ImVec2(-1, topSize), true, ImGuiWindowFlags_NoSavedSettings);
            // {
            //     if (srcexp.view != nullptr)
            //     {
            //         // ViewEntry(srcexp, *srcexp.view, false);
            //         // ImGui::Separator();
            //         // ViewData(srcexp, srcexp.view);
            //     }
            // }
            // ImGui::EndChild();
            // ImGui::Spacing();
            // ImGui::BeginChild("Bottom", ImVec2(-1, bottomSize), true, ImGuiWindowFlags_NoSavedSettings);
            // {
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
                    bool update = last != srcexp.view;

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
                        memEdit.DrawContents(&(srcexp.state.file.memory[0]), srcexp.state.file.size());
                    }
                    else if (dataMode == 1) // Header
                    {
                        if (update && srcexp.view != nullptr)
                            srcexp.buffer = raw
                                ? srcexp.view->header.data.memory
                                : srcexp.view->decodeHeader().memory;

                        memEdit.DrawContents(&(srcexp.buffer[0]), srcexp.buffer.size());
                    }
                    else if (dataMode == 2) // Data
                    {
                        if (update && srcexp.view != nullptr)
                            srcexp.buffer = raw
                                ? srcexp.view->data.data.memory
                                : srcexp.view->decode().memory;

                        memEdit.DrawContents(&(srcexp.buffer[0]), srcexp.buffer.size());
                    }
                    else dataMode = 0;

                    last = srcexp.view;
                }
                else if (selected == 1)
                {
                    if (srcexp.image.valid())
                        ImGui::Image((ImTextureID)(intptr_t)srcexp.image.get(),
                            ImVec2((float)srcexp.image.size().x, (float)srcexp.image.size().y));
                }
                else selected = 0;
            // }
            // ImGui::EndChild();
        }
        ImGui::EndChild();
        ImGui::End();
    }

    // ImGui::ShowDemoWindow();

    if (openGame)
    {
        static std::tuple<se::source_explorer_t&> data = {srcexp};

        static bool gotFile = false;
        static std::thread *load = nullptr;
        static std::atomic<bool> finished;

        if (lak::OpenFileThread("Load Game", srcexp.file, gotFile,
            load, finished, &se::LoadGame, data))
        {
            openGame = false;
            loaded = gotFile;
            gotFile = false;
        }
    }

    if (dumpImages)
    {
        static fs::path dir = "";
        static std::atomic<float> completed;
        static dump_data_t data = {completed, dir, srcexp.state};

        static bool gotFile = false;
        static std::thread *load = nullptr;
        static std::atomic<bool> finished;
        static std::string errtxt = "";

        static dump_bank_function_t *func = [](
            std::atomic<float> &completed,
            const fs::path &path,
            se::game_t &state)
        {
            // DumpBank(completed, path, state, chunk_t::IMAGEBANK, &DumpImage);
        };

        if (lak::SaveFileThread("Dump Images", dir, gotFile,
            load, finished, completed, func, data))
        {
            dumpImages = false;
            gotFile = false;
        }
    }

    if (dumpSounds)
    {
        static fs::path dir = "", file = "";
        static std::atomic<float> completed;
        static dump_data_t data = {completed, dir, srcexp.state};

        static bool gotFile = false;
        static std::thread *load = nullptr;
        static std::atomic<bool> finished;
        static std::string errtxt = "";

        static dump_bank_function_t *func = [](
            std::atomic<float> &completed,
            const fs::path &path,
            se::game_t &state)
        {
            // DumpBank(completed, path, state, chunk_t::SOUNDBANK, &DumpSound);
            // DumpBank(completed, path, state, chunk_t::MUSICBANK, &DumpMusic);
        };

        if (lak::OpenFileThread("Dump Sounds", dir, file,
            gotFile, load, finished, func, data))
        {
            dumpSounds = false;
            gotFile = false;
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

    se::source_explorer_t srcexp;
    MemoryEditor memEdit;

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

                // /*
                case SDL_DROPFILE: {
                    std::string fileDir = "";
                    if (event.drop.file != nullptr)
                    {
                        fileDir = event.drop.file;
                        SDL_free(event.drop.file);
                    }
                    SDL_ShowSimpleMessageBox(
                        SDL_MESSAGEBOX_INFORMATION,
                        "File dropped on window",
                        fileDir.c_str(),
                        window.window
                    );
                } break;

                case SDL_DROPTEXT: {
                    std::string fileDir = "";
                    if (event.drop.file != nullptr)
                    {
                        fileDir = event.drop.file;
                        SDL_free(event.drop.file);
                    }
                    SDL_ShowSimpleMessageBox(
                        SDL_MESSAGEBOX_INFORMATION,
                        "Text dropped on window",
                        fileDir.c_str(),
                        window.window
                    );
                } break;

                case SDL_DROPBEGIN: {
                    std::string fileDir = "";
                    if (event.drop.file != nullptr)
                    {
                        fileDir = event.drop.file;
                        SDL_free(event.drop.file);
                    }
                    printf("File '%s' drop begin on window %d\n",
                        fileDir.c_str(), event.window.windowID);
                } break;

                case SDL_DROPCOMPLETE: {
                    std::string fileDir = "";
                    if (event.drop.file != nullptr)
                    {
                        fileDir = event.drop.file;
                        SDL_free(event.drop.file);
                    }
                    printf("File '%s' drop complete on window %d\n",
                        fileDir.c_str(), event.window.windowID);
                } break;
                // */
            }
        }

        // --- END EVENTS ---

        ImGui::ImplNewFrame(window.window, frameTime);

        // --- BEGIN UPDATE ---

        Update(srcexp, memEdit);

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
