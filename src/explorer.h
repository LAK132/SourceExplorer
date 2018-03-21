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

struct ResourceEntry
{
    uint16_t ID;
    uint16_t mode;
    uint64_t dataLoc;
    uint32_t dataLen = 0;
    uint32_t compressedDataLen = 0;
    vector<uint8_t> preData;
    vector<ResourceEntry*> chunks;
    bool hasData;
    bool compressedData;
    void* extraData = nullptr;

    ResourceEntry();
    ResourceEntry(MemoryStream& strm, uint16_t* state);
    ~ResourceEntry();
    vector<uint8_t> readCompressed(MemoryStream& strm, uint32_t datalen, uint32_t complen, bool* decompress = nullptr);
    uint32_t findNext(MemoryStream& strm);
    int32_t findUntilNext(MemoryStream& strm, uint32_t pos, const vector<uint8_t>& toFind);
    bool fetchChild(MemoryStream& strm, uint16_t* state, ResourceEntry** chunk);
    void setChild(MemoryStream& strm, uint16_t* state, ResourceEntry* chunk);
};

struct GameEntry
{
    ResourceEntry* header = nullptr;
    ResourceEntry* title = nullptr;
    ResourceEntry* copyright = nullptr;
    ResourceEntry* author = nullptr;
    ResourceEntry* projectPath = nullptr;
    ResourceEntry* outputPath = nullptr;
    ResourceEntry* icon = nullptr;
    ResourceEntry* shaders = nullptr;
    ResourceEntry* extensions = nullptr;
    ResourceEntry* serial = nullptr;
    ResourceEntry* exeOnly = nullptr;
    ResourceEntry* menu = nullptr;
    ResourceEntry* frameHandles = nullptr;
    ResourceEntry* soundBank = nullptr;
    ResourceEntry* musicBank = nullptr;
    ResourceEntry* fontBank = nullptr;
    ResourceEntry* imageBank = nullptr;
    ResourceEntry* objectBank = nullptr;

    ResourceEntry* extendedHeader = nullptr;
    ResourceEntry* aboutText = nullptr;
    ResourceEntry* globalStrings = nullptr;
    ResourceEntry* globalValues = nullptr;
    ResourceEntry* fileBank = nullptr;

    vector<ResourceEntry*> frameBank;
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
    uint16_t gameState;
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
    void readGameData(uint16_t* state);
    uint64_t packData(MemoryStream& strm, uint16_t* state);
    void gameData(MemoryStream& strm);
    string product(uint16_t vnum);
};

#endif