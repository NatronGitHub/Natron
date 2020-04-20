Program received signal SIGSEGV, Segmentation fault.
0x00007ffff40199e3 in __dynamic_cast () from /opt/Natron2/bin/../lib/libstdc++.so.6

#0  0x00007ffff40199e3 in __dynamic_cast () from /opt/Natron2/bin/../lib/libstdc++.so.6
#1  0x0000000000620afc in Natron::ViewerGL::setParametricParamsPickerColor(OfxRGBAColourD const&, bool, bool) () at ViewerGL.cpp:2696
#2  0x00000000006282b6 in updateInfoWidgetColorPickerInternal (texIndex=0, dispW=..., rod=..., height=316, width=1222, widgetPos=..., imgPos=..., this=0x2242b50) at ViewerGL.cpp:3890
#3  Natron::ViewerGL::updateInfoWidgetColorPickerInternal (this=0x2242b50, imgPos=..., widgetPos=..., width=1222, height=316, rod=..., dispW=..., texIndex=0) at ViewerGL.cpp:3824
#4  0x000000000062f0cd in Natron::ViewerGL::updateInfoWidgetColorPicker(QPointF const&, QPoint const&) () at /usr/include/QtCore/qrect.h:304
#5  0x000000000062f37a in Natron::ViewerGL::penMotionInternal(int, int, double, double, QInputEvent*) () at /usr/include/QtCore/qpoint.h:123
#6  0x00000000006305f1 in Natron::ViewerGL::mouseMoveEvent (this=0x2242b50, e=0x7fffffffda80) at /usr/include/QtCore/qpoint.h:129
#7  0x00007ffff522e8c0 in QWidget::event(QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#8  0x00007ffff5c93af3 in QGLWidget::event(QEvent*) () from /opt/Natron2/bin/../lib/libQtOpenGL.so.4
#9  0x00007ffff51dfbcc in QApplicationPrivate::notify_helper(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#10 0x00007ffff51e6952 in QApplication::notify(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#11 0x00007ffff4950cbe in QCoreApplication::notifyInternal(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#12 0x00007ffff51e5993 in QApplicationPrivate::sendMouseEvent(QWidget*, QMouseEvent*, QWidget*, QWidget*, QWidget**, QPointer<QWidget>&, bool) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#13 0x00007ffff52574af in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#14 0x00007ffff5255c5c in QApplication::x11ProcessEvent(_XEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#15 0x00007ffff527c993 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#16 0x00007ffff07e0597 in g_main_context_dispatch () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#17 0x00007ffff07e07a8 in ?? () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
---Type <return> to continue, or q <return> to quit---
#18 0x00007ffff07e082c in g_main_context_iteration () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#19 0x00007ffff497c856 in QEventDispatcherGlib::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#20 0x00007ffff527caf7 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#21 0x00007ffff494f244 in QEventLoop::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#22 0x00007ffff494f556 in QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#23 0x00007ffff49549fc in QCoreApplication::exec() () from /opt/Natron2/bin/../lib/libQtCore.so.4
#24 0x00000000008b4a85 in Natron::AppManager::exec (this=<optimized out>) at /usr/include/QtCore/qcoreapplication.h:118
#25 0x000000000058aa2a in Natron::GuiApplicationManager::initGui(Natron::CLArgs const&) () at GuiApplicationManager.cpp:966
#26 0x00000000008c5dea in Natron::AppManager::loadInternal(Natron::CLArgs const&) () at /usr/include/QtCore/qdebug.h:93
#27 0x00000000008c63b0 in Natron::AppManager::loadFromArgs(Natron::CLArgs const&) () at AppManager.cpp:519
#28 0x00000000008c661f in Natron::AppManager::load (this=this@entry=0x7fffffffe510, argc=<optimized out>, argv=argv@entry=0x7fffffffe638, cl=...) at AppManager.cpp:536
#29 0x000000000057580f in main () at NatronApp_main.cpp:127
#30 0x00007ffff349d505 in __libc_start_main () from /lib64/libc.so.6
#31 0x00000000005849d4 in _start () at moc_GuiApplicationManager.cpp:96
