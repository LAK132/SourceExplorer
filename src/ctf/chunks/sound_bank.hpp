#ifndef SRCEXP_CTF_CHUNKS_SOUND_BANK_HPP
#define SRCEXP_CTF_CHUNKS_SOUND_BANK_HPP

#include "basic.hpp"

namespace srcexp
{
	namespace sound
	{
		struct item_t : public basic_item_t
		{
			enum struct flags_t : uint32_t
			{
				check = 1 << 0,

				unknown1 = 1 << 1,
				unknown2 = 1 << 2,
				unknown3 = 1 << 3,
				unknown4 = 1 << 4,

				decompressed = 1 << 5,

				unknown5 = 1 << 6,
				unknown6 = 1 << 7,

				has_name = 1 << 8,

				unknown7  = 1 << 9,
				unknown8  = 1 << 10,
				unknown9  = 1 << 11,
				unknown10 = 1 << 12,
				unknown11 = 1 << 13,

				name_crop = 1 << 14,
			};

			uint32_t checksum;
			uint32_t references;
			uint32_t decomp_len;
			flags_t flags;
			uint32_t frequency;
			uint32_t name_len;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(instance_t &inst) const;
		};

		struct end_t : public basic_chunk_t
		{
			error_t view(instance_t &inst) const;
		};

		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;
			lak::unique_ptr<end_t> end;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(instance_t &inst) const;
		};

		inline item_t::flags_t operator|(const item_t::flags_t A,
		                                 const item_t::flags_t B)
		{
			return (item_t::flags_t)((uint32_t)A | (uint32_t)B);
		}

		inline item_t::flags_t operator&(const item_t::flags_t A,
		                                 const item_t::flags_t B)
		{
			return (item_t::flags_t)((uint32_t)A & (uint32_t)B);
		}
	}
}

#endif
