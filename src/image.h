#include <stdint.h>
#include <iostream>
using std::cout;
using std::endl;
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <miniz.h>
#include <istream>
#include <fstream>
using std::ifstream;
using std::ios;
#include "defines.h"
#include "memorystream.h"

#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#ifndef IMAGE_H
#define IMAGE_H

struct Color
{
    uint8_t r = 0, g = 0, b = 0, a = 255;
    Color();
    Color(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255);
    static Color from8bit(uint8_t RGB);
    static Color from15bit(uint16_t RGB);
    static Color from16bit(uint16_t RGB);
    static Color from8bit(MemoryStream& strm);
    static Color from15bit(MemoryStream& strm);
    static Color from16bit(MemoryStream& strm);
    static Color from24bit(MemoryStream& strm);
};

struct Bitmap
{
    size_t w, h;
    vector<Color> pixels;
    Bitmap();
    Bitmap(size_t width, size_t height);
    vector<uint8_t> toRGB();
    vector<uint8_t> toRGBA();
    Color* operator[](size_t idx);
};

class Image
{
public:
    bool old = false;
    uint32_t dataSize = 0;
    uint16_t handle = 0;
    uint16_t checksum; //uint8_t for old
    uint32_t reference = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    uint8_t graphicsMode = 4;
    uint8_t flags = 0;
    uint8_t paletteEntries = 0;
    vector<Color> palette;
    uint32_t count = 0;
    uint16_t xHotspot = 0;
    uint16_t yHotspot = 0;
    uint16_t xAction = 0;
    uint16_t yAction = 0;
    uint8_t transparency[4];

    Bitmap bitmap;
    GLuint tex;

    Image();
    ~Image();
    GLuint generateImage(MemoryStream& strm, bool fucked = false, uint16_t widthovrd = 0, uint16_t heightovrd = 0); //returns tex

    static Color getNext(MemoryStream& strm, uint8_t mode);
    //static vector<Color> getNext(MemoryStream& strm, uint8_t mode, uint32_t count);
    static void getNext(MemoryStream& strm, uint8_t mode, uint32_t count);
    static uint8_t getSize(uint8_t mode);
    static uint16_t getPadding(uint16_t width, uint8_t colSize, uint8_t pad = 2);
};

#endif