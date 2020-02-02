// Copyright (c) Mathias Kaerlev 2012, LAK132 2019

// This file is part of Anaconda.

// Anaconda is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Anaconda is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Anaconda.  If not, see <http://www.gnu.org/licenses/>.

#include <string>

#ifndef DEFINES_H
#define DEFINES_H

// Executable Signature
static const uint16_t WIN_EXE_SIG = 0x5A4D;
// Position of Pointer to PE Header
static const uint8_t WIN_EXE_PNT = 0x3C;
// PE Signature
static const uint32_t WIN_PE_SIG = 0x00004550;

namespace SourceExplorer
{
    enum class error_t
    {
        OK = 0x0,

        INVALID_EXE_SIGNATURE   = 0x1,
        INVALID_PE_SIGNATURE    = 0x2,
        INVALID_GAME_HEADER     = 0x3,

        INVALID_STATE           = 0x4,
        INVALID_MODE            = 0x5,
        INVALID_CHUNK           = 0x6,

        NO_MODE0                = 0x7,
        NO_MODE1                = 0x8,
        NO_MODE2                = 0x9,
        NO_MODE3                = 0xA,

        OUT_OF_DATA             = 0xB
    };

    static const char *error_name(error_t err)
    {
        switch (err)
        {
            case error_t::OK:
                return "Ok";
            case error_t::INVALID_EXE_SIGNATURE:
                return "Invalid EXE Signature";
            case error_t::INVALID_PE_SIGNATURE:
                return "Invalid PE Signature";
            case error_t::INVALID_GAME_HEADER:
                return "Invalid Game Header";
            case error_t::INVALID_STATE:
                return "Invalid State";
            case error_t::INVALID_MODE:
                return "Invalid Mode";
            case error_t::INVALID_CHUNK:
                return "Invalid Chunk";
            case error_t::NO_MODE0:
                return "No MODE0";
            case error_t::NO_MODE1:
                return "No MODE1";
            case error_t::NO_MODE2:
                return "No MODE2";
            case error_t::NO_MODE3:
                return "No MODE3";
            case error_t::OUT_OF_DATA:
                return "Out Of Data";
            default:
                return "Invalid Error Code";
        }
    }

    //
    // Game Headers
    //
    static const uint64_t HEADER_PACK = 0x1247874977777777;
    // PAMU
    static const uint32_t HEADER_UNIC = 0x554D4150;
    // PAME
    static const uint32_t HEADER_GAME = 0x454D4150;

    //
    // Product code
    //
    enum class product_code_t : uint16_t
    {
        CNCV1VER= 0x0207,
        MMF1    = 0x0300,
        MMF15   = 0x0301,
        MMF2    = 0x0302
    };

    //
    // Old Data Chunks
    //
    enum class old_chunk_type_t : uint16_t
    {
        OLD_HEADER              = 0x2223,
        OLD_FRAMEITEMS          = 0x2229,
        // Dummy in some games (PA2)
        OLD_FRAMEITEMS2         = 0x223F,
        OLD_FRAME               = 0x3333,
        OLD_FRAMEHEADER         = 0x3334,
        // Object Instances
        OLD_OBJINST             = 0x3338,
        // Frame Events
        OLD_FRAMEEVENTS   = 0x333D,
        // Object Properties
        OLD_OBJPROP       = 0x4446
    };

    //
    // New Data Chunks
    //
    enum class chunk_t : uint16_t
    {
        //
        // States
        //
        DEFAULT     = 0x00,
        VITA        = 0x11,
        UNICODE     = 0x23,
        NEW         = 0x22,
        OLD         = 0x20,
        FRAME_STATE = 0x33,
        IMAGE_STATE = 0x66,
        FONT_STATE  = 0x67,
        SOUND_STATE = 0x68,
        MUSIC_STATE = 0x69,
        NOCHILD     = 0x80,
        SKIP        = 0x81,

        ENTRY           = 0x0302,
        // Vitalise Chunks (0x11XX)
        // Vitalise Preview
        VITAPREV        = 0x1122,
        // App Chunks (0x22XX)
        HEADER          = 0x2223,
        TITLE           = 0x2224,
        AUTHOR          = 0x2225,
        MENU            = 0x2226,
        EXTPATH         = 0x2227,
        // Deprecated // Not in Anaconda
        EXTENS          = 0x2228,
        // AKA FRAMEITEMS
        OBJECTBANK      = 0x2229,
        // Deprecated // Not in Anaconda
        GLOBALEVENTS    = 0x222A,
        FRAMEHANDLES    = 0x222B,
        EXTDATA         = 0x222C,
        // Deprecated // Not in Anaconda
        ADDEXTNS        = 0x222D,
        // Used for encryption
        PROJPATH        = 0x222E,
        OUTPATH         = 0x222F,
        APPDOC          = 0x2230,
        OTHEREXT        = 0x2231,
        GLOBALVALS      = 0x2232,
        GLOBALSTRS      = 0x2233,
        EXTNLIST        = 0x2234,
        ICON            = 0x2235,
        // Not in Anaconda
        DEMOVER         = 0x2236,
        SECNUM          = 0x2237,
        BINFILES        = 0x2238,
        // Not in Anaconda
        MENUIMAGES      = 0x2239,
        ABOUT           = 0x223A,
        COPYRIGHT       = 0x223B,
        // Not in Anaconda
        GLOBALVALNAMES  = 0x223C,
        // Not in Anaconda
        GLOBALSTRNAMES  = 0x223D,
        // Movement Extensions
        MOVEMNTEXTNS    = 0x223E,
        // UNKNOWN8        = 0x223F,
        // AKA FRAMEITEMS2
        OBJECTBANK2     = 0x223F,
        EXEONLY         = 0x2240,
        // 0x2241
        PROTECTION      = 0x2242,
        SHADERS         = 0x2243,
        // 0x2244
        EXTDHEADER      = 0x2245,
        SPACER          = 0x2246,
        // Means FRAMEHANDLES might be broken
        // Actually probably the Frame Bank
        FRAMEBANK       = 0x224D,
        CHUNK224F       = 0x224F,
        // "StringChunk" ?
        TITLE2          = 0x2251,
        // 16 bytes
        CHUNK2253       = 0x2253,
        // Recompiled games only (?), array of null terminated strings.
        OBJECTNAMES     = 0x2254,
        // "Empty"
        CHUNK2255       = 0x2255,
        // Recompiled games only (?)
        OBJECTPROPERTIES= 0x2256,
        // 4 bytes
        CHUNK2257       = 0x2257,
        // Recompiled games only (?)
        FONTMETA        = 0x2258,
        // Recompiled games only (?)
        FONTCHUNK       = 0x2259,
        // Frame Chunks (0x33XX)
        FRAME           = 0x3333,
        FRAMEHEADER     = 0x3334,
        FRAMENAME       = 0x3335,
        FRAMEPASSWORD   = 0x3336,
        FRAMEPALETTE    = 0x3337,
        // "WTF"?
        // OBJNAME2        = 0x3337,
        OBJINST         = 0x3338,
        // Frame Fade In Frame // Not in Anaconda
        FRAMEFADEIF     = 0x3339,
        // Frame Fade Out Frame // Not in Anaconda
        FRAMEFADEOF     = 0x333A,
        // Frame Fade In
        FRAMEFADEI      = 0x333B,
        // Frame Fade Out
        FRAMEFADEO      = 0x333C,
        // Frame Events
        FRAMEEVENTS     = 0x333D,
        // Frame Play Header // Not in Anaconda
        FRAMEPLYHEAD    = 0x333E,
        // Frame Additional Item // Not in Anaconda
        FRAMEADDITEM    = 0x333F,
        // Frame Additional Item Instance // Not in Anaconda
        FRAMEADDITEMINST= 0x3340,
        FRAMELAYERS     = 0x3341,
        FRAMEVIRTSIZE   = 0x3342,
        DEMOFILEPATH    = 0x3343,
        // Not in Anaconda
        RANDOMSEED      = 0x3344,
        FRAMELAYEREFFECT= 0x3345,
        // Frame BluRay Options // Not in Anaconda
        FRAMEBLURAY     = 0x3346,
        // Movement Timer Base
        MOVETIMEBASE    = 0x3347,
        // Mosaic Image Table // Not in Anaconda
        MOSAICIMGTABLE  = 0x3348,
        FRAMEEFFECTS    = 0x3349,
        // iPhone Options // Not in Anaconda
        FRAMEIPHONEOPTS = 0x334A,
        FRAMECHUNK334C  = 0x334C,
        // Object Chunks (0x44XX)
        // Not a data chunk
        PAERROR         = 0x4150,
        // Object Header
        OBJHEAD         = 0x4444,
        // Object Name
        OBJNAME         = 0x4445,
        // Object Properties
        OBJPROP         = 0x4446,
        // Object Unknown // Not in Anaconda
        OBJCHUNK4447    = 0x4447,
        // Object Effect
        OBJEFCT         = 0x4448,
        // Offset Chunks (0x55XX)
        ENDIMAGE        = 0x5555,
        ENDFONT         = 0x5556,
        ENDSOUND        = 0x5557,
        ENDMUSIC        = 0x5558,
        // Bank Chunks (0x66XX)
        IMAGEBANK       = 0x6666,
        FONTBANK        = 0x6667,
        SOUNDBANK       = 0x6668,
        MUSICBANK       = 0x6669,
        // Last Chunk
        LAST            = 0x7F7F
    };

    //
    // Modes
    //
    enum class encoding_t : uint16_t
    {
        // Uncompressed / Unencrypted
        MODE0 = 0,
        DEFAULT = MODE0,
        // Compressed / Unencrypted
        MODE1 = 1,
        COMPRESSED = MODE1,
        // Uncompressed / Encrypted
        MODE2 = 2,
        ENCRYPTED = MODE2,
        // Compressed / Encrypted
        MODE3 = 3,
        MODECOUNT
    };

    //
    // Sound Modes
    //
    enum class sound_mode_t : uint32_t
    {
        WAVE    = 1 << 0,
        MIDI    = 1 << 1,
        OGGS    = 1 << 2,
        // 1 << 3,
        LOC     = 1 << 4,
        PFD     = 1 << 5,
        LOADED  = 1 << 6
    };

    // static sound_mode_t operator | (const sound_mode_t A, const sound_mode_t B)
    // {
    //     return (sound_mode_t)((uint32_t)A | (uint32_t)B);
    // }

    //
    // Frame Header Data Flags
    //
    enum class frame_header_flag_t : uint16_t
    {
        DISPLAYNAME     = 1 << 0,
        GRABDESKTOP     = 1 << 1,
        KEEPDISPLAY     = 1 << 2,
        FADEIN          = 1 << 3,
        FADEOUT         = 1 << 4,
        TOTALCOLLISIONMASK = 1 << 5,
        PASSWORD        = 1 << 6,
        RESIZEATSTART   = 1 << 7,
        DONOTCENTER     = 1 << 8,
        FORCELOADONCALL = 1 << 9,
        NOSURFACE       = 1 << 10,
        RESERVED1       = 1 << 11,
        RESERVED2       = 1 << 12,
        RECORDDEMO      = 1 << 13,
        // 1 << 14,
        TIMEDMOVEMENTS  = 1 << 15
    };

    // static frame_header_flag_t operator | (const frame_header_flag_t A, const frame_header_flag_t B)
    // {
    //     return (frame_header_flag_t)((uint16_t)A | (uint16_t)B);
    // }

    //
    // Object Property Flags
    //
    enum class object_type_t : uint8_t // int8_t
    {
        QUICK_BACKDROP  = 0x00,
        BACKDROP        = 0x01,
        ACTIVE          = 0x02,
        TEXT            = 0x03,
        QUESTION        = 0x04,
        SCORE           = 0x05,
        LIVES           = 0x06,
        COUNTER         = 0x07,
        RTF             = 0x08,
        SUB_APPLICATION = 0x09,
        PLAYER          = 0xF9, // = -7,
        KEYBOARD        = 0xFA, // = -6,
        CREATE          = 0xFB, // = -5,
        TIMER           = 0xFC, // = -4,
        GAME            = 0xFD, // = -3,
        SPEAKER         = 0xFE, // = -2,
        SYSTEM          = 0xFF, // = -1,
    };

    enum class object_parent_type_t : uint16_t
    {
        NONE        = 0,
        FRAME       = 1,
        FRAME_ITEM  = 2,
        QUALIFIER   = 3
    };

    enum class shape_type_t : uint16_t
    {
        LINE        = 1,
        RECTANGLE   = 2,
        ELLIPSE     = 3
    };

    enum class fill_type_t : uint16_t
    {
        NONE        = 0,
        SOLID       = 1,
        GRADIENT    = 2,
        MOTIF       = 3
    };

    enum class line_flags_t : uint16_t
    {
        NONE        = 0,
        INVERSE_X   = 1 << 0,
        INVERSE_Y   = 1 << 1
    };

    enum class gradient_flags_t : uint16_t
    {
        HORIZONTAL  = 0,
        VERTICAL    = 1,
    };

    //
    // Image Properties
    //
    enum class graphics_mode_t : uint8_t
    {
        GRAPHICS2 = 2,
        GRAPHICS3 = 3,
        GRAPHICS6 = 6,
        GRAPHICS7 = 7,
        GRAPHICS4 = 4
    };

    enum class image_flag_t : uint8_t
    {
        NONE    = 0,
        RLE     = 1 << 0, // 0x1
        RLEW    = 1 << 1, // 0x2
        RLET    = 1 << 2, // 0x4
        LZX     = 1 << 3, // 0x8
        ALPHA   = 1 << 4, // 0x10
        ACE     = 1 << 5, // 0x20
        MAC     = 1 << 6  // 0x30
    };

    inline image_flag_t operator|(const image_flag_t A, const image_flag_t B)
    {
        return (image_flag_t)((uint8_t)A | (uint8_t)B);
    }

    inline image_flag_t operator&(const image_flag_t A, const image_flag_t B)
    {
        return (image_flag_t)((uint8_t)A & (uint8_t)B);
    }
}

#endif