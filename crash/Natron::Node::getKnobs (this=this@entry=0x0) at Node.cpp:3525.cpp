Program received signal SIGSEGV, Segmentation fault.
Natron::Node::getKnobs (this=this@entry=0x0) at Node.cpp:3525
3525	    return _imp->effect->getKnobs();
(gdb) bt
#0  Natron::Node::getKnobs (this=this@entry=0x0) at Node.cpp:3525
#1  0x0000000000681297 in Natron::CurveEditor::addNode(boost::shared_ptr<Natron::NodeGui>) () at /usr/include/boost169/boost/smart_ptr/detail/shared_count.hpp:651
#2  0x00000000006d1382 in Natron::Gui::addNodeGuiToCurveEditor(boost::shared_ptr<Natron::NodeGui> const&) () at /usr/include/boost169/boost/smart_ptr/detail/sp_counted_base_std_atomic.hpp:99
#3  0x00000000007b0c7b in Natron::NodeGui::ensurePanelCreated() [clone .localalias.521] () at NodeGui.cpp:444
#4  0x00000000007b0e65 in Natron::NodeGui::setVisibleSettingsPanel (this=this@entry=0x9e47050, b=b@entry=true) at NodeGui.cpp:2098
#5  0x000000000076ac86 in Natron::NodeGraph::showNodePanel(bool, bool, Natron::NodeGui*) () at NodeGraph25.cpp:77
#6  0x000000000076b1ba in Natron::NodeGraph::mouseDoubleClickEvent (this=0x9498610, e=0x7fffffffda70) at /usr/include/QtGui/qevent.h:79
#7  0x00007ffff522f26c in QWidget::event(QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#8  0x00007ffff5598cee in QFrame::event(QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#9  0x00007ffff5797ab3 in QGraphicsView::viewportEvent(QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#10 0x00007ffff4950dfc in QCoreApplicationPrivate::sendThroughObjectEventFilters(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#11 0x00007ffff51dfbac in QApplicationPrivate::notify_helper(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#12 0x00007ffff51e6952 in QApplication::notify(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#13 0x00007ffff4950cbe in QCoreApplication::notifyInternal(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#14 0x00007ffff51e5993 in QApplicationPrivate::sendMouseEvent(QWidget*, QMouseEvent*, QWidget*, QWidget*, QWidget**, QPointer<QWidget>&, bool) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#15 0x00007ffff52574af in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#16 0x00007ffff52563c3 in QApplication::x11ProcessEvent(_XEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#17 0x00007ffff527c993 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#18 0x00007ffff07e0597 in g_main_context_dispatch () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#19 0x00007ffff07e07a8 in ?? () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#20 0x00007ffff07e082c in g_main_context_iteration () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#21 0x00007ffff497c856 in QEventDispatcherGlib::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#22 0x00007ffff527caf7 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#23 0x00007ffff494f244 in QEventLoop::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#24 0x00007ffff494f556 in QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#25 0x00007ffff49549fc in QCoreApplication::exec() () from /opt/Natron2/bin/../lib/libQtCore.so.4
#26 0x00000000008b4a85 in Natron::AppManager::exec (this=<optimized out>) at /usr/include/QtCore/qcoreapplication.h:118
#27 0x000000000058aa2a in Natron::GuiApplicationManager::initGui(Natron::CLArgs const&) () at GuiApplicationManager.cpp:966
#28 0x00000000008c5dea in Natron::AppManager::loadInternal(Natron::CLArgs const&) () at /usr/include/QtCore/qdebug.h:93
#29 0x00000000008c63b0 in Natron::AppManager::loadFromArgs(Natron::CLArgs const&) () at AppManager.cpp:519
#30 0x00000000008c661f in Natron::AppManager::load (this=this@entry=0x7fffffffe500, argc=<optimized out>, argv=argv@entry=0x7fffffffe628, cl=...) at AppManager.cpp:536
#31 0x000000000057580f in main () at NatronApp_main.cpp:127
#32 0x00007ffff349d505 in __libc_start_main () from /lib64/libc.so.6
#33 0x00000000005849d4 in _start () at moc_GuiApplicationManager.cpp:96
