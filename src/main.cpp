#include "main.h"

SDL_Window* window = nullptr;
SDL_GLContext glContext;
float clearCol[4] = {0.45f, 0.55f, 0.60f, 1.00f};
ImGuiIO* io = nullptr;
ImGuiStyle* style = nullptr;

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

SourceExplorer srcexp;
MemoryEditor memEdit;
vector<uint8_t> gameMem;

void genTree(ResourceEntry* res, const char* name = "", Image* viewImage = nullptr)
{
	if (res == nullptr) return;
    static string errtxt = "";
    char str[100];
    sprintf(str, "%s 0x%I32x##%I64x", name, res->ID, (uint64_t)&(*res));
    if(ImGui::TreeNode(str))
    {
        sprintf(str, "ID: 0x%I32x", res->ID);
        ImGui::Text(str);
        sprintf(str, "Mode: 0x%I32x", res->mode);
        ImGui::Text(str);
        sprintf(str, "Location: 0x%zx", res->location);
        ImGui::Text(str);
        sprintf(str, "Pre Data Size: 0x%I64x", res->preData.fileLen);
        ImGui::Text(str);
        sprintf(str, "Data Location: 0x%I64x", res->mainData.location);
        ImGui::Text(str);
        sprintf(str, "Data Length: 0x%I64x", res->mainData.dataLen);
        ImGui::Text(str);
        if(ImGui::Button("View Pre Data"))
        {
            try
            {
                // gameMemPtr = &(res->preData[0]);
                // gameMemSize = res->preData.size();
                gameMem = *res->preData.read(srcexp.gameBuffer.data).data;
            }
            catch (std::exception e)
            {
                errtxt = "Error: ";
                errtxt += e.what();
                cout << errtxt << endl;
                flush(cout);
            }
        }
        if(ImGui::Button("View Raw Main Data"))
        {
            try
            {
                // gameMemPtr = &((*srcexp.gameBuffer.data)[res->dataLoc]);
                // gameMemSize = res->compressedDataLen;
                // auto begin = srcexp.gameBuffer.data->begin() + res->mainData.location;
                // auto end = begin + res->mainData.fileLen;
                // gameMem = vector<uint8_t>(begin, end);
                gameMem = *res->mainData.rawStream(srcexp.gameBuffer.data).data;
            }
            catch (std::exception e)
            {
                errtxt = "Error: ";
                errtxt += e.what();
                cout << errtxt << endl;
                flush(cout);
            }
        }
        if(ImGui::Button("View Decompressed Main Data"))
        {
            try
            {
                // gameMemPtr = &((*srcexp.gameBuffer.data)[res->dataLoc]);
                // gameMemSize = res->compressedDataLen;
                // auto begin = srcexp.gameBuffer.data->begin() + res->mainData.location;
                // auto end = begin + res->mainData.fileLen;
                // gameMem = readCompressed(vector<uint8_t>(begin, end), res->mainData.dataLen);
                gameMem = *res->mainData.decompressedStream(srcexp.gameBuffer.data).data;
            }
            catch (std::exception e)
            {
                errtxt = "Error: ";
                errtxt += e.what();
                cout << errtxt << endl;
                flush(cout);
            }
        }
        if(viewImage != nullptr) if(ImGui::Button("View As Image"))
        {
            try
            {
                // gameMemPtr = &((*srcexp.gameBuffer.data)[res->dataLoc]);
                // gameMemSize = res->compressedDataLen;
                // auto begin = srcexp.gameBuffer.data->begin() + res->mainData.location;
                // auto end = begin + res->mainData.fileLen;
                // vector<uint8_t> img = readCompressed(vector<uint8_t>(begin, end), res->mainData.dataLen);
                // MemoryStream ms(&img);
                viewImage->generateImage(res->mainData.read(srcexp.gameBuffer.data));
            }
            catch (std::exception e)
            {
                errtxt = "Error: ";
                errtxt += e.what();
                cout << errtxt << endl;
                flush(cout);
            }
        }
        ImGui::Text(errtxt.c_str());
        ImGui::Separator();
        if (res->chunks.size() > 0) for (auto it = res->chunks.begin(); it != res->chunks.end(); it++)
        {
            string str = "";
            switch(res->ID) {
                case CHUNK_IMAGEBANK: str = "Image"; break;
                case CHUNK_FONTBANK: str = "Font"; break;
                case CHUNK_SOUNDBANK: str = "Sound"; break;
                case CHUNK_MUSICBANK: str = "Music/MIDI"; break;
                default: break;
            }
            if (str == "") switch(it->ID) {
                case CHUNK_VITAPREV: str = "Vitalise Preview"; break;

                case CHUNK_HEADER: str = "Header"; break;
                case CHUNK_TITLE: str = "Title"; break;
                case CHUNK_AUTHOR: str = "Author"; break;
                case CHUNK_MENU: str = "Menu"; break;
                case CHUNK_EXTPATH: str = "Extra Path"; break;

                case CHUNK_EXTENS: str = "Extensions (deprecated)"; break;
                case CHUNK_OBJECTBANK: str = "Object Bank"; break;
                case CHUNK_OBJECTBANK2: str = "Object Bank 2"; break;

                case CHUNK_GLOBALEVENTS: str = "Global Events"; break;
                case CHUNK_FRAMEHANDLES: str = "Frame Handles"; break;
                case CHUNK_EXTDATA: str = "Extra Data"; break;

                case CHUNK_ADDEXTNS: str = "Additional Extensions (deprecated)"; break;
                case CHUNK_PROJPATH: str = "Project Path"; break;
                case CHUNK_OUTPATH: str = "Output Path"; break;
                case CHUNK_APPDOC: str = "App Doc"; break;
                case CHUNK_OTHEREXT: str = "Other Extension(s)"; break;
                case CHUNK_GLOBALVALS: str = "Global Values"; break;
                case CHUNK_GLOBALSTRS: str = "Global Strings"; break;
                case CHUNK_EXTNLIST: str = "Extensions List"; break;
                case CHUNK_ICON: str = "Icon"; break;

                case CHUNK_DEMOVER: str = "DEMOVER"; break;
                case CHUNK_SECNUM: str = "Security Number"; break;
                case CHUNK_BINFILES: str = "Binary Files"; break;

                case CHUNK_MENUIMAGES: str = "Menu Images"; break;
                case CHUNK_ABOUT: str = "About"; break;
                case CHUNK_COPYRIGHT: str = "Copyright"; break;

                case CHUNK_GLOBALVALNAMES: str = "Global Value Names"; break;
                case CHUNK_GLOBALSTRNAMES: str = "Global String Names"; break;

                case CHUNK_MOVEMNTEXTNS: str = "Movement Extensions"; break;
                //case CHUNK_UNKNOWN8: str = "UNKNOWN8"; break;
                case CHUNK_EXEONLY: str = "EXE Only"; break;

                case CHUNK_PROTECTION: str = "Protection"; break;
                case CHUNK_SHADERS: str = "Shaders"; break;

                case CHUNK_EXTDHEADER: str = "Extended Header"; break;
                case CHUNK_SPACER: str = "Spacer"; break;

                case CHUNK_FRAMEBANK: str = "Frame Bank"; break;
                case CHUNK_224F: str = "CHUNK_224F"; break;
                case CHUNK_TITLE2: str = "Title2"; break;

                case CHUNK_FRAME: str = "Frame"; break;
                case CHUNK_FRAMEHEADER: str = "Frame - Header"; break;
                case CHUNK_FRAMENAME: str = "Frame - Name"; break;
                case CHUNK_FRAMEPASSWORD: str = "Frame - Password"; break;
                case CHUNK_FRAMEPALETTE: str = "Frame - Palette"; break;
           
                case CHUNK_OBJINST: str = "Frame - Object Instances"; break;
                case CHUNK_FRAMEFADEIF: str = "Frame - Fade In Frame"; break;
                case CHUNK_FRAMEFADEOF: str = "Frame - Fade Out Frame"; break;
                case CHUNK_FRAMEFADEI: str = "Frame - Fade In"; break;
                case CHUNK_FRAMEFADEO: str = "Frame - Fade Out"; break;
                case CHUNK_FRAMEEVENTS: str = "Frame - Events"; break;
                case CHUNK_FRAMEPLYHEAD: str = "Frame - Play Header"; break;
                case CHUNK_FRAMEADDITEMINST: str = "Frame - Add Instance"; break;
                case CHUNK_FRAMELAYERS: str = "Frame - Layers"; break;
                case CHUNK_FRAMEVIRTSIZE: str = "Frame - Virtical Size"; break;
                case CHUNK_DEMOFILEPATH: str = "Demo File Path"; break;

                case CHUNK_RANDOMSEED: str = "Random Seed"; break;
                case CHUNK_FRAMELAYEREFFECT: str = "Frame - Layer Effect"; break;
                case CHUNK_FRAMEBLURAY: str = "Frame - BluRay Options"; break;
                case CHUNK_MOVETIMEBASE: str = "Frame - Movement Timer Base"; break;

                case CHUNK_MOSAICIMGTABLE: str = "Mosaic Image Table"; break;
                case CHUNK_FRAMEEFFECTS: str = "Frame - Effects"; break;

                case CHUNK_FRAMEIPHONEOPTS: str = "Frame - iPhone Options"; break;

                case CHUNK_PAERROR: str = "PAE ERROR"; break;

                case CHUNK_OBJHEAD: str = "Object - Header"; break;
                case CHUNK_OBJNAME: str = "Object - Name"; break;
                case CHUNK_OBJPROP: str = "Object - Properties"; break;
                case CHUNK_OBJUNKN: str = "Object - Unknown"; break;
                case CHUNK_OBJEFCT: str = "Object - Effect"; break;

                case CHUNK_IMAGEBANK: str = "Image Bank"; break;
                case CHUNK_SOUNDBANK: str = "Sound Bank"; break;
                case CHUNK_MUSICBANK: str = "Music Bank"; break;
                case CHUNK_FONTBANK: str = "Font Bank"; break;
                default: break;
            }
            genTree(&(*it), str.c_str(), viewImage);
        }
        ImGui::TreePop();
    }
}

void loop()
{
    bool mainOpen = true;
    static bool prevOpen = true;
    static bool openDiag = false;
    static bool dumpImages = false;
    static bool openCredits = false;
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
                ImGui::EndMenu();
            }
            ImGui::MenuItem("Credits", NULL, &openCredits);
            ImGui::Checkbox("Print to debug console? (May cause SE to run slower)", &debugConsole);
            ImGui::EndMenuBar();
        }

        ImGui::Spacing();
        if(ImGui::Button("Refresh from memory"))
        {
            srcexp.gameBuffer.position = 0;
            srcexp.readEntries();
        }

        ImGui::Text("Game: ");
        for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
            if (it->ID == CHUNK_TITLE) {
                ImGui::SameLine();
                string str = "";
                if (srcexp.unicode && it->mainData.dataLen > 0) 
                    str = readUnicode(it->mainData.read(srcexp.gameBuffer.data));
                else if (it->mainData.dataLen > 0) 
                    str = readASCII(it->mainData.read(srcexp.gameBuffer.data));
                else if (srcexp.unicode) 
                    str = readUnicode(it->preData.read(srcexp.gameBuffer.data));
                else 
                    str = readASCII(it->preData.read(srcexp.gameBuffer.data));
                ImGui::Text(str.c_str());
                ImGui::Spacing();
                break;
            }
        }
        for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
            if (it->ID == CHUNK_AUTHOR) {
                ImGui::Text("Author:"); ImGui::SameLine();
                string str = "";
                if (srcexp.unicode && it->mainData.dataLen > 0) 
                    str = readUnicode(it->mainData.read(srcexp.gameBuffer.data));
                else if (it->mainData.dataLen > 0) 
                    str = readASCII(it->mainData.read(srcexp.gameBuffer.data));
                else if (srcexp.unicode) 
                    str = readUnicode(it->preData.read(srcexp.gameBuffer.data));
                else 
                    str = readASCII(it->preData.read(srcexp.gameBuffer.data));
                ImGui::Text(str.c_str());
                ImGui::Spacing();
                break;
            }
        }
        for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
            if (it->ID == CHUNK_COPYRIGHT) {
                ImGui::Text("Copyright:"); ImGui::SameLine();
                string str = "";
                if (srcexp.unicode && it->mainData.dataLen > 0) 
                    str = readUnicode(it->mainData.read(srcexp.gameBuffer.data));
                else if (it->mainData.dataLen > 0) 
                    str = readASCII(it->mainData.read(srcexp.gameBuffer.data));
                else if (srcexp.unicode) 
                    str = readUnicode(it->preData.read(srcexp.gameBuffer.data));
                else 
                    str = readASCII(it->preData.read(srcexp.gameBuffer.data));
                ImGui::Text(str.c_str());
                ImGui::Spacing();
                break;
            }
        }
        for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
            if (it->ID == CHUNK_ABOUT) {
                ImGui::Text("About:"); ImGui::SameLine();
                string str = "";
                if (srcexp.unicode && it->mainData.dataLen > 0) 
                    str = readUnicode(it->mainData.read(srcexp.gameBuffer.data));
                else if (it->mainData.dataLen > 0) 
                    str = readASCII(it->mainData.read(srcexp.gameBuffer.data));
                else if (srcexp.unicode) 
                    str = readUnicode(it->preData.read(srcexp.gameBuffer.data));
                else 
                    str = readASCII(it->preData.read(srcexp.gameBuffer.data));
                ImGui::Text(str.c_str());
                ImGui::Spacing();
                break;
            }
        }
        if(ImGui::TreeNode("Frames:")) {
            for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                if (it->ID == CHUNK_FRAME) {
                    string framename = "";
                    for (auto it2 = it->chunks.begin(); it2 != it->chunks.end(); it2++) {
                        if (it2->ID == CHUNK_FRAMENAME) {
                            if (srcexp.unicode && it2->mainData.dataLen > 0) 
                                framename = readUnicode(it2->mainData.read(srcexp.gameBuffer.data));
                            else if (it2->mainData.dataLen > 0) 
                                framename = readASCII(it2->mainData.read(srcexp.gameBuffer.data));
                            else if (srcexp.unicode) 
                                framename = readUnicode(it2->preData.read(srcexp.gameBuffer.data));
                            else 
                                framename = readASCII(it2->preData.read(srcexp.gameBuffer.data));
                            break;
                        }
                    }
                    char cstr[100];
                    sprintf(cstr, "%s##%I64x", framename.c_str(), it->location);
                    if(ImGui::TreeNode(cstr)) {
                        string framepass = "";
                        for (auto it2 = it->chunks.begin(); it2 != it->chunks.end(); it2++) {
                            if (it2->ID == CHUNK_FRAMEPASSWORD) {
                                if (srcexp.unicode && it2->mainData.dataLen > 0) 
                                    framepass = readUnicode(it2->mainData.read(srcexp.gameBuffer.data));
                                else if (it2->mainData.dataLen > 0) 
                                    framepass = readASCII(it2->mainData.read(srcexp.gameBuffer.data));
                                else if (srcexp.unicode) 
                                    framepass = readUnicode(it2->preData.read(srcexp.gameBuffer.data));
                                else 
                                    framepass = readASCII(it2->preData.read(srcexp.gameBuffer.data));
                                break;
                            }
                        }
                        ImGui::Text("Password: "); ImGui::SameLine();
                        ImGui::Text(framepass.c_str());
                        genTree(&*it);
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("Objects:")) {
            ResourceEntry* objBank = nullptr;
            for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                if (it->ID == CHUNK_OBJECTBANK || it->ID == CHUNK_OBJECTBANK2) {
                    objBank = &*it;
                    break;
                }
            }
            if (objBank == nullptr) objBank = &(srcexp.game);
            for (auto it = objBank->chunks.begin(); it != objBank->chunks.end(); it++) {
                if (it->ID == CHUNK_OBJHEAD) {
                    string objname = "";
                    for (auto it3 = it->chunks.begin(); it3 != it->chunks.end(); it3++) {
                        if (it3->ID == CHUNK_OBJNAME) {
                            if (srcexp.unicode && it3->mainData.dataLen > 0) 
                                objname = readUnicode(it3->mainData.read(srcexp.gameBuffer.data));
                            else if (it3->mainData.dataLen > 0) 
                                objname = readASCII(it3->mainData.read(srcexp.gameBuffer.data));
                            else if (srcexp.unicode) 
                                objname = readUnicode(it3->preData.read(srcexp.gameBuffer.data));
                            else 
                                objname = readASCII(it3->preData.read(srcexp.gameBuffer.data));
                            break;
                        }
                    }
                    char cstr[100];
                    sprintf(cstr, "%s##%I64x", objname.c_str(), it->location);
                    if(ImGui::TreeNode(cstr)) {
                        genTree(&*it);
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::TreePop();
        }
        if(ImGui::TreeNode("Images:")) {
            for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                if (it->ID == CHUNK_IMAGEBANK) {
                    for (auto it2 = it->chunks.begin(); it2 != it->chunks.end(); it2++) {
                        char cstr[100];
                        sprintf(cstr, "Image 0x%x##%I64x", it2->ID, it2->location);
                        if(ImGui::TreeNode(cstr)) 
                        {
                            if(ImGui::Button("View As Image"))
                            {
                                try
                                {
                                    viewImage.generateImage(it2->mainData.read(srcexp.gameBuffer.data));
                                }
                                catch (std::exception e)
                                {
                                    string errtxt = "Error: ";
                                    errtxt += e.what();
                                    cout << errtxt << endl;
                                    flush(cout);
                                }
                            }
                            ImGui::TreePop();
                        }
                    }
                    break;
                }
            }
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::Text("Raw: ");

        genTree(&(srcexp.game), "Game", &viewImage);
        // if (((GameEntry*)srcexp.game.extraData) != nullptr)
        // {
        //     for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++)
        //     {
        //         if (it->ID == CHUNK_IMAGEBANK) genTree(&*it, "Image Bank", &viewImage);
        //         else if (it->ID == CHUNK_FONTBANK) genTree(&*it, "Font Bank");
        //         else if (it->ID == CHUNK_SOUNDBANK) genTree(&*it, "Sound Bank");
        //         else if (it->ID == CHUNK_MUSICBANK) genTree(&*it, "Music Bank");
        //     }
        // 	// genTree(&(((GameEntry*)srcexp.game.extraData)->imageBank), &viewImage);
        // 	// genTree(&(((GameEntry*)srcexp.game.extraData)->musicBank));
        // 	// genTree(&(((GameEntry*)srcexp.game.extraData)->soundBank));
        // 	// genTree(&(((GameEntry*)srcexp.game.extraData)->fontBank));
        // 	// genTree(&(((GameEntry*)srcexp.game.extraData)->objectBank));
        // }

        // if(ImGui::TreeNode("Game"))
        // {
        //     if(ImGui::Button("View Game Memory"))
        //     {
        //         gameMemPtr = &((*srcexp.gameBuffer.data)[0]);
        //         gameMemSize = srcexp.gameBuffer.data->size();
        //     }
        //     if(ImGui::Button("View Chunks Array Memory") && srcexp.game.chunks.size() > 0)
        //     {
        //         gameMemPtr = (uint8_t*)&(srcexp.game.chunks[0]);
        //         gameMemSize = srcexp.game.chunks.size()*(sizeof(ResourceEntry*)/sizeof(uint8_t));
        //     }
        //     static string errtxt = "";
        //     try
        //     {
        //     }
        //     catch (std::exception e)
        //     {
        //         errtxt = "Error: ";
        //         errtxt += e.what();
        //         cout << errtxt << endl;
        //         flush(cout);
        //     }
        //     ImGui::Text(errtxt.c_str());
        //     ImGui::TreePop();
        // }

        if (openCredits)
        {
            ImGui::OpenPopup("Credits");
            openCredits = false;
        }
        if (ImGui::BeginPopupModal("Credits", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            glakCredits();
            if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

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
                try
                {
                    // gameMemPtr = nullptr;
                    // gameMemSize = 0;
                    errtxt = "";
                    srcexp.loadGame(string(dir));
                    //static ResourceEntry* back = srcexp.game.chunks[0];
                    //srcexp.game.chunks[0] = (ResourceEntry*)0xDEAD; //completely bullshit pointer to fuck with whatever the fuck is fucking with this object
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
                try
                {
                    size_t dumpCount = 0;
                    errtxt = "";
                    for(auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++)
                    {
                        if(it->ID == CHUNK_IMAGEBANK)
                        {
                            for(auto img = it->chunks.begin(); img != it->chunks.end(); img++)
                            {
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
                                char imgdir[110];
                                sprintf(imgdir, "%s%x.png", dir, img->ID);
                                Image image;
                                int err = image.generateImage(img->mainData.read(srcexp.gameBuffer.data));
                                if (err == 0) throw std::exception("Error exporting images, make sure the directory exists!");
                                stbi_write_png(imgdir, image.bitmap.w, image.bitmap.h, 4, &(image.bitmap.toRGBA()[0]), image.bitmap.w*4);
                                dumpCount++;
                            }
                        }
                    }
                    cout << "Images dumped: " << std::dec << dumpCount << endl;
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
        ImGui::Image((ImTextureID)(intptr_t)viewImage.tex, ImVec2(viewImage.width, viewImage.height));
        ImGui::End();
    }

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
    memEdit.DrawWindow("Game Memory", (gameMem.size() > 0 ? &(gameMem[0]) : nullptr), gameMem.size(), 0x0);
}

void draw()
{
}

void init()
{
    glEnable(GL_DEPTH_TEST);
}

void destroy()
{
}

int main(int argc, char** argv)
{
    // Setup SDL
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup Window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    window = SDL_CreateWindow("ImGui SDL2 + OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);

    // Initialise gl3w
    gl3wInit();

    // Initialise ImGui
    ImGui::CreateContext();
    io = &ImGui::GetIO();
    ImGui_ImplSdlGL3_Init(window);
    ImGui::StyleColorsDark();
    style = &ImGui::GetStyle();
    style->WindowRounding = 0;

    init();

    bool done = false;
    while(!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
        }
        ImGui_ImplSdlGL3_NewFrame(window);
        
        loop();

        glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
        glClearColor(clearCol[0], clearCol[1], clearCol[2], clearCol[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw();

        ImGui::Render();
        ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
        
        SDL_GL_SwapWindow(window);
    }

    destroy();
    
    ImGui_ImplSdlGL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}