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
#ifndef SOURCE_EXPLORER_DUMP_H
#define SOURCE_EXPLORER_DUMP_H

#include "ctf/explorer.hpp"

#include <atomic>
#include <tuple>

namespace SourceExplorer
{
	[[nodiscard]] error_t SaveImage(const lak::image4_t &image,
	                                const fs::path &filename);

	[[nodiscard]] error_t SaveImage(source_explorer_t &srcexp,
	                                uint16_t handle,
	                                const fs::path &filename,
	                                const frame::item_t *frame);

	[[nodiscard]] lak::await_result<error_t> OpenGame(source_explorer_t &srcexp);

	using dump_data_t = std::tuple<source_explorer_t &, std::atomic<float> &>;

	using dump_function_t = void(source_explorer_t &, std::atomic<float> &);

	lak::file_open_error DumpStuff(source_explorer_t &srcexp,
	                               const char *str_id,
	                               dump_function_t *func);

	void DumpImages(source_explorer_t &srcexp, std::atomic<float> &completed);
	void DumpSortedImages(source_explorer_t &srcexp,
	                      std::atomic<float> &completed);
	void DumpAppIcon(source_explorer_t &srcexp, std::atomic<float> &completed);
	void DumpSounds(source_explorer_t &srcexp, std::atomic<float> &completed);
	void DumpMusic(source_explorer_t &srcexp, std::atomic<float> &completed);
	void DumpShaders(source_explorer_t &srcexp, std::atomic<float> &completed);
	void DumpBinaryFiles(source_explorer_t &srcexp,
	                     std::atomic<float> &completed);
	void SaveErrorLog(source_explorer_t &srcexp, std::atomic<float> &completed);
	void SaveBinaryBlock(source_explorer_t &srcexp,
	                     std::atomic<float> &completed);

	template<lak::concepts::invocable_result_of<lak::file_open_error,
	                                            file_state_t &> LOAD,
	         typename FINALISE>
	requires(lak::concepts::invocable_result_of<FINALISE,
	                                            lak::file_open_error,
	                                            const std::filesystem::path &> ||
	         lak::concepts::invocable_result_of<FINALISE, lak::file_open_error>)
	void Attempt(file_state_t &file_state, LOAD load, FINALISE finalise)
	{
		if (!file_state.attempt) return;

		if (!file_state.valid)
		{
			switch (load(file_state))
			{
				case lak::file_open_error::VALID:
					file_state.valid = true;
					break;

				case lak::file_open_error::CANCELED:
					[[fallthrough]];
				case lak::file_open_error::INVALID:
					file_state.valid   = false;
					file_state.attempt = false;
					return;

				case lak::file_open_error::INCOMPLETE:
					break;

				default:
				{
					ASSERT_NYI();
					break;
				}
			}
		}

		if (file_state.valid)
		{
			lak::file_open_error result;
			if constexpr (lak::is_invocable_v<FINALISE,
			                                  const std::filesystem::path &>)
				result = finalise(file_state.path);
			else
				result = finalise();
			switch (result)
			{
				case lak::file_open_error::VALID:
					file_state.attempt = false;
					break;

				case lak::file_open_error::CANCELED:
					[[fallthrough]];
				case lak::file_open_error::INVALID:
					file_state.valid   = false;
					file_state.attempt = false;
					return;

				case lak::file_open_error::INCOMPLETE:
					break;

				default:
				{
					ASSERT_NYI();
					break;
				}
			}
		}
	}

	template<typename FINALISE>
	void AttemptFile(file_state_t &file_state, FINALISE finalise, bool save)
	{
		Attempt(
		  file_state,
		  [save](file_state_t &file_state) -> lak::file_open_error
		  {
			  return lak::open_file_modal(file_state.path, save)
			    .unwrap_or(
			      [](const lak::error_code_error &ec) -> lak::file_open_error
			      {
				      ERROR(ec);
				      return lak::file_open_error::INVALID;
			      });
		  },
		  finalise);
	}

	template<typename FINALISE>
	void AttemptFolder(file_state_t &file_state, FINALISE finalise)
	{
		Attempt(
		  file_state,
		  [](file_state_t &file_state) -> lak::file_open_error
		  {
			  return lak::open_folder_modal(file_state.path)
			    .unwrap_or(
			      [](const lak::error_code_error &ec) -> lak::file_open_error
			      {
				      ERROR(ec);
				      return lak::file_open_error::INVALID;
			      });
		  },
		  finalise);
	}

	void AttemptExe(source_explorer_t &srcexp);
	void AttemptImages(source_explorer_t &srcexp);
	void AttemptSortedImages(source_explorer_t &srcexp);
	void AttemptAppIcon(source_explorer_t &srcexp);
	void AttemptSounds(source_explorer_t &srcexp);
	void AttemptMusic(source_explorer_t &srcexp);
	void AttemptShaders(source_explorer_t &srcexp);
	void AttemptBinaryFiles(source_explorer_t &srcexp);
	void AttemptErrorLog(source_explorer_t &srcexp);
	void AttemptBinaryBlock(source_explorer_t &srcexp);
}

#endif
