.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Expressions
===========

.. toctree::
   :maxdepth: 2

.. _Basic Expressions: http://www.partow.net/programming/exprtk/
.. _exprtkREADME: https://raw.githubusercontent.com/ArashPartow/exprtk/master/readme.txt

The value of any Node :doc:`parameter<PythonReference/NatronEngine/Param>` can be set by
Python expression.
An expression is a line of code that can either reference the value
of other parameters or apply mathematical functions to the current value.

Natron supports 2 types of expression languages:

    * :ref:`Python<paramExpr>`
    * Basic Expressions_

Basic Expression are very fast and simple and should cover 99% of the
needs for expressions. By default this is the language proposed to you when editing an
expression.

Python based expressions are using the same A.P.I as everything else using Python in Natron.
It allows to write really any kind of expression referencing external functions and data,
however it is much slower to evaluate and will impair performance compared to a basic expression.

Where possible, you should use basic expressions, unless you specifically need a feature available
in the Python API that is not available through the basic expressions language.


For more informations on developping Python expression, please refer to :ref:`this section<paramExpr>`.

The rest of this section will cover writing basic expressions.


Basic expressions
------------------


**Arithmetic & Assignment Operators**

+----------+---------------------------------------------------------+
| OPERATOR | DEFINITION                                              |
+==========+=========================================================+
|  \ +     | Addition between x and y.  (eg: x + y)                  |
+----------+---------------------------------------------------------+
|  \ -     | Subtraction between x and y.  (eg: x - y)               |
+----------+---------------------------------------------------------+
|  \ *     | Multiplication between x and y.  (eg: x * y)            |
+----------+---------------------------------------------------------+
|  /       | Division between x and y.  (eg: x / y)                  |
+----------+---------------------------------------------------------+
|  %       | Modulus of x with respect to y.  (eg: x % y)            |
+----------+---------------------------------------------------------+
|  ^       | x to the power of y.  (eg: x ^ y)                       |
+----------+---------------------------------------------------------+
|  :=      | Assign the value of x to y. Where y is either a variable|
|          | or vector type.  (eg: y := x)                           |
+----------+---------------------------------------------------------+
|  +=      | Increment x by the value of the expression on the right |
|          | hand side. Where x is either a variable or vector type. |
|          | (eg: x += abs(y - z))                                   |
+----------+---------------------------------------------------------+
|  -=      | Decrement x by the value of the expression on the right |
|          | hand side. Where x is either a variable or vector type. |
|          | (eg: x[i] -= abs(y + z))                                |
+----------+---------------------------------------------------------+
|  *=      | Assign the multiplication of x by the value of the      |
|          | expression on the righthand side to x. Where x is either|
|          | a variable or vector type.                              |
|          | (eg: x *= abs(y / z))                                   |
+----------+---------------------------------------------------------+
|  /=      | Assign the division of x by the value of the expression |
|          | on the right-hand side to x. Where x is either a        |
|          | variable or vector type.  (eg: x[i + j] /= abs(y * z))  |
+----------+---------------------------------------------------------+
|  %=      | Assign x modulo the value of the expression on the right|
|          | hand side to x. Where x is either a variable or vector  |
|          | type.  (eg: x[2] %= y ^ 2)                              |
+----------+---------------------------------------------------------+

**Equalities & Inequalities**

+----------+---------------------------------------------------------+
| OPERATOR | DEFINITION                                              |
+==========+=========================================================+
| == or =  | True only if x is strictly equal to y. (eg: x == y)     |
+----------+---------------------------------------------------------+
| <> or != | True only if x does not equal y. (eg: x <> y or x != y) |
+----------+---------------------------------------------------------+
|  <       | True only if x is less than y. (eg: x < y)              |
+----------+---------------------------------------------------------+
|  <=      | True only if x is less than or equal to y. (eg: x <= y) |
+----------+---------------------------------------------------------+
|  >       | True only if x is greater than y. (eg: x > y)           |
+----------+---------------------------------------------------------+
|  >=      | True only if x greater than or equal to y. (eg: x >= y) |
+----------+---------------------------------------------------------+

**Boolean Operations**

+----------+---------------------------------------------------------+
| OPERATOR | DEFINITION                                              |
+==========+=========================================================+
| true     | True state or any value other than zero (typically 1).  |
+----------+---------------------------------------------------------+
| false    | False state, value of zero.                             |
+----------+---------------------------------------------------------+
| and      | Logical AND, True only if x and y are both true.        |
|          | (eg: x and y)                                           |
+----------+---------------------------------------------------------+
| mand     | Multi-input logical AND, True only if all inputs are    |
|          | true. Left to right short-circuiting of expressions.    |
|          | (eg: mand(x > y, z < w, u or v, w and x))               |
+----------+---------------------------------------------------------+
| mor      | Multi-input logical OR, True if at least one of the     |
|          | inputs are true. Left to right short-circuiting of      |
|          | expressions.  (eg: mor(x > y, z < w, u or v, w and x))  |
+----------+---------------------------------------------------------+
| nand     | Logical NAND, True only if either x or y is false.      |
|          | (eg: x nand y)                                          |
+----------+---------------------------------------------------------+
| nor      | Logical NOR, True only if the result of x or y is false |
|          | (eg: x nor y)                                           |
+----------+---------------------------------------------------------+
| not      | Logical NOT, Negate the logical sense of the input.     |
|          | (eg: not(x and y) == x nand y)                          |
+----------+---------------------------------------------------------+
| or       | Logical OR, True if either x or y is true. (eg: x or y) |
+----------+---------------------------------------------------------+
| xor      | Logical XOR, True only if the logical states of x and y |
|          | differ.  (eg: x xor y)                                  |
+----------+---------------------------------------------------------+
| xnor     | Logical XNOR, True iff the biconditional of x and y is  |
|          | satisfied.  (eg: x xnor y)                              |
+----------+---------------------------------------------------------+
| &        | Similar to AND but with left to right expression short  |
|          | circuiting optimisation.  (eg: (x & y) == (y and x))    |
+----------+---------------------------------------------------------+
| |        | Similar to OR but with left to right expression short   |
|          | circuiting optimisation.  (eg: (x | y) == (y or x))     |
+----------+---------------------------------------------------------+

**General Purpose Functions**

+----------+---------------------------------------------------------+
| FUNCTION | DEFINITION                                              |
+==========+=========================================================+
| abs      | Absolute value of x.  (eg: abs(x))                      |
+----------+---------------------------------------------------------+
| avg      | Average of all the inputs.                              |
|          | (eg: avg(x,y,z,w,u,v) == (x + y + z + w + u + v) / 6)   |
+----------+---------------------------------------------------------+
| ceil     | Smallest integer that is greater than or equal to x.    |
+----------+---------------------------------------------------------+
| clamp    | Clamp x in range between r0 and r1, where r0 < r1.      |
|          | (eg: clamp(r0,x,r1))                                    |
+----------+---------------------------------------------------------+
| equal    | Equality test between x and y using normalized epsilon  |
+----------+---------------------------------------------------------+
| erf      | Error function of x.  (eg: erf(x))                      |
+----------+---------------------------------------------------------+
| erfc     | Complimentary error function of x.  (eg: erfc(x))       |
+----------+---------------------------------------------------------+
| exp      | e to the power of x.  (eg: exp(x))                      |
+----------+---------------------------------------------------------+
| expm1    | e to the power of x minus 1, where x is very small.     |
|          | (eg: expm1(x))                                          |
+----------+---------------------------------------------------------+
| floor    | Largest integer that is less than or equal to x.        |
|          | (eg: floor(x))                                          |
+----------+---------------------------------------------------------+
| frac     | Fractional portion of x.  (eg: frac(x))                 |
+----------+---------------------------------------------------------+
| hypot    | Hypotenuse of x and y (eg: hypot(x,y) = sqrt(x*x + y*y))|
+----------+---------------------------------------------------------+
| iclamp   | Inverse-clamp x outside of the range r0 and r1. Where   |
|          | r0 < r1. If x is within the range it will snap to the   |
|          | closest bound. (eg: iclamp(r0,x,r1)                     |
+----------+---------------------------------------------------------+
| inrange  | In-range returns 'true' when x is within the range r0   |
|          | and r1. Where r0 < r1.  (eg: inrange(r0,x,r1)           |
+----------+---------------------------------------------------------+
| log      | Natural logarithm of x.  (eg: log(x))                   |
+----------+---------------------------------------------------------+
| log10    | Base 10 logarithm of x.  (eg: log10(x))                 |
+----------+---------------------------------------------------------+
| log1p    | Natural logarithm of 1 + x, where x is very small.      |
|          | (eg: log1p(x))                                          |
+----------+---------------------------------------------------------+
| log2     | Base 2 logarithm of x.  (eg: log2(x))                   |
+----------+---------------------------------------------------------+
| logn     | Base N logarithm of x. where n is a positive integer.   |
|          | (eg: logn(x,8))                                         |
+----------+---------------------------------------------------------+
| max      | Largest value of all the inputs. (eg: max(x,y,z,w,u,v)) |
+----------+---------------------------------------------------------+
| min      | Smallest value of all the inputs. (eg: min(x,y,z,w,u))  |
+----------+---------------------------------------------------------+
| mul      | Product of all the inputs.                              |
|          | (eg: mul(x,y,z,w,u,v,t) == (x * y * z * w * u * v * t)) |
+----------+---------------------------------------------------------+
| ncdf     | Normal cumulative distribution function.  (eg: ncdf(x)) |
+----------+---------------------------------------------------------+
| nequal   | Not-equal test between x and y using normalized epsilon |
+----------+---------------------------------------------------------+
| pow      | x to the power of y.  (eg: pow(x,y) == x ^ y)           |
+----------+---------------------------------------------------------+
| root     | Nth-Root of x. where n is a positive integer.           |
|          | (eg: root(x,3) == x^(1/3))                              |
+----------+---------------------------------------------------------+
| round    | Round x to the nearest integer.  (eg: round(x))         |
+----------+---------------------------------------------------------+
| roundn   | Round x to n decimal places  (eg: roundn(x,3))          |
|          | where n > 0 and is an integer.                          |
|          | (eg: roundn(1.2345678,4) == 1.2346)                     |
+----------+---------------------------------------------------------+
| sgn      | Sign of x, -1 where x < 0, +1 where x > 0, else zero.   |
|          | (eg: sgn(x))                                            |
+----------+---------------------------------------------------------+
| sqrt     | Square root of x, where x >= 0.  (eg: sqrt(x))          |
+----------+---------------------------------------------------------+
| sum      | Sum of all the inputs.                                  |
|          | (eg: sum(x,y,z,w,u,v,t) == (x + y + z + w + u + v + t)) |
+----------+---------------------------------------------------------+
| swap     | Swap the values of the variables x and y and return the |
| <=>      | current value of y.  (eg: swap(x,y) or x <=> y)         |
+----------+---------------------------------------------------------+
| trunc    | Integer portion of x.  (eg: trunc(x))                   |
+----------+---------------------------------------------------------+

**Trigonometry Functions**

+----------+---------------------------------------------------------+
| FUNCTION | DEFINITION                                              |
+==========+=========================================================+
| acos     | Arc cosine of x expressed in radians. Interval [-1,+1]  |
|          | (eg: acos(x))                                           |
+----------+---------------------------------------------------------+
| acosh    | Inverse hyperbolic cosine of x expressed in radians.    |
|          | (eg: acosh(x))                                          |
+----------+---------------------------------------------------------+
| asin     | Arc sine of x expressed in radians. Interval [-1,+1]    |
|          | (eg: asin(x))                                           |
+----------+---------------------------------------------------------+
| asinh    | Inverse hyperbolic sine of x expressed in radians.      |
|          | (eg: asinh(x))                                          |
+----------+---------------------------------------------------------+
| atan     | Arc tangent of x expressed in radians. Interval [-1,+1] |
|          | (eg: atan(x))                                           |
+----------+---------------------------------------------------------+
| atan2    | Arc tangent of (x / y) expressed in radians. [-pi,+pi]  |
|          | eg: atan2(x,y)                                          |
+----------+---------------------------------------------------------+
| atanh    | Inverse hyperbolic tangent of x expressed in radians.   |
|          | (eg: atanh(x))                                          |
+----------+---------------------------------------------------------+
| cos      | Cosine of x.  (eg: cos(x))                              |
+----------+---------------------------------------------------------+
| cosh     | Hyperbolic cosine of x.  (eg: cosh(x))                  |
+----------+---------------------------------------------------------+
| cot      | Cotangent of x.  (eg: cot(x))                           |
+----------+---------------------------------------------------------+
| csc      | Cosecant of x.  (eg: csc(x))                            |
+----------+---------------------------------------------------------+
| sec      | Secant of x.  (eg: sec(x))                              |
+----------+---------------------------------------------------------+
| sin      | Sine of x.  (eg: sin(x))                                |
+----------+---------------------------------------------------------+
| sinc     | Sine cardinal of x.  (eg: sinc(x))                      |
+----------+---------------------------------------------------------+
| sinh     | Hyperbolic sine of x.  (eg: sinh(x))                    |
+----------+---------------------------------------------------------+
| tan      | Tangent of x.  (eg: tan(x))                             |
+----------+---------------------------------------------------------+
| tanh     | Hyperbolic tangent of x.  (eg: tanh(x))                 |
+----------+---------------------------------------------------------+
| deg2rad  | Convert x from degrees to radians.  (eg: deg2rad(x))    |
+----------+---------------------------------------------------------+
| deg2grad | Convert x from degrees to gradians.  (eg: deg2grad(x))  |
+----------+---------------------------------------------------------+
| rad2deg  | Convert x from radians to degrees.  (eg: rad2deg(x))    |
+----------+---------------------------------------------------------+
| grad2deg | Convert x from gradians to degrees.  (eg: grad2deg(x))  |
+----------+---------------------------------------------------------+

**String Processing**

+----------+---------------------------------------------------------+
| FUNCTION | DEFINITION                                              |
+==========+=========================================================+
|  = , ==  | All common equality/inequality operators are applicable |
|  !=, <>  | to strings and are applied in a case sensitive manner.  |
|  <=, >=  | In the following example x, y and z are of type string. |
|  < , >   | (eg: not((x <= 'AbC') and ('1x2y3z' <> y)) or (z == x)  |
+----------+---------------------------------------------------------+
| in       | True only if x is a substring of y.                     |
|          | (eg: x in y or 'abc' in 'abcdefgh')                     |
+----------+---------------------------------------------------------+
| like     | True only if the string x matches the pattern y.        |
|          | Available wildcard characters are '*' and '?' denoting  |
|          | zero or more and zero or one matches respectively.      |
|          | (eg: x like y or 'abcdefgh' like 'a?d*h')               |
+----------+---------------------------------------------------------+
| ilike    | True only if the string x matches the pattern y in a    |
|          | case insensitive manner. Available wildcard characters  |
|          | are '*' and '?' denoting zero or more and zero or one   |
|          | matches respectively.                                   |
|          | (eg: x ilike y or 'a1B2c3D4e5F6g7H' ilike 'a?d*h')      |
+----------+---------------------------------------------------------+
| [r0:r1]  | The closed interval [r0,r1] of the specified string.    |
|          | eg: Given a string x with a value of 'abcdefgh' then:   |
|          | 1. x[1:4] == 'bcde'                                     |
|          | 2. x[ :5] == x[:5] == 'abcdef'                          |
|          | 3. x[3: ] == x[3:] =='cdefgh'                           |
|          | 4. x[ : ] == x[:] == 'abcdefgh'                         |
|          | 5. x[4/2:3+2] == x[2:5] == 'cdef'                       |
|          |                                                         |
|          | Note: Both r0 and r1 are assumed to be integers, where  |
|          | r0 <= r1. They may also be the result of an expression, |
|          | in the event they have fractional components truncation |
|          | will be performed. (eg: 1.67 --> 1)                     |
+----------+---------------------------------------------------------+
|  :=      | Assign the value of x to y. Where y is a mutable string |
|          | or string range and x is either a string or a string    |
|          | range. eg:                                              |
|          | 1. y := x                                               |
|          | 2. y := 'abc'                                           |
|          | 3. y := x[:i + j]                                       |
|          | 4. y := '0123456789'[2:7]                               |
|          | 5. y := '0123456789'[2i + 1:7]                          |
|          | 6. y := (x := '0123456789'[2:7])                        |
|          | 7. y[i:j] := x                                          |
|          | 8. y[i:j] := (x + 'abcdefg'[8 / 4:5])[m:n]              |
|          |                                                         |
|          | Note: For options 7 and 8 the shorter of the two ranges |
|          | will denote the number characters that are to be copied.|
+----------+---------------------------------------------------------+
|  \ +     | Concatenation of x and y. Where x and y are strings or  |
|          | string ranges. eg                                       |
|          | 1. x + y                                                |
|          | 2. x + 'abc'                                            |
|          | 3. x + y[:i + j]                                        |
|          | 4. x[i:j] + y[2:3] + '0123456789'[2:7]                  |
|          | 5. 'abc' + x + y                                        |
|          | 6. 'abc' + '1234567'                                    |
|          | 7. (x + 'a1B2c3D4' + y)[i:2j]                           |
+----------+---------------------------------------------------------+
|  +=      | Append to x the value of y. Where x is a mutable string |
|          | and y is either a string or a string range. eg:         |
|          | 1. x += y                                               |
|          | 2. x += 'abc'                                           |
|          | 3. x += y[:i + j] + 'abc'                               |
|          | 4. x += '0123456789'[2:7]                               |
+----------+---------------------------------------------------------+
| <=>      | Swap the values of x and y. Where x and y are mutable   |
|          | strings.  (eg: x <=> y)                                 |
+----------+---------------------------------------------------------+
| []       | The string size operator returns the size of the string |
|          | being actioned.                                         |
|          | eg:                                                     |
|          | 1. 'abc'[] == 3                                         |
|          | 2. var max_str_length := max(s0[],s1[],s2[],s3[])       |
|          | 3. ('abc' + 'xyz')[] == 6                               |
|          | 4. (('abc' + 'xyz')[1:4])[] == 4                        |
+----------+---------------------------------------------------------+

**Control Structures**

+----------+---------------------------------------------------------+
|STRUCTURE | DEFINITION                                              |
+==========+=========================================================+
| if       | If x is true then return y else return z.               |
|          | eg:                                                     |
|          | 1. if (x, y, z)                                         |
|          | 2. if ((x + 1) > 2y, z + 1, w / v)                      |
|          | 3. if (x > y) z;                                        |
|          | 4. if (x <= 2*y) { z + w };                             |
+----------+---------------------------------------------------------+
| if-else  | The if-else/else-if statement. Subject to the condition |
|          | branch the statement will return either the value of the|
|          | consequent or the alternative branch.                   |
|          | eg:                                                     |
|          | 1. if (x > y) z; else w;                                |
|          | 2. if (x > y) z; else if (w != u) v;                    |
|          | 3. if (x < y) {z; w + 1;} else u;                       |
|          | 4. if ((x != y) and (z > w))                            |
|          |    {                                                    |
|          |      y := sin(x) / u;                                   |
|          |      z := w + 1;                                        |
|          |    }                                                    |
|          |    else if (x > (z + 1))                                |
|          |    {                                                    |
|          |      w := abs (x - y) + z;                              |
|          |      u := (x + 1) > 2y ? 2u : 3u;                       |
|          |    }                                                    |
+----------+---------------------------------------------------------+
| switch   | The first true case condition that is encountered will  |
|          | determine the result of the switch. If none of the case |
|          | conditions hold true, the default action is assumed as  |
|          | the final return value. This is sometimes also known as |
|          | a multi-way branch mechanism.                           |
|          | eg:                                                     |
|          | switch                                                  |
|          | {                                                       |
|          |   case x > (y + z) : 2 * x / abs(y - z);                |
|          |   case x < 3       : sin(x + y);                        |
|          |   default          : 1 + x;                             |
|          | }                                                       |
+----------+---------------------------------------------------------+
| while    | The structure will repeatedly evaluate the internal     |
|          | statement(s) 'while' the condition is true. The final   |
|          | statement in the final iteration will be used as the    |
|          | return value of the loop.                               |
|          | eg:                                                     |
|          | while ((x -= 1) > 0)                                    |
|          | {                                                       |
|          |   y := x + z;                                           |
|          |   w := u + y;                                           |
|          | }                                                       |
+----------+---------------------------------------------------------+
| repeat/  | The structure will repeatedly evaluate the internal     |
| until    | statement(s) 'until' the condition is true. The final   |
|          | statement in the final iteration will be used as the    |
|          | return value of the loop.                               |
|          | eg:                                                     |
|          | repeat                                                  |
|          |   y := x + z;                                           |
|          |   w := u + y;                                           |
|          | until ((x += 1) > 100)                                  |
+----------+---------------------------------------------------------+
| for      | The structure will repeatedly evaluate the internal     |
|          | statement(s) while the condition is true. On each loop  |
|          | iteration, an 'incrementing' expression is evaluated.   |
|          | The conditional is mandatory whereas the initialiser    |
|          | and incrementing expressions are optional.              |
|          | eg:                                                     |
|          | for (var x := 0; (x < n) and (x != y); x += 1)          |
|          | {                                                       |
|          |   y := y + x / 2 - z;                                   |
|          |   w := u + y;                                           |
|          | }                                                       |
+----------+---------------------------------------------------------+
| break    | Break terminates the execution of the nearest enclosed  |
| break[]  | loop, allowing for the execution to continue on external|
|          | to the loop. The default break statement will set the   |
|          | return value of the loop to NaN, where as the return    |
|          | based form will set the value to that of the break      |
|          | expression.                                             |
|          | eg:                                                     |
|          | while ((i += 1) < 10)                                   |
|          | {                                                       |
|          |   if (i < 5)                                            |
|          |     j -= i + 2;                                         |
|          |   else if (i % 2 == 0)                                  |
|          |     break;                                              |
|          |   else                                                  |
|          |     break[2i + 3];                                      |
|          | }                                                       |
+----------+---------------------------------------------------------+
| continue | Continue results in the remaining portion of the nearest|
|          | enclosing loop body to be skipped.                      |
|          | eg:                                                     |
|          | for (var i := 0; i < 10; i += 1)                        |
|          | {                                                       |
|          |   if (i < 5)                                            |
|          |     continue;                                           |
|          |   j -= i + 2;                                           |
|          | }                                                       |
+----------+---------------------------------------------------------+
| return   | Return immediately from within the current expression.  |
|          | With the option of passing back a variable number of    |
|          | values (scalar, vector or string). eg:                  |
|          | 1. return [1];                                          |
|          | 2. return [x, 'abx'];                                   |
|          | 3. return [x, x + y,'abx'];                             |
|          | 4. return [];                                           |
|          | 5. if (x < y)                                           |
|          |     return [x, x - y, 'result-set1', 123.456];          |
|          |    else                                                 |
|          |     return [y, x + y, 'result-set2'];                   |
+----------+---------------------------------------------------------+
| ?:       | Ternary conditional statement, similar to that of the   |
|          | above denoted if-statement.                             |
|          | eg:                                                     |
|          | 1. x ? y : z                                            |
|          | 2. x + 1 > 2y ? z + 1 : (w / v)                         |
|          | 3. min(x,y) > z ? (x < y + 1) ? x : y : (w * v)         |
+----------+---------------------------------------------------------+
| ~        | Evaluate each sub-expression, then return as the result |
|          | the value of the last sub-expression. This is sometimes |
|          | known as multiple sequence point evaluation.            |
|          | eg:                                                     |
|          | ~(i := x + 1, j := y / z, k := sin(w/u)) == (sin(w/u))) |
|          | ~{i := x + 1; j := y / z; k := sin(w/u)} == (sin(w/u))) |
+----------+---------------------------------------------------------+
|  \ [*]   | Evaluate any consequent for which its case statement is |
|          | true. The return value will be either zero or the result|
|          | of the last consequent to have been evaluated.          |
|          | eg:                                                     |
|          | [*]                                                     |
|          | {                                                       |
|          |   case (x + 1) > (y - 2)    : x := z / 2 + sin(y / pi); |
|          |   case (x + 2) < abs(y + 3) : w / 4 + min(5y,9);        |
|          |   case (x + 3) == (y * 4)   : y := abs(z / 6) + 7y;     |
|          | }                                                       |
+----------+---------------------------------------------------------+
| []       | The vector size operator returns the size of the vector |
|          | being actioned.                                         |
|          | eg:                                                     |
|          | 1. v[]                                                  |
|          | 2. max_size := max(v0[],v1[],v2[],v3[])                 |
+----------+---------------------------------------------------------+

To get more details on the language syntax and available mathematical functions, this is fully covered by the :ref:`README<exprtkREADME>`.


Additional variables and functions are made available by Natron to access values of other
parameters and effects.

Parameters
----------

Parameters value can be referenced by their *script-name*.
See :ref:`this section<nodeScriptName>` to learn how to determine the *script-name* of a node.
See :ref:`this section<paramScriptName>` to learn how to determine the *script-name* of a parameter.

For instance::

    # This is the value in the x dimension of the size parameter of the Blur1 node
    Blur1.size.x

    # The operator parameter is a 1-dimensional Choice parameter, there's no dimension
    # suffix
    Merge1.operator

Note that parameters on the node on which the expression is actually set can be referenced
without prefixing the script-name of the node::

    # Assuming we are writing an expression on the node Blur1,
    # its parameter size can be accessed directly
    size.y

In the same way, values of dimensions can be accessed directly using the special variable *thisParam*::

    # Assuming we are writing an expression for Merge1.operator
    thisParam

    # Assuming we are writing an expression for Blur1.size.y
    thisParam.x

.. warning::

    Returning the value of the parameter dimension for which the expression is being
    evaluated will not cause an infinite recursion but instead will return the value of
    the parameter without evaluating the expression.

Dimension names (x,y,r,g,b, w,h , etc...) are merely corresponding to a 0-based index,
and can be interchanged, e.g::

    # Referencing dimension 0 of the parameter size
    size.r

    # Referencing dimension 1 of the parameter size
    size.g

    # Referencing dimension 0 of the parameter size
    size.x

    # Referencing dimension 0 of the parameter size
    size.0

    # Referencing dimension 1 of the parameter size
    size.1

This allows to write easier expressions when referencing other parameters that do not
have the same dimensions.

Possible variants to reference a dimension is as follow:

    * r,g,b,a
    * x,y,z,w
    * 0,1,2,3

The dimension of the parameter on which the expression is currently executed can be
referenced with *dimension*::

    # Assuming we are writing an expression for size.y
    # We return the value of the masterSaturation parameter of the ColorCorrect1 node
    # at the same dimension (y)

    ColorCorrect1.masterSaturation.dimension

An expression on a parameter can reference any other parameter on the same node
and may also reference parameters on some other nodes, including:

    * Any other node in the same Group
    * If this node belongs to a sub-group, it may reference the Group node itself using the  special variable *thisGroup*
    * If this node is a Group itself, it may reference any node within its sub-group by prefixing *thisNode*, e.g::

    thisNode.Blur1

    # Assuming we are editing an expression on the *disabled* parameter of the
    # Group1.Blur1 node and that the Group1 node has a boolean parameter,
    # that was created with a script-name *enableBlur*,
    # we could write an expression referencing *enableBlur*
    # to enable or disable the internal Blur1 node as such:

    thisGroup.enableBlur

To easily get the input of a node, you may use the variable *input* followed by the index, e.g::

    # Assuming the input 0 of the Blur1 node is a Merge node, we
    # can reference the operator param of the input node
    Blur1.input0.operator

Animated parameters
-------------------

A parameter may be animated with keyframes.
Similarly, each curve may be different for each project view if the user split views for the parameter.

To retrieve the value at a different frame and view than the frame and view for which the expression,
is being evaluated you may specify it in arguments::

    # Returns the value at the current frame for the current view
    Blur1.size.x

    # Returns the value at frame + 1 for the current view
    Blur1.size.x(frame + 1)

    # Returns the value at frame + 1 for the given view
    Blur1.size.x(frame + 1, 'Right')

Pre-defined variables
---------------------

When the expression is called, a number of pre-declared variables can be used:

    * **thisNode**: It references the node holding the parameter being edited. This can only
    be used to reference parameters on the current node or sub-nodes if this node is a Group

    * **thisGroup**: It references the group containing *thisNode*. This is useful to reference
    a parameter on the group node containing this node

    * **thisParam**: It references the param being edited. This is useful to reference a value
    on the parameter on which the expression is evaluated

    * **thisItem**: If the expression is edited on a table item such as a Bezier or a Track
    this is the item holding the parameter to which the expression is being edited

    * **dimension**: It indicates the dimension (0-based index) of the parameter on which the expression
    is evaluated, this can only be used after a parameter, e.g: Blur1.size.dimension

    * **app**: References the project settings. This can be used as a prefix to reference
    project parameters, e.g: app.outputFormat

    * **frame**: It references the current time on the timeline at which the expression is evaluated
    this may be a floating point number if the parameter is referenced from a node that uses
    motion blur. If the parameter is a string parameter, the frame variable is already a string
    otherwise it will be a scalar.

    * **view**: It references the current view for which the expression is evaluated.
    If the parameter is a string parameter, the view will be the name of the view as seen
    in the project settings, otherwise this will be a 0-based index.

Script-name of a node or parameter
---------------------------------

The name of a parameter or Node can be returned in an expression using the attribute *name*::

    thisNode.name
    thisNode.input0.name
    thisKnob.name
    ...

Converting numbers to string
----------------------------

You can convert numbers to string with the **str(value, format, precision)** function.

The format controls how the number will be formatted in the string.
It must match one of the following letters:

* e format as [-]9.9e[+|-]999
* E format as [-]9.9E[+|-]999
* f format as [-]9.9
* g use e or f format, whichever is the most concise
* G use E or f format, whichever is the most concise

A precision is also specified with the argument format.
For the 'e', 'E', and 'f' formats, the precision represents the number of digits after the decimal point.
For the 'g' and 'G' formats, the precision represents the maximum number of significant digits (trailing zeroes are omitted).

Example::

    str(2.8940,'f',2) = "2.89"


Effect Region of Definition
---------------------------

It is possible for an expression to need the region of definition (size of the image) produced
by an effect. This can be retrieved with the variable **rod***

The **rod** itself is a vector variable containing 4 scalar being in order the *left*, *bottom*,
*right* and *top* coordinates of the rectangle produced by the effect.::

    # Expression on the translate.x parameter of a Transform node
    input0.rod[0]

Current parameter animation
---------------------------

To achieve complex motion design, (see examples below such as loop) an
expression may need to access the animation of the parameter for which the expression
is evaluated.
To access the underlying animation of a parameter the :func:`curve(frame, dimension, view)`
function can be used::

    # Loop a parameter animation curve over the [firstFrame, lastFrame] interval
    firstFrame = 0
    lastFrame = 10
    curve(((frame - firstFrame) % (lastFrame - firstFrame + 1)) + firstFrame)


Random
------

Some expression may need to use a pseudo random function. It is pseudo random because
the results of the random function are reproducible for each frame and seed.

*    def :meth:`random<ExprTk.random>` (seed)
*    def :meth:`randomInt<ExprTk.randomInt>` (seed)

.. method:: ExprTk.random([seed=0, min=0., max=1.])

    :param seed: :class:`float`
    :param min: :class:`float`
    :param max: :class:`float`
    :rtype: :class:`float`

Returns a pseudo-random value in the interval [min, max[.
The value is produced such that for a given parameter it will always be the same for a
given time on the timeline, so that the value can be reproduced exactly.
However, successive calls to this function within the same expression will yield different
results after each call.
By default the random is seed with the current frame, meaning that 2 expressions using
random and evaluated at the same frame will always return the same number.
If you want to force a different number for an expression, you can set the *seed* parameter
to a non zero value.

.. method:: ExprTk.randomInt([seed=0, min=INT_MIN, max=INT_MAX])

    :param seed: :class:`int`
    :param min: :class:`int`
    :param max: :class:`int`
    :rtype: :class:`int`

Same as  :func:`random(seed, min,max)<ExprTk.random>` but returns an integer in the
range [min,max[


Advanced noise functions
-------------------------

More advanced noise functions are available such as fractional brownian motion.
All the functions available in Python in the :ref:`NatronEngine.ExprUtils<ExprUtils>` class are
also available to basic expressions. See the their documentation in the Python API as they
have the same signature.

