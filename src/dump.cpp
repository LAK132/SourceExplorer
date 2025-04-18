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

#include "ctf/explorer.hpp"
#include "dump.h"
#include "tostring.hpp"

#include <lak/array.hpp>
#include <lak/char_utils.hpp>
#include <lak/result.hpp>
#include <lak/string.hpp>
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

srcexp::error_t srcexp::SaveImage(const lak::image4_t &image,
                                  const fs::path &filename)
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
		  srcexp::error(lak::streamify("Failed to save image '", filename, "'"))};
	}
	return lak::ok_t{};
}

srcexp::error_t srcexp::SaveImage(instance_t &inst,
                                  uint16_t handle,
                                  const fs::path &filename,
                                  const frame::item_t *frame)
{
	return GetImage(inst.state, handle)
	  .RES_ADD_TRACE("failed to get image item")
	  .and_then(
	    [&](const auto &item)
	    {
		    return item
		      .image(inst.dump_color_transparent,
		             (frame && frame->palette) ? frame->palette->colors.data()
		                                       : nullptr)
		      .RES_ADD_TRACE("failed to read image data");
	    })
	  .and_then([&](const auto &image) { return SaveImage(image, filename); });
}

lak::await_result<srcexp::error_t> srcexp::OpenGame(instance_t &inst)
{
	static lak::await<srcexp::error_t> awaiter;

	if (auto result = awaiter(LoadGame, std::ref(inst)); result.is_ok())
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
					ImGui::ProgressBar(inst.state.completed);
					ImGui::ProgressBar(inst.state.bank_completed);
					ImGui::ProgressBar(inst.state.item_completed);
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

lak::file_open_error srcexp::DumpStuff(instance_t &inst,
                                       const char *str_id,
                                       dump_function_t *func)
{
	static lak::await<srcexp::error_t> awaiter;
	static std::atomic<float> completed = 0.0f;

	auto functor = [&, func]() -> srcexp::error_t
	{
		completed = 0.0f;
		func(inst, completed);
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

void srcexp::DumpImages(instance_t &inst, std::atomic<float> &completed)
{
	if (!inst.state.game.image_bank)
	{
		ERROR("No Image Bank");
		return;
	}

	auto tasks{inst.allow_multithreading ? lak::tasks::hardware_max()
	                                     : lak::tasks(1)};

	auto do_dump = [](instance_t &inst,
	                  const srcexp::image::item_t &item) -> srcexp::error_t
	{
		RES_TRY_ASSIGN(lak::image4_t image =,
		               item.image(inst.dump_color_transparent)
		                 .RES_ADD_TRACE("Image ", item.entry.handle, " Failed"));
		fs::path filename =
		  inst.images.path / (std::to_string(item.entry.handle) + ".png");
		return SaveImage(image, filename).RES_ADD_TRACE("Save Failed");
	};

	const size_t count = inst.state.game.image_bank->items.size();
	std::atomic_size_t completed_index = 0;
	size_t loop_index                  = 0;
	for (const auto &item : inst.state.game.image_bank->items)
	{
		++loop_index;
		SCOPED_CHECKPOINT(
		  "Image ", loop_index, "/", count, " (", item.entry.handle, ")");
		tasks.push(
		  [&]
		  {
			  do_dump(inst, item).IF_ERR("Dump Failed");
			  completed = (float)((double)(++completed_index) / (double)count);
		  });
	}
}

void srcexp::DumpSortedImages(srcexp::instance_t &inst,
                              std::atomic<float> &completed)
{
	if (!inst.state.game.image_bank)
	{
		ERROR("No Image Bank");
		return;
	}

	if (!inst.state.game.frame_bank)
	{
		ERROR("No Frame Bank");
		return;
	}

	if (!inst.state.game.object_bank)
	{
		ERROR("No Object Bank");
		return;
	}

	auto LinkImages =
	  [](const fs::path &From,
	     const fs::path &To) -> lak::error_codes<std::error_code, lak::u8string>
	{
		auto errno_map =
		  [](std::error_code err) -> lak::variant<std::error_code, lak::u8string>
		{ return lak::var_t<0>(err); };

		return lak::path_exists(From)
		  .IF_ERR("from path ", From, " existence check failed")
		  .map_err(errno_map)
		  .map_expect_value(
		    true,
		    [&](auto &&) -> lak::variant<std::error_code, lak::u8string>
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
		    [&](auto &&) -> lak::variant<std::error_code, lak::u8string>
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

	auto HandleName = [](const lak::unique_ptr<string_chunk_t> &name,
	                     auto handle,
	                     lak::u16string extra = u""_str) -> lak::u16string
	{
		lak::u32string str;
		if (extra.size() > 0) str += lak::to_u32string(extra + u" ");
		if (name) str += U"'" + lak::to_u32string(name->value) + U"'";
		lak::u32string result;
		for (auto &c : str)
			if (c == U' ' || c == U'(' || c == U')' || c == U'[' || c == U']' ||
			    c == U'+' || c == U'-' || c == U'=' || c == U'_' || c == '\'' ||
			    (c >= U'0' && c <= U'9') || (c >= U'a' && c <= U'z') ||
			    (c >= U'A' && c <= U'Z') || c > 127)
				result += c;
		while (!result.empty() && lak::is_whitespace(result.back()))
			result.pop_back();
		return u"["_str + srcexp::to_u16string(handle) +
		       (result.empty() ? u"]" : u"] ") + lak::to_u16string(result);
	};

	fs::path root_path     = inst.sorted_images.path;
	fs::path unsorted_path = root_path / "[unsorted]";
	fs::create_directories(unsorted_path);
	std::error_code err;

	size_t image_index       = 0;
	const size_t image_count = inst.state.game.image_bank->items.size();
	for (const auto &image : inst.state.game.image_bank->items)
	{
		SCOPED_CHECKPOINT(
		  "Image ", image_index, "/", image_count, " (", image.entry.handle, ")");
		lak::u16string image_name =
		  srcexp::to_u16string(image.entry.handle) + u".png";
		fs::path image_path = unsorted_path / image_name;
		(void)SaveImage(image.image(inst.dump_color_transparent).UNWRAP(),
		                image_path);
		completed = (float)((double)++image_index / image_count);
	}

	size_t frame_index       = 0;
	const size_t frame_count = inst.state.game.frame_bank->items.size();
	for (const auto &frame : inst.state.game.frame_bank->items)
	{
		SCOPED_CHECKPOINT("Frame ",
		                  frame_index,
		                  "/",
		                  frame_count,
		                  " (",
		                  frame.name->u8string(),
		                  ")");
		lak::u16string frame_name = HandleName(frame.name, frame_index);
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
				      lak::as_ptr(srcexp::GetObject(inst.state, object.handle).ok());
				    obj)
				{
					lak::u16string object_name = HandleName(
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
						      lak::as_ptr(GetImage(inst.state, imghandle).ok());
						    img)
						{
							if (used_images.find(imghandle) == used_images.end())
							{
								SCOPED_CHECKPOINT("Image (", imghandle, ")");
								used_images.insert(imghandle);
								lak::u16string image_name =
								  srcexp::to_u16string(imghandle) + u".png";
								fs::path image_path = frame_path / "[unsorted]" / image_name;

								// check if 8bit image
								if (img->need_palette() && frame.palette)
									(void)SaveImage(img
									                  ->image(inst.dump_color_transparent,
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
								lak::u16string unsorted_image_name =
								  srcexp::to_u16string(imghandle) + u".png";
								fs::path unsorted_image_path =
								  frame_path / "[unsorted]" / unsorted_image_name;
								lak::u16string image_name = imgname + u".png";
								fs::path image_path       = object_path / image_name;
								if (const auto *i =
								      lak::as_ptr(GetImage(inst.state, imghandle).ok());
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

void srcexp::DumpAppIcon(instance_t &inst, std::atomic<float> &)
{
	if (!inst.state.game.icon)
	{
		ERROR("No Icon");
		return;
	}

	lak::image4_t &bitmap = inst.state.game.icon->bitmap;

	fs::path filename = inst.appicon.path / "favicon.ico";
	std::ofstream file(filename,
	                   std::ios::binary | std::ios::out | std::ios::ate);
	if (!file.is_open()) return;

	stbi_write_func *func = [](void *context, void *png, int len)
	{
		auto [out, image] =
		  *static_cast<lak::tuple<std::ofstream *, lak::image4_t *> *>(context);
		lak::binary_array_writer strm;
		strm.reserve(0x16);
		strm.write_u16(0).unwrap(); // reserved
		strm.write_u16(1).unwrap(); // .ICO
		strm.write_u16(1).unwrap(); // 1 image
		strm.write_u8(static_cast<uint8_t>(image->size().x)).unwrap();
		strm.write_u8(static_cast<uint8_t>(image->size().y)).unwrap();
		strm.write_u8(0).unwrap();      // no palette
		strm.write_u8(0).unwrap();      // reserved
		strm.write_u16(1).unwrap();     // color plane
		strm.write_u16(8 * 4).unwrap(); // bits per pixel
		strm.write_u32(len).unwrap();
		strm.write_u32(static_cast<uint32_t>(strm.size() + sizeof(uint32_t)))
		  .unwrap();
		auto result = strm.release();
		out->write(reinterpret_cast<const char *>(result.data()), result.size());
		out->write((const char *)png, len);
	};

	auto context = lak::tuple<std::ofstream *, lak::image4_t *>(&file, &bitmap);
	stbi_write_png_to_func(func,
	                       &context,
	                       (int)bitmap.size().x,
	                       (int)bitmap.size().y,
	                       4,
	                       bitmap.data(),
	                       (int)(bitmap.size().x * 4));

	file.close();
}

void srcexp::DumpSounds(instance_t &inst, std::atomic<float> &completed)
{
	if (!inst.state.game.sound_bank)
	{
		ERROR("No Sound Bank");
		return;
	}

	auto tasks{inst.allow_multithreading ? lak::tasks::hardware_max()
	                                     : lak::tasks(1)};

	const size_t count = inst.state.game.sound_bank->items.size();
	std::atomic_size_t completed_index = 0;
	size_t loop_index                  = 0;
	for (const auto &item : inst.state.game.sound_bank->items)
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

			  lak::u8string name =
			    u8"[" + srcexp::to_u8string(item.entry.handle) + u8"] ";
			  sound_mode_t type;

			  if (inst.state.old_game)
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

				  if (inst.state.unicode)
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

			  fs::path filename = inst.sounds.path / name;

			  DEBUG("Saving '", lak::to_u8string(filename), "'");

			  if (!lak::save_file(filename, result))
			  {
				  ERROR("Failed To Save File '", filename, "'");
			  }

			  completed = (float)((double)(++completed_index) / (double)count);
		  });
	}
}

void srcexp::DumpMusic(instance_t &inst, std::atomic<float> &completed)
{
	if (!inst.state.game.music_bank)
	{
		ERROR("No Music Bank");
		return;
	}

	auto tasks{inst.allow_multithreading ? lak::tasks::hardware_max()
	                                     : lak::tasks(1)};

	const size_t count = inst.state.game.music_bank->items.size();
	std::atomic_size_t completed_index = 0;
	size_t loop_index                  = 0;
	for (const auto &item : inst.state.game.music_bank->items)
	{
		++loop_index;
		SCOPED_CHECKPOINT(
		  "Music ", loop_index, "/", count, " (", item.entry.handle, ")");

		tasks.push(
		  [&]
		  {
			  data_reader_t sound(item.entry.decode_body().EXPECT(
			    "Item ", item.entry.handle, " Body Failed To Decode"));

			  lak::u8string name =
			    u8"[" + srcexp::to_u8string(item.entry.handle) + u8"] ";
			  sound_mode_t type;

			  if (inst.state.old_game)
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

				  if (inst.state.unicode)
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

			  fs::path filename = inst.music.path / name;

			  if (!lak::save_file(filename, sound.remaining()))
			  {
				  ERROR("Failed To Save File '", filename, "'");
			  }

			  completed = (float)((double)(++completed_index) / (double)count);
		  });
	}
}

void srcexp::DumpShaders(instance_t &inst, std::atomic<float> &completed)
{
	if (!inst.state.game.shaders)
	{
		ERROR("No Shaders");
		return;
	}

	data_reader_t strm(inst.state.game.shaders->entry.decode_body().UNWRAP());

	uint32_t count = strm.read_u32().UNWRAP();
	lak::array<uint32_t> offsets;
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
		fs::path filename = inst.shaders.path / strm.read_c_str<char>().UNWRAP();

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

void srcexp::DumpBinaryFiles(instance_t &inst, std::atomic<float> &completed)
{
	if (!inst.state.game.binary_files)
	{
		ERROR("No Binary Files");
		return;
	}

	data_reader_t strm(
	  inst.state.game.binary_files->entry.decode_body().UNWRAP());

	const size_t count = inst.state.game.binary_files->items.size();
	size_t index       = 0;
	for (const auto &file : inst.state.game.binary_files->items)
	{
		++index;
		SCOPED_CHECKPOINT("Binary ", index, "/", count, " (", file.name, ")");
		fs::path filename = lak::to_u16string(file.name);
		filename          = inst.binary_files.path / filename.filename();
		DEBUG(filename);
		if (!lak::save_file(filename, file.data))
		{
			ERROR("Failed To Save File '", filename, "'");
		}
		completed = (float)((double)index / (double)count);
	}
}

void srcexp::SaveErrorLog(instance_t &inst, std::atomic<float> &)
{
	if (!lak::save_file(inst.error_log.path, lak::debugger.str()))
	{
		ERROR("Failed To Save File '", inst.error_log.path, "'");
	}
}

void srcexp::SaveBinaryBlock(instance_t &inst, std::atomic<float> &)
{
	inst.binary_block.path += ".bin";
	if (!lak::save_file(inst.binary_block.path,
	                    lak::span<const byte_t>(inst.buffer)))
	{
		ERROR("Failed To Save File '", inst.binary_block.path, "'");
	}
}

void srcexp::AttemptExe(instance_t &inst)
{
	lak::debugger.clear();
	inst.loaded = false;
	AttemptFile(
	  inst.exe,
	  [&inst]() -> lak::file_open_error
	  {
		  if (auto result = OpenGame(inst); result.is_err())
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
			  inst.loaded = true;
			  return lak::file_open_error::INVALID;
		  }
		  else
		  {
			  inst.loaded = true;
			  if (inst.baby_mode)
			  {
				  // Autotragically dump everything

				  fs::path dump_dir =
				    inst.exe.path.parent_path() / inst.exe.path.stem();

				  std::error_code er;
				  if (inst.state.game.image_bank)
				  {
					  file_state_t &images =
					    inst.state.two_five_plus_game ? inst.images : inst.sorted_images;

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

				  if (inst.state.game.icon)
				  {
					  inst.appicon.path = dump_dir / "icon";
					  if (fs::create_directories(inst.appicon.path, er); er)
					  {
						  ERROR("Failed To Dump Icon");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  inst.appicon.attempt = true;
						  inst.appicon.valid   = true;
					  }
				  }

				  if (inst.state.game.sound_bank)
				  {
					  inst.sounds.path = dump_dir / "sounds";
					  if (fs::create_directories(inst.sounds.path, er); er)
					  {
						  ERROR("Failed To Dump Audio");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  inst.sounds.attempt = true;
						  inst.sounds.valid   = true;
					  }
				  }

				  if (inst.state.game.music_bank)
				  {
					  inst.music.path = dump_dir / "music";
					  if (fs::create_directories(inst.sounds.path, er); er)
					  {
						  ERROR("Failed To Dump Audio");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  inst.music.attempt = true;
						  inst.music.valid   = true;
					  }
				  }

				  if (inst.state.game.shaders)
				  {
					  inst.shaders.path = dump_dir / "shaders";
					  if (fs::create_directories(inst.shaders.path, er); er)
					  {
						  ERROR("Failed To Dump Shaders");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  inst.shaders.attempt = true;
						  inst.shaders.valid   = true;
					  }
				  }

				  if (inst.state.game.binary_files)
				  {
					  inst.binary_files.path = dump_dir / "binary_files";
					  if (fs::create_directories(inst.binary_files.path, er); er)
					  {
						  ERROR("Failed To Dump Binary Files");
						  ERROR("File System Error: ", er.message());
					  }
					  else
					  {
						  inst.binary_files.attempt = true;
						  inst.binary_files.valid   = true;
					  }
				  }
			  }
			  return lak::file_open_error::VALID;
		  }
	  },
	  false,
	  "CTF{.exe,.ccn,.dat,.gam,.ugh},"
	  ".*");
}

void srcexp::AttemptDatabase(instance_t &inst)
{
	AttemptFile(
	  inst.database,
	  [&inst] { return DumpStuff(inst, "Saving database", &DumpDatabase); },
	  true);
}

void srcexp::AttemptImages(instance_t &inst)
{
	AttemptFolder(inst.images,
	              [&inst]
	              { return DumpStuff(inst, "Saving images", &DumpImages); });
}

void srcexp::AttemptAppIcon(instance_t &inst)
{
	AttemptFolder(inst.appicon,
	              [&inst]
	              { return DumpStuff(inst, "Saving app icon", &DumpAppIcon); });
}

void srcexp::AttemptSounds(instance_t &inst)
{
	AttemptFolder(inst.sounds,
	              [&inst]
	              { return DumpStuff(inst, "Saving sounds", &DumpSounds); });
}

void srcexp::AttemptMusic(instance_t &inst)
{
	AttemptFolder(inst.music,
	              [&inst]
	              { return DumpStuff(inst, "Saving music", &DumpMusic); });
}

void srcexp::AttemptShaders(instance_t &inst)
{
	AttemptFolder(inst.shaders,
	              [&inst]
	              { return DumpStuff(inst, "Saving shaders", &DumpShaders); });
}

void srcexp::AttemptBinaryFiles(instance_t &inst)
{
	AttemptFolder(
	  inst.binary_files,
	  [&inst]
	  { return DumpStuff(inst, "Saving binary files", &DumpBinaryFiles); });
}

void srcexp::AttemptErrorLog(instance_t &inst)
{
	AttemptFile(
	  inst.error_log,
	  [&inst] { return DumpStuff(inst, "Saving error log", &SaveErrorLog); },
	  true);
}

void srcexp::AttemptBinaryBlock(instance_t &inst)
{
	AttemptFile(
	  inst.binary_block,
	  [&inst]
	  { return DumpStuff(inst, "Saving binary block", &SaveBinaryBlock); },
	  true);
}
