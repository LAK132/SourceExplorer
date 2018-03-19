#include "explorer.h"

void SourceExplorer::loadGame(string path)
{
    gamePath = path;
    cout << "Game Path: " << gamePath << endl;

    gameDir = path.substr(0, path.find_last_of('\\') + 1);
    cout << "Game Dir: " << gameDir << endl;

    ifstream in(gamePath, ios::binary | ios::ate);
    size_t fileSize = in.tellg();
    cout << "File Size: 0x" << std::hex << fileSize << endl;

    gameBuffer.data->resize(fileSize);
    in.seekg(0);
    //in.get((char*)&((*gameBuffer.data)[0]), fileSize);
    for (size_t i = 0; i < fileSize; i++)
    {
        (*gameBuffer.data)[i] = in.get();
    }
    readEntries();
}

void SourceExplorer::readEntries()
{
    entry = true;
    readGameData(&gameState);
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