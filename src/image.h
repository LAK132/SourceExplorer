#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include <istream>
#include <fstream>
#include <cmath>
extern "C" {
#include <GL/gl3w.h>
}

#include "lak.h"

#include "defines.h"

#ifndef IMAGE_H
#define IMAGE_H

namespace SourceExplorer
{
    // struct color_t
    // {
    //     uint8_t r = 0, g = 0, b = 0, a = 255;
    // };

    lak::color4_t ColorFrom8bit(uint8_t RGB);
    lak::color4_t ColorFrom15bit(uint16_t RGB);
    lak::color4_t ColorFrom16bit(uint16_t RGB);

    lak::color4_t ColorFrom8bit(lak::memstrm_t &strm);
    lak::color4_t ColorFrom15bit(lak::memstrm_t &strm);
    lak::color4_t ColorFrom16bit(lak::memstrm_t &strm);
    lak::color4_t ColorFrom24bit(lak::memstrm_t &strm);
    lak::color4_t ColorFrom32bit(lak::memstrm_t &strm);

    lak::color4_t ColorFromMode(lak::memstrm_t &strm, const graphics_mode_t mode);

    struct bitmap_t
    {
        lak::vec2s_t size;
        std::vector<lak::color4_t> pixels;

        lak::color4_t &operator[](const size_t index);
        lak::color4_t &operator[](const lak::vec2s_t index);
        lak::color4_t &operator()(const size_t x, const size_t y);
        void resize(const lak::vec2s_t toSize);
    };

    uint16_t BitmapPaddingSize(
        uint16_t width,
        uint8_t colSize,
        uint8_t pad = 2
    );

    struct image_t
    {
        using handle_t = uint16_t;

        bool old = false;
        uint32_t dataSize = 0;
        handle_t handle = 0;
        uint16_t checksum; // uint8_t if old
        uint32_t reference = 0;
        lak::vec2u16_t size = {};
        graphics_mode_t graphicsMode = GRAPHICS4;
        image_flag_t flags = RLE;

        uint32_t count = 0;
        lak::vec2u16_t hotspot = {};
        lak::vec2u16_t action = {};

        uint8_t paletteEntries = 0;
        std::vector<lak::color4_t> palette = {};
        lak::color4_t transparent = {};
        bitmap_t bitmap = {};

        lak::glTexture_t texture;
        image_t &initTexture();
    };

    image_t CreateImage(
        lak::memstrm_t &strm,
        const bool old
    );

    // "fucked" version
    image_t CreateImage(
        lak::memstrm_t &strm,
        const bool old,
        const lak::vec2u16_t sizeOverride
    );

    GLuint CreateTexture(
        const image_t &image
    );

    void ViewImage(
        const image_t &image
    );
}

#endif
