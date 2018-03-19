#include <stdint.h>
#include <iostream>
using std::cout;
using std::endl;
#include <assert.h>
#include <string>
using std::string;
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

    void loadGame(string path);
    void readEntries();
    void readGameData(uint16_t* state);
    uint64_t packData(MemoryStream& strm, uint16_t* state);
    void gameData(MemoryStream& strm);
    string product(uint16_t vnum);
};

#endif