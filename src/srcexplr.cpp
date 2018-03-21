// ImGui - standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)
//#include <vector>
//using std::vector;
//vector<uint8_t> exeDataVec(0x50000000);

#include "srcexplr.h"

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

SourceExplorer srcexp;

static uint8_t* gameMemPtr = &((*srcexp.gameBuffer.data)[0]);
static size_t gameMemSize = srcexp.gameBuffer.data->size();

void genTree(ResourceEntry* res)
{
    static string errtxt = "";
    for (auto it = res->chunks.begin(); it != res->chunks.end(); it++)
    {
        // if (*it == nullptr || *it == (ResourceEntry*)0xDEAD) continue;
        // uint16_t id = 0;
        // uint64_t dloc = 0;
        // uint16_t md = 0;
        // uint32_t dlen = 0;
        // size_t predlen = 0;
        // try
        // {
        //     id = (*it)->ID;
        //     dloc = (*it)->dataLoc;
        //     md = (*it)->mode;
        //     dlen = (*it)->dataLen;
        //     predlen = (*it)->preData.size();
        // }
        // catch (std::exception e)
        // {
        //     errtxt = "Error: ";
        //     errtxt += e.what();
        //     cout << errtxt << endl;
        //     flush(cout);
        //     break;
        // }
        
        // char str[100];
        // sprintf(str, "TreeNode0x%I32x%I64x", id, dloc);
        // ImGui::PushID(str);
        // sprintf(str, "0x%I32x", id);
        // if(ImGui::TreeNode(str))
        // {
        //     sprintf(str, "ID: 0x%I32x", id);
        //     ImGui::Text(str);
        //     sprintf(str, "Mode: 0x%I32x", md);
        //     ImGui::Text(str);
        //     sprintf(str, "Pre Data Size: 0x%I64x", predlen);
        //     ImGui::Text(str);
        //     sprintf(str, "Data Location: 0x%I64x", dloc);
        //     ImGui::Text(str);
        //     sprintf(str, "Data Length: 0x%I32x", dlen);
        //     ImGui::Text(str);
        //     if(ImGui::Button("View Pre Data"))
        //     {
        //         try
        //         {
        //             gameMemPtr = &((*it)->preData[0]);
        //             gameMemSize = (*it)->preData.size();
        //         }
        //         catch (std::exception e)
        //         {
        //             errtxt = "Error: ";
        //             errtxt += e.what();
        //             cout << errtxt << endl;
        //             flush(cout);
        //         }
        //     }
        //     genTree(*it);
        //     ImGui::TreePop();
        // }
        // ImGui::PopID();
    }
    ImGui::Text(errtxt.c_str());
}

int main(int, char**)
{
    //srcexp.gameBuffer.data->reserve(0x50000000);
    //srcexp.gameBuffer = MemoryStream(&exeDataVec);
    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Source Explorer", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    gl3wInit();

    // Setup ImGui binding
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfwGL3_Init(window, true);
    //io.NavFlags |= ImGuiNavFlags_EnableKeyboard;  // Enable Keyboard Controls
    //io.NavFlags |= ImGuiNavFlags_EnableGamepad;   // Enable Gamepad Controls

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them. 
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple. 
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // bool show_demo_window = true;
    // bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        try{
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        static bool mainOpen = true;
        static bool prevOpen = true;
        static bool openDiag = false;

        ImGui::SetNextWindowSize(ImVec2(550,680), ImGuiCond_FirstUseEver);
        if(ImGui::Begin("Source Explorer", &mainOpen, ImGuiWindowFlags_MenuBar))
        {
            if(ImGui::BeginMenuBar())
            {
                if(ImGui::BeginMenu("File"))
                {
                    ImGui::MenuItem("Open..", NULL, &openDiag);
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::Spacing();

            if(ImGui::TreeNode("Game"))
            {
                if(ImGui::Button("View Game Memory"))
                {
                    gameMemPtr = &((*srcexp.gameBuffer.data)[0]);
                    gameMemSize = srcexp.gameBuffer.data->size();
                }
                if(ImGui::Button("View Chunks Array Memory") && srcexp.game.chunks.size() > 0)
                {
                    gameMemPtr = (uint8_t*)&(srcexp.game.chunks[0]);
                    gameMemSize = srcexp.game.chunks.size()*(sizeof(ResourceEntry*)/sizeof(uint8_t));
                }
                static string errtxt = "";
                try
                {
                    genTree(&(srcexp.game));
                }
                catch (std::exception e)
                {
                    errtxt = "Error: ";
                    errtxt += e.what();
                    cout << errtxt << endl;
                    flush(cout);
                }
                ImGui::Text(errtxt.c_str());
                ImGui::TreePop();
            }
            if (openDiag)
            {
                ImGui::OpenPopup("Open");
                openDiag = false;
            }
            if (ImGui::BeginPopupModal("Open", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                static char dir[100];
                static string errtxt = "";
                ImGui::InputText("Game Dir", dir, IM_ARRAYSIZE(dir));
                if (ImGui::Button("Open File"))
                {
                    try
                    {
                        errtxt = "";
                        srcexp.loadGame(string(dir));
                        static ResourceEntry* back = srcexp.game.chunks[0];
                        srcexp.game.chunks[0] = (ResourceEntry*)0xDEAD; //completely bullshit pointer to fuck with whatever the fuck is fucking with this object
                        ImGui::CloseCurrentPopup();
                    }
                    catch (std::exception e)
                    {
                        errtxt = "Error: ";
                        errtxt += e.what();
                        cout << errtxt << endl;
                        flush(cout);
                    }
                }
                if (ImGui::Button("Cancel"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Text(errtxt.c_str());
                ImGui::EndPopup();
            }
            ImGui::End();
        }

        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(550,680), ImGuiCond_FirstUseEver);
        if(ImGui::Begin("Preview", &prevOpen))
        {
            ImGui::End();
        }

        static MemoryEditor memEdit;
        memEdit.DrawWindow("Game Memory", gameMemPtr, gameMemSize, 0x0);

        // 1. Show a simple window.
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug"
        // {
        //     static float f = 0.0f;
        //     static int counter = 0;
        //     ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
        //     ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
        //     ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        //     //ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
        //     //ImGui::Checkbox("Another Window", &show_another_window);

        //     if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
        //         counter++;
        //     ImGui::SameLine();
        //     ImGui::Text("counter = %d", counter);

        //     ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        // }

        // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
        // if (show_another_window)
        // {
        //     ImGui::Begin("Another Window", &show_another_window);
        //     ImGui::Text("Hello from another window!");
        //     if (ImGui::Button("Close Me"))
        //         show_another_window = false;
        //     ImGui::End();
        // }

        // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
        // if (show_demo_window)
        // {
        //     ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
        //     ImGui::ShowDemoWindow(&show_demo_window);
        // }

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        glfwSwapBuffers(window);
        }
        catch (std::exception e)
        {
            cout << e.what() << endl;
            flush(cout);
        }
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}
