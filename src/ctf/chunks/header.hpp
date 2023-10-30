#ifndef SRCEXP_CTF_CHUNKS_HEADER_HPP
#define SRCEXP_CTF_CHUNKS_HEADER_HPP

#include "additional_extensions.hpp"
#include "application_doc.hpp"
#include "basic.hpp"
#include "binary_files.hpp"
#include "chunk_224F.hpp"
#include "chunk_2253.hpp"
#include "chunk_2255.hpp"
#include "chunk_2257.hpp"
#include "compressed.hpp"
#include "demo_version.hpp"
#include "exe.hpp"
#include "extended_header.hpp"
#include "extension_data.hpp"
#include "extension_list.hpp"
#include "extension_path.hpp"
#include "extensions.hpp"
#include "font_bank.hpp"
#include "frame_bank.hpp"
#include "global_events.hpp"
#include "global_string_names.hpp"
#include "global_strings.hpp"
#include "global_value_names.hpp"
#include "global_values.hpp"
#include "icon.hpp"
#include "image_bank.hpp"
#include "last.hpp"
#include "menu.hpp"
#include "menu_images.hpp"
#include "movement_extensions.hpp"
#include "music_bank.hpp"
#include "object_bank.hpp"
#include "object_bank2.hpp"
#include "object_names.hpp"
#include "object_properties.hpp"
#include "other_extension.hpp"
#include "protection.hpp"
#include "security_number.hpp"
#include "shaders.hpp"
#include "sound_bank.hpp"
#include "spacer.hpp"
#include "string.hpp"
#include "title2.hpp"
#include "truetype_fonts.hpp"
#include "truetype_fonts_meta.hpp"
#include "two_five_plus_object_properties.hpp"
#include "vitalise_preview.hpp"


namespace SourceExplorer
{
	struct header_t : public basic_chunk_t
	{
		chunk_ptr<string_chunk_t> title;
		chunk_ptr<string_chunk_t> author;
		chunk_ptr<string_chunk_t> copyright;
		chunk_ptr<string_chunk_t> output_path;
		chunk_ptr<string_chunk_t> project_path;

		chunk_ptr<vitalise_preview_t> vitalise_preview;
		chunk_ptr<menu_t> menu;
		chunk_ptr<extension_path_t> extension_path;
		chunk_ptr<extensions_t> extensions; // deprecated
		chunk_ptr<extension_data_t> extension_data;
		chunk_ptr<additional_extensions_t> additional_extensions;
		chunk_ptr<application_doc_t> app_doc;
		chunk_ptr<other_extension_t> other_extension;
		chunk_ptr<extension_list_t> extension_list;
		chunk_ptr<icon_t> icon;
		chunk_ptr<demo_version_t> demo_version;
		chunk_ptr<security_number_t> security;
		chunk_ptr<binary_files_t> binary_files;
		chunk_ptr<menu_images_t> menu_images;
		chunk_ptr<string_chunk_t> about;
		chunk_ptr<movement_extensions_t> movement_extensions;
		chunk_ptr<object_bank2_t> object_bank_2;
		chunk_ptr<exe_t> exe;
		chunk_ptr<protection_t> protection;
		chunk_ptr<shaders_t> shaders;
		chunk_ptr<shaders_t> shaders2;
		chunk_ptr<extended_header_t> extended_header;
		chunk_ptr<spacer_t> spacer;
		chunk_ptr<chunk_224F_t> chunk224F;
		chunk_ptr<title2_t> title2;

		chunk_ptr<global_events_t> global_events;
		chunk_ptr<global_strings_t> global_strings;
		chunk_ptr<global_string_names_t> global_string_names;
		chunk_ptr<global_values_t> global_values;
		chunk_ptr<global_value_names_t> global_value_names;

		chunk_ptr<frame::handles_t> frame_handles;
		chunk_ptr<frame::bank_t> frame_bank;
		chunk_ptr<object::bank_t> object_bank;
		chunk_ptr<image::bank_t> image_bank;
		chunk_ptr<sound::bank_t> sound_bank;
		chunk_ptr<music::bank_t> music_bank;
		chunk_ptr<font::bank_t> font_bank;

		// Recompiled games (?):
		chunk_ptr<chunk_2253_t> chunk2253;
		chunk_ptr<object_names_t> object_names;
		chunk_ptr<chunk_2255_t> chunk2255;
		chunk_ptr<two_five_plus_object_properties_t>
		  two_five_plus_object_properties;
		chunk_ptr<chunk_2257_t> chunk2257;
		chunk_ptr<object_properties_t> object_properties;
		chunk_ptr<truetype_fonts_meta_t> truetype_fonts_meta;
		chunk_ptr<truetype_fonts_t> truetype_fonts;

		// Unknown chunks:
		lak::array<basic_chunk_t> unknown_chunks;
		lak::array<strings_chunk_t> unknown_strings;
		lak::array<compressed_chunk_t> unknown_compressed;

		chunk_ptr<last_t> last;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
