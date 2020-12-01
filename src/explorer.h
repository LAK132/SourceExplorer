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

#ifndef EXPLORER_H
#define EXPLORER_H

#include "imgui_impl_lak.h"
#include "imgui_utils.hpp"
#include <imgui/misc/softraster/texture.h>
#include <imgui_memory_editor.h>
#include <imgui_stdlib.h>

#include "defines.h"
#include "encryption.h"
#include "stb_image.h"

#include <lak/debug.hpp>
#include <lak/file.hpp>
#include <lak/memory.hpp>
#include <lak/opengl/state.hpp>
#include <lak/opengl/texture.hpp>
#include <lak/strconv.hpp>
#include <lak/string.hpp>
#include <lak/tinflate.hpp>

#include <assert.h>
#include <atomic>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <optional>
#include <stack>
#include <stdint.h>
#include <unordered_map>
#include <variant>
#include <vector>

namespace fs = std::filesystem;

namespace SourceExplorer
{
  extern bool forceCompat;
  extern m128i_t _xmmword;
  extern std::vector<uint8_t> _magic_key;
  extern uint8_t _magic_char;

  enum class game_mode_t : uint8_t
  {
    _OLD,
    _284,
    _288
  };
  extern game_mode_t _mode;

  struct game_t;
  struct source_explorer_t;

  using texture_t =
    std::variant<std::monostate, lak::opengl::texture, texture_color32_t>;

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

    lak::memory decode(size_t max_size = SIZE_MAX) const;
    lak::memory decodeHeader(size_t max_size = SIZE_MAX) const;
    const lak::memory &raw() const;
    const lak::memory &rawHeader() const;
  };

  struct chunk_entry_t : public basic_entry_t
  {
    error_t read(game_t &game, lak::memory &strm);
    void view(source_explorer_t &srcexp) const;
  };

  struct item_entry_t : public basic_entry_t
  {
    error_t read(game_t &game,
                 lak::memory &strm,
                 bool compressed,
                 size_t headersize = 0);
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
    mutable std::u16string value;

    error_t read(game_t &game, lak::memory &strm);
    error_t view(source_explorer_t &srcexp,
                 const char *name,
                 const bool preview = false) const;

    lak::u16string u16string() const;
    lak::u8string u8string() const;
    lak::astring astring() const;
  };

  struct strings_chunk_t : public basic_chunk_t
  {
    mutable std::vector<std::u16string> values;

    error_t read(game_t &game, lak::memory &strm);
    error_t basic_view(source_explorer_t &srcexp, const char *name) const;
    error_t view(source_explorer_t &srcexp) const;
  };

  struct compressed_chunk_t : public basic_chunk_t
  {
    error_t view(source_explorer_t &srcexp) const;
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

  struct other_extension_t : public basic_chunk_t
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

  struct binary_file_t
  {
    std::u16string name;
    lak::memory data;

    error_t read(game_t &game, lak::memory &strm);
    error_t view(source_explorer_t &srcexp) const;
  };

  struct binary_files_t : public basic_chunk_t
  {
    std::vector<binary_file_t> items;

    error_t read(game_t &game, lak::memory &strm);
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

  struct object_names_t : public strings_chunk_t
  {
    error_t view(source_explorer_t &srcexp) const;
  };

  struct object_properties_t : public basic_chunk_t
  {
    std::vector<item_entry_t> items;

    error_t read(game_t &game, lak::memory &strm);
    error_t view(source_explorer_t &srcexp) const;
  };

  struct truetype_fonts_meta_t : public basic_chunk_t
  {
    error_t view(source_explorer_t &srcexp) const;
  };

  struct truetype_fonts_t : public basic_chunk_t
  {
    std::vector<item_entry_t> items;

    error_t read(game_t &game, lak::memory &strm);
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

    // ObjectInfo + ObjectHeader
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

      std::unordered_map<uint32_t, std::vector<std::u16string>> image_handles()
        const;
    };

    // aka FrameItems
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

      lak::image4_t image() const;
    };

    struct object_instance_t
    {
      uint16_t handle;
      uint16_t info;
      lak::vec2i32_t position;
      object_parent_type_t parentType;
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

    struct play_header_r : public basic_chunk_t
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
      std::unique_ptr<play_header_r> playHead;
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
      std::vector<uint16_t> handles;

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

  namespace image
  {
    struct item_t : public basic_item_t
    {
      uint32_t checksum; // uint16_t for old
      uint32_t reference;
      uint32_t dataSize;
      lak::vec2u16_t size;
      graphics_mode_t graphicsMode; // uint8_t
      image_flag_t flags;           // uint8_t
      uint16_t unknown;             // not for old
      lak::vec2u16_t hotspot;
      lak::vec2u16_t action;
      lak::color4_t transparent; // not for old
      size_t dataPosition;

      error_t read(game_t &game, lak::memory &strm);
      error_t view(source_explorer_t &srcexp) const;

      lak::memory image_data() const;
      bool need_palette() const;
      lak::image4_t image(const bool colorTrans,
                          const lak::color4_t palette[256] = nullptr) const;
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

  template<typename T>
  struct chunk_ptr
  {
    using value_type = T;

    std::unique_ptr<T> ptr;

    auto &operator=(std::unique_ptr<T> &&p)
    {
      ptr = lak::move(p);
      return *this;
    }

    template<typename... ARGS>
    void view(ARGS &&... args) const
    {
      if (ptr) ptr->view(args...);
    }

    operator bool() const { return static_cast<bool>(ptr); }

    auto *operator->() { return ptr.get(); }
    auto *operator->() const { return ptr.get(); }

    auto &operator*() { return *ptr; }
    auto &operator*() const { return *ptr; }
  };

  struct header_t : public basic_chunk_t
  {
    chunk_ptr<string_chunk_t> title;
    chunk_ptr<string_chunk_t> author;
    chunk_ptr<string_chunk_t> copyright;
    chunk_ptr<string_chunk_t> outputPath;
    chunk_ptr<string_chunk_t> projectPath;

    chunk_ptr<vitalise_preview_t> vitalisePreview;
    chunk_ptr<menu_t> menu;
    chunk_ptr<extension_path_t> extensionPath;
    chunk_ptr<extensions_t> extensions; // deprecated
    chunk_ptr<extension_data_t> extensionData;
    chunk_ptr<additional_extensions_t> additionalExtensions;
    chunk_ptr<application_doc_t> appDoc;
    chunk_ptr<other_extension_t> otherExtension;
    chunk_ptr<extension_list_t> extensionList;
    chunk_ptr<icon_t> icon;
    chunk_ptr<demo_version_t> demoVersion;
    chunk_ptr<security_number_t> security;
    chunk_ptr<binary_files_t> binaryFiles;
    chunk_ptr<menu_images_t> menuImages;
    chunk_ptr<string_chunk_t> about;
    chunk_ptr<movement_extensions_t> movementExtensions;
    chunk_ptr<object_bank2_t> objectBank2;
    chunk_ptr<exe_t> exe;
    chunk_ptr<protection_t> protection;
    chunk_ptr<shaders_t> shaders;
    chunk_ptr<extended_header_t> extendedHeader;
    chunk_ptr<spacer_t> spacer;
    chunk_ptr<chunk_224F_t> chunk224F;
    chunk_ptr<title2_t> title2;

    chunk_ptr<global_events_t> globalEvents;
    chunk_ptr<global_strings_t> globalStrings;
    chunk_ptr<global_string_names_t> globalStringNames;
    chunk_ptr<global_values_t> globalValues;
    chunk_ptr<global_value_names_t> globalValueNames;

    chunk_ptr<frame::handles_t> frameHandles;
    chunk_ptr<frame::bank_t> frameBank;
    chunk_ptr<object::bank_t> objectBank;
    chunk_ptr<image::bank_t> imageBank;
    chunk_ptr<sound::bank_t> soundBank;
    chunk_ptr<music::bank_t> musicBank;
    chunk_ptr<font::bank_t> fontBank;

    // Recompiled games (?):
    chunk_ptr<object_names_t> objectNames;
    chunk_ptr<object_properties_t> objectProperties;
    chunk_ptr<truetype_fonts_meta_t> truetypeFontsMeta;
    chunk_ptr<truetype_fonts_t> truetypeFonts;

    // Unknown chunks:
    std::vector<basic_chunk_t> unknownChunks;
    std::vector<strings_chunk_t> unknownStrings;
    std::vector<compressed_chunk_t> unknownCompressed;

    chunk_ptr<last_t> last;

    error_t read(game_t &game, lak::memory &strm);
    error_t view(source_explorer_t &srcexp) const;
  };

  struct game_t
  {
    static std::atomic<float> completed;

    lak::astring gamePath;
    lak::astring gameDir;

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

    bool unicode    = false;
    bool oldGame    = false;
    bool compat     = false;
    bool cnc        = false;
    bool recompiled = false;
    std::vector<uint8_t> protection;

    header_t game;

    lak::u16string project;
    lak::u16string title;
    lak::u16string copyright;

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
    lak::graphics_mode graphicsMode;

    game_t state;

    bool loaded         = false;
    bool babyMode       = true;
    bool dumpColorTrans = true;
    file_state_t exe;
    file_state_t images;
    file_state_t sortedImages;
    file_state_t sounds;
    file_state_t music;
    file_state_t shaders;
    file_state_t binaryFiles;
    file_state_t appicon;
    file_state_t errorLog;

    MemoryEditor editor;

    const basic_entry_t *view = nullptr;
    texture_t image;
    std::vector<uint8_t> buffer;
  };

  error_t LoadGame(source_explorer_t &srcexp);

  void GetEncryptionKey(game_t &gameState);

  error_t ParsePEHeader(lak::memory &strm, game_t &gameState);

  uint64_t ParsePackData(lak::memory &strm, game_t &gameState);

  texture_t CreateTexture(const lak::image4_t &bitmap,
                          const lak::graphics_mode mode);

  void ViewImage(source_explorer_t &srcexp, const float scale = 1.0f);

  const char *GetTypeString(const basic_entry_t &entry);

  const char *GetObjectTypeString(object_type_t type);

  const char *GetObjectParentTypeString(object_parent_type_t type);

  std::pair<bool, std::vector<uint8_t>> Decode(
    const std::vector<uint8_t> &encoded, chunk_t ID, encoding_t mode);

  std::optional<std::vector<uint8_t>> Inflate(
    const std::vector<uint8_t> &compressed,
    bool skip_header,
    bool anaconda,
    size_t max_size = SIZE_MAX);

  bool Inflate(std::vector<uint8_t> &out,
               const std::vector<uint8_t> &compressed,
               bool skip_header,
               bool anaconda,
               size_t max_size = SIZE_MAX);

  bool Inflate(lak::memory &out,
               const std::vector<uint8_t> &compressed,
               bool skip_header,
               bool anaconda,
               size_t max_size = SIZE_MAX);

  std::vector<uint8_t> Inflate(const std::vector<uint8_t> &compressed);

  std::vector<uint8_t> StreamDecompress(lak::memory &strm,
                                        unsigned int outSize);

  std::pair<bool, std::vector<uint8_t>> Decrypt(
    const std::vector<uint8_t> &encrypted, chunk_t ID, encoding_t mode);

  object::item_t *GetObject(game_t &game, uint16_t handle);

  image::item_t *GetImage(game_t &game, uint32_t handle);
}
#endif