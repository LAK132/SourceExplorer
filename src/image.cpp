#include "image.h"

Color::Color(){}

Color::Color(uint8_t R, uint8_t G, uint8_t B, uint8_t A) : r(R), g(G), b(B), a(A) {}

Color Color::from8bit(uint8_t RGB)
{
    Color rtn;
    rtn.r = (((RGB >> 4) & 0x7) * (0xFF / 0x3));    // 1110 0000
    rtn.g = (((RGB >> 2) & 0x7) * (0xFF / 0x3));    // 0001 1100
    rtn.b = ((RGB & 0x2) * (0xFF / 0x2));           // 0000 0011
    return rtn;
}

Color Color::from15bit(uint16_t RGB)
{
    Color rtn;
    rtn.r = (RGB & 0x7C00) >> 7;    // 0111 1100 0000 0000
    rtn.g = (RGB & 0x03E0) >> 2;    // 0000 0011 1110 0000
    rtn.b = (RGB & 0x001F) << 3;    // 0000 0000 0001 1111
    return rtn;
}

Color Color::from16bit(uint16_t RGB)
{
    Color rtn;
    rtn.r = (RGB & 0xF800) >> 7;    // 1111 1000 0000 0000
    rtn.g = (RGB & 0x07E0) >> 3;    // 0000 0111 1110 0000
    rtn.b = (RGB & 0x001F) << 3;    // 0000 0000 0001 1111
    return rtn;
}

Color Color::from8bit(MemoryStream& strm)
{
    return from8bit(strm.readInt<uint8_t>());
}

Color Color::from15bit(MemoryStream& strm)
{
    uint16_t val = strm.readInt<uint8_t>();
    val |= strm.readInt<uint8_t>() << 8;
    return from15bit(val);
}

Color Color::from16bit(MemoryStream& strm)
{
    uint16_t val = strm.readInt<uint8_t>();
    val |= strm.readInt<uint8_t>() << 8;
    return from15bit(val);
}

Color Color::from24bit(MemoryStream& strm)
{
    Color rtn;
    rtn.r = strm.readInt<uint8_t>();
    rtn.g = strm.readInt<uint8_t>();
    rtn.b = strm.readInt<uint8_t>();
    return rtn;
}

vector<uint8_t> Bitmap::toRGB()
{
    vector<uint8_t> rtn(w * h * 3);
    size_t i = 0;
    for (size_t y = 0; y < h; y++)
    {
        for (size_t x = 0; x < w; x++)
        {
            rtn[i++] = (*this)[x][y].b; //x/y reverse for OpenGL
            rtn[i++] = (*this)[x][y].g;
            rtn[i++] = (*this)[x][y].r;
        }
    }
    return rtn;
}

vector<uint8_t> Bitmap::toRGBA()
{
    vector<uint8_t> rtn(w * h * 4);
    size_t i = 0;
    for (size_t y = 0; y < h; y++)
    {
        for (size_t x = 0; x < w; x++)
        {
            rtn[i++] = (*this)[x][y].b; //x/y reverse for OpenGL
            rtn[i++] = (*this)[x][y].g;
            rtn[i++] = (*this)[x][y].r;
            rtn[i++] = (*this)[x][y].a;
        }
    }
    return rtn;
}

Bitmap::Bitmap()
{
    pixels.clear();
}

Bitmap::Bitmap(size_t width, size_t height) : w(width), h(height)
{
    pixels.resize(w*h);
}

Color* Bitmap::operator[](size_t idx)
{
    return &(pixels[idx * h]);
}

Image::Image()
{
    palette.clear();
    glGenTextures(1, &tex);
    
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    #ifdef SMOOTH_TEX
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    #else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    #endif

    uint8_t pix[] = {125, 125, 125, 125};

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix);
}

Image::~Image()
{
    glDeleteTextures(1, &tex);
}

Color Image::getNext(MemoryStream& strm, uint8_t mode)
{
    switch(mode)
    {
        case 2:
        case 3:
            return Color::from8bit(strm);
        case 6:
            return Color::from15bit(strm);
        case 7:
            return Color::from16bit(strm);
        case 4:
        default:
            return Color::from24bit(strm);
    }
}

// vector<Color> getNext(MemoryStream& strm, uint8_t mode, uint32_t count)
// {
//     vector<Color> rtn(count);
//     for (auto it = rtn.begin(); it != rtn.end(); it++)
//     {
//         *it = getNext(strm, mode);
//     }
//     return rtn;
// }

void Image::getNext(MemoryStream& strm, uint8_t mode, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        getNext(strm, mode);
    }
}

uint8_t Image::getSize(uint8_t mode)
{
    switch(mode)
    {
        case 2:
        case 3:
            return 1;
        case 6:
        case 7:
            return 2;
        case 4:
        default:
            return 3;
    }
}

uint16_t Image::getPadding(uint16_t width, uint8_t colSize, uint8_t pad)
{
    uint16_t num = pad - ((width * colSize) % pad);
    return (num == pad ? 0 : num);
}

GLuint Image::generateImage(MemoryStream& strm, bool fucked, uint16_t widthovrd, uint16_t heightovrd)
{
    if (!fucked)
    {
        handle = strm.readInt<uint16_t>();
        if (!old) checksum = strm.readInt<uint16_t>();
        else checksum = strm.readInt<uint8_t>();
        reference = strm.readInt<uint32_t>();
        dataSize = strm.readInt<uint32_t>();
        width = strm.readInt<uint16_t>();
        height = strm.readInt<uint16_t>();
        graphicsMode = strm.readInt<uint8_t>();
        flags = strm.readInt<uint8_t>();
        /*if (graphicsMode <= 3)
        {
            paletteEntries = strm.readInt<uint8_t>();
            for(size_t i = 0; i < palette.size(); i++) // where is this size coming from???
            {
                palette[i].r = strm.readInt<uint8_t>(); // not sure about order
                palette[i].g = strm.readInt<uint8_t>();
                palette[i].b = strm.readInt<uint8_t>();
                palette[i].a = strm.readInt<uint8_t>();
            }
            cout = strm.readInt<uint32_t>();
        }*/
        if (!old) strm.readInt<uint16_t>(); // padding?
        xHotspot = strm.readInt<uint16_t>();
        yHotspot = strm.readInt<uint16_t>();
        xAction = strm.readInt<uint16_t>();
        yAction = strm.readInt<uint16_t>();
        if (!old)
        {
            transparency[0] = strm.readInt<uint8_t>();
            transparency[1] = strm.readInt<uint8_t>();
            transparency[2] = strm.readInt<uint8_t>();
            transparency[3] = strm.readInt<uint8_t>();
        }
        // size_t pos = strm.position; // why???
    }
    else 
    {
        width = widthovrd;
        height = heightovrd;
    }
    bitmap = Bitmap(width, height);

    uint8_t colSize = getSize(graphicsMode);
    uint16_t pad = getPadding(width, colSize);

    uint32_t n = 0;
    size_t pos = strm.position;
    uint32_t length = dataSize / colSize;

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            bitmap[x][y] = getNext(strm, graphicsMode);
            n += colSize;
        }
        getNext(strm, graphicsMode, pad); // skip
        n += pad * colSize;
        //if (pos + dataSize < strm.position) i = 0;
    }
    uint32_t leftover = dataSize - n;
    if ((flags & IMAGE_ALPHA) != 0)
    {
        uint32_t count = (leftover - (height * width)) / height;
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                bitmap[x][y].a = strm.readInt<uint8_t>();
            }
            strm.readBytes((count > 0 ? count : 0));
        }
    }
    
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    #ifdef SMOOTH_TEX
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    #else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    #endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.w, bitmap.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(bitmap.toRGBA()[0]));

    return tex;
}