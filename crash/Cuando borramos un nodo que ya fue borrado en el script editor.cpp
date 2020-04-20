Program received signal SIGSEGV, Segmentation fault.
Natron::Node::getEffectInstance (this=0x0) at Node.cpp:2667
2667	    return _imp->effect;

#0  Natron::Node::getEffectInstance (this=0x0) at Node.cpp:2667
#1  0x0000000000ac6474 in Natron::NodeGroup::notifyNodeDeactivated(boost::shared_ptr<Natron::Node> const&) () at /usr/include/boost169/boost/smart_ptr/detail/shared_count.hpp:488
#2  0x0000000000aa9db5 in Natron::Node::deactivate(std::list<boost::shared_ptr<Natron::Node>, std::allocator<boost::shared_ptr<Natron::Node> > > const&, bool, bool, bool, bool, bool) [clone .localalias.1127] () at Node.cpp:2995
#3  0x00000000007a32d6 in Natron::RemoveMultipleNodesCommand::redo() () at /usr/include/boost169/boost/smart_ptr/detail/shared_count.hpp:488
#4  0x00007ffff57d0e43 in QUndoStack::push(QUndoCommand*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#5  0x0000000000771316 in Natron::NodeGraph::deleteSelection() () at NodeGraph30.cpp:356
#6  0x000000000076b4bc in Natron::NodeGraph::keyPressEvent(QKeyEvent*) () at NodeGraph25.cpp:252
#7  0x00007ffff522f062 in QWidget::event(QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#8  0x00007ffff5598cee in QFrame::event(QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#9  0x00007ffff5612f43 in QAbstractScrollArea::event(QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#10 0x00007ffff51dfbcc in QApplicationPrivate::notify_helper(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#11 0x00007ffff51e6e03 in QApplication::notify(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#12 0x00007ffff4950cbe in QCoreApplication::notifyInternal(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#13 0x00007ffff5279ecf in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#14 0x00007ffff527a230 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#15 0x00007ffff5255f5e in QApplication::x11ProcessEvent(_XEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#16 0x00007ffff527c993 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#17 0x00007ffff07e0597 in g_main_context_dispatch () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
---Type <return> to continue, or q <return> to quit---
#18 0x00007ffff07e07a8 in ?? () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#19 0x00007ffff07e082c in g_main_context_iteration () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#20 0x00007ffff497c87e in QEventDispatcherGlib::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#21 0x00007ffff527caf7 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#22 0x00007ffff494f244 in QEventLoop::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#23 0x00007ffff494f556 in QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#24 0x00007ffff49549fc in QCoreApplication::exec() () from /opt/Natron2/bin/../lib/libQtCore.so.4
#25 0x00000000008b4a85 in Natron::AppManager::exec (this=<optimized out>) at /usr/include/QtCore/qcoreapplication.h:118
#26 0x000000000058aa2a in Natron::GuiApplicationManager::initGui(Natron::CLArgs const&) () at GuiApplicationManager.cpp:966
#27 0x00000000008c5dea in Natron::AppManager::loadInternal(Natron::CLArgs const&) () at /usr/include/QtCore/qdebug.h:93
#28 0x00000000008c63b0 in Natron::AppManager::loadFromArgs(Natron::CLArgs const&) () at AppManager.cpp:519
#29 0x00000000008c661f in Natron::AppManager::load (this=this@entry=0x7fffffffe510, argc=<optimized out>, argv=argv@entry=0x7fffffffe638, cl=...) at AppManager.cpp:536
#30 0x000000000057580f in main () at NatronApp_main.cpp:127
#31 0x00007ffff349d505 in __libc_start_main () from /lib64/libc.so.6
#32 0x00000000005849d4 in _start () at moc_GuiApplicationManager.cpp:96
