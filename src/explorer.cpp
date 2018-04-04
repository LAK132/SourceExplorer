#include "explorer.h"

bool debugConsole = false;
bool throwErrors = false;

void SourceExplorer::getEntries(MemoryStream& strm, vector<uint16_t>& state)
{
    frame.clear();
    image.clear();
    sound.clear();
    music.clear();
    font.clear();
    while (strm.position < strm.data->size())
    {
        DataPoint dp(strm);
        if (state.back() == STATE_NEW && (dp.ID < CHUNK_VITAPREV || dp.ID > CHUNK_LAST))
            throw std::exception("Invalid State/ID");
        if (dp.mode != MODE_0 && dp.mode != MODE_1 && dp.mode != MODE_2 && dp.mode != MODE_3)
            throw std::exception("Invalid Mode");
        switch(dp.ID)
        {
            case CHUNK_HEADER: { 
                size_t pos = strm.position; 
                gameHeader = dp; 
                gameHeader(strm); 
                frame.reserve(gameHeader.numFrames); 
                strm.position = pos; 
            } break;
			case CHUNK_FRAME: { 
                Frame frm; 
                frm = dp; 
                size_t pos = strm.position; 
                frm(strm, unicode); 
                frame.push_back(frm); 
                strm.position = pos; 
            } break;
            case CHUNK_OBJECTBANK2: 
            case CHUNK_OBJECTBANK: {
                size_t pos = strm.position;
                objectBank = dp;
                objectBank(strm);
                strm.position = pos;
            } break;
            case CHUNK_TITLE: title = dp; break;
            case CHUNK_AUTHOR: author = dp; break;
            case CHUNK_COPYRIGHT: copyright = dp; break;
            case CHUNK_IMAGEBANK: imageBank = dp; break;
            case CHUNK_SOUNDBANK: soundBank = dp; break;
            case CHUNK_MUSICBANK: musicBank = dp; break;
            case CHUNK_FONTBANK: fontBank = dp; break;
            default: /*strm.position += strmSize;*/ continue;
        }

        DEBUG 
        {
            string str = "";
            for (size_t i = 0; i < state.size() - 2; i++)
                str += "|";
            cout << str << endl << str <<
            "-New Resource"  << endl << str <<
            " |-State: 0x"     << std::hex << state.back() << endl << str <<
            " |-Pos: 0x:"      << std::hex << dp.location << endl << str << 
            " |-ID: 0x"        << std::hex << dp.ID << endl << str <<
            " |-Mode: 0x"      << std::hex << dp.mode << endl << str <<
            " --Size in file: 0x" << std::hex << dp.fileLen << endl;
        }
    }

    if(title.location != -1)
    {
        title.getData(strm);//, 0, (title.mode & MODE_FLAG_COMPRESSED) != 0);
        DEBUG cout << std::hex << title.dataLocation << endl;
        DEBUG cout << (title.compressed ? "Compressed" : "Uncompressed") << endl;
        titlestr = (unicode ? readUnicode(title.read(strm.data)) : readASCII(title.read(strm.data)));
        DEBUG cout << "Title: " << titlestr << endl;
    }
    else titlestr = "";

    if(author.location != -1)
    {
        author.getData(strm);//, 0, (author.mode & MODE_FLAG_COMPRESSED) != 0);
        DEBUG cout << std::hex << author.dataLocation << endl;
        DEBUG cout << (author.compressed ? "Compressed" : "Uncompressed") << endl;
        authorstr = (unicode ? readUnicode(author.read(strm.data)) : readASCII(author.read(strm.data)));
        DEBUG cout << "Author: " << authorstr << endl;
    }
    else authorstr = "";

    if(copyright.location != -1)
    {
        copyright.getData(strm);//, 0, (copyright.mode & MODE_FLAG_COMPRESSED) != 0);
        DEBUG cout << std::hex << copyright.dataLocation << endl;
        DEBUG cout << (copyright.compressed ? "Compressed" : "Uncompressed") << endl;
        copyrightstr = (unicode ? readUnicode(copyright.read(strm.data)) : readASCII(copyright.read(strm.data)));
        DEBUG cout << "Copyright: " << copyrightstr << endl;
    }
    else copyrightstr = "";

    if(imageBank.location != -1)
    {
        strm.position = imageBank.location;
        image.resize(strm.readInt<uint32_t>());
        for(auto img = image.begin(); img != image.end() && strm.position < strm.data->size(); img++)
        {
            img->ID = strm.readInt<uint16_t>();
            img->mode = strm.readInt<uint16_t>();
            uint32_t inflen = strm.readInt<uint32_t>();
            uint32_t deflen = strm.readInt<uint32_t>();
            (*img)(strm, deflen, inflen);
        }
    }

    if(soundBank.location != -1)
    {
        strm.position = soundBank.location;
        sound.resize(strm.readInt<uint32_t>());
        for(auto snd = sound.begin(); snd != sound.end() && strm.position < strm.data->size(); snd++)
        {
            snd->ID = strm.readInt<uint16_t>();
            snd->mode = strm.readInt<uint16_t>();
            strm.position += 0x8;
            uint32_t inflen = strm.readInt<uint32_t>();
            strm.position += 0xC;
            uint32_t deflen = strm.readInt<uint32_t>();
            (*snd)(strm, deflen, inflen);
            // (*snd)(strm, 0);
            // snd->getData(strm, 0x18, false);

        }
    }

    if(musicBank.location != -1)
    {
        strm.position = musicBank.location;
        music.resize(strm.readInt<uint32_t>());
        for(auto msc = music.begin(); msc != music.end() && strm.position < strm.data->size(); msc++)
        {
            msc->ID = strm.readInt<uint16_t>();
            msc->mode = strm.readInt<uint16_t>();
            uint32_t inflen = strm.readInt<uint32_t>();
            uint32_t deflen = strm.readInt<uint32_t>();
            (*msc)(strm, deflen, inflen);
        }
    }

    if(fontBank.location != -1)
    {
        strm.position = fontBank.location;
        font.resize(strm.readInt<uint32_t>());
        for(auto fnt = font.begin(); fnt != font.end() && strm.position < strm.data->size(); fnt++)
        {
            fnt->ID = strm.readInt<uint16_t>();
            fnt->mode = strm.readInt<uint16_t>();
            uint32_t inflen = strm.readInt<uint32_t>();
            uint32_t deflen = strm.readInt<uint32_t>();
            (*fnt)(strm, deflen, inflen);
        }
    }
};

void SourceExplorer::draw(renderMenu_t rm)
{
    ImGui::Text("Game: "); ImGui::SameLine(); ImGui::Text(titlestr.c_str());
    ImGui::Text("Author: "); ImGui::SameLine(); ImGui::Text(authorstr.c_str());
    ImGui::Text("Copyright: "); ImGui::SameLine(); ImGui::Text(copyrightstr.c_str());
    // if(ImGui::TreeNode("Frame Bank"))
    // {
    //     for(auto frm = frame.begin(); frm != frame.end(); frm++)
    //     {
    //         char str[100];
	// 		sprintf(str, "%s##%zx", frm->namestr.c_str(), frm->location);
    //         if(ImGui::TreeNode(str))
    //         {
    //             if(ImGui::Button("View Raw Data"))
    //             {
    //                 *rm.memedit = *frm->read(gameBuffer.data).data;
    //             }
    //             for(auto obj = frm->instances.items.begin(); obj != frm->instances.items.end(); obj++)
    //             {
    //                 ObjectHeader* objh = nullptr;
    //                 for(auto it = objectBank.items.begin(); it != objectBank.items.end(); it++)
    //                 {
    //                     if(it->handle == obj->handle)
    //                     {
    //                         objh = &*it;
    //                         break;
    //                     }
    //                 }
    //                 if(objh == nullptr)
    //                 {
    //                     sprintf(str, "Object 0x%x", obj->handle);
    //                     ImGui::Text(str);
    //                 }
    //                 else
    //                 {
    //                     sprintf(str, "Object 0x%x", obj->handle);
    //                     if(ImGui::TreeNode(str))
    //                     {
    //                         sprintf(str, "Parent Handle: 0x%x", obj->parentHandle);
    //                         ImGui::Text(str);
    //                         sprintf(str, "Location: 0x%zx", objh->location);
    //                         ImGui::Text(str);
    //                         ImGui::TreePop();
    //                     }
    //                 }
    //             }
    //             ImGui::TreePop();
    //         }
    //     }
    //     ImGui::TreePop();
    // }
    // if(ImGui::TreeNode("Object Bank"))
    // {
    //     for(auto obj = objectBank.items.begin(); obj != objectBank.items.end(); obj++)
    //     {
    //         char str[100];
    //         sprintf(str, "Object 0x%x##%zx", obj->handle, obj->location);
    //         if(ImGui::TreeNode(str))
    //         {
    //             ImGui::TreePop();
    //         }
    //     }
    //     ImGui::TreePop();
    // }
    if(ImGui::TreeNode("Image Bank"))
    {
        char loc[100];
        sprintf(loc, "Location: 0x%zx", imageBank.location);
        ImGui::Text(loc);
        for(auto img = image.begin(); img != image.end(); img++)
        {
            char str[100];
            sprintf(str, "View Image 0x%x", img->ID);
            if(ImGui::Button(str))
            {
                rm.viewImage->generateImage(img->read(gameBuffer.data));
            }
        }
        ImGui::TreePop();
    }
    if(ImGui::TreeNode("Sound Bank"))
    {
        char loc[100];
        sprintf(loc, "Location: 0x%zx", soundBank.location);
        ImGui::Text(loc);
        for(auto snd = sound.begin(); snd != sound.end(); snd++)
        {
            char str[100];
            sprintf(str, "View Sound 0x%x", snd->ID);
            if(ImGui::Button(str))
            {
                *rm.memedit = *snd->read(gameBuffer.data).data;
            }
        }
        ImGui::TreePop();
    }
    if(ImGui::TreeNode("Music Bank"))
    {
        char loc[100];
        sprintf(loc, "Location: 0x%zx", musicBank.location);
        ImGui::Text(loc);
        for(auto msc = music.begin(); msc != music.end(); msc++)
        {
            char str[100];
            sprintf(str, "View Music 0x%x", msc->ID);
            if(ImGui::Button(str))
            {
                *rm.memedit = *msc->read(gameBuffer.data).data;
            }
        }
        ImGui::TreePop();
    }
    if(ImGui::TreeNode("Font Bank"))
    {
        char loc[100];
        sprintf(loc, "Location: 0x%zx", fontBank.location);
        ImGui::Text(loc);
        for(auto fnt = font.begin(); fnt != font.end(); fnt++)
        {
            char str[100];
            sprintf(str, "View Font 0x%x", fnt->ID);
            if(ImGui::Button(str))
            {
                *rm.memedit = *fnt->read(gameBuffer.data).data;
            }
        }
        ImGui::TreePop();
    }
}

vector<uint8_t> readCompressed(MemoryStream& strm, uint32_t datalen, uint32_t complen, bool* decompress)
{
    if (complen > 0 && datalen > 0)
    {
        vector<uint8_t> compressed(strm.readBytes(complen));
        return readCompressed(compressed, datalen, decompress);
    }
    if (decompress != nullptr) *decompress = false;
    return vector<uint8_t>({0});
}

vector<uint8_t> readCompressed(vector<uint8_t>& compressed, uint32_t datalen, bool* decompress)
{
    if (compressed.size() > 0 && datalen > 0)
    {
        if (decompress != nullptr && !*decompress) return compressed;
        
        vector<uint8_t> rtn(datalen);
        int cmpStatus = uncompress(&(rtn[0]), (mz_ulong*)&datalen, &(compressed[0]), compressed.size());
        if (cmpStatus == Z_OK)
        {
            if (decompress != nullptr) *decompress = true;
            return rtn;
        }

        try
        {
            size_t dlen = datalen;
            // int bytesread = tinf_uncompress(&(rtn[0]), &dlen, &(compressed[0]), compressed.size());
            uint8_t* decomp = (uint8_t*)tinfl_decompress_mem_to_heap(&(compressed[0]), compressed.size(), &dlen, 0);
            rtn.resize(dlen);
            for(size_t i = 0; i < dlen; i++) rtn[i] = decomp[i];
            mz_free(decomp);
            if (dlen == datalen)
            {
                if (decompress != nullptr) *decompress = true;
                return rtn;
            }
        } catch(std::exception e){}

        if (decompress != nullptr) *decompress = false;
        return compressed;
    }
    if (decompress != nullptr) *decompress = false;
    return vector<uint8_t>({0});
}

string readASCII(MemoryStream& strm)
{
    return string((char*)&((*strm.data)[0]), strm.data->size());
}

string readUnicode(MemoryStream& strm)
{
    string str(strm.data->size()/2, 0);
    for (auto it = str.begin(); it != str.end(); it++)
    {
        *it = (char)strm.readInt<uint16_t>();
    }
    return str;
}

string readString(MemoryStream& strm, bool unicode)
{
    if (unicode)
        return readUnicode(strm);
    else
        return readASCII(strm);
}

DataPoint::DataPoint(){}

DataPoint::DataPoint(MemoryStream& strm)
{
    (*this)(strm);
}

DataPoint::DataPoint(size_t loc, size_t flen)
{
    (*this)(loc, flen);
}

DataPoint::DataPoint(size_t loc, size_t flen, size_t alen)
{
    (*this)(loc, flen, alen);
}

DataPoint::DataPoint(MemoryStream& strm, size_t flen)
{
    (*this)(strm, flen);
}

DataPoint::DataPoint(MemoryStream& strm, size_t flen, size_t alen)
{
    (*this)(strm, flen, alen);
}

void DataPoint::operator()(MemoryStream& strm)
{
    ID = strm.readInt<uint16_t>();
    mode = strm.readInt<uint16_t>();
    fileLen = strm.readInt<uint32_t>();
    location = strm.position;
    dataLocation = location;
    defDataLen = fileLen;
    compressed = false;
    strm.position = location + fileLen;
}

void DataPoint::operator()(MemoryStream& strm, size_t flen)
{
    (*this)(strm.position, flen);
    strm.position += flen;
}

void DataPoint::operator()(MemoryStream& strm, size_t flen, size_t alen)
{
    (*this)(strm.position, flen, alen);
    strm.position += flen;
}

void DataPoint::operator()(size_t loc, size_t flen)
{
    location = loc;
    fileLen = flen;
    compressed = false;
}

void DataPoint::operator()(size_t loc, size_t flen, size_t alen)
{
    location = loc;
    dataLocation = loc;
    fileLen = flen;
    defDataLen = flen;
    infDataLen = alen;
    compressed = true;
}

void DataPoint::clear()
{
    *this = DataPoint();
}

void DataPoint::getData(MemoryStream& strm, size_t offset)//, bool comp)
{
	strm.position = location + offset;
	compressed = false;

	if (mode == MODE_2 || mode == MODE_3) // Clickteam "Mode 3"
	{
		defDataLen = fileLen - 0x4;
		dataLocation = strm.position;
	}

	if (mode == MODE_1)// || mode == MODE_3) // Compressed
	{
		compressed = true;
		infDataLen = strm.readInt<uint32_t>();
	}

	if (mode == MODE_0 || mode == MODE_1) // Zlib
	{
		defDataLen = strm.readInt<uint32_t>();
		// dataLocation = strm.position;
	}
    
    dataLocation = strm.position;
    uint32_t tfileLen = (dataLocation - location) + defDataLen;
    if(tfileLen > fileLen) fileLen = tfileLen;
    strm.position = location + fileLen;
}

MemoryStream DataPoint::rawStream(vector<uint8_t>* memory)
{
    auto begin = memory->begin() + dataLocation;
    auto end = begin + defDataLen;
    return MemoryStream(vector<uint8_t>(begin, end));
}

MemoryStream DataPoint::decompressedStream(vector<uint8_t>* memory)
{
    auto begin = memory->begin() + dataLocation;
    auto end = begin + defDataLen;
    return MemoryStream(readCompressed(vector<uint8_t>(begin, end), infDataLen));
}

MemoryStream DataPoint::read(vector<uint8_t>* memory)
{
    if (compressed)
        return decompressedStream(memory);
    else
        return rawStream(memory);
}

// Read game from file into memory
void SourceExplorer::loadIntoMem(string path)
{
    loaded = false;
    gameState.clear();
    gameState.push_back(STATE_DEFAULT);
    gamePath = path;
    DEBUG cout << "Game Path: " << gamePath << endl;

    gameDir = path.substr(0, path.find_last_of('\\') + 1);
    DEBUG cout << "Game Dir: " << gameDir << endl;

    ifstream in(gamePath, ios::binary | ios::ate);
    size_t fileSize = in.tellg();
    DEBUG cout << "File Size: 0x" << std::hex << fileSize << endl;

    try 
    {
        gameBuffer.data->resize(fileSize);
    }
    catch(std::exception e)
    {
        cout << "Bad directory or you probably don't have enough RAM" << endl;
        THROW(e);
        throw std::exception("Bad directory or you probably don't have enough RAM");
    }
    //if (fileSize != gameBuffer.data->size()) 
    DEBUG cout << "resize: 0x" << std::hex << gameBuffer.data->size() << endl;
    in.seekg(0);
    //in.get((char*)&((*gameBuffer.data)[0]), fileSize);
    for (size_t i = 0; i < gameBuffer.data->size(); i++)
    {
        (*gameBuffer.data)[i] = in.get();
    }
    // readEntries();
    loadFromMem();
}

// Read game from memory
void SourceExplorer::loadFromMem()
{
    loaded = false;
    DEBUG cout << "Reading game data" << endl;
    MemoryStream& strm = gameBuffer;
    strm.position = 0;
    uint16_t exeSig = strm.readInt<uint16_t>();
    DEBUG cout << "Executable Signature 0x" << std::hex << exeSig << endl;
    if (exeSig != WIN_EXE_SIG) throw std::exception("Invalid Executable Signature");

    strm.position = WIN_EXE_PNT;
    strm.position = strm.readInt<uint16_t>();
    DEBUG cout << "EXE Pointer: 0x" << std::hex << strm.position << endl;

    int32_t peSig = strm.readInt<int32_t>();
    DEBUG cout << "PE Signature: 0x" << std::hex << peSig << endl;
    DEBUG cout << "Pos: 0x" << std::hex << strm.position << endl;

    if (peSig != WIN_PE_SIG) throw std::exception("Invalid PE Signature");

    strm.position += 2;

    numHeaderSections = strm.readInt<uint16_t>();
    DEBUG cout << "Number Of Sections: 0x" << std::hex << numHeaderSections << endl;

    strm.position += 16;

    int optionalHeader = 0x60;
    int dataDir = 0x80;
    strm.position += optionalHeader + dataDir;

    DEBUG cout << "Pos: 0x" << std::hex << strm.position << endl;

    uint64_t pos = 0;
    for (uint16_t i = 0; i < numHeaderSections; i++)
    {
        uint64_t strt = strm.position;
        string name = strm.readString();
        if (name == ".extra")
        {
            DEBUG cout << name << endl;
            strm.position = strt + 0x14;
            pos = strm.readInt<int32_t>();
            break;
        }
        else if (i >= numHeaderSections - 1)
        {
            strm.position = strt + 0x10;
            uint32_t size = strm.readInt<uint32_t>();
            uint32_t addr = strm.readInt<uint32_t>();
            DEBUG cout << "size: 0x" << std::hex << size << endl;
            DEBUG cout << "addr: 0x" << std::hex << addr << endl;
            pos = size + addr;
            break;
        }
        strm.position = strt + 0x28;
        DEBUG cout << "Pos: 0x" << std::hex << strm.position << endl;
    }

    DEBUG cout << "First Pos: 0x" << std::hex << pos << endl;
    strm.position = pos;
    uint16_t firstShort = strm.readInt<uint16_t>();
    strm.position = pos;
    uint32_t pameMagic = strm.readInt<uint32_t>();
    strm.position = pos;
    uint64_t packMagic = strm.readInt<uint64_t>();
    strm.position = pos;

    DEBUG cout << "First Short 0x" << std::hex << firstShort << endl;
    DEBUG cout << "PAME Magic 0x" << std::hex << pameMagic << endl;
    DEBUG cout << "Pack Magic 0x" << std::hex << packMagic << endl;
    if (firstShort == CHUNK_HEADER || pameMagic == HEADER_GAME)
    {
        oldGame = true;
        gameState.push_back(STATE_OLD);
    }
    else if (packMagic == HEADER_PACK)
    {
        gameState.push_back(STATE_NEW);
        pos = packData(strm, gameState);
    }
    else throw std::exception("Invalid Pack Header");

    uint32_t header = strm.readInt<uint32_t>();
    DEBUG cout << "Header: 0x" << std::hex << header << endl;

    if (header == HEADER_UNIC) unicode = true;
    else if (header != HEADER_GAME) throw std::exception("Invalid Game Header");

    gameData(strm);
    //strm.position = pos;
    dataLocation = strm.position;
    loaded = true;
    getEntries(gameBuffer, gameState);
}

uint64_t SourceExplorer::packData(MemoryStream& strm, vector<uint16_t>& state)
{
    uint64_t strt = strm.position;
    uint64_t header = strm.readInt<uint64_t>();
    DEBUG cout << "Header: 0x" << std::hex << header << endl;
    uint32_t headerSize = strm.readInt<uint32_t>();
    DEBUG cout << "Header Size: 0x" << std::hex << headerSize << endl;
    uint32_t dataSize = strm.readInt<uint32_t>();
    DEBUG cout << "Data Size: 0x" << std::hex << dataSize << endl;

    strm.position = strt + dataSize - 0x20;

    header = strm.readInt<uint32_t>();
    DEBUG cout << "Head: 0x" << std::hex << header << endl;

    if (header == HEADER_UNIC)
    {
        oldGame = false;
        unicode = true;
    }
    if (oldGame) cout << "Old game" << endl;
    else if (!unicode) cout << "New non-unicode game" << endl;
    else cout << "New unicode game" << endl;

    strm.position = strt + 0x10;

    uint32_t formatVersion = strm.readInt<uint32_t>();
    DEBUG cout << "Format Version: 0x" << std::hex << formatVersion << endl;

    strm.position += 0x8;

    int32_t count = strm.readInt<int32_t>();
    DEBUG cout << "Count: 0x" << std::hex << count << endl;

    uint64_t off = strm.position;
    DEBUG cout << "Offset: 0x" << std::hex << off << endl;
    for (uint32_t i = 0; i < count; i++)
    {
        if ((strm.data->size() - strm.position) < 2) break;

        uint32_t val = strm.readInt<uint16_t>();
        if ((strm.data->size() - strm.position) < val) break;

        strm.position += val;
        if ((strm.data->size() - strm.position) < 4) break;

        val = strm.readInt<uint32_t>();
        if ((strm.data->size() - strm.position) < val) break;

        strm.position += val;
    }

    header = strm.readInt<uint32_t>();
    DEBUG cout << "Header: 0x" << std::hex << header << endl;

    bool hasBingo = (header != HEADER_GAME) && (header != HEADER_UNIC);
    if (hasBingo) cout << "Has Bingo" << endl;
    else cout << "No Bingo" << endl;
    
    strm.position = off;

    packFiles.resize(count);

    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t read = strm.readInt<uint16_t>();

        if (unicode) packFiles[i].filename = strm.readString<uint16_t>(read);
        else packFiles[i].filename = strm.readString(read);
        if (read < 64) cout << "Pack File Name: " << packFiles[i].filename << endl;

        if (hasBingo) packFiles[i].bingo = strm.readInt<uint32_t>();
        else packFiles[i].bingo = 0;
        DEBUG cout << "Pack File Bingo: " << packFiles[i].bingo << endl;

        if (unicode) read = strm.readInt<uint32_t>();
        else read = strm.readInt<uint16_t>();
        DEBUG cout << "Pack File Data Size: " << read << endl;

        packFiles[i].data = strm.readBytes(read);
    }

    header = strm.readInt<uint32_t>(); //PAMU sometimes
    DEBUG cout << "Header: 0x" << std::hex << header << endl;

    if (header == HEADER_GAME || header == HEADER_UNIC)
    {
        state.push_back(STATE_NEW); //we can't get to here if it's an old game so this is redundant
        uint32_t pos = strm.position;
        strm.position -= 0x4;
        return pos;
    }
    return strm.position;
}

void SourceExplorer::gameData(MemoryStream& strm)
{
    game.location = strm.position;
    uint16_t firstShort = strm.readInt<uint16_t>();
    // game.ID = firstShort;
    if (firstShort == CNCV1VER)
    {
        cnc = true;
        //readCNC(strm);
        DEBUG cout << "Read CNC" << endl;
        return;
    }
    runtimeVersion = firstShort;
    runtimeSubVersion = strm.readInt<uint16_t>();
    productVersion = strm.readInt<uint32_t>();
    productBuild = strm.readInt<uint32_t>();

    game(strm.position, strm.data->size() - strm.position);

    DEBUG cout << product(runtimeVersion) << endl;
    if (runtimeVersion == PROD_MMF15) oldGame = true;
    else if (runtimeVersion != PROD_MMF2) throw std::exception("Invalid Product");
}

string SourceExplorer::product(uint16_t vnum)
{
    switch(vnum)
    {
        case PROD_MMF1:
            return "MMF1";
        case PROD_MMF15:
            return "MMF1.5";
        case PROD_MMF2:
            return "MMF2";
        default:
            return "Invalid";
    }
}