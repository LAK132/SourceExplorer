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

#include <stdint.h>
#include <iostream>
#include <assert.h>
#include <string>
#include <istream>
#include <fstream>
#include <memory>
#include <atomic>
#include <stack>
#include <vector>
#include <unordered_map>
#include <iterator>

#include "imgui_impl_lak.h"
#include "lak.h"
#include <strconv/strconv.hpp>
#include "defines.h"
#include "tinflate.hpp"
#include "tinf.h"

#ifndef EXPLORER_H
#define EXPLORER_H

#include "encryption.h"
#include "image.h"

#ifndef DEBUG_LINE_FILE
#undef DEBUG_LINE_FILE
#endif
#ifdef _WIN32
#define DEBUG_LINE_FILE "(" << __FILE__ << ":" << std::dec << __LINE__ << ")"
#else
#define DEBUG_LINE_FILE "\x1B[2m(" << __FILE__ << ":" << std::dec << __LINE__ << ")\x1B[0m"
#endif

#ifndef WDEBUG_LINE_FILE
#undef WDEBUG_LINE_FILE
#endif
#ifdef _WIN32
#define WDEBUG_LINE_FILE L"(" << __FILE__ << L":" << std::dec << __LINE__ << L")"
#else
#define WDEBUG_LINE_FILE L"\x1B[2m(" << __FILE__ << L":" << std::dec << __LINE__ << L")\x1B[0m"
#endif

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG(x) if (SourceExplorer::debugConsole) { if (SourceExplorer::developerConsole) { std::cout << "DEBUG" << DEBUG_LINE_FILE << ": "; } std::cout << std::hex << x << "\n" << std::flush; }

#ifdef WDEBUG
#undef WDEBUG
#endif
#define WDEBUG(x) if (SourceExplorer::debugConsole) { if (SourceExplorer::developerConsole) { std::wcout << L"DEBUG" << DEBUG_LINE_FILE << L": "; } std::wcout << std::hex << x << L"\n" << std::flush; }

#ifdef ERROR
#undef ERROR
#endif
#ifdef _WIN32
#define ERROR(x) std::cerr << "ERROR" << DEBUG_LINE_FILE << ": " << std::hex << x << "\n" << std::flush;
#else
#define ERROR(x) std::cerr << "\x1B[91m\x1B[1mERROR\x1B[0m\x1B[91m" << DEBUG_LINE_FILE << "\x1B[91m: " << std::hex << x << "\x1B[0m\n" << std::flush;
#endif

#ifdef WERROR
#undef WERROR
#endif
#ifdef _WIN32
#define WERROR(x) std::wcerr << L"ERROR" << WDEBUG_LINE_FILE << L": " << std::hex << x << L"\n" << std::flush;
#else
#define WERROR(x) std::wcerr << L"\x1B[91m\x1B[1mERROR\x1B[0m\x1B[91m" << WDEBUG_LINE_FILE << L"\x1B[91m: " << std::hex << x << L"\x1B[0m\n" << std::flush;
#endif

// char8_t typdef for C++ < 20
#if __cplusplus <= 201703L
using char8_t = uint_least8_t;
namespace std
{
    using u8string = basic_string<char8_t>;
}
#endif

namespace SourceExplorer
{
    extern bool debugConsole;
    extern bool developerConsole;
    extern bool forceCompat;
    extern m128i_t _xmmword;
    extern std::vector<uint8_t> _magic_key;
    extern uint8_t _magic_char;

    enum class game_mode_t : uint8_t { _OLD, _284, _288 };
    extern game_mode_t _mode;

    struct game_t;
    struct source_explorer_t;

    struct pack_file_t
    {
        std::u16string filename;
        bool wide;
        uint32_t bingo;
        std::vector<uint8_t> data;
    };

    struct data_point_t
    {
        size_t position = 0;
        size_t expectedSize;
        lak::memory data;
        lak::memory decode(const chunk_t ID, const encoding_t mode) const;
    };

    struct basic_entry_t
    {
        union
        {
            uint32_t handle;
            chunk_t ID;
        };
        encoding_t mode;
        size_t position;
        size_t end;
        bool old;

        data_point_t header;
        data_point_t data;

        lak::memory decode() const;
        lak::memory decodeHeader() const;
        lak::memory raw() const;
        lak::memory rawHeader() const;
    };

    struct chunk_entry_t : public basic_entry_t
    {
        error_t read(game_t &game, lak::memory &strm);
        void view(source_explorer_t &srcexp) const;
    };

    struct item_entry_t : public basic_entry_t
    {
        error_t read(game_t &game, lak::memory &strm, bool compressed, size_t headersize = 0);
        void view(source_explorer_t &srcexp) const;
    };

    struct basic_chunk_t
    {
        chunk_entry_t entry;

        error_t read(game_t &game, lak::memory &strm);
        error_t basic_view(source_explorer_t &srcexp, const char *name) const;
    };

    struct basic_item_t
    {
        item_entry_t entry;

        error_t read(game_t &game, lak::memory &strm);
        error_t basic_view(source_explorer_t &srcexp, const char *name) const;
    };

    struct string_chunk_t : public basic_chunk_t
    {
        std::u16string value;

        error_t read(game_t &game, lak::memory &strm);
        error_t view(source_explorer_t &srcexp, const char *name, const bool preview = false) const;

        std::u16string u16string() const;
        std::u8string u8string() const;
        std::string string() const;
    };

    struct vitalise_preview_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct menu_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct extension_path_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct extensions_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct global_events_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct extension_data_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct additional_extensions_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct application_doc_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct other_extenion_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct global_values_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct global_strings_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct extension_list_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct icon_t : public basic_chunk_t
    {
        lak::image4_t bitmap;

        error_t read(game_t &game, lak::memory &strm);
        error_t view(source_explorer_t &srcexp) const;
    };

    struct demo_version_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct security_number_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct binary_files_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct menu_images_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct global_value_names_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct global_string_names_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct movement_extensions_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct object_bank2_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct exe_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct protection_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct shaders_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct extended_header_t : public basic_chunk_t
    {
        uint32_t flags;
        uint32_t buildType;
        uint32_t buildFlags;
        uint16_t screenRatioTolerance;
        uint16_t screenAngle;

        error_t read(game_t &game, lak::memory &strm);
        error_t view(source_explorer_t &srcexp) const;
    };

    struct spacer_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct chunk_224F_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct title2_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    struct last_t : public basic_chunk_t
    {
        error_t view(source_explorer_t &srcexp) const;
    };

    namespace object
    {
        struct effect_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct shape_t
        {
            fill_type_t fill;
            shape_type_t shape;
            line_flags_t line;
            gradient_flags_t gradient;
            uint16_t borderSize;
            lak::color4_t borderColor;
            lak::color4_t color1, color2;
            uint16_t handle;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct quick_backdrop_t : public basic_chunk_t
        {
            uint32_t size;
            uint16_t obstacle;
            uint16_t collision;
            lak::vec2u32_t dimension;
            shape_t shape;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct backdrop_t : public basic_chunk_t
        {
            uint32_t size;
            uint16_t obstacle;
            uint16_t collision;
            lak::vec2u32_t dimension;
            uint16_t handle;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct animation_direction_t
        {
            uint8_t minSpeed;
            uint8_t maxSpeed;
            uint16_t repeat;
            uint16_t backTo;
            std::vector<uint16_t> handles;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct animation_t
        {
            uint16_t offsets[32];
            animation_direction_t directions[32];

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct animation_header_t
        {
            uint16_t size;
            std::vector<uint16_t> offsets;
            std::vector<animation_t> animations;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct common_t : public basic_chunk_t
        {
            uint32_t size;

            uint16_t movementsOffset;
            uint16_t animationsOffset;
            uint16_t counterOffset;
            uint16_t systemOffset;
            uint32_t fadeInOffset;
            uint32_t fadeOutOffset;
            uint16_t valuesOffset;
            uint16_t stringsOffset;
            uint16_t extensionOffset;

            std::unique_ptr<animation_header_t> animations;

            uint16_t version;
            uint32_t flags;
            uint32_t newFlags;
            uint32_t preferences;
            uint32_t identifier;
            lak::color4_t backColor;

            game_mode_t mode;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct item_t : public basic_chunk_t // OBJHEAD
        {
            std::unique_ptr<string_chunk_t> name;
            std::unique_ptr<effect_t> effect;
            std::unique_ptr<last_t> end;

            uint16_t handle;
            object_type_t type;
            uint32_t inkEffect;
            uint32_t inkEffectParam;

            std::unique_ptr<quick_backdrop_t> quickBackdrop;
            std::unique_ptr<backdrop_t> backdrop;
            std::unique_ptr<common_t> common;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct bank_t : public basic_chunk_t
        {
            std::vector<item_t> items;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    namespace frame
    {
        struct header_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct password_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct palette_t : public basic_chunk_t
        {
            uint32_t unknown;
            lak::color4_t colors[256];

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct object_instance_t
        {
            uint16_t handle;
            uint16_t info;
            lak::vec2i32_t position;
            uint16_t parentType;
            uint16_t parentHandle;
            uint16_t layer;
            uint16_t unknown;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct object_instances_t : public basic_chunk_t
        {
            std::vector<object_instance_t> objects;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct fade_in_frame_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct fade_out_frame_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct fade_in_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct fade_out_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct events_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct play_head_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct additional_item_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct additional_item_instance_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct layers_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct virtual_size_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct demo_file_path_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct random_seed_t : public basic_chunk_t
        {
            int16_t value;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct layer_effect_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct blueray_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct movement_time_base_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct mosaic_image_table_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct effects_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct iphone_options_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct chunk_334C_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct item_t : public basic_chunk_t
        {
            std::unique_ptr<string_chunk_t> name;
            std::unique_ptr<header_t> header;
            std::unique_ptr<password_t> password;
            std::unique_ptr<palette_t> palette;
            std::unique_ptr<object_instances_t> objectInstances;
            std::unique_ptr<fade_in_frame_t> fadeInFrame;
            std::unique_ptr<fade_out_frame_t> fadeOutFrame;
            std::unique_ptr<fade_in_t> fadeIn;
            std::unique_ptr<fade_out_t> fadeOut;
            std::unique_ptr<events_t> events;
            std::unique_ptr<play_head_t> playHead;
            std::unique_ptr<additional_item_t> additionalItem;
            std::unique_ptr<additional_item_instance_t> additionalItemInstance;
            std::unique_ptr<layers_t> layers;
            std::unique_ptr<virtual_size_t> virtualSize;
            std::unique_ptr<demo_file_path_t> demoFilePath;
            std::unique_ptr<random_seed_t> randomSeed;
            std::unique_ptr<layer_effect_t> layerEffect;
            std::unique_ptr<blueray_t> blueray;
            std::unique_ptr<movement_time_base_t> movementTimeBase;
            std::unique_ptr<mosaic_image_table_t> mosaicImageTable;
            std::unique_ptr<effects_t> effects;
            std::unique_ptr<iphone_options_t> iphoneOptions;
            std::unique_ptr<chunk_334C_t> chunk334C;
            std::unique_ptr<last_t> end;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct handles_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct bank_t : public basic_chunk_t
        {
            std::vector<item_t> items;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    namespace image
    {
        struct item_t : public basic_item_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct end_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct bank_t : public basic_chunk_t
        {
            std::vector<item_t> items;
            std::unique_ptr<end_t> end;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    namespace font
    {
        struct item_t : public basic_item_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct end_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct bank_t : public basic_chunk_t
        {
            std::vector<item_t> items;
            std::unique_ptr<end_t> end;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    namespace sound
    {
        struct item_t : public basic_item_t
        {
            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct end_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct bank_t : public basic_chunk_t
        {
            std::vector<item_t> items;
            std::unique_ptr<end_t> end;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    namespace music
    {
        struct item_t : public basic_item_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct end_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct bank_t : public basic_chunk_t
        {
            std::vector<item_t> items;
            std::unique_ptr<end_t> end;

            error_t read(game_t &game, lak::memory &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    struct header_t : public basic_chunk_t
    {
        std::unique_ptr<string_chunk_t> title;
        std::unique_ptr<string_chunk_t> author;
        std::unique_ptr<string_chunk_t> copyright;
        std::unique_ptr<string_chunk_t> outputPath;
        std::unique_ptr<string_chunk_t> projectPath;

        std::unique_ptr<vitalise_preview_t> vitalisePreview;
        std::unique_ptr<menu_t> menu;
        std::unique_ptr<extension_path_t> extensionPath;
        std::unique_ptr<extensions_t> extensions; // deprecated
        std::unique_ptr<extension_data_t> extensionData;
        std::unique_ptr<additional_extensions_t> additionalExtensions;
        std::unique_ptr<application_doc_t> appDoc;
        std::unique_ptr<other_extenion_t> otherExtension;
        std::unique_ptr<extension_list_t> extensionList;
        std::unique_ptr<icon_t> icon;
        std::unique_ptr<demo_version_t> demoVersion;
        std::unique_ptr<security_number_t> security;
        std::unique_ptr<binary_files_t> binaryFiles;
        std::unique_ptr<menu_images_t> menuImages;
        std::unique_ptr<string_chunk_t> about;
        std::unique_ptr<movement_extensions_t> movementExtensions;
        std::unique_ptr<object_bank2_t> objectBank2;
        std::unique_ptr<exe_t> exe;
        std::unique_ptr<protection_t> protection;
        std::unique_ptr<shaders_t> shaders;
        std::unique_ptr<extended_header_t> extendedHeader;
        std::unique_ptr<spacer_t> spacer;
        std::unique_ptr<chunk_224F_t> chunk224F;
        std::unique_ptr<title2_t> title2;

        std::unique_ptr<global_events_t> globalEvents;
        std::unique_ptr<global_strings_t> globalStrings;
        std::unique_ptr<global_string_names_t> globalStringNames;
        std::unique_ptr<global_values_t> globalValues;
        std::unique_ptr<global_value_names_t> globalValueNames;

        std::unique_ptr<frame::handles_t> frameHandles;
        std::unique_ptr<frame::bank_t> frameBank;
        std::unique_ptr<object::bank_t> objectBank;
        std::unique_ptr<image::bank_t> imageBank;
        std::unique_ptr<sound::bank_t> soundBank;
        std::unique_ptr<music::bank_t> musicBank;
        std::unique_ptr<font::bank_t> fontBank;

        std::unique_ptr<last_t> last;

        error_t read(game_t &game, lak::memory &strm);
        error_t view(source_explorer_t &srcexp) const;
    };

    struct game_t
    {
        std::string gamePath;
        std::string gameDir;

        lak::memory file;

        std::vector<pack_file_t> packFiles;
        uint64_t dataPos;
        uint16_t numHeaderSections;
        uint16_t numSections;

        product_code_t runtimeVersion;
        uint16_t runtimeSubVersion;
        uint32_t productVersion;
        uint32_t productBuild;

        std::stack<chunk_t> state;

        bool unicode = false;
        bool oldGame = false;
        bool compat = false;
        bool cnc = false;
        std::vector<uint8_t> protection;

        header_t game;

        std::u16string project;
        std::u16string title;
        std::u16string copyright;

        std::unordered_map<uint32_t, size_t> imageHandles;
        std::unordered_map<uint16_t, size_t> objectHandles;
    };

    struct file_state_t
    {
        fs::path path;
        bool valid;
        bool attempt;
    };

    struct source_explorer_t
    {
        game_t state;

        bool loaded = false;
        bool dumpColorTrans = true;
        file_state_t exe;
        file_state_t images;
        file_state_t sortedImages;
        file_state_t sounds;
        file_state_t music;
        file_state_t shaders;
        file_state_t appicon;

        MemoryEditor editor;

        // const resource_entry_t *view = nullptr;
        const basic_entry_t *view = nullptr;
        lak::glTexture_t image;
        std::vector<uint8_t> buffer;
    };

    struct resource_entry_t
    {
        union
        {
            chunk_t ID = chunk_t::DEFAULT;
            uint32_t handle;
        };
        chunk_t parent = chunk_t::DEFAULT;

        encoding_t mode = encoding_t::DEFAULT;

        data_point_t info;
        data_point_t header;
        data_point_t data;

        std::vector<resource_entry_t> chunks;

        lak::memory streamHeader() const;
        lak::memory stream() const;
        lak::memory decodeHeader() const;
        lak::memory decode() const;
    };

    // struct object_entry_t
    // {
    //     const resource_entry_t &entry;
    //     const resource_entry_t *propEntry = nullptr;
    //     const resource_entry_t *nameEntry = nullptr;
    //     uint16_t handle;
    //     int16_t type;
    //     std::u16string name;
    // };

    // struct image_entry_t
    // {
    //     const resource_entry_t &entry;
    //     uint16_t handle;
    // };

    error_t LoadGame(
        source_explorer_t &srcexp
    );

    void GetEncryptionKey(
        game_t &gameState
    );

    error_t ParsePEHeader(
        lak::memory &strm,
        game_t &gameState
    );

    uint64_t ParsePackData(
        lak::memory &strm,
        game_t &gameState
    );

    // error_t ReadEntry(
    //     lak::memory &strm,
    //     std::stack<chunk_t> &state,
    //     resource_entry_t &entry
    // );

    // error_t ReadEntryData(
    //     lak::memory &strm,
    //     resource_entry_t &entry,
    //     get_data_flag_t readMode
    // );

    error_t ReadFixedData(
        lak::memory &strm,
        data_point_t &data,
        const size_t size
    );

    error_t ReadDynamicData(
        lak::memory &strm,
        data_point_t &data
    );

    error_t ReadToCompressedData(
        lak::memory &strm,
        data_point_t &data
    );

    // <uncompressed size> <data size> <data>
    error_t ReadCompressedData(
        lak::memory &strm,
        data_point_t &data
    );

    // <chunk size> <uncompressed size> <data>
    error_t ReadRevCompressedData(
        lak::memory &strm,
        data_point_t &data
    );

    // <uncompressed size> <data>
    error_t ReadStreamCompressedData(
        lak::memory &strm,
        data_point_t &data
    );

    std::u16string ReadString(
        const resource_entry_t &entry,
        const bool unicode
    );

    const char *GetTypeString(
        const resource_entry_t &entry
    );

    std::string GetObjectTypeString(
        object_type_t type
    );

    std::pair<bool, std::vector<uint8_t>> Decode(
        const std::vector<uint8_t> &encoded,
        chunk_t ID,
        encoding_t mode
    );

    std::vector<uint8_t> Inflate(
        const std::vector<uint8_t> &deflated
    );

    std::vector<uint8_t> Decompress(
        const std::vector<uint8_t> &compressed,
        unsigned int outSize
    );

    std::vector<uint8_t> StreamDecompress(
        lak::memory &strm,
        unsigned int outSize
    );

    std::pair<bool, std::vector<uint8_t>> Decrypt(
        const std::vector<uint8_t> &encrypted,
        chunk_t ID,
        encoding_t mode
    );

    object::item_t *GetObject(
        game_t &game,
        uint16_t handle
    );

    image::item_t *GetImage(
        game_t &game,
        uint32_t handle
    );
}
#endif