#ifndef PYTHONIC_INCLUDE_NUMPY_BITWISE_XOR_HPP
#define PYTHONIC_INCLUDE_NUMPY_BITWISE_XOR_HPP

#include "pythonic/include/utils/proxy.hpp"
#include "pythonic/include/types/ndarray.hpp"
#include "pythonic/include/types/numpy_broadcast.hpp"
#include "pythonic/include/utils/numpy_traits.hpp"
#include "pythonic/include/operator_/__xor__.hpp"

namespace pythonic
{

  namespace numpy
  {

#define NUMPY_NARY_FUNC_NAME bitwise_xor
#define NUMPY_NARY_FUNC_SYM pythonic::operator_::__xor__
#include "pythonic/include/types/numpy_nary_expr.hpp"
  }
}

#endif
