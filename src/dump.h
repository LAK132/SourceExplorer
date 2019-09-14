/*
MIT License

Copyright (c) 2019 Lucas Kleiss (LAK132)

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

#include "explorer.h"

#include <tuple>
#include <atomic>

namespace SourceExplorer
{
  bool SaveImage(
      const lak::image4_t &image,
      const fs::path &filename);

  bool SaveImage(
      source_explorer_t &srcexp,
      uint16_t handle,
      const fs::path &filename,
      const frame::item_t *frame);

  bool OpenGame(source_explorer_t &srcexp);

  using dump_data_t = std::tuple<
      source_explorer_t &,
      std::atomic<float> &>;

  using dump_function_t = void (
      source_explorer_t &,
      std::atomic<float> &);

  bool DumpStuff(source_explorer_t &srcexp,
                 const char *str_id,
                 dump_function_t *func);

  void DumpImages      (source_explorer_t &srcexp,
                        std::atomic<float> &completed);
  void DumpSortedImages(source_explorer_t &srcexp,
                        std::atomic<float> &completed);
  void DumpAppIcon     (source_explorer_t &srcexp,
                        std::atomic<float> &completed);
  void DumpSounds      (source_explorer_t &srcexp,
                        std::atomic<float> &completed);
  void DumpMusic       (source_explorer_t &srcexp,
                        std::atomic<float> &completed);
  void DumpShaders     (source_explorer_t &srcexp,
                        std::atomic<float> &completed);
  void DumpBinaryFiles (source_explorer_t &srcexp,
                        std::atomic<float> &completed);

  void AttemptExe(source_explorer_t &srcexp);
  void AttemptImages(source_explorer_t &srcexp);
  void AttemptSortedImages(source_explorer_t &srcexp);
  void AttemptAppIcon(source_explorer_t &srcexp);
  void AttemptSounds(source_explorer_t &srcexp);
  void AttemptMusic(source_explorer_t &srcexp);
  void AttemptShaders(source_explorer_t &srcexp);
  void AttemptBinaryFiles(source_explorer_t &srcexp);
}

#endif
