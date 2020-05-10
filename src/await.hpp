/*
MIT License

Copyright (c) 2019, 2020 LAK132

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

#ifndef LAK_AWAIT_HPP
#define LAK_AWAIT_HPP

#include <atomic>
#include <thread>
#include <tuple>
#include <vector>

namespace lak
{
  template<typename R, typename... T, typename... D>
  bool await(std::unique_ptr<std::thread> &thread,
             std::atomic<bool> &finished,
             R (*func)(T...),
             const std::tuple<D...> &data)
  {
    if (!thread)
    {
      finished.store(false);
      void (*functor)(
        std::atomic<bool> *, R(*)(T...), const std::tuple<D...> *) =
        [](std::atomic<bool> *finished,
           R (*f)(T...),
           const std::tuple<D...> *data) {
          try
          {
            std::apply(f, *data);
          }
          catch (...)
          {
          }
          finished->store(true);
        };

      thread = std::make_unique<std::thread>(
        std::thread(functor, &finished, func, &data));
    }
    else if (finished.load())
    {
      thread->join();
      thread.reset();
      return true;
    }
    return false;
  }
}

#endif
