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

// void genTree(ResourceEntry* res, const char* name = "", Image* viewImage = nullptr)
// {
// 	if (res == nullptr) return;
//     static string errtxt = "";
//     char str[100];
//     sprintf(str, "%s 0x%I32x##%I64x", name, res->ID, (uint64_t)&(*res));
//     if(ImGui::TreeNode(str))
//     {
//         sprintf(str, "ID: 0x%I32x", res->ID);
//         ImGui::Text(str);
//         sprintf(str, "Mode: 0x%I32x", res->mode);
//         ImGui::Text(str);
//         sprintf(str, "Location: 0x%zx", res->location);
//         ImGui::Text(str);
//         // sprintf(str, "Pre Data Size: 0x%I64x", res->preData.fileLen);
//         // ImGui::Text(str);
//         sprintf(str, "Data Location: 0x%I64x", res->data.location);
//         ImGui::Text(str);
//         sprintf(str, "Data Length: 0x%I64x", res->data.fileLen);
//         ImGui::Text(str);
//         if(ImGui::Button("View Pre Data"))
//         {
//             try
//             {
//                 // gameMemPtr = &(res->preData[0]);
//                 // gameMemSize = res->preData.size();
//                 gameMem = *res->data.read(srcexp.gameBuffer.data).data;
//             }
//             catch (std::exception e)
//             {
//                 errtxt = "Error: ";
//                 errtxt += e.what();
//                 cout << errtxt << endl;
//                 flush(cout);
//             }
//         }
//         // if(ImGui::Button("View Raw Main Data"))
//         // {
//         //     try
//         //     {
//         //         // gameMemPtr = &((*srcexp.gameBuffer.data)[res->dataLoc]);
//         //         // gameMemSize = res->compressedDataLen;
//         //         // auto begin = srcexp.gameBuffer.data->begin() + res->mainData.location;
//         //         // auto end = begin + res->mainData.fileLen;
//         //         // gameMem = vector<uint8_t>(begin, end);
//         //         gameMem = *res->mainData.rawStream(srcexp.gameBuffer.data).data;
//         //     }
//         //     catch (std::exception e)
//         //     {
//         //         errtxt = "Error: ";
//         //         errtxt += e.what();
//         //         cout << errtxt << endl;
//         //         flush(cout);
//         //     }
//         // }
//         // if(ImGui::Button("View Decompressed Main Data"))
//         // {
//         //     try
//         //     {
//         //         // gameMemPtr = &((*srcexp.gameBuffer.data)[res->dataLoc]);
//         //         // gameMemSize = res->compressedDataLen;
//         //         // auto begin = srcexp.gameBuffer.data->begin() + res->mainData.location;
//         //         // auto end = begin + res->mainData.fileLen;
//         //         // gameMem = readCompressed(vector<uint8_t>(begin, end), res->mainData.dataLen);
//         //         gameMem = *res->mainData.decompressedStream(srcexp.gameBuffer.data).data;
//         //     }
//         //     catch (std::exception e)
//         //     {
//         //         errtxt = "Error: ";
//         //         errtxt += e.what();
//         //         cout << errtxt << endl;
//         //         flush(cout);
//         //     }
//         // }
//         // if(viewImage != nullptr) if(ImGui::Button("View As Image"))
//         // {
//         //     try
//         //     {
//         //         // gameMemPtr = &((*srcexp.gameBuffer.data)[res->dataLoc]);
//         //         // gameMemSize = res->compressedDataLen;
//         //         // auto begin = srcexp.gameBuffer.data->begin() + res->mainData.location;
//         //         // auto end = begin + res->mainData.fileLen;
//         //         // vector<uint8_t> img = readCompressed(vector<uint8_t>(begin, end), res->mainData.dataLen);
//         //         // MemoryStream ms(&img);
//         //         viewImage->generateImage(res->mainData.read(srcexp.gameBuffer.data));
//         //     }
//         //     catch (std::exception e)
//         //     {
//         //         errtxt = "Error: ";
//         //         errtxt += e.what();
//         //         cout << errtxt << endl;
//         //         flush(cout);
//         //     }
//         // }
//         ImGui::Text(errtxt.c_str());
//         ImGui::Separator();
//         // if (res->chunks.size() > 0) for (auto it = res->chunks.begin(); it != res->chunks.end(); it++)
//         // {
//         //     string str = "";
//         //     switch(res->ID) {
//         //         case CHUNK_IMAGEBANK: str = "Image"; break;
//         //         case CHUNK_FONTBANK: str = "Font"; break;
//         //         case CHUNK_SOUNDBANK: str = "Sound"; break;
//         //         case CHUNK_MUSICBANK: str = "Music/MIDI"; break;
//         //         default: break;
//         //     }
//         //     if (str == "") switch(it->ID) {
//         //         case CHUNK_VITAPREV: str = "Vitalise Preview"; break;

//         //         case CHUNK_HEADER: str = "Header"; break;
//         //         case CHUNK_TITLE: str = "Title"; break;
//         //         case CHUNK_AUTHOR: str = "Author"; break;
//         //         case CHUNK_MENU: str = "Menu"; break;
//         //         case CHUNK_EXTPATH: str = "Extra Path"; break;

//         //         case CHUNK_EXTENS: str = "Extensions (deprecated)"; break;
//         //         case CHUNK_OBJECTBANK: str = "Object Bank"; break;
//         //         case CHUNK_OBJECTBANK2: str = "Object Bank 2"; break;

//         //         case CHUNK_GLOBALEVENTS: str = "Global Events"; break;
//         //         case CHUNK_FRAMEHANDLES: str = "Frame Handles"; break;
//         //         case CHUNK_EXTDATA: str = "Extra Data"; break;

//         //         case CHUNK_ADDEXTNS: str = "Additional Extensions (deprecated)"; break;
//         //         case CHUNK_PROJPATH: str = "Project Path"; break;
//         //         case CHUNK_OUTPATH: str = "Output Path"; break;
//         //         case CHUNK_APPDOC: str = "App Doc"; break;
//         //         case CHUNK_OTHEREXT: str = "Other Extension(s)"; break;
//         //         case CHUNK_GLOBALVALS: str = "Global Values"; break;
//         //         case CHUNK_GLOBALSTRS: str = "Global Strings"; break;
//         //         case CHUNK_EXTNLIST: str = "Extensions List"; break;
//         //         case CHUNK_ICON: str = "Icon"; break;

//         //         case CHUNK_DEMOVER: str = "DEMOVER"; break;
//         //         case CHUNK_SECNUM: str = "Security Number"; break;
//         //         case CHUNK_BINFILES: str = "Binary Files"; break;

//         //         case CHUNK_MENUIMAGES: str = "Menu Images"; break;
//         //         case CHUNK_ABOUT: str = "About"; break;
//         //         case CHUNK_COPYRIGHT: str = "Copyright"; break;

//         //         case CHUNK_GLOBALVALNAMES: str = "Global Value Names"; break;
//         //         case CHUNK_GLOBALSTRNAMES: str = "Global String Names"; break;

//         //         case CHUNK_MOVEMNTEXTNS: str = "Movement Extensions"; break;
//         //         //case CHUNK_UNKNOWN8: str = "UNKNOWN8"; break;
//         //         case CHUNK_EXEONLY: str = "EXE Only"; break;

//         //         case CHUNK_PROTECTION: str = "Protection"; break;
//         //         case CHUNK_SHADERS: str = "Shaders"; break;

//         //         case CHUNK_EXTDHEADER: str = "Extended Header"; break;
//         //         case CHUNK_SPACER: str = "Spacer"; break;

//         //         case CHUNK_FRAMEBANK: str = "Frame Bank"; break;
//         //         case CHUNK_224F: str = "CHUNK_224F"; break;
//         //         case CHUNK_TITLE2: str = "Title2"; break;

//         //         case CHUNK_FRAME: str = "Frame"; break;
//         //         case CHUNK_FRAMEHEADER: str = "Frame - Header"; break;
//         //         case CHUNK_FRAMENAME: str = "Frame - Name"; break;
//         //         case CHUNK_FRAMEPASSWORD: str = "Frame - Password"; break;
//         //         case CHUNK_FRAMEPALETTE: str = "Frame - Palette"; break;
           
//         //         case CHUNK_OBJINST: str = "Frame - Object Instances"; break;
//         //         case CHUNK_FRAMEFADEIF: str = "Frame - Fade In Frame"; break;
//         //         case CHUNK_FRAMEFADEOF: str = "Frame - Fade Out Frame"; break;
//         //         case CHUNK_FRAMEFADEI: str = "Frame - Fade In"; break;
//         //         case CHUNK_FRAMEFADEO: str = "Frame - Fade Out"; break;
//         //         case CHUNK_FRAMEEVENTS: str = "Frame - Events"; break;
//         //         case CHUNK_FRAMEPLYHEAD: str = "Frame - Play Header"; break;
//         //         case CHUNK_FRAMEADDITEMINST: str = "Frame - Add Instance"; break;
//         //         case CHUNK_FRAMELAYERS: str = "Frame - Layers"; break;
//         //         case CHUNK_FRAMEVIRTSIZE: str = "Frame - Virtical Size"; break;
//         //         case CHUNK_DEMOFILEPATH: str = "Demo File Path"; break;

//         //         case CHUNK_RANDOMSEED: str = "Random Seed"; break;
//         //         case CHUNK_FRAMELAYEREFFECT: str = "Frame - Layer Effect"; break;
//         //         case CHUNK_FRAMEBLURAY: str = "Frame - BluRay Options"; break;
//         //         case CHUNK_MOVETIMEBASE: str = "Frame - Movement Timer Base"; break;

//         //         case CHUNK_MOSAICIMGTABLE: str = "Mosaic Image Table"; break;
//         //         case CHUNK_FRAMEEFFECTS: str = "Frame - Effects"; break;

//         //         case CHUNK_FRAMEIPHONEOPTS: str = "Frame - iPhone Options"; break;

//         //         case CHUNK_PAERROR: str = "PAE ERROR"; break;

//         //         case CHUNK_OBJHEAD: str = "Object - Header"; break;
//         //         case CHUNK_OBJNAME: str = "Object - Name"; break;
//         //         case CHUNK_OBJPROP: str = "Object - Properties"; break;
//         //         case CHUNK_OBJUNKN: str = "Object - Unknown"; break;
//         //         case CHUNK_OBJEFCT: str = "Object - Effect"; break;

//         //         case CHUNK_IMAGEBANK: str = "Image Bank"; break;
//         //         case CHUNK_SOUNDBANK: str = "Sound Bank"; break;
//         //         case CHUNK_MUSICBANK: str = "Music Bank"; break;
//         //         case CHUNK_FONTBANK: str = "Font Bank"; break;
//         //         default: break;
//         //     }
//         //     genTree(&(*it), str.c_str(), viewImage);
//         // }
//         ImGui::TreePop();
//     }
// }

int main(int, char**)
{
    // tinf_init();
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
        #ifndef DEBUG
        try{
        #endif
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
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

            {
                // ImGui::Text("Game: ");
                // for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                //     if (it->ID == CHUNK_TITLE) {
                //         ImGui::SameLine();
                //         string str = "";
                //         if (srcexp.unicode && it->mainData.dataLen > 0) 
                //             str = readUnicode(it->mainData.read(srcexp.gameBuffer.data));
                //         else if (it->mainData.dataLen > 0) 
                //             str = readASCII(it->mainData.read(srcexp.gameBuffer.data));
                //         else if (srcexp.unicode) 
                //             str = readUnicode(it->preData.read(srcexp.gameBuffer.data));
                //         else 
                //             str = readASCII(it->preData.read(srcexp.gameBuffer.data));
                //         ImGui::Text(str.c_str());
                //         ImGui::Spacing();
                //         break;
                //     }
                // }
                // for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                //     if (it->ID == CHUNK_AUTHOR) {
                //         ImGui::Text("Author:"); ImGui::SameLine();
                //         string str = "";
                //         if (srcexp.unicode && it->mainData.dataLen > 0) 
                //             str = readUnicode(it->mainData.read(srcexp.gameBuffer.data));
                //         else if (it->mainData.dataLen > 0) 
                //             str = readASCII(it->mainData.read(srcexp.gameBuffer.data));
                //         else if (srcexp.unicode) 
                //             str = readUnicode(it->preData.read(srcexp.gameBuffer.data));
                //         else 
                //             str = readASCII(it->preData.read(srcexp.gameBuffer.data));
                //         ImGui::Text(str.c_str());
                //         ImGui::Spacing();
                //         break;
                //     }
                // }
                // for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                //     if (it->ID == CHUNK_COPYRIGHT) {
                //         ImGui::Text("Copyright:"); ImGui::SameLine();
                //         string str = "";
                //         if (srcexp.unicode && it->mainData.dataLen > 0) 
                //             str = readUnicode(it->mainData.read(srcexp.gameBuffer.data));
                //         else if (it->mainData.dataLen > 0) 
                //             str = readASCII(it->mainData.read(srcexp.gameBuffer.data));
                //         else if (srcexp.unicode) 
                //             str = readUnicode(it->preData.read(srcexp.gameBuffer.data));
                //         else 
                //             str = readASCII(it->preData.read(srcexp.gameBuffer.data));
                //         ImGui::Text(str.c_str());
                //         ImGui::Spacing();
                //         break;
                //     }
                // }
                // for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                //     if (it->ID == CHUNK_ABOUT) {
                //         ImGui::Text("About:"); ImGui::SameLine();
                //         string str = "";
                //         if (srcexp.unicode && it->mainData.dataLen > 0) 
                //             str = readUnicode(it->mainData.read(srcexp.gameBuffer.data));
                //         else if (it->mainData.dataLen > 0) 
                //             str = readASCII(it->mainData.read(srcexp.gameBuffer.data));
                //         else if (srcexp.unicode) 
                //             str = readUnicode(it->preData.read(srcexp.gameBuffer.data));
                //         else 
                //             str = readASCII(it->preData.read(srcexp.gameBuffer.data));
                //         ImGui::Text(str.c_str());
                //         ImGui::Spacing();
                //         break;
                //     }
                // }
                // if(ImGui::TreeNode("Frames:")) {
                //     for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                //         if (it->ID == CHUNK_FRAME) {
                //             string framename = "";
                //             for (auto it2 = it->chunks.begin(); it2 != it->chunks.end(); it2++) {
                //                 if (it2->ID == CHUNK_FRAMENAME) {
                //                     if (srcexp.unicode && it2->mainData.dataLen > 0) 
                //                         framename = readUnicode(it2->mainData.read(srcexp.gameBuffer.data));
                //                     else if (it2->mainData.dataLen > 0) 
                //                         framename = readASCII(it2->mainData.read(srcexp.gameBuffer.data));
                //                     else if (srcexp.unicode) 
                //                         framename = readUnicode(it2->preData.read(srcexp.gameBuffer.data));
                //                     else 
                //                         framename = readASCII(it2->preData.read(srcexp.gameBuffer.data));
                //                     break;
                //                 }
                //             }
                //             char cstr[100];
                //             sprintf(cstr, "%s##%I64x", framename.c_str(), it->location);
                //             if(ImGui::TreeNode(cstr)) {
                //                 string framepass = "";
                //                 for (auto it2 = it->chunks.begin(); it2 != it->chunks.end(); it2++) {
                //                     if (it2->ID == CHUNK_FRAMEPASSWORD) {
                //                         if (srcexp.unicode && it2->mainData.dataLen > 0) 
                //                             framepass = readUnicode(it2->mainData.read(srcexp.gameBuffer.data));
                //                         else if (it2->mainData.dataLen > 0) 
                //                             framepass = readASCII(it2->mainData.read(srcexp.gameBuffer.data));
                //                         else if (srcexp.unicode) 
                //                             framepass = readUnicode(it2->preData.read(srcexp.gameBuffer.data));
                //                         else 
                //                             framepass = readASCII(it2->preData.read(srcexp.gameBuffer.data));
                //                         break;
                //                     }
                //                 }
                //                 ImGui::Text("Password: "); ImGui::SameLine();
                //                 ImGui::Text(framepass.c_str());
                //                 genTree(&*it);
                //                 ImGui::TreePop();
                //             }
                //         }
                //     }
                //     ImGui::TreePop();
                // }
                // if(ImGui::TreeNode("Objects:")) {
                //     ResourceEntry* objBank = nullptr;
                //     for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                //         if (it->ID == CHUNK_OBJECTBANK || it->ID == CHUNK_OBJECTBANK2) {
                //             objBank = &*it;
                //             break;
                //         }
                //     }
                //     if (objBank == nullptr) objBank = &(srcexp.game);
                //     for (auto it = objBank->chunks.begin(); it != objBank->chunks.end(); it++) {
                //         if (it->ID == CHUNK_OBJHEAD) {
                //             string objname = "";
                //             for (auto it3 = it->chunks.begin(); it3 != it->chunks.end(); it3++) {
                //                 if (it3->ID == CHUNK_OBJNAME) {
                //                     if (srcexp.unicode && it3->mainData.dataLen > 0) 
                //                         objname = readUnicode(it3->mainData.read(srcexp.gameBuffer.data));
                //                     else if (it3->mainData.dataLen > 0) 
                //                         objname = readASCII(it3->mainData.read(srcexp.gameBuffer.data));
                //                     else if (srcexp.unicode) 
                //                         objname = readUnicode(it3->preData.read(srcexp.gameBuffer.data));
                //                     else 
                //                         objname = readASCII(it3->preData.read(srcexp.gameBuffer.data));
                //                     break;
                //                 }
                //             }
                //             char cstr[100];
                //             sprintf(cstr, "%s##%I64x", objname.c_str(), it->location);
                //             if(ImGui::TreeNode(cstr)) {
                //                 genTree(&*it);
                //                 ImGui::TreePop();
                //             }
                //         }
                //     }
                //     ImGui::TreePop();
                // }
                // if(ImGui::TreeNode("Images:")) {
                //     for (auto it = srcexp.game.chunks.begin(); it != srcexp.game.chunks.end(); it++) {
                //         if (it->ID == CHUNK_IMAGEBANK) {
                //             for (auto it2 = it->chunks.begin(); it2 != it->chunks.end(); it2++) {
                //                 char cstr[100];
                //                 sprintf(cstr, "Image 0x%x##%I64x", it2->ID, it2->location);
                //                 if(ImGui::TreeNode(cstr)) 
                //                 {
                //                     if(ImGui::Button("View As Image"))
                //                     {
                //                         try
                //                         {
                //                             viewImage.generateImage(it2->mainData.read(srcexp.gameBuffer.data));
                //                         }
                //                         catch (std::exception e)
                //                         {
                //                             string errtxt = "Error: ";
                //                             errtxt += e.what();
                //                             cout << errtxt << endl;
                //                             flush(cout);
                //                         }
                //                     }
                //                     ImGui::TreePop();
                //                 }
                //             }
                //             break;
                //         }
                //     }
                //     ImGui::TreePop();
                // }
            }

            ImGui::Separator();
            ImGui::Text("Raw: ");

            // genTree(&(srcexp.game), "Game", &viewImage);

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
