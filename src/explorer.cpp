#include "explorer.h"

ResourceEntry::ResourceEntry()
{
    if (extraData != nullptr)
        free(extraData);
    chunks.clear();
}

ResourceEntry::ResourceEntry(MemoryStream& strm, vector<uint16_t>& state)
{
	location = strm.position;
    if (extraData != nullptr)
        free(extraData);
    chunks.clear();
    string str = "|";
    for (size_t i = 0; i < state.size(); i++) 
		str += "-";
    DEBUG cout << "|" <<  endl << str <<
    "New Resource" << endl << str <<
    "Pos: 0x" << std::hex << strm.position << endl << str <<
    "State: 0x" << std::hex << state.back() << endl;
    ID = strm.readInt<uint16_t>();
    DEBUG cout << str << "Resource ID: 0x" << std::hex << ID << endl;
    mode = strm.readInt<uint16_t>();
    DEBUG cout << str << "Resource Mode: 0x" << std::hex << mode << endl;
	if (state.back() == STATE_NEW && (ID < CHUNK_ENTRY || ID > CHUNK_LAST))
		throw std::exception("Early Invalid State/ID");
    if (mode != MODE_0 && mode != MODE_1 && mode != MODE_2 && mode != MODE_3)
        throw std::exception("Early Invalid Mode");
    //create instance based on type
    switch(state.back()) {
        //case STATE_IMAGE:
        //case STATE_FONT:
        //case STATE_SOUND:
        //case STATE_MUSIC: {
        case CHUNK_IMAGEBANK:
        case CHUNK_SOUNDBANK:
        case CHUNK_MUSICBANK:
        case CHUNK_FONTBANK: {
            if (ID == CHUNK_ENDIMAGE || ID == CHUNK_ENDMUSIC || 
                ID == CHUNK_ENDSOUND || ID == CHUNK_ENDFONT) 
            {
                switch(mode) {
                    case MODE_0: {
                        uint32_t predlen = findNext(strm);
                        preData(strm, (predlen < 8 ? 0 : predlen - 8));
                    } break;
                    case MODE_1: {
                        uint32_t predlen = findNext(strm);
                        preData(strm, (predlen < 8 ? 0 : predlen - 8));
                        uint32_t dlen = strm.readInt<uint32_t>();
                        uint32_t flen = strm.readInt<uint32_t>();
                        mainData(strm, flen, dlen);
                    } break;
                    case MODE_2:
                    case MODE_3: {
                        uint32_t predlen = findNext(strm);
                        preData(strm, (predlen < 8 ? 0 : predlen - 8));
                        mainData(strm, strm.readInt<uint32_t>());
                    } break;
                    default: {
                        DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
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
                        preData(strm, (predlen < 8 ? 0 : predlen - 8));
                        uint32_t dlen = strm.readInt<uint32_t>();
                        uint32_t flen = strm.readInt<uint32_t>();
                        mainData(strm, flen, dlen);
                    } break;
                    case MODE_2:
                    case MODE_3: {
                        preData.clear();
                        mainData(strm, strm.readInt<uint32_t>());
                    } break;
                    default: {
                        DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                        throw std::exception("Invalid Mode");
                    } break;
                }
                state.push_back(STATE_NOCHILD);
            }
        } break;
        case CHUNK_FRAME: {
            if ((ID > CHUNK_FRAMEIPHONEOPTS || ID < CHUNK_FRAMEHEADER) && ID != CHUNK_LAST) { strm.position = location; return; }
        } // do not break
        case STATE_DEFAULT:
        case STATE_VITA:
        case STATE_NEW:
        case STATE_OLD:
        default: {
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
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                        } break;
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        case MODE_2:
                        case MODE_3: {
                            preData.clear();
                            mainData(strm, strm.readInt<uint32_t>());
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
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
                            preData(strm, 0x8);
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_MENU:
                case CHUNK_FRAMEEFFECTS: {
                    switch(mode) {
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_ICON: {
                    switch(mode) {
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_EXTNLIST: {
                    switch(mode) {
                        case MODE_0: {
                            preData(strm, 0x8);
                        } break;
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
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
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        case MODE_3: {
                            preData.clear();
                            mainData(strm, strm.readInt<uint32_t>());
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_EXEONLY: {
                    switch(mode) {
                        case MODE_0: {
                            preData(strm, 0x15);
                            DEBUG cout << str << "Pre Data Length: 0x" << std::hex << 0x15 << endl;
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_FRAMENAME:
                case CHUNK_OBJNAME: {
                    switch(mode) {
                        case MODE_0: {
                            preData(strm, strm.readInt<uint32_t>());
                        } break;
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        case MODE_2:
                        case MODE_3: {
                            preData.clear();
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_PROTECTION: {
                    switch(mode) {
                        case MODE_0: {
                            preData(strm, 0x8);
                        } break;
                        case MODE_2: {
                            preData.clear();
                            mainData(strm, strm.readInt<uint32_t>());
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_SHADERS: {
                    switch(mode) {
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_MUSICBANK:
                case CHUNK_SOUNDBANK:
                case CHUNK_IMAGEBANK: {
                    // if (ID == CHUNK_MUSICBANK) state.push_back(STATE_MUSIC_BANK);
                    // else if (ID == CHUNK_SOUNDBANK) state.push_back(STATE_SOUND_BANK);
                    // else state.push_back(STATE_IMAGE_BANK);
                    switch(mode) {
                        case MODE_0: {
                            preData(strm, 0x8);
                        } break;
                        case MODE_1:
                        case MODE_2:
                        case MODE_3: {
                            preData(strm, 0x8);
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_FONTBANK: {
                    // state.push_back(STATE_FONT_BANK);
                    switch(mode) {
                        case MODE_0:
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        case MODE_2:
                        case MODE_3: {
                            preData.clear();
                            mainData(strm, strm.readInt<uint32_t>());
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_TITLE2:
                case CHUNK_OBJECTBANK: {
                    switch(mode) {
                        case MODE_0: {
                            preData(strm, 0x8);
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_FRAME:
                case CHUNK_LAST: {
                    switch(mode) {
                        case MODE_0: {
                            preData(strm, 0x4);
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
                case CHUNK_TITLE:
                //case CHUNK_TITLE2:
                case CHUNK_AUTHOR:
                case CHUNK_PROJPATH:
                case CHUNK_OUTPATH:
                case CHUNK_COPYRIGHT:
                default: {
                    switch(mode) {
                        case MODE_0: {
                            uint32_t predlen = findNext(strm);
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                        } break;
                        case MODE_1: {
                            uint32_t predlen = findNext(strm);
                            preData(strm, (predlen < 8 ? 0 : predlen - 8));
                            uint32_t dlen = strm.readInt<uint32_t>();
                            uint32_t flen = strm.readInt<uint32_t>();
                            mainData(strm, flen, dlen);
                        } break;
                        case MODE_2:
                        case MODE_3: {
                            preData.clear();
                            mainData(strm, strm.readInt<uint32_t>());
                        } break;
                        default: {
                            DEBUG cout << "Invalid Mode: 0x" << std::hex << mode << endl;
                            throw std::exception("Invalid Mode");
                        } break;
                    }
                } break;
            }
        } break; 
        // default: {
        //     DEBUG cout << "Invalid State: 0x" << std::hex << state.back() << endl;
        //     throw std::exception("Invalid State");
        // } break;
    }
    // switch(ID) {
    //     case CHUNK_IMAGEBANK:
    //     case CHUNK_MUSICBANK:
    //     case CHUNK_SOUNDBANK:
    //     case CHUNK_FONTBANK: {
    //         state.push_back(ID);
    //     } break;
    //     default: {
    //         state.push_back(state.back());
    //     } break;
    // }

    DEBUG cout << str << "Resource Data Length: 0x" << std::hex << mainData.dataLen << endl;
    DEBUG cout << str << "Resource File Data Length: 0x" << std::hex << mainData.fileLen << endl;
    DEBUG cout << str << "Resource Data Location: 0x" << std::hex << mainData.location << endl;
    DEBUG cout << str << "Pre Data Length: 0x" << std::hex << preData.fileLen << endl;
    if (mainData.compressed)
        DEBUG cout << str << "Has compressed data" << endl;
    else
        DEBUG cout << str << "No compressed data" << endl;
    // Must read in order to move the stream position correctly
    //readCompressed(strm, dataLen, compressedDataLen, &compressedData);
    //strm.position += mainData.fileLen;
    if (state.back() == STATE_NOCHILD) { state.pop_back(); return; }
    state.push_back(ID);
    DEBUG cout << str << "Fetching Child Objects" << endl;
    while (strm.position < strm.data->size())
    {
        ResourceEntry chunk;
        bool more = fetchChild(strm, state, &chunk);
        if (!more) break;
        DEBUG cout << str << "Got Child: 0x" << std::hex << chunk.ID << endl;
        setChild(strm, state, chunk);
    }
    state.pop_back();
}

bool ResourceEntry::fetchChild(MemoryStream& strm, vector<uint16_t>& state, ResourceEntry* chunk)
{
    switch(state.back()) {
        case CHUNK_HEADER:
        case CHUNK_OBJHEAD:{
            *chunk = ResourceEntry(strm, state);
            return chunk->ID != CHUNK_LAST; //we still need to consume the last chunk, but wont add it in
        } break;
        case CHUNK_FRAME: {
            *chunk = ResourceEntry(strm, state);
            return chunk->ID > CHUNK_FRAME && chunk->ID <= CHUNK_FRAMEIPHONEOPTS;
        } break;
        case CHUNK_OBJECTBANK: {
            size_t pos = strm.position;
            *chunk = ResourceEntry(strm, state);
            if (chunk->ID != CHUNK_OBJHEAD){ strm.position = pos; return false; }
            return true;
        } break;
        case CHUNK_IMAGEBANK: {
            //state.push_back(STATE_IMAGE);
            *chunk = ResourceEntry(strm, state);
            //state.pop_back();
            return chunk->ID != CHUNK_ENDIMAGE;
        } break;
        case CHUNK_FONTBANK: {
            //state.push_back(STATE_FONT);
            *chunk = ResourceEntry(strm, state);
            //state.pop_back();
            return chunk->ID != CHUNK_ENDFONT;
        } break;
        case CHUNK_SOUNDBANK: {
            //state.push_back(STATE_SOUND);
            *chunk = ResourceEntry(strm, state);
            //state.pop_back();
            return chunk->ID != CHUNK_ENDSOUND;
        } break;
        case CHUNK_MUSICBANK: {
            //state.push_back(STATE_MUSIC);
            *chunk = ResourceEntry(strm, state);
            //state.pop_back();
            return chunk->ID != CHUNK_ENDMUSIC;
        } break;
        case STATE_NEW:
        case STATE_OLD:
        case STATE_VITA:
        case STATE_DEFAULT:
        case STATE_IMAGE:
        case STATE_SOUND:
        case STATE_MUSIC:
        case STATE_FONT:
        default: {
            return false;
            // switch(ID) {
            //     default: {
            //         return false;
            //     } break;
            // }
        } break;
    }
    return false; //return true if there is more to read
}

void ResourceEntry::setChild(MemoryStream& strm, vector<uint16_t>& state, ResourceEntry& chunk)
{
    bool old = true;
    for (auto it = state.begin(); it != state.end(); it++)
    {
        if (*it == STATE_NEW) { old = false; break; }
        else if (*it == STATE_OLD) { old = false; break; }
    }
    switch(state.back()) {
        //case CHUNK_OLD_HEADER:
        case CHUNK_HEADER: {
            if (old) goto oldobj;
            else goto newobj;
        } break;
        case STATE_OLD: {
            oldobj:
            switch(chunk.ID) {
                // case CHUNK_OLD_HEADER: {
                //     ((GameEntry*)extraData)->header = chunk;
                // } break;
                // case CHUNK_OLD_FRAMEITEMS: {
                //     ((GameEntry*)extraData)->objectBank = chunk;
                // } break;
                // case CHUNK_OLD_FRAME: {
                //     //((GameEntry*)extraData)->frameBank.push_back(chunk);
                //     chunks.push_back(chunk);
                // } break;
                default: {
                   goto newobj; //I'm so sorry...
                } break;
            }
        } break;
        case STATE_NEW: {
            newobj:
            switch(chunk.ID) {
                // case CHUNK_HEADER: {
                //     ((GameEntry*)extraData)->header = chunk;
                // } break;
                // case CHUNK_EXTDHEADER: {
                //     ((GameEntry*)extraData)->extendedHeader = chunk;
                // } break;
                // case CHUNK_TITLE: {
                //     ((GameEntry*)extraData)->title = chunk;
                // } break;
                // case CHUNK_COPYRIGHT: {
                //     ((GameEntry*)extraData)->copyright = chunk;
                // } break;
                // case CHUNK_ABOUT: {
                //     ((GameEntry*)extraData)->aboutText = chunk;
                // } break;
                // case CHUNK_AUTHOR: {
                //     ((GameEntry*)extraData)->author = chunk;
                // } break;
                // case CHUNK_PROJPATH: {
                //     ((GameEntry*)extraData)->projectPath = chunk;
                // } break;
                // case CHUNK_OUTPATH: {
                //     ((GameEntry*)extraData)->outputPath = chunk;
                // } break;
                // case CHUNK_EXEONLY: {
                //     ((GameEntry*)extraData)->exeOnly = chunk;
                // } break;
                // case CHUNK_MENU: {
                //     ((GameEntry*)extraData)->menu = chunk;
                // } break;
                // case CHUNK_ICON: {
                //     ((GameEntry*)extraData)->icon = chunk;
                // } break;
                // case CHUNK_SHADERS: {
                //     ((GameEntry*)extraData)->shaders = chunk;
                // } break;
                // case CHUNK_GLOBALSTRS: {
                //     ((GameEntry*)extraData)->globalStrings = chunk;
                // } break;
                // case CHUNK_GLOBALVALS: {
                //     ((GameEntry*)extraData)->globalValues = chunk;
                // } break;
                // case CHUNK_EXTNLIST: {
                //     ((GameEntry*)extraData)->extensions = chunk;
                // } break;
                // case CHUNK_FRAMEHANDLES: {
                //     ((GameEntry*)extraData)->frameHandles = chunk;
                // } break;
                // case CHUNK_FRAME: {
                //     //((GameEntry*)extraData)->frameBank.push_back(chunk);
                //     chunks.push_back(chunk);
                // } break;
                // case CHUNK_OBJECTBANK: {
                //     ((GameEntry*)extraData)->objectBank = chunk;
                // } break;
                // case CHUNK_SOUNDBANK: {
                //     ((GameEntry*)extraData)->soundBank = chunk;
                // } break;
                // case CHUNK_MUSICBANK: {
                //     ((GameEntry*)extraData)->musicBank = chunk;
                // } break;
                // case CHUNK_FONTBANK: {
                //     ((GameEntry*)extraData)->fontBank = chunk;
                // } break;
                // case CHUNK_IMAGEBANK: {
                //     ((GameEntry*)extraData)->imageBank = chunk;
                // } break;
                // case CHUNK_SECNUM: {
                //     ((GameEntry*)extraData)->serial = chunk;
                // } break;
                // case CHUNK_BINFILES: {
                //     ((GameEntry*)extraData)->fileBank = chunk;
                // } break;
                default: {
                    chunks.push_back(chunk);
                } break;
            }
        } break;
        case STATE_DEFAULT:
        case CHUNK_IMAGEBANK:
        case CHUNK_SOUNDBANK:
        case CHUNK_MUSICBANK:
        case CHUNK_FONTBANK:
        default: {
            switch(chunk.ID) {
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
        else 
        {
            if (decompress != nullptr) *decompress = false;
            return compressed;
        }
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

DataPoint::DataPoint(){}

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

void DataPoint::operator()(size_t loc, size_t flen)
{
    location = loc;
    fileLen = flen;
    dataLen = 0;
    compressed = false;
}

void DataPoint::operator()(size_t loc, size_t flen, size_t alen)
{
    location = loc;
    fileLen = flen;
    dataLen = alen;
    compressed = true;
}

void DataPoint::operator()(MemoryStream& strm, size_t flen)
{
    location = strm.position;
    strm.position += flen;
    fileLen = flen;
    dataLen = 0;
    compressed = false;
}

void DataPoint::operator()(MemoryStream& strm, size_t flen, size_t alen)
{
    location = strm.position;
    strm.position += flen;
    fileLen = flen;
    dataLen = alen;
    compressed = true;
}

void DataPoint::clear()
{
    location = 0;
    fileLen = 0;
    dataLen = 0;
    compressed = false;
}

MemoryStream DataPoint::rawStream(vector<uint8_t>* memory)
{
    auto begin = memory->begin() + location;
    auto end = begin + fileLen;
    return MemoryStream(vector<uint8_t>(begin, end));
}

MemoryStream DataPoint::decompressedStream(vector<uint8_t>* memory)
{
    auto begin = memory->begin() + location;
    auto end = begin + fileLen;
    return MemoryStream(readCompressed(vector<uint8_t>(begin, end), dataLen));
}

MemoryStream DataPoint::read(vector<uint8_t>* memory)
{
    if (compressed)
        return decompressedStream(memory);
    else
        return rawStream(memory);
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
			if ((*strm.data)[pos + index1 + index2] != toFind[index2]) { flag = false; break; }
        }
        if (flag) return index1;
    }
    return -1;
}

void SourceExplorer::loadGame(string path)
{
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
        throw std::exception("You probably don't have enough RAM");
    }
    //if (fileSize != gameBuffer.data->size()) 
    DEBUG cout << "resize: 0x" << std::hex << gameBuffer.data->size() << endl;
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
    readGameData(gameState);
    //fetchEntry -> entry
    game = ResourceEntry(gameBuffer, gameState);
    //entry -> fetchChildren
    //entry -> readData
    //ResourceEntry.push_back(entry)

    //GameDeconstructor gd;
    //MemoryStream& gameStream = gd.fetchGameData(gameBuffer, &state);
    ///*GameEntry*/ ResourceEntry game = /*(GameEntry)*/gd.fetchEntry(gameStream, &state);
    //game.fetchChildren();
}

void SourceExplorer::readGameData(vector<uint16_t>& state)
{
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
        state.push_back(STATE_OLD);
    }
    else if (packMagic == HEADER_PACK)
    {
        state.push_back(STATE_NEW);
        pos = packData(strm, state);
    }
    else throw std::exception("Invalid Pack Header");

    uint32_t header = strm.readInt<uint32_t>();
    DEBUG cout << "Header: 0x" << std::hex << header << endl;

    if (header == HEADER_UNIC) unicode = true;
    else if (header != HEADER_GAME) throw std::exception("Invalid Game Header");

    gameData(strm);
    //strm.position = pos;
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
    uint16_t firstShort = strm.readInt<uint16_t>();
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
            return "None";
    }
}