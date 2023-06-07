/*
MIT License

Copyright (c) 2019 LAK132

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifdef _WIN32
#	define STBI_MSC_SECURE_CRT
#endif
#include <stb_image_write.h>

#include "dump.h"
#include "explorer.h"
#include "tostring.hpp"

#include <lak/char_utils.hpp>
#include <lak/result.hpp>
#include <lak/string_literals.hpp>
#include <lak/string_utils.hpp>
#include <lak/tasks.hpp>
#include <lak/visit.hpp>

#include <algorithm>
#include <execution>
#include <unordered_set>

#ifdef GetObject
#	undef GetObject
#endif

namespace se = SourceExplorer;

se::error_t se::SaveImage(const lak::image4_t &image, const fs::path &filename)
{
	if (stbi_write_png(
	      reinterpret_cast<const char *>(filename.u8string().c_str()),
	      (int)image.size().x,
	      (int)image.size().y,
	      4,
	      &(image[0].r),
	      (int)(image.size().x * 4)) != 1)
	{
		return lak::err_t{
		  se::error(lak::streamify("Failed to save image '", filename, "'"))};
	}
	return lak::ok_t{};
}

se::error_t se::SaveImage(source_explorer_t &srcexp,
                          uint16_t handle,
                          const fs::path &filename,
                          const frame::item_t *frame)
{
	return GetImage(srcexp.state, handle)
	  .RES_ADD_TRACE("failed to get image item")
	  .and_then(
	    [&](const auto &item)
	    {
		    return item
		      .image(srcexp.dump_color_transparent,
		             (frame && frame->palette) ? frame->palette->colors.data()
		                                       : nullptr)
		      .RES_ADD_TRACE("failed to read image data");
	    })
	  .and_then([&](const auto &image) { return SaveImage(image, filename); });
}

lak::await_result<se::error_t> se::OpenGame(source_explorer_t &srcexp)
{
	static lak::await<se::error_t> awaiter;

	if (auto result = awaiter(LoadGame, std::ref(srcexp)); result.is_ok())
	{
		return lak::ok_t{result.unwrap().RES_ADD_TRACE("OpenGame")};
	}
	else
	{
		switch (result.unwrap_err())
		{
			case lak::await_error::running:
			{
				const auto str_id = "Open Game";
				if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Loading, please wait...");
					ImGui::Checkbox("Print to debug console?",
					                &lak::debugger.live_output_enabled);
					if (lak::debugger.live_output_enabled)
					{
						ImGui::Checkbox("Only errors?", &lak::debugger.live_errors_only);
						ImGui::Checkbox("Developer mode?",
						                &lak::debugger.line_info_enabled);
					}
					ImGui::ProgressBar(srcexp.state.completed);
					ImGui::ProgressBar(srcexp.state.bank_completed);
					ImGui::EndPopup();
				}
				else
				{
					ImGui::OpenPopup(str_id);
				}

				return lak::err_t{lak::await_error::running};
			}
			break;

			case lak::await_error::failed:
				return lak::err_t{lak::await_error::failed};
				break;

			default:
				ASSERT_NYI();
				// return lak::err_t{lak::await_error::failed};
				// break;
		}
	}
}

lak::file_open_error se::DumpStuff(source_explorer_t &srcexp,
                                   const char *str_id,
                                   dump_function_t *func)
{
	static lak::await<se::error_t> awaiter;
	static std::atomic<float> completed = 0.0f;

	auto functor = [&, func]() -> se::error_t
	{
		completed = 0.0f;
		func(srcexp, completed);
		return lak::ok_t{};
	};

	if (auto result = awaiter(functor); result.is_ok())
	{
		return lak::file_open_error::VALID;
	}
	else
	{
		switch (result.unwrap_err())
		{
			case lak::await_error::running:
			{
				if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("%s, please wait...", str_id);
					ImGui::Checkbox("Print to debug console?",
					                &lak::debugger.live_output_enabled);
					if (lak::debugger.live_output_enabled)
					{
						ImGui::Checkbox("Only errors?", &lak::debugger.live_errors_only);
						ImGui::Checkbox("Developer mode?",
						                &lak::debugger.line_info_enabled);
					}
					ImGui::ProgressBar(completed);
					ImGui::EndPopup();
				}
				else
				{
					ImGui::OpenPopup(str_id);
				}
				return lak::file_open_error::INCOMPLETE;
			}
			break;

			case lak::await_error::failed:
				return lak::file_open_error::INVALID;

			default:
			{
				ASSERT_NYI();
				return lak::file_open_error::INVALID;
			}
		}
	}
}

void se::DumpImages(source_explorer_t &srcexp, std::atomic<float> &completed)
{
	if (!srcexp.state.game.image_bank)
	{
		ERROR("No Image Bank");
		return;
	}

	auto tasks{srcexp.allow_multithreading ? lak::tasks::hardware_max()
	                                       : lak::tasks(1)};

	auto do_dump = [](source_explorer_t &srcexp,
	                  const se::image::item_t &item) -> se::error_t
	{
		RES_TRY_ASSIGN(lak::image4_t image =,
		               item.image(srcexp.dump_color_transparent)
		                 .RES_ADD_TRACE("Image ", item.entry.handle, " Failed"));
		fs::path filename =
		  srcexp.images.path / (std::to_string(item.entry.handle) + ".png");
		return SaveImage(image, filename).RES_ADD_TRACE("Save Failed");
	};

	const size_t count = srcexp.state.game.image_bank->items.size();
	std::atomic_size_t completed_index = 0;
	size_t loop_index                  = 0;
	for (const auto &item : srcexp.state.game.image_bank->items)
	{
		++loop_index;
		SCOPED_CHECKPOINT(
		  "Image ", loop_index, "/", count, " (", item.entry.handle, ")");
		tasks.push(
		  [&]
		  {
			  do_dump(srcexp, item).IF_ERR("Dump Failed");
			  completed = (float)((double)(++completed_index) / (double)count);
		  });
	}
}

void se::DumpSortedImages(se::source_explorer_t &srcexp,
                          std::atomic<float> &completed)
{
	if (!srcexp.state.game.image_bank)
	{
		ERROR("No Image Bank");
		return;
	}

	if (!srcexp.state.game.frame_bank)
	{
		ERROR("No Frame Bank");
		return;
	}

	if (!srcexp.state.game.object_bank)
	{
		ERROR("No Object Bank");
		return;
	}

	auto LinkImages = [](const fs::path &From, const fs::path &To)
	  -> lak::error_codes<lak::error_code_error, lak::u8string>
	{
		auto errno_map = [](lak::error_code_error err)
		  -> lak::variant<lak::error_code_error, lak::u8string>
		{ return lak::var_t<0>(err); };

		return lak::path_exists(From)
		  .IF_ERR("from path ", From, " existence check failed")
		  .map_err(errno_map)
		  .map_expect_value(
		    true,
		    [&](auto &&) -> lak::variant<lak::error_code_error, lak::u8string>
		    { return lak::var_t<1>(lak::streamify(From, " does not exist")); })
		  .and_then(
		    [&](auto &&)
		    {
			    return lak::path_exists(To)
			      .IF_ERR("to path ", To, " existence check failed")
			      .map_err(errno_map);
		    })
		  .map_expect_value(
		    false,
		    [&](auto &&) -> lak::variant<lak::error_code_error, lak::u8string>
		    { return lak::var_t<1>(lak::streamify(From, " already exist")); })
		  .and_then(
		    [&](auto &&)
		    {
			    return lak::create_directory(To.parent_path())
			      .IF_ERR("create directory failed")
			      .map_err(errno_map);
		    })
		  .and_then(
		    [&](auto &&)
		    {
			    return lak::create_hard_link(From, To)
			      .IF_ERR_WARN("create hard link from ",
			                   From,
			                   " to ",
			                   To,
			                   " failed, trying copy instead")
			      .or_else(
			        [&](auto &&)
			        {
				        return lak::copy_file(From, To).IF_ERR(
				          "copy file from ", From, " to ", To, " failed");
			        })
			      .map_err(errno_map);
		    });
	};

	using namespace std::string_literals;

	auto HandleName = [](const std::unique_ptr<string_chunk_t> &name,
	                     auto handle,
	                     std::u16string extra = u"")
	{
		std::u32string str;
		if (extra.size() > 0) str += lak::to_u32string(extra + u" ");
		if (name) str += U"'" + lak::to_u32string(name->value) + U"'";
		std::u32string result;
		for (auto &c : str)
			if (c == U' ' || c == U'(' || c == U')' || c == U'[' || c == U']' ||
			    c == U'+' || c == U'-' || c == U'=' || c == U'_' || c == '\'' ||
			    (c >= U'0' && c <= U'9') || (c >= U'a' && c <= U'z') ||
			    (c >= U'A' && c <= U'Z') || c > 127)
				result += c;
		while (!result.empty() && lak::is_whitespace(result.back()))
			result.pop_back();
		return u"["s + se::to_u16string(handle) + (result.empty() ? u"]" : u"] ") +
		       lak::to_u16string(result);
	};

	fs::path root_path     = srcexp.sorted_images.path;
	fs::path unsorted_path = root_path / "[unsorted]";
	fs::create_directories(unsorted_path);
	std::error_code err;

	size_t image_index       = 0;
	const size_t image_count = srcexp.state.game.image_bank->items.size();
	for (const auto &image : srcexp.state.game.image_bank->items)
	{
		SCOPED_CHECKPOINT(
		  "Image ", image_index, "/", image_count, " (", image.entry.handle, ")");
		std::u16string image_name = se::to_u16string(image.entry.handle) + u".png";
		fs::path image_path       = unsorted_path / image_name;
		(void)SaveImage(image.image(srcexp.dump_color_transparent).UNWRAP(),
		                image_path);
		completed = (float)((double)++image_index / image_count);
	}

	size_t frame_index       = 0;
	const size_t frame_count = srcexp.state.game.frame_bank->items.size();
	for (const auto &frame : srcexp.state.game.frame_bank->items)
	{
		SCOPED_CHECKPOINT("Frame ",
		                  frame_index,
		                  "/",
		                  frame_count,
		                  " (",
		                  frame.name->u8string(),
		                  ")");
		std::u16string frame_name = HandleName(frame.name, frame_index);
		fs::path frame_path       = root_path / frame_name;
		fs::create_directories(frame_path / "[unsorted]", err);
		if (err)
		{
			ERROR("File System Error: (", err.value(), ")", err.message());
			continue;
		}

		if (frame.object_instances)
		{
			std::unordered_set<uint32_t> used_images;
			std::unordered_set<uint16_t> used_objects;
			for (const auto &object : frame.object_instances->objects)
			{
				if (used_objects.find(object.handle) != used_objects.end()) continue;
				used_objects.insert(object.handle);
				if (const auto *obj =
				      lak::as_ptr(se::GetObject(srcexp.state, object.handle).ok());
				    obj)
				{
					std::u16string object_name = HandleName(
					  obj->name,
					  obj->handle,
					  u"[" +
					    lak::to_u16string(lak::astring(GetObjectTypeString(obj->type))) +
					    u"]");
					fs::path object_path = frame_path / object_name;
					fs::create_directories(object_path, err);
					if (err)
					{
						ERROR("File System Error: (", err.value(), ")", err.message());
						continue;
					}

					for (auto [imghandle, imgnames] : obj->image_handles())
					{
						if (imghandle == 0xFFFF) continue;
						if (const auto *img =
						      lak::as_ptr(GetImage(srcexp.state, imghandle).ok());
						    img)
						{
							if (used_images.find(imghandle) == used_images.end())
							{
								SCOPED_CHECKPOINT("Image (", imghandle, ")");
								used_images.insert(imghandle);
								std::u16string image_name =
								  se::to_u16string(imghandle) + u".png";
								fs::path image_path = frame_path / "[unsorted]" / image_name;

								// check if 8bit image
								if (img->need_palette() && frame.palette)
									(void)SaveImage(img
									                  ->image(srcexp.dump_color_transparent,
									                          frame.palette->colors.data())
									                  .UNWRAP(),
									                image_path);
								else if (auto res =
								           LinkImages(unsorted_path / image_name, image_path);
								         res.is_err())
									lak::visit(
									  lak::overloaded{
									    [](const std::error_code &err) {
										    ERROR("Linking Failed: (",
										          err.value(),
										          ")",
										          err.message());
									    },
									    [](const auto &err) { ERROR("Linking Failed: ", err); },
									  },
									  res.unwrap_err());
							}
							for (const auto &imgname : imgnames)
							{
								std::u16string unsorted_image_name =
								  se::to_u16string(imghandle) + u".png";
								fs::path unsorted_image_path =
								  frame_path / "[unsorted]" / unsorted_image_name;
								std::u16string image_name = imgname + u".png";
								fs::path image_path       = object_path / image_name;
								if (const auto *i =
								      lak::as_ptr(GetImage(srcexp.state, imghandle).ok());
								    i)
									if (auto res = LinkImages(unsorted_image_path, image_path);
									    res.is_err())
										lak::visit(
										  lak::overloaded{
										    [](const std::error_code &err) {
											    ERROR("Linking Failed: (",
											          err.value(),
											          ")",
											          err.message());
										    },
										    [](const auto &err)
										    { ERROR("Linking Failed: ", err); },
										  },
										  res.unwrap_err());
							}
						}
					}
				}
			}
		}
		completed = (float)((double)++frame_index / frame_count);
	}
}

void se::DumpAppIcon(source_explorer_t &srcexp, std::atomic<float> &)
{
	if (!srcexp.state.game.icon)
	{
		ERROR("No Icon");
		return;
	}

	lak::image4_t &bitmap = srcexp.state.game.icon->bitmap;

	fs::path filename = srcexp.appicon.path / "favicon.ico";
	std::ofstream file(filename,
	                   std::ios::binary | std::ios::out | std::ios::ate);
	if (!file.is_open()) return;

	stbi_write_func *func = [](void *context, void *png, int len)
	{
		auto [out, image] =
		  *static_cast<std::tuple<std::ofstream *, lak::image4_t *> *>(context);
		lak::binary_array_writer strm;
		strm.reserve(0x16);
		strm.write_u16(0); // reserved
		strm.write_u16(1); // .ICO
		strm.write_u16(1); // 1 image
		strm.write_u8(static_cast<uint8_t>(image->size().x));
		strm.write_u8(static_cast<uint8_t>(image->size().y));
		strm.write_u8(0);      // no palette
		strm.write_u8(0);      // reserved
		strm.write_u16(1);     // color plane
		strm.write_u16(8 * 4); // bits per pixel
		strm.write_u32(len);
		strm.write_u32(static_cast<uint32_t>(strm.size() + sizeof(uint32_t)));
		auto result = strm.release();
		out->write(reinterpret_cast<const char *>(result.data()), result.size());
		out->write((const char *)png, len);
	};

	auto context = std::tuple<std::ofstream *, lak::image4_t *>(&file, &bitmap);
	stbi_write_png_to_func(func,
	                       &context,
	                       (int)bitmap.size().x,
	                       (int)bitmap.size().y,
	                       4,
	                       bitmap.data(),
	                       (int)(bitmap.size().x * 4));

	file.close();
}

void se::DumpSounds(source_explorer_t &srcexp, std::atomic<float> &completed)
{
	if (!srcexp.state.game.sound_bank)
	{
		ERROR("No Sound Bank");
		return;
	}

	auto tasks{srcexp.allow_multithreading ? lak::tasks::hardware_max()
	                                       : lak::tasks(1)};

	const size_t count = srcexp.state.game.sound_bank->items.size();
	std::atomic_size_t completed_index = 0;
	size_t loop_index                  = 0;
	for (const auto &item : srcexp.state.game.sound_bank->items)
	{
		++loop_index;
		SCOPED_CHECKPOINT(
		  "Sound ", loop_index, "/", count, " (", item.entry.handle, ")");

		tasks.push(
		  [&]
		  {
			  data_reader_t sound(item.entry.decode_body().EXPECT(
			    "Item ", item.entry.handle, " Body Failed To Decode"));
			  lak::array<byte_t> result;

			  std::u8string name =
			    u8"[" + se::to_u8string(item.entry.handle) + u8"] ";
			  sound_mode_t type;

			  if (srcexp.state.old_game)
			  {
				  [[maybe_unused]] uint16_t checksum   = sound.read_u16().UNWRAP();
				  [[maybe_unused]] uint32_t references = sound.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t decomp_len = sound.read_u32().UNWRAP();
				  type = (sound_mode_t)sound.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t reserved = sound.read_u32().UNWRAP();
				  const uint32_t name_len            = sound.read_u32().UNWRAP();

				  name += sound.read_exact_c_str<char8_t>(name_len).UNWRAP();
				  name.erase(std::remove(name.begin(), name.end(), u8'\0'),
				             name.end());
				  DEBUG("u8string name: '", name, "'");

				  uint16_t format                   = sound.read_u16().UNWRAP();
				  uint16_t channel_count            = sound.read_u16().UNWRAP();
				  uint32_t sample_rate              = sound.read_u32().UNWRAP();
				  uint32_t byte_rate                = sound.read_u32().UNWRAP();
				  uint16_t block_align              = sound.read_u16().UNWRAP();
				  uint16_t bits_per_sample          = sound.read_u16().UNWRAP();
				  [[maybe_unused]] uint16_t unknown = sound.read_u16().UNWRAP();
				  uint32_t chunk_size               = sound.read_u32().UNWRAP();
				  auto data = sound.read<byte_t>(chunk_size).UNWRAP();

				  lak::binary_array_writer output;
				  output.write("RIFF"_span);
				  output.write_s32(static_cast<uint32_t>(data.size() - 44));
				  output.write("WAVEfmt "_span);
				  output.write_u32(0x10);
				  output.write_u16(format);
				  output.write_u16(channel_count);
				  output.write_u32(sample_rate);
				  output.write_u32(byte_rate);
				  output.write_u16(block_align);
				  output.write_u16(bits_per_sample);
				  output.write("data"_span);
				  output.write_u32(chunk_size);
				  output.write(lak::span(data));
				  result = output.release();
			  }
			  else
			  {
				  data_reader_t header(item.entry.decode_head().EXPECT(
				    "Item ", item.entry.handle, " Head Failed To Decode"));

				  [[maybe_unused]] uint32_t checksum   = header.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t references = header.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t decomp_len = header.read_u32().UNWRAP();
				  type = (sound_mode_t)header.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t reserved = header.read_u32().UNWRAP();
				  uint32_t name_len                  = header.read_u32().UNWRAP();

				  if (srcexp.state.unicode)
				  {
					  name += lak::to_u8string(
					    sound.read_exact_c_str<char16_t>(name_len).UNWRAP());
					  name.erase(std::remove(name.begin(), name.end(), u8'\0'),
					             name.end());
					  DEBUG("u16string name: '", name, "'");
				  }
				  else
				  {
					  name += sound.read_exact_c_str<char8_t>(name_len).UNWRAP();
					  name.erase(std::remove(name.begin(), name.end(), u8'\0'),
					             name.end());
					  DEBUG("u8string name: '", name, "'");
				  }

				  if (const auto peek = sound.peek<char>(4).UNWRAP();
				      lak::string_view(lak::span(peek)) == "OggS"_view)
				  {
					  type = sound_mode_t::oggs;
				  }
				  else if (lak::string_view(lak::span(peek)) == "Exte"_view)
				  {
					  type = sound_mode_t::xm;
				  }

				  result = lak::array<byte_t>(sound.remaining().begin(),
				                              sound.remaining().end());
			  }

			  switch (type)
			  {
				  case sound_mode_t::wave:
					  name += u8".wav";
					  break;
				  case sound_mode_t::midi:
					  name += u8".midi";
					  break;
				  case sound_mode_t::oggs:
					  name += u8".ogg";
					  break;
				  case sound_mode_t::xm:
					  name += u8".xm";
					  break;
				  default:
					  name += u8".mp3";
					  break;
			  }

			  DEBUG("Sound ", (size_t)item.entry.ID);

			  fs::path filename = srcexp.sounds.path / name;

			  DEBUG("Saving '", lak::to_u8string(filename), "'");

			  if (!lak::save_file(filename, result))
			  {
				  ERROR("Failed To Save File '", filename, "'");
			  }

			  completed = (float)((double)(++completed_index) / (double)count);
		  });
	}
}

void se::DumpMusic(source_explorer_t &srcexp, std::atomic<float> &completed)
{
	if (!srcexp.state.game.music_bank)
	{
		ERROR("No Music Bank");
		return;
	}

	auto tasks{srcexp.allow_multithreading ? lak::tasks::hardware_max()
	                                       : lak::tasks(1)};

	const size_t count = srcexp.state.game.music_bank->items.size();
	std::atomic_size_t completed_index = 0;
	size_t loop_index                  = 0;
	for (const auto &item : srcexp.state.game.music_bank->items)
	{
		++loop_index;
		SCOPED_CHECKPOINT(
		  "Music ", loop_index, "/", count, " (", item.entry.handle, ")");

		tasks.push(
		  [&]
		  {
			  data_reader_t sound(item.entry.decode_body().EXPECT(
			    "Item ", item.entry.handle, " Body Failed To Decode"));

			  std::u8string name =
			    u8"[" + se::to_u8string(item.entry.handle) + u8"] ";
			  sound_mode_t type;

			  if (srcexp.state.old_game)
			  {
				  [[maybe_unused]] uint16_t checksum   = sound.read_u16().UNWRAP();
				  [[maybe_unused]] uint32_t references = sound.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t decomp_len = sound.read_u32().UNWRAP();
				  type = (sound_mode_t)sound.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t reserved = sound.read_u32().UNWRAP();
				  uint32_t name_len                  = sound.read_u32().UNWRAP();

				  name += sound.read_exact_c_str<char8_t>(name_len).UNWRAP();
			  }
			  else
			  {
				  [[maybe_unused]] uint32_t checksum   = sound.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t references = sound.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t decomp_len = sound.read_u32().UNWRAP();
				  type = (sound_mode_t)sound.read_u32().UNWRAP();
				  [[maybe_unused]] uint32_t reserved = sound.read_u32().UNWRAP();
				  uint32_t name_len                  = sound.read_u32().UNWRAP();

				  if (srcexp.state.unicode)
				  {
					  name += lak::to_u8string(
					    sound.read_exact_c_str<char16_t>(name_len).UNWRAP());
				  }
				  else
				  {
					  name += sound.read_exact_c_str<char8_t>(name_len).UNWRAP();
				  }
			  }

			  name.erase(std::remove(name.begin(), name.end(), u8'\0'), name.end());

			  switch (type)
			  {
				  case sound_mode_t::wave:
					  name += u8".wav";
					  break;
				  case sound_mode_t::midi:
					  name += u8".midi";
					  break;
				  default:
					  name += u8".mp3";
					  break;
			  }

			  fs::path filename = srcexp.music.path / name;

			  if (!lak::save_file(filename, sound.remaining()))
			  {
				  ERROR("Failed To Save File '", filename, "'");
			  }

			  completed = (float)((double)(++completed_index) / (double)count);
		  });
	}
}

void se::DumpShaders(source_explorer_t &srcexp, std::atomic<float> &completed)
{
	if (!srcexp.state.game.shaders)
	{
		ERROR("No Shaders");
		return;
	}

	data_reader_t strm(srcexp.state.game.shaders->entry.decode_body().UNWRAP());

	uint32_t count = strm.read_u32().UNWRAP();
	std::vector<uint32_t> offsets;
	offsets.reserve(count);

	while (count-- > 0) offsets.push_back(strm.read_u32().UNWRAP());

	for (auto offset : offsets)
	{
		strm.seek(offset).UNWRAP();
		uint32_t name_offset                   = strm.read_u32().UNWRAP();
		uint32_t data_offset                   = strm.read_u32().UNWRAP();
		[[maybe_unused]] uint32_t param_offset = strm.read_u32().UNWRAP();
		[[maybe_unused]] uint32_t bank_tex     = strm.read_u32().UNWRAP();

		strm.seek(offset + name_offset).UNWRAP();
		fs::path filename = srcexp.shaders.path / strm.read_c_str<char>().UNWRAP();

		strm.seek(offset + data_offset).UNWRAP();
		lak::astring file = strm.read_c_str<char>().UNWRAP();

		DEBUG(filename);
		if (!lak::save_file(
		      filename,
		      lak::span(reinterpret_cast<const byte_t *>(file.c_str()),
		                file.size())))
		{
			ERROR("Failed To Save File '", filename, "'");
		}

		completed = (float)((double)++count / (double)offsets.size());
	}
}

void se::DumpBinaryFiles(source_explorer_t &srcexp,
                         std::atomic<float> &completed)
{
	if (!srcexp.state.game.binary_files)
	{
		ERROR("No Binary Files");
		return;
	}

	data_reader_t strm(
	  srcexp.state.game.binary_files->entry.decode_body().UNWRAP());

	const size_t count = srcexp.state.game.binary_files->items.size();
	size_t index       = 0;
	for (const auto &file : srcexp.state.game.binary_files->items)
	{
		++index;
		SCOPED_CHECKPOINT("Binary ", index, "/", count, " (", file.name, ")");
		fs::path filename = lak::to_u16string(file.name);
		filename          = srcexp.binary_files.path / filename.filename();
		DEBUG(filename);
		if (!lak::save_file(filename, file.data))
		{
			ERROR("Failed To Save File '", filename, "'");
		}
		completed = (float)((double)index / (double)count);
	}
}

void se::SaveErrorLog(source_explorer_t &srcexp, std::atomic<float> &)
{
	if (!lak::save_file(srcexp.error_log.path, lak::debugger.str()))
	{
		ERROR("Failed To Save File '", srcexp.error_log.path, "'");
	}
}

void se::SaveBinaryBlock(source_explorer_t &srcexp, std::atomic<float> &)
{
	srcexp.binary_block.path += ".bin";
	if (!lak::save_file(srcexp.binary_block.path,
	                    lak::span<const byte_t>(srcexp.buffer)))
	{
		ERROR("Failed To Save File '", srcexp.binary_block.path, "'");
	}
}

void se::AttemptExe(source_explorer_t &srcexp)
{
	lak::debugger.clear();
	srcexp.loaded = false;
	AttemptFile(
	  srcexp.exe,
	  [&srcexp]() -> lak::file_open_error
	  {
		  if (auto result = OpenGame(srcexp); result.is_err())
		  {
			  ASSERT(result.unwrap_err() == lak::await_error::running);
			  return lak::file_open_error::INCOMPLETE;
		  }
		  else if (result.unwrap().is_err())
		  {
			  result.unwrap().IF_ERR("AttemptExe failed").discard();
			  // ERROR(result.unwrap()
			  //         .RES_ADD_TRACE("AttemptExe failed")
			  //         .unwrap_err());
			  srcexp.loaded = true;
			  return lak::file_open_error::INVALID;
		  }
		  else
		  {
			  srcexp.loaded = true;
			  if (srcexp.baby_mode)
			  {
				  // Autotragically dump everything

				  fs::path dump_dir =
				    srcexp.exe.path.parent_path() / srcexp.exe.path.stem();

				  std::error_code er;
				  if (srcexp.state.game.image_bank)
				  {
					  file_state_t &images = srcexp.state.two_five_plus_game
					                           ? srcexp.images
					                           : srcexp.sorted_images;

					  images.path = dump_dir / "images";
					  if (fs::create_directories(images.path, er); er)
					  {
						  ERROR("Failed To Dump Images");
						  ERROR("File System Error: (", er.value(), ")", er.message());
					  }
					  else
					  {
						  images.attempt = true;
						  images.valid   = true;
					  }
				  }

				  if (srcexp.state.game.icon)
				  {
					  srcexp.appicon.path = dump_dir / "icon";
					  if (fs::create_directories(srcexp.appicon.path, er); er)
					  {
						  ERROR("Failed To Dump Icon");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  srcexp.appicon.attempt = true;
						  srcexp.appicon.valid   = true;
					  }
				  }

				  if (srcexp.state.game.sound_bank)
				  {
					  srcexp.sounds.path = dump_dir / "sounds";
					  if (fs::create_directories(srcexp.sounds.path, er); er)
					  {
						  ERROR("Failed To Dump Audio");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  srcexp.sounds.attempt = true;
						  srcexp.sounds.valid   = true;
					  }
				  }

				  if (srcexp.state.game.music_bank)
				  {
					  srcexp.music.path = dump_dir / "music";
					  if (fs::create_directories(srcexp.sounds.path, er); er)
					  {
						  ERROR("Failed To Dump Audio");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  srcexp.music.attempt = true;
						  srcexp.music.valid   = true;
					  }
				  }

				  if (srcexp.state.game.shaders)
				  {
					  srcexp.shaders.path = dump_dir / "shaders";
					  if (fs::create_directories(srcexp.shaders.path, er); er)
					  {
						  ERROR("Failed To Dump Shaders");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  srcexp.shaders.attempt = true;
						  srcexp.shaders.valid   = true;
					  }
				  }

				  if (srcexp.state.game.binary_files)
				  {
					  srcexp.binary_files.path = dump_dir / "binary_files";
					  if (fs::create_directories(srcexp.binary_files.path, er); er)
					  {
						  ERROR("Failed To Dump Binary Files");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  srcexp.binary_files.attempt = true;
						  srcexp.binary_files.valid   = true;
					  }
				  }
			  }
			  return lak::file_open_error::VALID;
		  }
	  },
	  false);
}

void se::AttemptDatabase(source_explorer_t &srcexp)
{
	AttemptFile(
	  srcexp.database,
	  [&srcexp] { return DumpStuff(srcexp, "Saving database", &DumpDatabase); },
	  true);
}

void se::AttemptImages(source_explorer_t &srcexp)
{
	AttemptFolder(srcexp.images,
	              [&srcexp]
	              { return DumpStuff(srcexp, "Saving images", &DumpImages); });
}

void se::AttemptSortedImages(source_explorer_t &srcexp)
{
	AttemptFolder(
	  srcexp.sorted_images,
	  [&srcexp]
	  { return DumpStuff(srcexp, "Saving sorted images", &DumpSortedImages); });
}

void se::AttemptAppIcon(source_explorer_t &srcexp)
{
	AttemptFolder(
	  srcexp.appicon,
	  [&srcexp] { return DumpStuff(srcexp, "Saving app icon", &DumpAppIcon); });
}

void se::AttemptSounds(source_explorer_t &srcexp)
{
	AttemptFolder(srcexp.sounds,
	              [&srcexp]
	              { return DumpStuff(srcexp, "Saving sounds", &DumpSounds); });
}

void se::AttemptMusic(source_explorer_t &srcexp)
{
	AttemptFolder(srcexp.music,
	              [&srcexp]
	              { return DumpStuff(srcexp, "Saving music", &DumpMusic); });
}

void se::AttemptShaders(source_explorer_t &srcexp)
{
	AttemptFolder(srcexp.shaders,
	              [&srcexp]
	              { return DumpStuff(srcexp, "Saving shaders", &DumpShaders); });
}

void se::AttemptBinaryFiles(source_explorer_t &srcexp)
{
	AttemptFolder(
	  srcexp.binary_files,
	  [&srcexp]
	  { return DumpStuff(srcexp, "Saving binary files", &DumpBinaryFiles); });
}

void se::AttemptErrorLog(source_explorer_t &srcexp)
{
	AttemptFile(
	  srcexp.error_log,
	  [&srcexp] { return DumpStuff(srcexp, "Saving error log", &SaveErrorLog); },
	  true);
}

void se::AttemptBinaryBlock(source_explorer_t &srcexp)
{
	AttemptFile(
	  srcexp.binary_block,
	  [&srcexp]
	  { return DumpStuff(srcexp, "Saving binary block", &SaveBinaryBlock); },
	  true);
}
