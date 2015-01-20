.. _autovar:

Python Auto-declared variables
==============================

A lot of Python variables are pre-declared by Natron upon the creation of specific objects.
This applies currently to the following objects:

	*	:doc:`PythonReference/NatronEngine/Effect`
	*	:doc:`PythonReference/NatronEngine/Layer`
	*	:doc:`PythonReference/NatronEngine/BezierCurve`
	*	:doc:`PythonReference/NatronEngine/App`
	
The idea is that it is simpler to access a simple variable like this::
	
	node = app1.Blur1
	
rather than call a bunch of functions such as::

	node = app1.getNode("app1.Blur1")
	
To achieve this, auto-declared objects must be named with a correct syntax in
a python script.
For instance, the following variable would not work in Python::

	>>> my variable = 2
	File "<stdin>", line 1
	my variable = 2
              ^
	SyntaxError: invalid syntax 
	
But the following would work::

	>>> myVariable = 2

To overcome this issue, all auto-declared variables in Natron have 2 names:

	1. A script-name: The name that will be used to auto-declare the variable to Python.
	This name cannot be changed and is set once by Natron the first time the object is
	created. This name contains only alpha-numeric characters and does not start
	with a digit.
	
	2. A label: The label is what is displayed on the graphical user interface. For example
	the node label is visible in the node graph. This label can contain any character 
	without any restriction.