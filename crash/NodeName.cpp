rogram received signal SIGSEGV, Segmentation fault.
0x0000000000af0ab4 in Natron::Node::getScriptName_mt_safe() const () at NodeName.cpp:135

#0  0x0000000000af0ab4 in Natron::Node::getScriptName_mt_safe() const () at NodeName.cpp:135
#1  0x0000000000b9ce34 in Natron::Python::Effect::getScriptName() const () at PyNode.cpp:341
#2  0x0000000000daeeee in Sbk_EffectFunc_getScriptName () at NatronEngine/effect_wrapper.cpp:1055
#3  0x00007ffff6f1d0c3 in PyEval_EvalFrameEx () from /opt/Natron2/bin/../lib/libpython2.7.so.1.0
#4  0x00007ffff6f1ed38 in PyEval_EvalCodeEx () from /opt/Natron2/bin/../lib/libpython2.7.so.1.0
#5  0x00007ffff6f1ef49 in PyEval_EvalCode () from /opt/Natron2/bin/../lib/libpython2.7.so.1.0
#6  0x00007ffff6f40df6 in PyRun_StringFlags () from /opt/Natron2/bin/../lib/libpython2.7.so.1.0
#7  0x00000000008b7453 in Natron::Python::interpretPythonScript(std::string const&, std::string*, std::string*) () at AppManager.cpp:3992
#8  0x0000000000810c62 in Natron::ScriptEditor::onExecScriptClicked() () at /usr/include/QtCore/qatomic_x86_64.h:134
#9  0x00000000008175ac in Natron::ScriptEditor::keyPressEvent(QKeyEvent*) () at ScriptEditor.cpp:541
#10 0x00007ffff522f062 in QWidget::event(QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#11 0x00007ffff51dfbcc in QApplicationPrivate::notify_helper(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#12 0x00007ffff51e6e03 in QApplication::notify(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#13 0x00007ffff4950cbe in QCoreApplication::notifyInternal(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#14 0x00007ffff5279ecf in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#15 0x00007ffff527a230 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#16 0x00007ffff5255f5e in QApplication::x11ProcessEvent(_XEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#17 0x00007ffff527c993 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
---Type <return> to continue, or q <return> to quit---
#18 0x00007ffff07e0597 in g_main_context_dispatch () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#19 0x00007ffff07e07a8 in ?? () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#20 0x00007ffff07e082c in g_main_context_iteration () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#21 0x00007ffff497c87e in QEventDispatcherGlib::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#22 0x00007ffff527caf7 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#23 0x00007ffff494f244 in QEventLoop::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#24 0x00007ffff494f556 in QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#25 0x00007ffff49549fc in QCoreApplication::exec() () from /opt/Natron2/bin/../lib/libQtCore.so.4
#26 0x00000000008b4a85 in Natron::AppManager::exec (this=<optimized out>) at /usr/include/QtCore/qcoreapplication.h:118
#27 0x000000000058aa2a in Natron::GuiApplicationManager::initGui(Natron::CLArgs const&) () at GuiApplicationManager.cpp:966
#28 0x00000000008c5dea in Natron::AppManager::loadInternal(Natron::CLArgs const&) () at /usr/include/QtCore/qdebug.h:93
#29 0x00000000008c63b0 in Natron::AppManager::loadFromArgs(Natron::CLArgs const&) () at AppManager.cpp:519
#30 0x00000000008c661f in Natron::AppManager::load (this=this@entry=0x7fffffffe510, argc=<optimized out>, argv=argv@entry=0x7fffffffe638, cl=...) at AppManager.cpp:536
#31 0x000000000057580f in main () at NatronApp_main.cpp:127
#32 0x00007ffff349d505 in __libc_start_main () from /lib64/libc.so.6
#33 0x00000000005849d4 in _start () at moc_GuiApplicationManager.cpp:96
