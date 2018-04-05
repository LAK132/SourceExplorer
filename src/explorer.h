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
// #include "tinf.h"
#include "imgui.h"
#include "defines.h"
#include "memorystream.h"
#include "image.h"
//#include "gamedeconstructor.h"
//#include "resources/resourceentry.h"

#ifndef EXPLORER_H
#define EXPLORER_H

extern bool debugConsole;
extern bool throwErrors;

struct PackFile
{
    string filename;
    uint32_t bingo;
    vector<uint8_t> data;
};

class SourceExplorer;

struct renderMenu_t
{
    SourceExplorer* srcexp = nullptr;
    vector<uint8_t>* memedit = nullptr;
    string* errtxt = nullptr;
    Image* viewImage = nullptr;
};

//
// Save Entry DataPoint -> skip over containing data ->
// Save Main DataPoints -> skip over containing data
// When needed create inner DataPoints from Main DataPoint MemoryStream
//

vector<uint8_t> readCompressed(MemoryStream* strm, uint32_t datalen, uint32_t complen, bool* decompress = nullptr);
vector<uint8_t> readCompressed(vector<uint8_t>* compressed, uint32_t datalen, bool* decompress = nullptr);
string readASCII(MemoryStream* strm);
string readUnicode(MemoryStream* strm);
string readString(MemoryStream* strm, bool unicode = true);

struct DataPoint
{
    uint16_t ID = 0xFFFF;
    uint16_t mode = 0xFFFF;
    size_t location = -1;
    size_t fileLen = 0;

    size_t dataLocation = -1;
    size_t defDataLen = 0;
    size_t infDataLen = 0;

    bool compressed = false;
    DataPoint();
    DataPoint(MemoryStream* strm);
    DataPoint(size_t loc, size_t flen);
    DataPoint(size_t loc, size_t flen, size_t alen);
    DataPoint(MemoryStream* strm, size_t flen);
    DataPoint(MemoryStream* strm, size_t flen, size_t alen);
    void operator()(MemoryStream* strm);
    void operator()(size_t loc, size_t flen);
    void operator()(size_t loc, size_t flen, size_t alen);
    void operator()(MemoryStream* strm, size_t flen);
    void operator()(MemoryStream* strm, size_t flen, size_t alen);
    void clear();
    void getData(MemoryStream* strm, size_t offset = 0);// bool comp = true);
    MemoryStream read(vector<uint8_t>* memory);
    MemoryStream rawStream(vector<uint8_t>* memory);
    MemoryStream decompressedStream(vector<uint8_t>* memory);
};

struct Keys
{
    uint16_t up, down, left, right, btn1, btn2, btn3, btn4;
    Keys(){}
    Keys(MemoryStream* strm){
        up = strm->readInt<uint16_t>();
        down = strm->readInt<uint16_t>();
        left = strm->readInt<uint16_t>();
        right = strm->readInt<uint16_t>();
        btn1 = strm->readInt<uint16_t>();
        btn2 = strm->readInt<uint16_t>();
        btn3 = strm->readInt<uint16_t>();
        btn4 = strm->readInt<uint16_t>();
    }
};

struct PlayerControls
{
    const static size_t numControls = 4;
    uint16_t controlType[numControls];
    Keys keys[numControls];
    PlayerControls(){}
    PlayerControls(MemoryStream* strm){
        for(size_t i = 0; i < numControls; i++){
            controlType[i] = strm->readInt<uint16_t>();
        }
        for(size_t i = 0; i < numControls; i++){
            keys[i] = Keys(strm);
        }
    }
};

struct AppHeader : public DataPoint
{
    uint32_t size;
    uint16_t flags;
    uint16_t newFlags;
    uint16_t graphicsMode;          // graphics modes?
    uint16_t otherFlags;
    uint16_t windowWidth;
    uint16_t windowHeight;
    uint32_t initialScore;
    uint32_t initialLives;
    PlayerControls controls;
    // uint8_t borderColor[4];
    Color borderColor;
    uint32_t numFrames;
    uint32_t frameRate;
    uint8_t windowsMenuIndex;
    AppHeader(){}
    AppHeader(MemoryStream* strm){ (*this)(strm); }
    void operator()(MemoryStream* strm){
        getData(strm);//, 0, (mode & MODE_FLAG_COMPRESSED) != 0);
        MemoryStream& strm2 = read(strm->data);
        size = strm2.readInt<uint32_t>();
        flags = strm2.readInt<uint16_t>();
        newFlags = strm2.readInt<uint16_t>();
        graphicsMode = strm2.readInt<uint16_t>();
        otherFlags = strm2.readInt<uint16_t>();
        windowWidth = strm2.readInt<uint16_t>();
        windowHeight = strm2.readInt<uint16_t>();
        initialScore = strm2.readInt<uint32_t>() ^ 0xFFFFFFFF;
        initialLives = strm2.readInt<uint32_t>() ^ 0xFFFFFFFF;
        controls = PlayerControls(&strm2);
        // borderColor[0] = strm->readInt<uint8_t>();
        // borderColor[1] = strm->readInt<uint8_t>();
        // borderColor[2] = strm->readInt<uint8_t>();
        // borderColor[3] = strm->readInt<uint8_t>();
        borderColor = Color::from32bit(&strm2);
        numFrames = strm2.readInt<uint32_t>();
        frameRate = strm2.readInt<uint32_t>();
        windowsMenuIndex = strm2.readInt<uint8_t>();
    }
    void operator=(const DataPoint& rhs){ ID = rhs.ID; mode = rhs.mode; location = rhs.location; fileLen = rhs.fileLen; dataLocation = rhs.dataLocation; defDataLen = rhs.defDataLen; infDataLen = infDataLen; }
};

struct ExtendedHeader : public DataPoint
{
    uint32_t flags;
    uint32_t buildType;
    uint32_t buildFlags;
    uint16_t screenRatioTolerance;
    uint16_t screenAngle;
    ExtendedHeader(){}
    ExtendedHeader(MemoryStream* strm){ (*this)(strm); }
    void operator()(MemoryStream* strm){
        getData(strm);//, 0, (mode & MODE_FLAG_COMPRESSED) != 0);
        MemoryStream& strm2 = read(strm->data);
		strm2.position = location;
        flags = strm2.readInt<uint32_t>();
        buildType = strm2.readInt<uint32_t>();
        buildFlags = strm2.readInt<uint32_t>();
        screenRatioTolerance = strm2.readInt<uint16_t>();
        screenAngle = strm2.readInt<uint16_t>();
        // strm2.position += 0x4; // no idea tbh
    }
    void operator=(const DataPoint& rhs){ ID = rhs.ID; mode = rhs.mode; location = rhs.location; fileLen = rhs.fileLen; dataLocation = rhs.dataLocation; defDataLen = rhs.defDataLen; infDataLen = infDataLen; }
};

struct FramePalette : public DataPoint
{
    uint32_t unk;
    Color items[256];
    FramePalette(){}
    FramePalette(MemoryStream* strm){ (*this)(strm); }
    void operator()(MemoryStream* strm){
        strm->position = location;
        unk = strm->readInt<uint32_t>();
        for(auto col = &(items[0]); col != &(items[255]); col++)
            *col = Color::from32bit(strm);
    }
    void operator=(const DataPoint& rhs){ ID = rhs.ID; mode = rhs.mode; location = rhs.location; fileLen = rhs.fileLen; dataLocation = rhs.dataLocation; defDataLen = rhs.defDataLen; infDataLen = infDataLen; }
};

struct ObjectInstance
{
    uint16_t handle;
    uint16_t objectInfo;
    uint32_t x, y;
    uint16_t parentType;
    uint16_t parentHandle;
    uint16_t layer;
    uint16_t unk;
    ObjectInstance(){}
    ObjectInstance(MemoryStream* strm){
        handle = strm->readInt<uint16_t>();
        objectInfo = strm->readInt<uint16_t>();
        x = strm->readInt<uint32_t>();
        y = strm->readInt<uint32_t>();
        parentType = strm->readInt<uint16_t>();
        parentHandle = strm->readInt<uint16_t>();
        layer = strm->readInt<uint16_t>();
        unk = strm->readInt<uint16_t>();
    }
};

struct ObjectInstances : public DataPoint
{
    vector<ObjectInstance> items;
    uint32_t unk;
    ObjectInstances(){ items.clear(); }
    ObjectInstances(MemoryStream* strm){ (*this)(strm); }
    void operator()(MemoryStream* strm){
        /*if (mode == MODE_FLAG_COMPRESSED)*/ getData(strm);//, 0, (mode & MODE_FLAG_COMPRESSED) != 0);
        MemoryStream& strm2 = read(strm->data);
        items.resize(strm2.readInt<uint32_t>());
        for(auto obj = items.begin(); obj != items.end(); obj++)
            *obj = ObjectInstance(&strm2);
        unk = strm2.readInt<uint32_t>();
    }
    void operator=(const DataPoint& rhs){ ID = rhs.ID; mode = rhs.mode; location = rhs.location; fileLen = rhs.fileLen; dataLocation = rhs.dataLocation; defDataLen = rhs.defDataLen; infDataLen = infDataLen; }
};

struct Frame : public DataPoint
{
    string namestr = "";
    DataPoint name; //FrameName
    string passwordstr = "";
    DataPoint password; //FramePassword
    DataPoint header; //FrameHeader
    //  width
    //  height
    //  background
    //  flags
    DataPoint virtSize; //VirtualSize
    //  top
    //  bottom
    //  left
    //  right
    ObjectInstances instances; //ObjectInstances
    DataPoint layers; //Layers
    DataPoint layerEffects; //LayerEffects
    DataPoint events; //Events
    uint16_t maxObjects; //events.maxObjects, >300
    // Color unkColor; //unkColor
    // Color palette[256]; //FramePalette
    FramePalette palette; //FramePalette
    DataPoint movementTimer; //MovementTimerBase.value
    DataPoint fadeIn; //FadeIn
    DataPoint fadeOut; //FadeOut
    //checksum
    Frame(){}
    Frame(MemoryStream* strm, bool unicode){ (*this)(strm, unicode); }
    void operator()(MemoryStream* strm, bool unicode){
		strm->position = location;
        while(strm->position < location + fileLen && strm->position < strm->data->size())
        {
            DataPoint dp(strm);
            if (dp.ID < CHUNK_VITAPREV || dp.ID > CHUNK_LAST)
                throw std::exception("Invalid Mode");
            if (dp.mode != MODE_0 && dp.mode != MODE_1 && dp.mode != MODE_2 && dp.mode != MODE_3)
                throw std::exception("Invalid State/ID");
            switch(dp.ID)
            {
                case CHUNK_FRAMENAME: name = dp; break;
                case CHUNK_FRAMEPASSWORD: password = dp; break;
                // case CHUNK_FRAMEHEADER: header = dp; break;
                // case CHUNK_FRAMEVIRTSIZE: virtSize = dp; break;
                case CHUNK_OBJINST: instances = dp; break;
                // case CHUNK_FRAMELAYERS: layers = dp; break;
                // case CHUNK_FRAMELAYEREFFECT: layerEffects = dp; break;
                // case CHUNK_FRAMEEVENTS: events = dp; break;
                case CHUNK_FRAMEPALETTE: palette = dp; break;
                // case CHUNK_MOVETIMEBASE: movementTimer = dp; break;
                // case CHUNK_FRAMEFADEI: fadeIn = dp; break;
                // case CHUNK_FRAMEFADEO: fadeOut = dp; break;
                default: continue;
            }
        }
        if(name.location != -1) {
            bool comp = (name.mode & MODE_FLAG_COMPRESSED) != 0;
            if (comp) name.getData(strm);//, 0, comp);
            namestr = readString(&name.read(strm->data), unicode);
        }
        // if(password.location != -1) password(strm);
        // if(header.locaiton != -1) header(strm);
        if(instances.location != -1) {
            instances(strm);
        }
        // if(palette.location != -1) palette(strm);
    }
    void operator=(const DataPoint& rhs){ ID = rhs.ID; mode = rhs.mode; location = rhs.location; fileLen = rhs.fileLen; dataLocation = rhs.dataLocation; defDataLen = rhs.defDataLen; infDataLen = infDataLen; }
};

struct ObjectHeader : public DataPoint
{
    uint16_t handle;
    uint16_t objType;
    uint16_t flags;
    uint16_t reserved;
    uint32_t inkEffect;
    uint32_t inkEffectParam;
    ObjectHeader(){}
    ObjectHeader(MemoryStream* strm) : DataPoint(strm) { (*this)(strm); } //need to add build version and 'compat'
    void operator()(MemoryStream* strm){
		if (ID == CHUNK_LAST) return;
        getData(strm);//, 0, mode);//mode == MODE_FLAG_COMPRESSED);
        MemoryStream& strm2 = read(strm->data);
        handle = strm2.readInt<uint16_t>();
        objType = strm2.readInt<uint16_t>();
        flags = strm2.readInt<uint16_t>();
        reserved = strm2.readInt<uint16_t>();
        inkEffect = strm2.readInt<uint32_t>();
        inkEffectParam = strm2.readInt<uint32_t>();
        strm->position = location + fileLen;
		while (DataPoint(strm).ID != CHUNK_LAST);
    }
    void operator=(const DataPoint& rhs){ ID = rhs.ID; mode = rhs.mode; location = rhs.location; fileLen = rhs.fileLen; dataLocation = rhs.dataLocation; defDataLen = rhs.defDataLen; infDataLen = infDataLen; }
};

struct ObjectBank : public DataPoint
{
    vector<ObjectHeader> items;
    ObjectBank(){ items.clear(); }
    ObjectBank(MemoryStream* strm){ (*this)(strm); }
    void operator()(MemoryStream* strm){
		if ((mode & MODE_FLAG_COMPRESSED) != 0)
		{
			getData(strm);//, 0, (mode & MODE_FLAG_COMPRESSED) != 0);
			MemoryStream& strm2 = read(strm->data);
			items.resize(strm2.readInt<uint32_t>());
			strm->position = dataLocation + defDataLen;
		}
		else
		{
			strm->position = dataLocation;
			items.resize(strm->readInt<uint32_t>());
		}
        for(auto it = items.begin(); it != items.end(); it++){
            *it = ObjectHeader(strm);
        }
    }
    void operator=(const DataPoint& rhs){ ID = rhs.ID; mode = rhs.mode; location = rhs.location; fileLen = rhs.fileLen; dataLocation = rhs.dataLocation; defDataLen = rhs.defDataLen; infDataLen = infDataLen; }
};

struct SourceExplorer
{
    vector<PackFile> packFiles;
    uint16_t numHeaderSections;
    string gamePath;
    string gameDir;
    MemoryStream gameBuffer;
    // ResourceEntry game;
    uint64_t dataPos;
    //vector<ResourceEntry> resources;
    // vector<uint64_t> resourcePos;
    vector<uint16_t> gameState;
    bool unicode = false;
    bool oldGame;
    bool cnc = false;
    uint16_t numSections;
    uint16_t runtimeVersion;
    uint16_t runtimeSubVersion;
    uint32_t productVersion;
    uint32_t productBuild;
    bool loaded = false;
    size_t dataLocation = 0;

    void loadIntoMem(string path);
    //void readEntries();
    void loadFromMem();
    uint64_t packData(MemoryStream* strm, vector<uint16_t>* state);
    void gameData(MemoryStream* strm);
    string product(uint16_t vnum);
    
    DataPoint game;
    AppHeader gameHeader;
    DataPoint title;
    string titlestr = "";
    DataPoint author;
    string authorstr = "";
    DataPoint copyright;
    string copyrightstr = "";
    vector<Frame> frame;
    ObjectBank objectBank;
    DataPoint imageBank;
    vector<DataPoint> image;
    DataPoint soundBank;
    vector<DataPoint> sound;
    DataPoint musicBank;
    vector<DataPoint> music;
    DataPoint fontBank;
    vector<DataPoint> font;

    void getEntries(MemoryStream* strm, vector<uint16_t>* state);
    void draw(renderMenu_t rm);
};

#endif