Developer Tutorial
##################

This is a long tutorial to help new Pythran developer discover the Pythran
architecture. This is *not* a developer documentation, but it aims at giving a
good overview of Pythran capacity.

It requires you are comfortable with Python, and eventually with C++11. It also
assumes you have some compilation background, i.e. you know what an AST is and
you don't try to escape when hearing the words alias analysis, memory effect
computations and such.

Parsing Python Code
-------------------

Python ships a standard module, ``ast`` to turn Python code into an AST. For instance::

  >>> import gast as ast
  >>> from __future__ import print_function
  >>> code = "a=1"
  >>> tree = ast.parse(code)  # turn the code into an AST

Deciphering the above line, one learns that the single assignment is parsed as
a module containing a single statement, which is an assignment to a single
target, a ``ast.Name`` with the identifier ``a``, of the literal value ``1``.

Eventually, one needs to parse more complex codes, and things get a bit more cryptic, but you get the idea::

  >>> fib_src = """
  ... def fib(n):
  ...     return n if n< 2 else fib(n-1) + fib(n-2)"""
  >>> tree = ast.parse(fib_src)

The idea remains the same. The whole Python syntax is described in
http://docs.python.org/3/library/ast.html and is worth a glance, otherwise
you'll be in serious trouble understanding the following.

Pythran Pass Manager
--------------------

A pass is a code transformation, i.e. a function that turns an AST node into a
new AST node with refined behavior. As a compiler infrastructure, Pythran
proposes a pass manager that (guess what?) manages pass scheduling, that is
the order in which pass is applied to achieve the ultimate goal, world
domination. Oooops, efficient C++11 code generation.

One first need to instantiate a pass manager with a module name::

  >>> from pythran import passmanager
  >>> pm = passmanager.PassManager("tutorial_module")

The pass manager has three methods and three attributes::

  >>> [x for x in dir(pm) if not x.startswith('_')]
  ['apply', 'code', 'dump', 'gather', 'module_dir', 'module_name']

``apply``
    applies a code transformation

``dump``
    dumps a node using a dedicated backend

``gather``
    gathers information about the node

Pythran Backends
----------------

Pythran currently has two backends. The main one is used to dump Pythran AST (a
subset of Python AST) into a C++ AST::

  >>> from pythran import backend
  >>> cxx = pm.dump(backend.Cxx, tree)
  >>> str(cxx)
  '#include <pythonic/include/operator_/add.hpp>\n#include <pythonic/include/operator_/lt.hpp>\n#include <pythonic/include/operator_/sub.hpp>\n#include <pythonic/operator_/add.hpp>\n#include <pythonic/operator_/lt.hpp>\n#include <pythonic/operator_/sub.hpp>\nnamespace \n{\n  namespace __pythran_tutorial_module\n  {\n    struct fib\n    {\n      typedef void callable;\n      typedef void pure;\n      template <typename argument_type0 >\n      struct type\n      {\n        typedef typename std::remove_cv<typename std::remove_reference<argument_type0>::type>::type __type0;\n        typedef typename pythonic::returnable<__type0>::type __type1;\n        typedef __type1 result_type;\n      }  \n      ;\n      template <typename argument_type0 >\n      inline\n      typename type<argument_type0>::result_type operator()(argument_type0 n) const\n      ;\n    }  ;\n    template <typename argument_type0 >\n    inline\n    typename fib::type<argument_type0>::result_type fib::operator()(argument_type0 n) const\n    {\n      return (pythonic::builtins::functor::bool_{}(pythonic::operator_::lt(n, 2L)) ? typename __combined<decltype(n), decltype(pythonic::operator_::add(pythonic::types::call(fib(), pythonic::operator_::sub(n, 1L)), pythonic::types::call(fib(), pythonic::operator_::sub(n, 2L))))>::type(n) : typename __combined<decltype(n), decltype(pythonic::operator_::add(pythonic::types::call(fib(), pythonic::operator_::sub(n, 1L)), pythonic::types::call(fib(), pythonic::operator_::sub(n, 2L))))>::type(pythonic::operator_::add(pythonic::types::call(fib(), pythonic::operator_::sub(n, 1L)), pythonic::types::call(fib(), pythonic::operator_::sub(n, 2L)))));\n    }\n  }\n}'

The above string is understandable by a C++11 compiler, but it quickly reaches the limit of our developer brain, so most of the time, we are more comfortable with the Python backend::

  >>> py = pm.dump(backend.Python, tree)
  >>> print(py)
  def fib(n):
      return (n if (n < 2) else (fib((n - 1)) + fib((n - 2))))

Passes
------

There are many code transformations in Pythran. Some of them are used to lower
the representation from Python AST to the simpler Pythran AST. For instance
there is no tuple unpacking in Pythran, so Pythran provides an adequate
transformation::

  >>> from pythran import transformations
  >>> tree = ast.parse("def foo(): a,b = 1,3.5")
  >>> _ = pm.apply(transformations.NormalizeTuples, tree)  # in-place
  >>> print(pm.dump(backend.Python, tree))
  def foo():
      __tuple0 = (1, 3.5)
      a = __tuple0[0]
      b = __tuple0[1]

Pythran transforms the tuple unpacking into an intermediate tuple assignment.
Note that if the unpacking statement is marked as critical using an OpenMP
statement, then a temporary variable is used to hold the left hand side
computation, if any::

  >>> from pythran import transformations
  >>> tree = ast.parse("""
  ... def foo(x):
  ...     #omp critical
  ...     a,b = 1, x + 1
  ...     return a + b""")
  >>> _ = pm.apply(transformations.NormalizeTuples, tree)  # in-place
  >>> print(pm.dump(backend.Python, tree))
  def foo(x):
      __tuple0 = (1, (x + 1))
      a = __tuple0[0]
      b = __tuple0[1]
      return (a + b)

There are many small passes used iteratively to produce the Pythran AST. For instance the implicit return at the end of every function is made explicit::

  >>> tree = ast.parse('def foo():pass')
  >>> _ = pm.apply(transformations.NormalizeReturn, tree)
  >>> print(pm.dump(backend.Python, tree))
  def foo():
      pass
      return None

More complex ones rely on introspection to implement constant folding::

  >>> from __future__ import print_function
  >>> code = [fib_src, 'def foo(): return builtins.map(fib, [1,2,3])']
  >>> fib_call = '\n'.join(code)
  >>> tree = ast.parse(fib_call)
  >>> from pythran import optimizations as optim
  >>> _ = pm.apply(optim.ConstantFolding, tree)
  >>> print(pm.dump(backend.Python, tree))
  def fib(n):
      return (n if (n < 2) else (fib((n - 1)) + fib((n - 2))))
  def foo():
      return [1, 1, 2]

One can also detect some common generator expression patterns to call the itertool module::

  >>> norm = 'def norm(l): return builtins.sum(n*n for n in l)'
  >>> tree = ast.parse(norm)
  >>> _ = pm.apply(optim.ComprehensionPatterns, tree)
  >>> 'map' in pm.dump(backend.Python, tree)
  True


Analysis
--------

All Pythran passes are backed up by analysis. Pythran provides three levels of analysis::

  >>> passmanager.FunctionAnalysis
  <class 'pythran.passmanager.FunctionAnalysis'>
  >>> passmanager.ModuleAnalysis
  <class 'pythran.passmanager.ModuleAnalysis'>
  >>> passmanager.NodeAnalysis
  <class 'pythran.passmanager.NodeAnalysis'>

Lets examine the information Pythran can extract from a Pythran-compatible
Python code.

A simple analyse gathers informations concerning used identifiers across the
module. It can be used, for instance, to generate new unique identifiers::

  >>> from pythran import analyses
  >>> code = 'a = b = 1'
  >>> tree = ast.parse(code)
  >>> sorted(pm.gather(analyses.Identifiers, tree))
  ['a', 'b']

One can also computes the state of ``globals()``::

  >>> code = 'import math\n'
  >>> code += 'def foo(a): b = math.cos(a) ; return [b] * 3'
  >>> tree = ast.parse(code)
  >>> sorted(list(pm.gather(analyses.Globals, tree)))
  ['__dispatch__', 'builtins', 'foo', 'math']

One can also compute the state of ``locals()`` at any point of the program::

  >>> l = pm.gather(analyses.Locals, tree)
  >>> fdef = tree.body[-1]
  >>> freturn = fdef.body[-1]
  >>> sorted(l[freturn])
  ['a', 'b', 'math']

The ``ConstantFolding`` pass relies on the eponymous analyse that flags all
constant expressions. In the previous code, there is only two constant
*expressions* but only one can be evaluate::

  >>> ce = pm.gather(analyses.ConstantExpressions, tree)
  >>> sorted(map(ast.dump, ce))
  ["Attribute(value=Name(id='math', ctx=Load(), annotation=None, type_comment=None), attr='cos', ctx=Load())", 'Constant(value=3, kind=None)']

One of the most critical analyse of Pythran is the points-to analysis. There
are two flavors of this analyse, one that computes an over-set of the aliased
variable, and one that computes an under set. ``Aliases`` computes an over-set::

  >>> code = 'def foo(c, d): b= c or d ; return b'
  >>> tree = ast.parse(code)
  >>> al = pm.gather(analyses.Aliases, tree)
  >>> returned = tree.body[-1].body[-1].value
  >>> print(ast.dump(returned))
  Name(id='b', ctx=Load(), annotation=None, type_comment=None)
  >>> sorted(a.id for a in al[returned])
  ['c', 'd']

Pythran also implements an inter-procedural analyse to compute which arguments
are updated, for instance using an augmented assign, or the ``append`` method::

  >>> code = 'def foo(l,a): l+=[a]\ndef bar(g): foo(g, 1)'
  >>> tree = ast.parse(code)
  >>> ae = pm.gather(analyses.ArgumentEffects, tree)
  >>> foo, bar = tree.body[0], tree.body[1]
  >>> ae[foo]
  [True, False]
  >>> ae[bar]
  [True]

From this analyse and the ``GlobalEffects`` analyse, one can compute the set of
pure functions, i.e. functions that have no side effects::

  >>> code = 'import random\ndef f():pass\ndef b(l): random.seed(0)'
  >>> tree = ast.parse(code)
  >>> pf = pm.gather(analyses.PureExpressions, tree)
  >>> f = tree.body[1]
  >>> b = tree.body[2]
  >>> f in pf
  True
  >>> b in pf
  False

Pure functions are also interesting in the context of ``map``, as the
application of a pure functions using a map results in a parallel ``map``::

  >>> code = 'def foo(x): return x*x\n'
  >>> code += 'builtins.map(foo, builtins.range(100))'
  >>> tree = ast.parse(code)
  >>> pmaps = pm.gather(analyses.ParallelMaps, tree)
  >>> len(pmaps)
  1
