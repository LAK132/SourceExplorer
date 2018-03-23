#include <stdint.h>
#include <iostream>
using std::cout;
using std::endl;
#include <assert.h>
#include <string>
using std::string;
#include <miniz.h>
#include <istream>
#include <fstream>
using std::ifstream;
using std::ios;
#include "defines.h"
#include "memorystream.h"
#include "image.h"
//#include "gamedeconstructor.h"
//#include "resources/resourceentry.h"

#ifndef EXPLORER_H
#define EXPLORER_H

struct PackFile
{
    string filename;
    uint32_t bingo;
    vector<uint8_t> data;
};

vector<uint8_t> readCompressed(MemoryStream& strm, uint32_t datalen, uint32_t complen, bool* decompress = nullptr);
vector<uint8_t> readCompressed(vector<uint8_t>& compressed, uint32_t datalen, bool* decompress = nullptr);
string readASCII(MemoryStream& strm);
string readUnicode(MemoryStream& strm);

struct DataPoint
{
    size_t location = 0;
    size_t dataLen = 0;
    size_t fileLen = 0;
    bool compressed = false;
    DataPoint();
    DataPoint(size_t loc, size_t flen);
    DataPoint(size_t loc, size_t flen, size_t alen);
    DataPoint(MemoryStream& strm, size_t flen);
    DataPoint(MemoryStream& strm, size_t flen, size_t alen);
    void operator()(size_t loc, size_t flen);
    void operator()(size_t loc, size_t flen, size_t alen);
    void operator()(MemoryStream& strm, size_t flen);
    void operator()(MemoryStream& strm, size_t flen, size_t alen);
    void clear();
    MemoryStream read(vector<uint8_t>* memory);
    MemoryStream rawStream(vector<uint8_t>* memory);
    MemoryStream decompressedStream(vector<uint8_t>* memory);
};

struct ResourceEntry
{
    uint16_t ID = 0xFFFF;
    uint16_t mode = 0xFFFF;
    size_t location = 0;

    DataPoint preData;
    DataPoint mainData;

    //uint64_t dataLoc;
    //uint32_t dataLen = 0;
    //uint32_t compressedDataLen = 0;
    //vector<uint8_t> preData;
    //vector<ResourceEntry*> chunks; //glfw3 is fucking with these pointers
    vector<ResourceEntry> chunks;
    void* extraData = nullptr;

    ResourceEntry();
    ResourceEntry(MemoryStream& strm, vector<uint16_t>& state);
    ~ResourceEntry();
    uint32_t findNext(MemoryStream& strm);
    int32_t findUntilNext(MemoryStream& strm, uint32_t pos, const vector<uint8_t>& toFind);
    bool fetchChild(MemoryStream& strm, vector<uint16_t>& state, ResourceEntry* chunk);
    void setChild(MemoryStream& strm, vector<uint16_t>& state, ResourceEntry& chunk);
};

struct GameEntry
{
    ResourceEntry header;
    ResourceEntry title;
    ResourceEntry copyright;
    ResourceEntry author;
    ResourceEntry projectPath;
    ResourceEntry outputPath;
    ResourceEntry icon;
    ResourceEntry shaders;
    ResourceEntry extensions;
    ResourceEntry serial;
    ResourceEntry exeOnly;
    ResourceEntry menu;
    ResourceEntry frameHandles;

    ResourceEntry soundBank;
    ResourceEntry musicBank;
    ResourceEntry fontBank;
    ResourceEntry imageBank;
    ResourceEntry objectBank;

    ResourceEntry extendedHeader;
    ResourceEntry aboutText;
    ResourceEntry globalStrings;
    ResourceEntry globalValues;
    ResourceEntry fileBank;

    vector<ResourceEntry> frameBank;
};

struct FrameEntry
{
    ResourceEntry* frameHeaderEntry = nullptr;
    ResourceEntry* nameEntry = nullptr;
    ResourceEntry* objectsEntry = nullptr;
};

class SourceExplorer
{
public:
    vector<PackFile> packFiles;
    uint16_t numHeaderSections;
    string gamePath;
    string gameDir;
    MemoryStream gameBuffer;
    uint64_t dataPos;
    //vector<ResourceEntry> resources;
    vector<uint64_t> resourcePos;
    bool addingTextures;
    bool addingSounds;
    bool addingFonts;
    bool entry;
    vector<uint16_t> gameState;
    bool unicode = false;
    bool oldGame;
    bool cnc = false;
    uint16_t numSections;
    uint16_t runtimeVersion;
    uint16_t runtimeSubVersion;
    uint32_t productVersion;
    uint32_t productBuild;
    ResourceEntry game;

    void loadGame(string path);
    void readEntries();
    void readGameData(vector<uint16_t>& state);
    uint64_t packData(MemoryStream& strm, vector<uint16_t>& state);
    void gameData(MemoryStream& strm);
    string product(uint16_t vnum);
};

#endif