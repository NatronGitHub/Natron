.. _rendering:

Rendering
=========

To start rendering in Natron you need to use the :func:`render(task)<NatronEngine.App.render>` or :func:`render(tasks)<NatronEngine.App.render>`
of the :ref:`App<App>` class.
The parameters passed are:

	* The *writeNode*: This should point to the node you want to start rendering with
	* The *firstFrame*: This is the first frame to render in the sequence
	* The *lastFrame*: This is the last frame to render in the sequence
	
Natron always renders from the *firstFrame* to the *lastFrame* and renders each frame.
Generally Natron uses multiple threads to render concurrently several frames, you can control
this behaviour with the parameters in the :ref:`settings<AppSettings>`.

Let's imagine there's a node called **WriteOIIO1** in your project and that you want to render
frames 20 to 50 included, you would call it the following way::

	app.render(app.WriteOIIO1,20,50)
	
.. note:: 

	Note that when the render is launched from a :ref:`GuiApp<GuiApp>`, it is not *blocking*, i.e:
	this function will return immediately even though the render is not finished.

	On the other hand, if called from a :ref:`background application<App>`, this call will be blocking 
	and return once the render is finished.
	
	If you need to have a blocking render whilst using Natron Gui, you can use the 
	:func:`renderBlocking()<NatronGui.GuiApp.renderBlocking>` function but bear in mind that
	it will freeze the user interface until the render is finished.

You can use the :ref:`after render callback<afterRenderCallback>` to call code to be run once the render 
is finished.


For convenience, the :ref:`App<App>` class also have a :func:`render(tasks)<>` function taking
a sequence of tuples [Effect,int,int] in parameter.


Let's imagine we were to render 2 write nodes concurrently, we could do the following call::

	app.render([ (app.WriteOIIO1,1,10),
				 (app.WriteFFmpeg1,1,50) ])
				
.. note::
	The same restrictions apply to this variant of the render function: it is blocking in background mode
	and not blocking in GUI mode.

When executing multiple renders with the same call, each render is called concurrently from the others.


Using the DiskCache node
-------------------------

All the above can be applied to the **DiskCache** node to pre-render a sequence.
Just pass the DiskCache node instead of the Write node to the render function.
