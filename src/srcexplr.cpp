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
MemoryEditor memEdit;
vector<uint8_t> gameMem;

int main(int, char**)
{
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

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        #ifndef DEBUG
        try{
        #endif
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        bool mainOpen = true;
        static bool prevOpen = true;
        static bool openDiag = false;
        static bool dumpImages = false;
        static bool dumpDLL = false;
        static Image viewImage;

        ImGui::SetNextWindowSize(ImVec2(550,680), ImGuiCond_FirstUseEver);
        if(ImGui::Begin("Source Explorer", &mainOpen, ImGuiWindowFlags_MenuBar))
        {
            if(ImGui::BeginMenuBar())
            {
                if(ImGui::BeginMenu("File"))
                {
                    ImGui::MenuItem("Open..", NULL, &openDiag);
                    ImGui::MenuItem("Dump Images", NULL, &dumpImages);
                    ImGui::MenuItem("Dump mmfs2.dll", NULL, &dumpDLL);
                    ImGui::EndMenu();
                }
                ImGui::Checkbox("Print to debug console?", &debugConsole);
                ImGui::Checkbox("Crash on error?", &throwErrors);
                ImGui::EndMenuBar();
            }

            ImGui::Spacing();
            if(ImGui::Button("Refresh from memory"))
            {
                srcexp.gameBuffer.position = 0;
                srcexp.loadFromMem();
            }

            if(srcexp.loaded)
            {
                renderMenu_t rm;
                static string errtxt = "";
                rm.errtxt = &errtxt;
                rm.memedit = &gameMem;
                rm.srcexp = &srcexp;
                rm.viewImage = &viewImage;
				srcexp.gameBuffer.position = srcexp.dataLocation;
                // while(srcexp.gameBuffer.position < srcexp.gameBuffer.data->size())
                // {
                //     ResourceEntry res(srcexp.gameBuffer, srcexp.gameState);
                //     res.renderMenu(rm);
                //     if (res.ID == CHUNK_LAST) break;
                // }
                // srcexp.game.renderMenu(rm);
                srcexp.draw(rm);
                ImGui::Text(errtxt.c_str());
            }

            ImGui::Separator();
            ImGui::Text("Raw: ");

            if (openDiag)
            {
                ImGui::OpenPopup("Open");
                openDiag = false;
            }
            if (ImGui::BeginPopupModal("Open", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                static char dir[100] = DEFAULT_GAME;
                static string errtxt = "";
                ImGui::InputText("Game Dir", dir, IM_ARRAYSIZE(dir));
                if (ImGui::Button("Open File"))
                {
                    #ifndef DEBUG
                    try
                    {
                    #endif
                        // gameMemPtr = nullptr;
                        // gameMemSize = 0;
                        errtxt = "";
                        srcexp.loadIntoMem(string(dir));
                        //static ResourceEntry* back = srcexp.game.chunks[0];
                        //srcexp.game.chunks[0] = (ResourceEntry*)0xDEAD; //completely bullshit pointer to fuck with whatever the fuck is fucking with this object
                        ImGui::CloseCurrentPopup();
                    #ifndef DEBUG
                    }
                    catch (std::exception e)
                    {
                        errtxt = "Error: ";
                        errtxt += e.what();
                        cout << errtxt << endl;
                        flush(cout);
                        THROW(e);
                    }
                    #endif
                }
                if (ImGui::Button("Cancel"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Text(errtxt.c_str());
                ImGui::EndPopup();
            }
            if (dumpImages)
            {
                ImGui::OpenPopup("Dump Images");
                dumpImages = false;
            }
            if (ImGui::BeginPopupModal("Dump Images", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                static char dir[100] = DEFAULT_DUMP;
                static string errtxt = "";
                ImGui::InputText("Dir", dir, IM_ARRAYSIZE(dir));
                if (ImGui::Button("Dump Images"))
                {
                    #ifndef DEBUG
                    try
                    {
                    #endif
                        size_t dumpCount = 0;
                        errtxt = "";
                        // for(auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++)
                        // {
                        //     if(it->ID == CHUNK_IMAGEBANK)
                        //     {
                        //         for(auto img = it->chunks.begin(); img != it->chunks.end(); img++)
                        //         {
                        //             for(auto c = &(dir[1]); c != &(dir[99-1]); c++) {
                        //                 if(*c == 0) {
                        //                     if (*(c-1) != '\\')
                        //                     {
                        //                         *c = '\\';
                        //                         *(c+1) = 0; // just to be sure it's still null-terminated
                        //                     }
                        //                     else break;
                        //                 }
                        //             }
                        //             char imgdir[110];
                        //             sprintf(imgdir, "%s%x.png", dir, img->ID);
                        //             Image image;
                        //             int err = image.generateImage(img->mainData.read(srcexp.gameBuffer.data));
                        //             if (err == 0) throw std::exception("Error exporting images, make sure the directory exists!");
                        //             stbi_write_png(imgdir, image.bitmap.w, image.bitmap.h, 4, &(image.bitmap.toRGBA()[0]), image.bitmap.w*4);
                        //             dumpCount++;
                        //         }
                        //     }
                        // }
                        cout << "Images dumped: " << std::dec << dumpCount << endl;
                        ImGui::CloseCurrentPopup();
                    #ifndef DEBUG
                    }
                    catch (std::exception e)
                    {
                        errtxt = "Error: ";
                        errtxt += e.what();
                        cout << errtxt << endl;
                        flush(cout);
                        THROW(e);
                    }
                    #endif
                }
                if (ImGui::Button("Cancel"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Text(errtxt.c_str());
                ImGui::EndPopup();
            }
            if (dumpDLL)
            {
                ImGui::OpenPopup("Dump DLL");
                dumpDLL = false;
            }
            if (ImGui::BeginPopupModal("Dump DLL", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                static char dir[100] = DEFAULT_DUMP;
                static string errtxt = "";
                ImGui::InputText("Dir", dir, IM_ARRAYSIZE(dir));
                if (ImGui::Button("Dump DLL"))
                {
                    #ifndef DEBUG
                    try
                    {
                    #endif
                        size_t dumpCount = 0;
                        errtxt = "";
                        for(auto c = &(dir[1]); c != &(dir[99-1]); c++) {
                            if(*c == 0) {
                                if (*(c-1) != '\\')
                                {
                                    *c = '\\';
                                    *(c+1) = 0; // just to be sure it's still null-terminated
                                }
                                else break;
                            }
                        }
                        char dir2[100];
                        sprintf(dir2, "%smmfs2.dll", dir);
                        FILE* pfile = fopen(dir2, "w");
                        if(pfile != NULL)
                        {
                            for(auto it = srcexp.packFiles.begin(); it != srcexp.packFiles.end(); it++)
                            {
                                if(it->filename == "mmfs2.dll")
                                {
                                    for(auto it2 = it->data.begin(); it2 != it->data.end(); it2++)
                                    {
                                        fputc(*it2, pfile);
                                    }
                                }
                            }
                            fclose(pfile);
                        }
                        // for(auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++)
                        // {
                        //     if(it->ID == CHUNK_IMAGEBANK)
                        //     {
                        //         for(auto img = it->chunks.begin(); img != it->chunks.end(); img++)
                        //         {
                        //             for(auto c = &(dir[1]); c != &(dir[99-1]); c++) {
                        //                 if(*c == 0) {
                        //                     if (*(c-1) != '\\')
                        //                     {
                        //                         *c = '\\';
                        //                         *(c+1) = 0; // just to be sure it's still null-terminated
                        //                     }
                        //                     else break;
                        //                 }
                        //             }
                        //             char imgdir[110];
                        //             sprintf(imgdir, "%s%x.png", dir, img->ID);
                        //             Image image;
                        //             int err = image.generateImage(img->mainData.read(srcexp.gameBuffer.data));
                        //             if (err == 0) throw std::exception("Error exporting images, make sure the directory exists!");
                        //             stbi_write_png(imgdir, image.bitmap.w, image.bitmap.h, 4, &(image.bitmap.toRGBA()[0]), image.bitmap.w*4);
                        //             dumpCount++;
                        //         }
                        //     }
                        // }
                        cout << "Images dumped: " << std::dec << dumpCount << endl;
                        ImGui::CloseCurrentPopup();
                    #ifndef DEBUG
                    }
                    catch (std::exception e)
                    {
                        errtxt = "Error: ";
                        errtxt += e.what();
                        cout << errtxt << endl;
                        flush(cout);
                        THROW(e);
                    }
                    #endif
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
            ImGui::Image((ImTextureID)(intptr_t)viewImage.tex, ImVec2(viewImage.width, viewImage.height));
            ImGui::End();
        }

        memEdit.DrawWindow("Game Memory", (gameMem.size() > 0 ? &(gameMem[0]) : nullptr), gameMem.size(), 0x0);

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        glfwSwapBuffers(window);
        #ifndef DEBUG
        }
        catch (std::exception e)
        {
            cout << e.what() << endl;
            flush(cout);
			//throw e;
            THROW(e);
        }
        #endif
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}
