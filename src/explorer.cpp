#include "explorer.h"

ResourceEntry::ResourceEntry()
{
    if (extraData != nullptr)
        free(extraData);
    chunks.clear();
    preData.clear();
}

ResourceEntry::ResourceEntry(MemoryStream& strm, uint16_t* state)
{
    if (extraData != nullptr)
        free(extraData);
    chunks.clear();
    preData.clear();

    cout << endl << 
    "New Resource" << endl << 
    "Pos: 0x" << std::hex << strm.position << endl <<
    "State: 0x" << std::hex << *state << endl;
    ID = strm.readInt<uint16_t>();
    cout << "Resource ID: 0x" << std::hex << ID << endl;
    mode = strm.readInt<uint16_t>();
    cout << "Resource Mode: 0x" << std::hex << mode << endl;
    if (mode != MODE_0 && mode != MODE_1 && mode != MODE_2 && mode != MODE_3)
        throw std::exception("Invalid Mode");
    //create instance based on type
    dataLen = 0;
    switch(*state) {
        case STATE_IMAGE:
        case STATE_FONT:
        case STATE_SOUND:
        case STATE_MUSIC: {
            if (ID == CHUNK_ENDIMAGE || ID == CHUNK_ENDMUSIC || 
                ID == CHUNK_ENDSOUND || ID == CHUNK_ENDFONT) 
            {
                *state = STATE_NEW;
                switch(mode) {
                    case MODE_0: {
                        uint32_t predlen = findNext(strm);
                        predlen = (predlen < 8 ? 0 : predlen - 8);
                        cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                        preData = strm.readBytes(predlen);
                    } break;
                    case MODE_1: {
                        uint32_t predlen = findNext(strm);
                        predlen = (predlen < 8 ? 0 : predlen - 8);
                        cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                        preData = strm.readBytes(predlen);
                        dataLen = strm.readInt<uint32_t>();
                        compressedDataLen = strm.readInt<uint32_t>();
                    } break;
                    case MODE_2:
                    case MODE_3: {
                        uint32_t predlen = findNext(strm);
                        predlen = (predlen < 8 ? 0 : predlen - 8);
                        cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                        preData = strm.readBytes(predlen);
                        dataLen = strm.readInt<uint32_t>();
                    } break;
                    default: {
                        throw std::exception("Invalid Mode");
                    } break;
                }
            }
            else
            {
                switch(mode) {
                    case MODE_0:
                    case MODE_1: {
                        uint32_t predlen = findNext(strm);
                        predlen = (predlen < 8 ? 0 : predlen - 8);
                        cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                        preData = strm.readBytes(predlen);
                        dataLen = strm.readInt<uint32_t>();
                        compressedDataLen = strm.readInt<uint32_t>();
                    } break;
                    case MODE_2:
                    case MODE_3: {
                        preData.clear();
                        dataLen = strm.readInt<uint32_t>();
                    } break;
                    default: {
                        throw std::exception("Invalid Mode");
                    } break;
                }
            }
        } break;
        case STATE_DEFAULT:
        case STATE_VITA:
        case STATE_NEW:
        case STATE_OLD: {
            switch(ID) {
                case CHUNK_HEADER: {
                    //extraData = new GameEntry();
                    extraData = malloc(sizeof(GameEntry));
                    //*(GameEntry*)extraData = GameEntry();
                    //((GameEntry*)extraData)->frameBank.clear();
					new((GameEntry*)extraData) GameEntry();
                    switch(mode) {
                        case MODE_0: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                        } break;
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        case MODE_2:
                        case MODE_3: {
                            preData.clear();
                            dataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_SPACER:
                case CHUNK_ENTRY:
                case CHUNK_EXTDATA:
                case CHUNK_MOVETIMEBASE: 
                case CHUNK_UNKNOWN8: {
                    switch(mode) {
                        case MODE_0: {
                            preData = strm.readBytes(0x8);
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_MENU:
                case CHUNK_FRAMEEFFECTS: {
                    switch(mode) {
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_ICON: {
                    switch(mode) {
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_EXTNLIST: {
                    switch(mode) {
                        case MODE_0: {
                            preData = strm.readBytes(0x8);
                        } break;
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_SECNUM:
                case CHUNK_FRAMEHEADER:
                case CHUNK_FRAMEHANDLES:
                case CHUNK_FRAMEVIRTSIZE:
                case CHUNK_FRAMEPALETTE:
                case CHUNK_FRAMELAYERS:
                case CHUNK_FRAMELAYEREFFECT:
                case CHUNK_OBJINST:
                case CHUNK_FRAMEEVENTS:
                case CHUNK_FRAMEFADEI:
                case CHUNK_FRAMEFADEO:
                case CHUNK_OBJHEAD: {
                    switch(mode) {
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        case MODE_3: {
                            preData.clear();
                            dataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_EXEONLY: {
                    switch(mode) {
                        case MODE_0: {
                            preData = strm.readBytes(0x15);
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_FRAMENAME:
                case CHUNK_OBJNAME: {
                    switch(mode) {
                        case MODE_0: {
                            preData = strm.readBytes(strm.readInt<uint32_t>());
                        } break;
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        case MODE_2:
                        case MODE_3: {
                            preData.clear();
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_PROTECTION: {
                    switch(mode) {
                        case MODE_0: {
                            preData = strm.readBytes(0x8);
                        } break;
                        case MODE_2: {
                            preData.clear();
                            dataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                }
                case CHUNK_SHADERS: {
                    switch(mode) {
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_MUSICBANK:
                case CHUNK_SOUNDBANK:
                case CHUNK_IMAGEBANK: {
                    if (ID == CHUNK_MUSICBANK) *state = STATE_MUSIC;
                    else if (ID == CHUNK_SOUNDBANK) *state = STATE_SOUND;
                    else *state = STATE_IMAGE;
                    switch(mode) {
                        case MODE_0: {
                            preData = strm.readBytes(0x8);
                        } break;
                        case MODE_1:
                        case MODE_2:
                        case MODE_3: {
                            preData = strm.readBytes(0x8);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                }
                case CHUNK_FONTBANK: {
                    *state = STATE_FONT;
                    switch(mode) {
                        case MODE_0:
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        case MODE_2:
                        case MODE_3: {
                            preData.clear();
                            dataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_OBJECTBANK: {
                    switch(mode) {
                        case MODE_0: {
                            preData = strm.readBytes(0x8);
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_FRAME:
                case CHUNK_LAST: {
                    switch(mode) {
                        case MODE_0: {
                            preData = strm.readBytes(0x4);
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_TITLE:
                case CHUNK_TITLE2:
                case CHUNK_AUTHOR:
                case CHUNK_PROJPATH:
                case CHUNK_OUTPATH:
                case CHUNK_COPYRIGHT:
                default: {
                    switch(mode) {
                        case MODE_0: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                        } break;
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            predlen = (predlen < 8 ? 0 : predlen - 8);
                            cout << "Pre Data Length: 0x" << std::hex << predlen << endl;
                            preData = strm.readBytes(predlen);
                            dataLen = strm.readInt<uint32_t>();
                            compressedDataLen = strm.readInt<uint32_t>();
                        } break;
                        case MODE_2:
                        case MODE_3: {
                            preData.clear();
                            dataLen = strm.readInt<uint32_t>();
                        } break;
                        default: {
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
            }
        } break; 
        default: {
            throw std::exception("Invalid State");
        } break;
    }
    cout << "Resource Data Length: 0x" << std::hex << dataLen << endl;
    cout << "Resource Compressed Data Length: 0x" << std::hex << compressedDataLen << endl;
    dataLoc = (uint64_t)strm.position;
    cout << "Resource Data Location: 0x" << std::hex << dataLoc << endl;
    hasData = dataLen > 0;
    compressedData = hasData && (mode & MODE_FLAG_COMPRESSED);
    if (compressedData)
        cout << "Has compressed data" << endl;
    else
        cout << "No compressed data" << endl;
    // Must read in order to move the stream position correctly
    readCompressed(strm, dataLen, compressedDataLen, &compressedData);
    // Fetch Children
    cout << "Fetching Child Objects" << endl;
    while (strm.position < strm.data->size())
    {
        ResourceEntry* chunk = nullptr;
        bool more = fetchChild(strm, state, &chunk);
        if (!more) return;
        cout << "Got Child: 0x" << std::hex << chunk->ID << endl;
        //chunk.fetchChildren(strm, state); //already called in initialiser
        setChild(strm, state, chunk);
    }
}

bool ResourceEntry::fetchChild(MemoryStream& strm, uint16_t* state, ResourceEntry** chunk)
{
    switch(*state) {
        case STATE_NEW: {
            switch(ID) {
                case CHUNK_HEADER: {
                    //*chunk = new /*(std::nothrow)*/ ResourceEntry(strm, state);
                    *chunk = (ResourceEntry*)malloc(sizeof(ResourceEntry));
                    cout << "Addr: 0x" << std::hex << (uint64_t)*chunk << endl;
                    flush(cout);
                    //**chunk = ResourceEntry(strm, state);
                    new(*chunk) ResourceEntry(strm, state);
                    return (*chunk)->ID != CHUNK_LAST;
                } break;
                default: {
                    return false;
                } break;
            }
        }
        case STATE_IMAGE: {
            // *chunk = new /*(std::nothrow)*/ ResourceEntry(strm, state);
            // if (*chunk == nullptr || (*chunk)->ID == CHUNK_ENDIMAGE) {
            //     *state = STATE_NEW;
            //     return false;
            // }
            *chunk = (ResourceEntry*)malloc(sizeof(ResourceEntry));
            **chunk = ResourceEntry(strm, state);
            return (*chunk)->ID != CHUNK_ENDIMAGE;
        } break;
        case STATE_FONT: {
            *chunk = (ResourceEntry*)malloc(sizeof(ResourceEntry));
            **chunk = ResourceEntry(strm, state);
            return (*chunk)->ID != CHUNK_ENDFONT;
        } break;
        case STATE_SOUND: {
            *chunk = (ResourceEntry*)malloc(sizeof(ResourceEntry));
            **chunk = ResourceEntry(strm, state);
            return (*chunk)->ID != CHUNK_ENDSOUND;
        } break;
        case STATE_MUSIC: {
            *chunk = (ResourceEntry*)malloc(sizeof(ResourceEntry));
            **chunk = ResourceEntry(strm, state);
            return (*chunk)->ID != CHUNK_ENDMUSIC;
        } break;
        case STATE_OLD:
        case STATE_VITA:
        case STATE_DEFAULT:
        default: {
            switch(ID) {
                default: {
                    return false;
                } break;
            }
        } break;
    }
    return false; //return true if there is more to read
}

void ResourceEntry::setChild(MemoryStream& strm, uint16_t* state, ResourceEntry* chunk)
{
    if (chunk == nullptr)
    {
        return; // just checking
    }
    switch(*state) {
        case STATE_OLD: {
            switch(chunk->ID) {
                case CHUNK_OLD_HEADER: {
                    if (((GameEntry*)extraData)->header != nullptr) 
                        free(((GameEntry*)extraData)->header);
                    ((GameEntry*)extraData)->header = chunk;
                } break;
                case CHUNK_OLD_FRAMEITEMS: {
                    if (((GameEntry*)extraData)->objectBank != nullptr) 
                        free(((GameEntry*)extraData)->objectBank);
                    ((GameEntry*)extraData)->objectBank = chunk;
                } break;
                case CHUNK_OLD_FRAME: {
                    ((GameEntry*)extraData)->frameBank.push_back(chunk);
                } break;
                default: {
                   goto newobj; //I'm so sorry...
                } break;
            }
        } break;
        case STATE_NEW: {
            newobj:
            switch(chunk->ID) {
                case CHUNK_HEADER: {
                    if (((GameEntry*)extraData)->header != nullptr) 
                        free(((GameEntry*)extraData)->header);
                    ((GameEntry*)extraData)->header = chunk;
                } break;
                case CHUNK_EXTDHEADER: {
                    if (((GameEntry*)extraData)->extendedHeader != nullptr) 
                        free(((GameEntry*)extraData)->extendedHeader);
                    ((GameEntry*)extraData)->extendedHeader = chunk;
                } break;
                case CHUNK_TITLE: {
                    if (((GameEntry*)extraData)->title != nullptr) 
                        free(((GameEntry*)extraData)->title);
                    ((GameEntry*)extraData)->title = chunk;
                } break;
                case CHUNK_COPYRIGHT: {
                    if (((GameEntry*)extraData)->copyright != nullptr) 
                        free(((GameEntry*)extraData)->copyright);
                    ((GameEntry*)extraData)->copyright = chunk;
                } break;
                case CHUNK_ABOUT: {
                    if (((GameEntry*)extraData)->aboutText != nullptr) 
                        free(((GameEntry*)extraData)->aboutText);
                    ((GameEntry*)extraData)->aboutText = chunk;
                } break;
                case CHUNK_AUTHOR: {
                    if (((GameEntry*)extraData)->author != nullptr) 
                        free(((GameEntry*)extraData)->author);
                    ((GameEntry*)extraData)->author = chunk;
                } break;
                case CHUNK_PROJPATH: {
                    if (((GameEntry*)extraData)->projectPath != nullptr) 
                        free(((GameEntry*)extraData)->projectPath);
                    ((GameEntry*)extraData)->projectPath = chunk;
                } break;
                case CHUNK_OUTPATH: {
                    if (((GameEntry*)extraData)->outputPath != nullptr) 
                        free(((GameEntry*)extraData)->outputPath);
                    ((GameEntry*)extraData)->outputPath = chunk;
                } break;
                case CHUNK_EXEONLY: {
                    if (((GameEntry*)extraData)->exeOnly != nullptr) 
                        free(((GameEntry*)extraData)->exeOnly);
                    ((GameEntry*)extraData)->exeOnly = chunk;
                } break;
                case CHUNK_MENU: {
                    if (((GameEntry*)extraData)->menu != nullptr) 
                        free(((GameEntry*)extraData)->menu);
                    ((GameEntry*)extraData)->menu = chunk;
                } break;
                case CHUNK_ICON: {
                    if (((GameEntry*)extraData)->icon != nullptr) 
                        free(((GameEntry*)extraData)->icon);
                    ((GameEntry*)extraData)->icon = chunk;
                } break;
                case CHUNK_SHADERS: {
                    if (((GameEntry*)extraData)->shaders != nullptr) 
                        free(((GameEntry*)extraData)->shaders);
                    ((GameEntry*)extraData)->shaders = chunk;
                } break;
                case CHUNK_GLOBALSTRS: {
                    if (((GameEntry*)extraData)->globalStrings != nullptr) 
                        free(((GameEntry*)extraData)->globalStrings);
                    ((GameEntry*)extraData)->globalStrings = chunk;
                } break;
                case CHUNK_GLOBALVALS: {
                    if (((GameEntry*)extraData)->globalValues != nullptr) 
                        free(((GameEntry*)extraData)->globalValues);
                    ((GameEntry*)extraData)->globalValues = chunk;
                } break;
                case CHUNK_EXTNLIST: {
                    if (((GameEntry*)extraData)->extensions != nullptr) 
                        free(((GameEntry*)extraData)->extensions);
                    ((GameEntry*)extraData)->extensions = chunk;
                } break;
                case CHUNK_FRAMEHANDLES: {
                    if (((GameEntry*)extraData)->frameHandles != nullptr) 
                        free(((GameEntry*)extraData)->frameHandles);
                    ((GameEntry*)extraData)->frameHandles = chunk;
                } break;
                case CHUNK_FRAME: {
                    ((GameEntry*)extraData)->frameBank.push_back(chunk);
                } break;
                case CHUNK_OBJECTBANK: {
                    if (((GameEntry*)extraData)->objectBank != nullptr) 
                        free(((GameEntry*)extraData)->objectBank);
                    ((GameEntry*)extraData)->objectBank = chunk;
                } break;
                case CHUNK_SOUNDBANK: {
                    if (((GameEntry*)extraData)->soundBank != nullptr) 
                        free(((GameEntry*)extraData)->soundBank);
                    ((GameEntry*)extraData)->soundBank = chunk;
                } break;
                case CHUNK_MUSICBANK: {
                    if (((GameEntry*)extraData)->musicBank != nullptr) 
                        free(((GameEntry*)extraData)->musicBank);
                    ((GameEntry*)extraData)->musicBank = chunk;
                } break;
                case CHUNK_FONTBANK: {
                    if (((GameEntry*)extraData)->fontBank != nullptr) 
                        free(((GameEntry*)extraData)->fontBank);
                    ((GameEntry*)extraData)->fontBank = chunk;
                } break;
                case CHUNK_IMAGEBANK: {
                    if (((GameEntry*)extraData)->imageBank != nullptr) 
                        free(((GameEntry*)extraData)->imageBank);
                    ((GameEntry*)extraData)->imageBank = chunk;
                } break;
                case CHUNK_SECNUM: {
                    if (((GameEntry*)extraData)->serial != nullptr) 
                        free(((GameEntry*)extraData)->serial);
                    ((GameEntry*)extraData)->serial = chunk;
                } break;
                case CHUNK_BINFILES: {
                    if (((GameEntry*)extraData)->fileBank != nullptr) 
                        free(((GameEntry*)extraData)->fileBank);
                    ((GameEntry*)extraData)->fileBank = chunk;
                } break;
                default: {
                    chunks.push_back(chunk);
                } break;
            }
        } break;
        case STATE_DEFAULT:
        default: {
            switch(ID) {
                default: {
                    chunks.push_back(chunk);
                } break;
            }
        } break;
    }
}

ResourceEntry::~ResourceEntry()
{
    if (extraData != nullptr)
        free(extraData);
    for (auto it = chunks.begin(); it != chunks.end(); it++)
    {
        (*it)->~ResourceEntry();
        free(*it);
    }
    chunks.clear();
    preData.clear();
}

vector<uint8_t> ResourceEntry::readCompressed(MemoryStream& strm, uint32_t datalen, uint32_t complen, bool* decompress)
{
    if (complen > 0 && datalen > 0)
    {
        vector<uint8_t> compressed(strm.readBytes(complen));
        if (decompress != nullptr) if (!*decompress) return compressed;
        
        vector<uint8_t> rtn(datalen);
        int cmpStatus = uncompress(&(rtn[0]), (mz_ulong*)&datalen, &(compressed[0]), complen);
        if (cmpStatus == Z_OK)
        {
            *decompress = true;
            return rtn;
        }
        else 
        {
            *decompress = false;
            return compressed;
        }
    }
    *decompress = false;
    return vector<uint8_t>({0});
}

uint32_t ResourceEntry::findNext(MemoryStream& strm)
{
    int8_t count = -1;
    int32_t untilNext = 0;
    int32_t pos = untilNext + strm.position;
    while (count < 0 || count > 0x7F)
    {
        untilNext += findUntilNext(strm, pos + 2, vector<uint8_t>({0x78, 0xDA})) + 2;
        pos = untilNext + strm.position;
        count = (*strm.data)[pos - 1];
    }
    return untilNext;
}

int32_t ResourceEntry::findUntilNext(MemoryStream& strm, uint32_t pos, const vector<uint8_t>& toFind)
{
    for (int index1 = 0; pos + index1 < strm.data->size() - toFind.size(); index1++)
    {
        bool flag = true;
        for (int index2 = 0; index2 < toFind.size(); index2++)
        {
            if ((*strm.data)[pos + index1 + index2] != toFind[index2]) flag = false;
        }
        if (flag) return index1;
    }
    return -1;
}

void SourceExplorer::loadGame(string path)
{
    gamePath = path;
    cout << "Game Path: " << gamePath << endl;

    gameDir = path.substr(0, path.find_last_of('\\') + 1);
    cout << "Game Dir: " << gameDir << endl;

    ifstream in(gamePath, ios::binary | ios::ate);
    size_t fileSize = in.tellg();
    cout << "File Size: 0x" << std::hex << fileSize << endl;

    try 
    {
        gameBuffer.data->resize(fileSize);
    }
    catch(std::exception e)
    {
        throw std::exception("You probably don't have enough RAM");
    }
    //if (fileSize != gameBuffer.data->size()) 
    cout << "resize: 0x" << std::hex << gameBuffer.data->size() << endl;
    in.seekg(0);
    //in.get((char*)&((*gameBuffer.data)[0]), fileSize);
    for (size_t i = 0; i < gameBuffer.data->size(); i++)
    {
        (*gameBuffer.data)[i] = in.get();
    }
    readEntries();
}

void SourceExplorer::readEntries()
{
    entry = true;
    readGameData(&gameState);
    //fetchEntry -> entry
    game = ResourceEntry(gameBuffer, &gameState);
    //entry -> fetchChildren
    //entry -> readData
    //ResourceEntry.push_back(entry)

    //GameDeconstructor gd;
    //MemoryStream& gameStream = gd.fetchGameData(gameBuffer, &state);
    ///*GameEntry*/ ResourceEntry game = /*(GameEntry)*/gd.fetchEntry(gameStream, &state);
    //game.fetchChildren();
}

void SourceExplorer::readGameData(uint16_t* state)
{
    cout << "Reading game data" << endl;
    MemoryStream& strm = gameBuffer;
    strm.position = 0;
    uint16_t exeSig = strm.readInt<uint16_t>();
    cout << "Executable Signature 0x" << std::hex << exeSig << endl;
    if (exeSig != WIN_EXE_SIG) throw std::exception("Invalid Executable Signature");

    strm.position = WIN_EXE_PNT;
    strm.position = strm.readInt<uint16_t>();
    cout << "EXE Pointer: 0x" << std::hex << strm.position << endl;

    int32_t peSig = strm.readInt<int32_t>();
    cout << "PE Signature: 0x" << std::hex << peSig << endl;
    cout << "Pos: 0x" << std::hex << strm.position << endl;

    if (peSig != WIN_PE_SIG) throw std::exception("Invalid PE Signature");

    strm.position += 2;

    numHeaderSections = strm.readInt<uint16_t>();
    cout << "Number Of Sections: 0x" << std::hex << numHeaderSections << endl;

    strm.position += 16;

    int optionalHeader = 0x60;
    int dataDir = 0x80;
    strm.position += optionalHeader + dataDir;

    cout << "Pos: 0x" << std::hex << strm.position << endl;

    uint64_t pos = 0;
    for (uint16_t i = 0; i < numHeaderSections; i++)
    {
        uint64_t strt = strm.position;
        string name = strm.readString();
        if (name == ".extra")
        {
            cout << name << endl;
            strm.position = strt + 0x14;
            pos = strm.readInt<int32_t>();
            break;
        }
        else if (i >= numHeaderSections - 1)
        {
            strm.position = strt + 0x10;
            uint32_t size = strm.readInt<uint32_t>();
            uint32_t addr = strm.readInt<uint32_t>();
            cout << "size: 0x" << std::hex << size << endl;
            cout << "addr: 0x" << std::hex << addr << endl;
            pos = size + addr;
            break;
        }
        strm.position = strt + 0x28;
        cout << "Pos: 0x" << std::hex << strm.position << endl;
    }

    cout << "First Pos: 0x" << std::hex << pos << endl;
    strm.position = pos;
    uint16_t firstShort = strm.readInt<uint16_t>();
    strm.position = pos;
    uint32_t pameMagic = strm.readInt<uint32_t>();
    strm.position = pos;
    uint64_t packMagic = strm.readInt<uint64_t>();
    strm.position = pos;

    cout << "First Short 0x" << std::hex << firstShort << endl;
    cout << "PAME Magic 0x" << std::hex << pameMagic << endl;
    cout << "Pack Magic 0x" << std::hex << packMagic << endl;
    if (firstShort == CHUNK_HEADER || pameMagic == HEADER_GAME)
    {
        oldGame = true;
        *state = STATE_OLD;
    }
    else if (packMagic == HEADER_PACK)
    {
        *state = STATE_NEW;
        pos = packData(strm, state);
    }
    else throw std::exception("Invalid Pack Header");

    uint32_t header = strm.readInt<uint32_t>();
    cout << "Header: 0x" << std::hex << header << endl;

    if (header == HEADER_UNIC) unicode = true;
    else if (header != HEADER_GAME) throw std::exception("Invalid Game Header");

    gameData(strm);
    //strm.position = pos;
}

uint64_t SourceExplorer::packData(MemoryStream& strm, uint16_t* state)
{
    uint64_t strt = strm.position;
    uint64_t header = strm.readInt<uint64_t>();
    cout << "Header: 0x" << std::hex << header << endl;
    uint32_t headerSize = strm.readInt<uint32_t>();
    cout << "Header Size: 0x" << std::hex << headerSize << endl;
    uint32_t dataSize = strm.readInt<uint32_t>();
    cout << "Data Size: 0x" << std::hex << dataSize << endl;

    strm.position = strt + dataSize - 0x20;

    header = strm.readInt<uint32_t>();
    cout << "Head: 0x" << std::hex << header << endl;

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
    cout << "Format Version: 0x" << std::hex << formatVersion << endl;

    strm.position += 0x8;

    int32_t count = strm.readInt<int32_t>();
    cout << "Count: 0x" << std::hex << count << endl;

    uint64_t off = strm.position;
    cout << "Offset: 0x" << std::hex << off << endl;
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
    cout << "Header: 0x" << std::hex << header << endl;

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
        cout << "Pack File Bingo: " << packFiles[i].bingo << endl;

        if (unicode) read = strm.readInt<uint32_t>();
        else read = strm.readInt<uint16_t>();
        cout << "Pack File Data Size: " << read << endl;

        packFiles[i].data = strm.readBytes(read);
    }

    header = strm.readInt<uint32_t>(); //PAMU sometimes
    cout << "Header: 0x" << std::hex << header << endl;

    if (header == HEADER_GAME || header == HEADER_UNIC)
    {
        *state = STATE_NEW;
        uint32_t pos = strm.position;
        strm.position -= 0x4;
        return pos;
    }
    return strm.position;
}

void SourceExplorer::gameData(MemoryStream& strm)
{
    uint16_t firstShort = strm.readInt<uint16_t>();
    if (firstShort == CNCV1VER)
    {
        cnc = true;
        //readCNC(strm);
        cout << "Read CNC" << endl;
        return;
    }
    runtimeVersion = firstShort;
    runtimeSubVersion = strm.readInt<uint16_t>();
    productVersion = strm.readInt<uint32_t>();
    productBuild = strm.readInt<uint32_t>();

    cout << product(runtimeVersion) << endl;
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
            return "None";
    }
}