.. _net.sf.cimg.CImgExpression:

GMICExpr node
=============

|pluginIcon| 

*This documentation is for version 2.1 of GMICExpr.*

Description
-----------

Quickly generate or process image from mathematical formula evaluated for each pixel. Full documentation for `G'MIC <http://gmic.eu/>`__/`CImg <http://cimg.eu/>`__ expressions is reproduced below and available online from the `G'MIC help <http://gmic.eu/reference.shtml#section9>`__. The only additions of this plugin are the predefined variables ``T`` (current time) and ``K`` (render scale).

Uses the 'fill' function from the CImg library. CImg is a free, open-source library distributed under the CeCILL-C (close to the GNU LGPL) or CeCILL (compatible with the GNU GPL) licenses. It can be used in commercial applications (see http://cimg.eu).

Sample expressions
~~~~~~~~~~~~~~~~~~

-  ``j(sin(y/100/K+T/10)*20*K,sin(x/100/K+T/10)*20*K)'`` distorts the image with time-varying waves.
-  ``0.5*(j(1)-j(-1))`` will estimate the X-derivative of an image with a classical finite difference scheme.
-  ``if(x%10==0,1,i)`` will draw blank vertical lines on every 10th column of an image.

Expression language
~~~~~~~~~~~~~~~~~~~

-  The expression is evaluated for each pixel of the selected images.
-  The mathematical parser understands the following set of functions, operators and variables:

   -  Usual operators: ``||`` (logical or), ``&&`` (logical and), ``|`` (bitwise or), ``&`` (bitwise and), ``!=``, ``==``, ``<=``, ``>=``, ``<``, ``>``, ``<<`` (left bitwise shift), ``>>`` (right bitwise shift), ``-``, ``+``, ``*``, ``/``, ``%`` (modulo), ``^`` (power), ``!`` (logical not), ``~`` (bitwise not), ``++``, ``--``, ``+=``, ``-=``, ``*=``, ``/=``, ``%=``, ``&=``, ``|=``, ``^=``, ``>>=``, ``<<=`` (in-place operators).
   -  Usual functions: ``abs()``, ``acos()``, ``arg()``, ``argmax()``, ``argmin()``, ``asin()``, ``atan()``, ``atan2()``, ``cbrt()``, ``cos()``, ``cosh()``, ``cut()``, ``exp()``, ``fact()``, ``fibo()``, ``gauss()``, ``hypoth()``, ``int()``, ``isval()``, ``isnan()``, ``isinf()``, ``isint()``, ``isbool()``, ``isfile()``, ``isdir()``, ``isin()``, ``kth()``, ``log()``, ``log2()``, ``log10()``, ``max()``, ``mean()``, ``med()``, ``min()``, ``narg()``, ``prod()``, ``rol()`` (left bit rotation), ``ror()`` (right bit rotation), ``round()``, ``sign()``, ``sin()``, ``sinc()``, ``sinh()``, ``sqrt()``, ``std()``, ``sum()``, ``tan()``, ``tanh()``, ``variance()``.

      -  ``atan2(x,y)`` is the version of ``atan()`` with two arguments ``y`` and ``x`` (as in C/C++).
      -  ``hypoth(x,y)`` computes the square root of the sum of the squares of x and y.
      -  ``permut(k,n,with_order)`` computes the number of permutations of k objects from a set of k objects.
      -  ``gauss(x,_sigma)`` returns ``exp(-x^2/(2*s^2))/sqrt(2*pi*sigma^2)``.
      -  ``cut(value,min,max)`` returns value if it is in range [min,max], or min or max otherwise.
      -  ``narg(a_1,...,a_N)`` returns the number of specified arguments (here, N).
      -  ``arg(i,a_1,..,a_N)`` returns the ith argument ``a_i``.
      -  ``isval()``, ``isnan()``, ``isinf()``, ``isint()``, ``isbool()`` test the type of the given number or expression, and return 0 (false) or 1 (true).
      -  ``isfile()`` (resp. ``isdir()``) returns 0 (false) or 1 (true) whether its argument is a valid path to a file (resp. to a directory) or not.
      -  ``isin(v,a_1,...,a_n)`` returns 0 (false) or 1 (true) whether the first value ``v`` appears in the set of other values ``a_i``.
      -  ``argmin()``, ``argmax()``, ``kth()``, ``max()``, ``mean()``, ``med()``, ``min()``, ``std()``, ``sum()`` and ``variance()`` can be called with an arbitrary number of scalar/vector arguments.
      -  ``round(value,rounding_value,direction)`` returns a rounded value. ``direction`` can be { -1=to-lowest \| 0=to-nearest \| 1=to-highest }.

   -  Variable names below are pre-defined. They can be overrided.

      -  ``l``: length of the associated list of images, if any (0 otherwise).
      -  ``w``: width of the associated image, if any (0 otherwise).
      -  ``h``: height of the associated image, if any (0 otherwise).
      -  ``d``: depth of the associated image, if any (0 otherwise).
      -  ``s``: spectrum of the associated image, if any (0 otherwise).
      -  ``r``: shared state of the associated image, if any (0 otherwise).
      -  ``wh``: shortcut for width x height.
      -  ``whd``: shortcut for width x height x depth.
      -  ``whds``: shortcut for width x height x depth x spectrum (i.e. total number of pixel values).
      -  ``i``: current processed pixel value (i.e. value located at (x,y,z,c)) in the associated image, if any (0 otherwise).
      -  ``iN``: Nth channel value of current processed pixel (i.e. value located at (x,y,z,N)) in the associated image, if any (0 otherwise). ``N`` must be an integer in range [0,7].
      -  ``R``,\ ``G``,\ ``B`` and ``A`` are equivalent to ``i0``, ``i1``, ``i2`` and ``i3`` respectively.
      -  ``im``,\ ``iM``,\ ``ia``,\ ``iv``,\ ``is``,\ ``ip``,\ ``ic``: Respectively the minimum, maximum, average values, variance, sum, product and median value of the associated image, if any (0 otherwise).
      -  ``xm``,\ ``ym``,\ ``zm``,\ ``cm``: The pixel coordinates of the minimum value in the associated image, if any (0 otherwise).
      -  ``xM``,\ ``yM``,\ ``zM``,\ ``cM``: The pixel coordinates of the maximum value in the associated image, if any (0 otherwise).
      -  You may add ``#ind`` to any of the variable name above to retrieve the information for any numbered image ``[ind]`` of the list (when this makes sense). For instance ``ia#0`` denotes the average value of the first image).
      -  ``x``: current processed column of the associated image, if any (0 otherwise).
      -  ``y``: current processed row of the associated image, if any (0 otherwise).
      -  ``z``: current processed slice of the associated image, if any (0 otherwise).
      -  ``c``: current processed channel of the associated image, if any (0 otherwise).
      -  ``t``: thread id when an expression is evaluated with multiple threads (0 means ``master thread``).
      -  ``T``: current time [OpenFX-only].
      -  ``K``: render scale (1 means full scale, 0.5 means half scale) [OpenFX-only].
      -  ``e``: value of e, i.e. 2.71828..
      -  ``pi``: value of pi, i.e. 3.1415926..
      -  ``u``: a random value between [0,1], following a uniform distribution.
      -  ``g``: a random value, following a gaussian distribution of variance 1 (roughly in [-6,6]).
      -  ``interpolation``: value of the default interpolation mode used when reading pixel values with the pixel access operators (i.e. when the interpolation argument is not explicitly specified, see below for more details on pixel access operators). Its initial default value is 0.
      -  ``boundary``: value of the default boundary conditions used when reading pixel values with the pixel access operators (i.e. when the boundary condition argument is not explicitly specified, see below for more details on pixel access operators). Its initial default value is 0.

   -  Vector calculus: Most operators are also able to work with vector-valued elements.

      -  ``[ a0,a1,..,aN ]`` defines a (N+1)-dimensional vector with specified scalar coefficients ak.
      -  ``vectorN(a0,a1,,..,)`` does the same, with the ak being repeated periodically.
      -  In both expressions, the ak can be vectors themselves, to be concatenated into a single vector.
      -  The scalar element ak of a vector X is retrieved by ``X[k]``.
      -  The sub-vector [ ap..aq ] of a vector X is retrieved by ``X[p,q]``.
      -  Equality/inequality comparisons between two vectors is possible with the operators ``==`` and ``!=``.
      -  Some vector-specific functions can be used on vector values: ``cross(X,Y)`` (cross product), ``dot(X,Y)`` (dot product), ``size(X)`` (vector dimension), ``sort(X,_is_increasing,_chunk_size)`` (sorting values), ``reverse(A)`` (reverse order of components) and ``same(A,B,_nb_vals,_is_case_sensitive)`` (vector equality test).
      -  Function ``resize(A,size,_interpolation)`` returns a resized version of vector ``A`` with specified interpolation mode. ``interpolation`` can be { -1=none (memory content) \| 0=none \| 1=nearest \| 2=average \| 3=linear \| 4=grid \| 5=bicubic \| 6=lanczos }.
      -  Function ``find(A,B,_is_forward,_starting_indice)`` returns the index where sub-vector B appears in vector A, (or -1 if B is not found in A). Argument A can be also replaced by an image indice #ind.
      -  A 2-dimensional vector may be seen as a complex number and used in those particular functions/operators: ``**`` (complex multiplication), ``//`` (complex division), ``^^`` (complex exponentiation), ``**=`` (complex self-multiplication), ``//=`` (complex self-division), ``^^=`` (complex self-exponentiation), ``cabs()`` (complex modulus), ``carg()`` (complex argument), ``cconj()`` (complex conjugate), ``cexp()`` (complex exponential) and ``clog()`` (complex logarithm).
      -  A MN-dimensional vector may be seen as a M x N matrix and used in those particular functions/operators: ``**`` (matrix-vector multiplication), ``det(A)`` (determinant), ``diag(V)`` (diagonal matrix from vector), ``eig(A)`` (eigenvalues/eigenvectors), ``eye(n)`` (n x n identity matrix), ``inv(A)`` (matrix inverse), ``mul(A,B,_nb_colsB)`` (matrix-matrix multiplication), ``rot(x,y,z,angle)`` (3d rotation matrix), ``rot(angle)`` (2d rotation matrix), ``solve(A,B,_nb_colsB)`` (least-square solver of linear system A.X = B), ``trace(A)`` (matrix trace) and ``transp(A,nb_colsA)`` (matrix transpose). Argument ``nb_colsB`` may be omitted if equal to 1.
      -  Specifying a vector-valued math expression as an expression modifies the whole spectrum range of the processed image(s), for each spatial coordinates (x,y,z). The command does not loop over the C-axis in this case.

   -  String manipulation: Character strings are defined and managed as vectors objects. Dedicated functions and initializers to manage strings are

      -  ``[ 'string' ]`` and ``'string'`` define a vector whose values are the ascii codes of the specified character string (e.g. ``'foo'`` is equal to ``[ 102,111,111 ]``).
      -  ``_'character'`` returns the (scalar) ascii code of the specified character (e.g. ``_'A'`` is equal to 65).
      -  A special case happens for empty strings: Values of both expressions ``[ '' ]`` and ``''`` are 0.
      -  Functions ``lowercase()`` and ``uppercase()`` return string with all string characters lowercased or uppercased.

   -  Special operators can be used:

      -  ``;``: expression separator. The returned value is always the last encountered expression. For instance expression ``1;2;pi`` is evaluated as ``pi``.
      -  ``=``: variable assignment. Variables in mathematical parser can only refer to numerical values. Variable names are case-sensitive. Use this operator in conjunction with ``;`` to define more complex evaluable expressions, such as ``t=cos(x);3*t^2+2*t+1``. These variables remain local to the mathematical parser and cannot be accessed outside the evaluated expression.

   -  The following specific functions are also defined:

      -  ``normP(u1,...,un)`` computes the LP-norm of the specified vector (P being an unsigned integer or ``inf``).
      -  ``u(max)`` or ``u(min,max)``: return a random value between [0,max] or [min,max], following a uniform distribution.
      -  ``i(_a,_b,_c,_d,_interpolation_type,_boundary_conditions)``: return the value of the pixel located at position (a,b,c,d) in the associated image, if any (0 otherwise). ``interpolation_type`` can be { 0=nearest neighbor \| other=linear }. ``boundary_conditions`` can be { 0=dirichlet \| 1=neumann \| 2=periodic }. Omitted coordinates are replaced by their default values which are respectively x, y, z, c, interpolation and boundary. For instance expression ``0.5*(i(x+1)-i(x-1))`` will estimate the X-derivative of an image with a classical finite difference scheme.
      -  ``j(_dx,_dy,_dz,_dc,_interpolation_type,_boundary_conditions)`` does the same for the pixel located at position (x+dx,y+dy,z+dz,c+dc) (pixel access relative to the current coordinates).
      -  ``i[offset,_boundary_conditions]`` returns the value of the pixel located at specified ``offset`` in the associated image buffer (or 0 if offset is out-of-bounds).
      -  ``j[offset,_boundary_conditions]`` does the same for an offset relative to the current pixel (x,y,z,c).
      -  ``i(#ind,_x,_y,_z,_c,_interpolation,_boundary)``, ``j(#ind,_dx,_dy,_dz,_dc,_interpolation,_boundary)``, ``i[#ind,offset,_boundary]`` and ``i[offset,_boundary]`` are similar expressions used to access pixel values for any numbered image ``[ind]`` of the list.
      -  ``I/J[offset,_boundary_conditions]`` and ``I/J(#ind,_x,_y,_z,_interpolation,_boundary)`` do the same as ``i/j[offset,_boundary_conditions]`` and ``i/j(#ind,_x,_y,_z,_c,_interpolation,_boundary)`` but return a vector instead of a scalar (e.g. a vector [ R,G,B ] for a pixel at (a,b,c) in a color image).
      -  ``crop(_#ind,_x,_y,_z,_c,_dx,_dy,_dz,_dc,_boundary)`` returns a vector whose values come from the cropped region of image ``[ind]`` (or from default image selected if ``ind`` is not specified). Cropped region starts from point (x,y,z,c) and has a size of dx x dy x dz x dc. Arguments for coordinates and sizes can be omitted if they are not ambiguous (e.g. ``crop(#ind,x,y,dx,dy)`` is a valid invokation of this function).
      -  ``draw(_#ind,S,x,y,z,c,dx,_dy,_dz,_dc,_opacity,_M,_max_M)`` draws a sprite S in image ``[ind]`` (or in default image selected if ``ind`` is not specified) at specified coordinates (x,y,z,c). The size of the sprite dx x dy x dz x dc must be specified. You can also specify a corresponding opacity mask M if its size matches S.
      -  ``if(condition,expr_then,_expr_else)``: return value of ``expr_then`` or ``expr_else``, depending on the value of ``condition`` (0=false, other=true). ``expr_else`` can be omitted in which case 0 is returned if the condition does not hold. Using the ternary operator ``condition?expr_then[:expr_else]`` gives an equivalent expression. For instance, G'MIC expressions ``if(x%10==0,255,i)`` and ``x%10?i:255`` both draw blank vertical lines on every 10th column of an image.
      -  ``dowhile(expression,_condition)`` repeats the evaluation of ``expression`` until ``condition`` vanishes (or until ``expression`` vanishes if no ``condition`` is specified). For instance, the expression: ``if(N<2,N,n=N-1;F0=0;F1=1;dowhile(F2=F0+F1;F0=F1;F1=F2,n=n-1))`` returns the Nth value of the Fibonacci sequence, for N>=0 (e.g., 46368 for N=24). ``dowhile(expression,condition)`` always evaluates the specified expression at least once, then check for the nullity condition. When done, it returns the last value of ``expression``.
      -  ``for(init,condition,_procedure,body)`` first evaluates the expression ``init``, then iteratively evaluates ``body`` (followed by ``procedure`` if specified) while ``condition`` is verified (i.e. not zero). It may happen that no iteration is done, in which case the function returns 0. Otherwise, it returns the last value of ``body``. For instance, the expression: ``if(N<2,N,for(n=N;F0=0;F1=1,n=n-1,F2=F0+F1;F0=F1;F1=F2))`` returns the Nth value of the Fibonacci sequence, for N>=0 (e.g., 46368 for N=24).
      -  ``whiledo(condition,expression)`` is exactly the same as ``for(init,condition,expression)`` without the specification of an initializing expression.
      -  ``date(attr,path)`` returns the date attribute for the given ``path`` (file or directory), with ``attr`` being { 0=year \| 1=month \| 2=day \| 3=day of week \| 4=hour \| 5=minute \| 6=second }.
      -  ``date(_attr)`` returns the specified attribute for the current (locale) date.
      -  ``print(expression)`` prints the value of the specified expression on the console (and returns its value).
      -  ``debug(expression)`` prints detailed debug information about the sequence of operations done by the math parser to evaluate the expression (and returns its value).
      -  ``init(expression)`` evaluates the specified expression only once, even when multiple evaluations are required (e.g. in ``init(foo=0);++foo``).
      -  ``copy(dest,src,_nb_elts,_inc_d,_inc_s)`` copies an entire memory block of ``nb_elts`` elements starting from a source value ``src`` to a specified destination ``dest``, with increments defined by ``inc_d`` and ``inc_s`` respectively for the destination and source pointers.

   -  User-defined functions:

      -  Custom macro functions can be defined in a math expression, using the assignment operator ``=``, e.g. ``foo(x,y) = cos(x + y); result = foo(1,2) + foo(2,3)``.
      -  Overriding a built-in function has no effect.
      -  Overriding an already defined macro function replaces its old definition.
      -  Macro functions are indeed processed as macros by the mathematical evaluator. You should avoid invoking them with arguments that are themselves results of assignments or self-operations. For instance, ``foo(x) = x + x; z = 0; result = foo(++x)`` will set ``result = 4`` rather than expected value ``2``.

   -  Multi-threaded and in-place evaluation:

      -  If your image data are large enough and you have several CPUs available, it is likely that the math expression is evaluated in parallel, using multiple computation threads.
      -  Starting an expression with ``:`` or ``*`` forces the evaluations required for an image to be run in parallel, even if the amount of data to process is small (beware, it may be slower to evaluate!). Specify ``:`` (instead of ``*``) to avoid possible image copy done before evaluating the expression (this saves memory, but do this only if you are sure this step is not required!)
      -  If the specified expression starts with ``>`` or ``<``, the pixel access operators ``i()``, ``i[]``, ``j()`` and ``j[]`` return values of the image being currently modified, in forward (``>``) or backward (``<``) order. The multi-threading evaluation of the expression is also disabled in this case.
      -  Function ``(operands)`` forces the execution of the given operands in a single thread at a time.

   -  Expressions ``i(_#ind,x,_y,_z,_c)=value``, ``j(_#ind,x,_y,_z,_c)=value``, ``i[_#ind,offset]=value`` and ``j[_#ind,offset]=value`` set a pixel value at a different location than the running one in the image ``[ind]`` (or in the associated image if argument ``#ind`` is omitted), either with global coordinates/offsets (with ``i(...)`` and ``i[...]``), or relatively to the current position (x,y,z,c) (with ``j(...)`` and ``j[...]``). These expressions always return ``value``.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | Yes        |
+----------+---------------+------------+
| Mask     |               | Yes        |
+----------+---------------+------------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+--------------------------------+-----------+-----------+--------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name        | Type      | Default   | Function                                                                                                                             |
+================================+===========+===========+======================================================================================================================================+
| Expression / ``expression``    | String    | i         | G'MIC/CImg expression, see the plugin description/help, or http://gmic.eu/reference.shtml#section9                                   |
+--------------------------------+-----------+-----------+--------------------------------------------------------------------------------------------------------------------------------------+
| Help... / ``help``             | Button    |           | Display help for writing GMIC expressions.                                                                                           |
+--------------------------------+-----------+-----------+--------------------------------------------------------------------------------------------------------------------------------------+
| (Un)premult / ``premult``      | Boolean   | Off       | Divide the image by the alpha channel before processing, and re-multiply it afterwards. Use if the input images are premultiplied.   |
+--------------------------------+-----------+-----------+--------------------------------------------------------------------------------------------------------------------------------------+
| Invert Mask / ``maskInvert``   | Boolean   | Off       | When checked, the effect is fully applied where the mask is 0.                                                                       |
+--------------------------------+-----------+-----------+--------------------------------------------------------------------------------------------------------------------------------------+
| Mix / ``mix``                  | Double    | 1         | Mix factor between the original and the transformed image.                                                                           |
+--------------------------------+-----------+-----------+--------------------------------------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: net.sf.cimg.CImgExpression.png
   :width: 10.0%
