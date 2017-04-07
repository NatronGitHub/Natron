.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Expressions
===========

.. toctree::
   :maxdepth: 2
   
.. _exprtkURL: http://www.python.org/
.. _exprtkREADME: https://raw.githubusercontent.com/ArashPartow/exprtk/master/readme.txt
   
The value of any Node :doc:`parameter<PythonReference/NatronEngine/Param>` can be set by
Python expression.
An expression is a line of code that can either reference the value
of other parameters or apply mathematical functions to the current value.

Natron supports 2 types of expression languages: 

    * Python: :ref:`Natron documentation<paramExpr>`
    * ExprTk: exprtkURL_
    
ExprTk is a very fast and simple expression language which should cover 90% of the 
needs for expressions. By default this is the language proposed to you when editing an
expression.

Python based expressions are using the same A.P.I as everything else using Python in Natron.
It allows to write really any kind of expression referencing external functions and data,
however it is much slower to evaluate and will impair performance compared to a simple
ExprTk expression.

Where possible, you should use ExprTk, unless you specifically need a feature available
in the Python API that is not available through the ExprTk language.


For more informations on developping Python expression, please refer to :ref:`this section<paramExpr>`.

The rest of this section will cover writing expressions in ExprTk.


ExprTk expressions
==================

The language syntax and available mathematical functions are well covered by the `README<exprtkREADME>`.

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
    * If this node belongs to a sub-group, it may reference the Group node itself using the 
    special variable *thisGroup*
    * If this node is a Group itself, it may reference any node within its sub-group by
    prefixing *thisNode*, e.g: thisNode.Blur1
    
::
    
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
	
	* **project**: References the project settings. This can be used as a prefix to reference
	project parameters, e.g: project.outputFormat
	
	* **frame**: It references the current time on the timeline at which the expression is evaluated
	this may be a floating point number if the parameter is referenced from a node that uses
	motion blur. If the parameter is a string parameter, the frame variable is already a string
	otherwise it will be a scalar.
	
	* **view**: It references the current view for which the expression is evaluated.
	If the parameter is a string parameter, the view will be the name of the view as seen
	in the project settings, otherwise this will be a 0-based index.
	
Name of a node or parameter
---------------------------

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

* e	format as [-]9.9e[+|-]999
* E	format as [-]9.9E[+|-]999
* f	format as [-]9.9
* g	use e or f format, whichever is the most concise
* G	use E or f format, whichever is the most concise

A precision is also specified with the argument format.
For the 'e', 'E', and 'f' formats, the precision represents the number of digits after the decimal point.
For the 'g' and 'G' formats, the precision represents the maximum number of significant digits (trailing zeroes are omitted).

Example::

    str(2.8940,'f',2) = "2.89"

	
Effect Region of Definition
---------------------------

It is possible for an expression to need the region of definition (size of the image) produced
by an effect. This can be retrieved with the variable *rod*/

The *rod* itself is a vector variable containing 4 scalar being in order the left, bottom,
right and top coordinates of the rectangle produced by the effect.

    # Expression on the translate.x parameter of a Transform node
    input0.rod[0]

Random and advanced functions
------------------------------




Examples
========
