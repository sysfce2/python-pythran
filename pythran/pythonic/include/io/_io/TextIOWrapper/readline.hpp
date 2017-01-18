#ifndef PYTHONIC_INCLUDE_IO__IO_TEXTIOWRAPPER_READLINE_HPP
#define PYTHONIC_INCLUDE_IO__IO_TEXTIOWRAPPER_READLINE_HPP

#include "pythonic/include/__builtin__/file/readline.hpp"

namespace pythonic
{
  namespace io
  {

    namespace _io
    {
      namespace TextIOWrapper
      {
        USING_FUNCTOR(readline, __builtin__::file::functor::readline);
      }
    }
  }
}
#endif