// Copyright (c) Mathias Kaerlev 2012, Lucas Kleiss 2019

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
#include "strconv.h"
#include "defines.h"
#include "tinflate.hpp"
#include "tinf.h"

#ifndef EXPLORER_H
#define EXPLORER_H

#include "encryption.h"
#include "image.h"

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG(x) if (SourceExplorer::debugConsole) std::cout << std::hex << x << "\n" << std::flush;

#ifdef WDEBUG
#undef WDEBUG
#endif
#define WDEBUG(x) if (SourceExplorer::debugConsole) std::wcout << std::hex << x << L"\n" << std::flush;

#ifdef ERROR
#undef ERROR
#endif
#define ERROR(x) std::cerr << std::hex << x << "\n" << std::flush;

#ifdef WERROR
#undef WERROR
#endif
#define WERROR(x) std::wcerr << std::hex << x << L"\n" << std::flush;

namespace SourceExplorer
{
    extern bool debugConsole;
    extern m128i_t _xmmword;
    extern std::vector<uint8_t> _magic_key;
    extern uint8_t _magic_char;
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
        lak::memstrm_t data;
        lak::memstrm_t decode(const chunk_t ID, const encoding_t mode) const;
    };

    struct entry_t
    {
        chunk_t ID;
        encoding_t mode;
        size_t position;
        size_t end;
        bool old;

        data_point_t header;
        data_point_t data;

        error_t read(game_t &game, lak::memstrm_t &strm);
        error_t readSound(game_t &game, lak::memstrm_t &strm, const size_t headerSize);
        error_t readItem(game_t &game, lak::memstrm_t &strm, const size_t headerSize = 0);
        void view(source_explorer_t &srcexp) const;

        lak::memstrm_t decode() const;
        lak::memstrm_t decodeHeader() const;
        lak::memstrm_t raw() const;
        lak::memstrm_t rawHeader() const;
    };

    struct basic_chunk_t
    {
        entry_t entry;

        error_t read(game_t &game, lak::memstrm_t &strm);
        error_t basic_view(source_explorer_t &srcexp, const char *name) const;
    };

    struct string_chunk_t : public basic_chunk_t
    {
        std::u16string value;

        error_t read(game_t &game, lak::memstrm_t &strm);
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
        struct properties_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct effect_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct item_t : public basic_chunk_t // OBJHEAD
        {
            std::unique_ptr<string_chunk_t> name;
            std::unique_ptr<properties_t> properties;
            std::unique_ptr<effect_t> effect;
            std::unique_ptr<last_t> end;

            error_t read(game_t &game, lak::memstrm_t &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct bank_t : public basic_chunk_t
        {
            std::vector<item_t> items;

            error_t read(game_t &game, lak::memstrm_t &strm);
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
            error_t view(source_explorer_t &srcexp) const;
        };

        struct object_instance_t : public basic_chunk_t
        {
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

            error_t read(game_t &game, lak::memstrm_t &strm);
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

        struct item_t
        {
            entry_t entry;
            std::unique_ptr<string_chunk_t> name;
            std::unique_ptr<header_t> header;
            std::unique_ptr<password_t> password;
            std::unique_ptr<palette_t> palette;
            std::unique_ptr<object_instance_t> objectInstance;
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

            error_t read(game_t &game, lak::memstrm_t &strm);
            error_t view(source_explorer_t &srcexp) const;
        };

        struct handles_t : public basic_chunk_t
        {
            error_t view(source_explorer_t &srcexp) const;
        };

        struct bank_t : public basic_chunk_t
        {
            std::vector<item_t> items;

            error_t read(game_t &game, lak::memstrm_t &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    namespace image
    {
        struct item_t : public basic_chunk_t
        {
            error_t read(game_t &game, lak::memstrm_t &strm);
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

            error_t read(game_t &game, lak::memstrm_t &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    namespace font
    {
        struct item_t : public basic_chunk_t
        {
            error_t read(game_t &game, lak::memstrm_t &strm);
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

            error_t read(game_t &game, lak::memstrm_t &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    namespace sound
    {
        struct item_t : public basic_chunk_t
        {
            error_t read(game_t &game, lak::memstrm_t &strm);
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

            error_t read(game_t &game, lak::memstrm_t &strm);
            error_t view(source_explorer_t &srcexp) const;
        };
    }

    namespace music
    {
        struct item_t : public basic_chunk_t
        {
            error_t read(game_t &game, lak::memstrm_t &strm);
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

            error_t read(game_t &game, lak::memstrm_t &strm);
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

        error_t read(game_t &game, lak::memstrm_t &strm);
        error_t view(source_explorer_t &srcexp) const;
    };

    struct game_t
    {
        std::string gamePath;
        std::string gameDir;

        lak::memstrm_t file;

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
        bool cnc = false;
        std::vector<uint8_t> protection;

        header_t game;

        std::u16string project;
        std::u16string title;
        std::u16string copyright;
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
        file_state_t sounds;
        file_state_t music;
        file_state_t shaders;

        MemoryEditor editor;

        // const resource_entry_t *view = nullptr;
        const entry_t *view = nullptr;
        lak::glTexture_t image;
        std::vector<uint8_t> buffer;
    };

    struct resource_entry_t
    {
        chunk_t ID = DEFAULT;
        chunk_t parent = DEFAULT;

        encoding_t mode = DEFAULTMODE;

        data_point_t info;
        data_point_t header;
        data_point_t data;

        std::vector<resource_entry_t> chunks;

        lak::memstrm_t streamHeader() const;
        lak::memstrm_t stream() const;
        lak::memstrm_t decodeHeader() const;
        lak::memstrm_t decode() const;
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

    error_t ParsePEHeader(
        lak::memstrm_t &strm,
        game_t &gameState
    );

    uint64_t ParsePackData(
        lak::memstrm_t &strm,
        game_t &gameState
    );

    // error_t ReadEntry(
    //     lak::memstrm_t &strm,
    //     std::stack<chunk_t> &state,
    //     resource_entry_t &entry
    // );

    // error_t ReadEntryData(
    //     lak::memstrm_t &strm,
    //     resource_entry_t &entry,
    //     get_data_flag_t readMode
    // );

    error_t ReadFixedData(
        lak::memstrm_t &strm,
        data_point_t &data,
        const size_t size
    );

    error_t ReadDynamicData(
        lak::memstrm_t &strm,
        data_point_t &data
    );

    error_t ReadToCompressedData(
        lak::memstrm_t &strm,
        data_point_t &data
    );

    // <uncompressed size> <data size> <data>
    error_t ReadCompressedData(
        lak::memstrm_t &strm,
        data_point_t &data
    );

    // <chunk size> <uncompressed size> <data>
    error_t ReadRevCompressedData(
        lak::memstrm_t &strm,
        data_point_t &data
    );

    // <uncompressed size> <data>
    error_t ReadStreamCompressedData(
        lak::memstrm_t &strm,
        data_point_t &data
    );

    std::u16string ReadString(
        const resource_entry_t &entry,
        const bool unicode
    );

    const char *GetTypeString(
        const resource_entry_t &entry
    );

    std::vector<uint8_t> Decode(
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
        lak::memstrm_t &strm,
        unsigned int outSize
    );

    std::vector<uint8_t> Decrypt(
        const std::vector<uint8_t> &encrypted,
        chunk_t ID,
        encoding_t mode
    );
}
#endif