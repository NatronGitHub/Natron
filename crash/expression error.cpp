Program received signal SIGSEGV, Segmentation fault.
0x0000000000a30f08 in size (this=0x1a74bef8) at Knob.cpp:2800
2800	        hadExpression = !_imp->expressions[dimension].originalExpression.empty();
(gdb) bt
#0  0x0000000000a30f08 in size (this=0x1a74bef8) at Knob.cpp:2800
#1  empty (this=0x1a74bef8) at /opt/rh/devtoolset-8/root/usr/include/c++/8/bits/basic_string.h:3949
#2  Natron::KnobHelper::clearExpression(int, bool) () at Knob.cpp:2800
#3  0x0000000000a3749e in Natron::KnobHelper::setExpressionInternal(int, std::string const&, bool, bool, bool) () at Knob.cpp:2685
#4  0x0000000000ba7f6c in setExpression (failIfInvalid=true, hasRetVariable=true, expression=..., dimension=1, this=0x1a74bbb0) at ../../natron/Engine/Knob.h:660
#5  Natron::Python::AnimatedParam::setExpression(QString const&, bool, int) () at PyParameter.cpp:618
#6  0x0000000000d9a586 in Sbk_AnimatedParamFunc_setExpression () at NatronEngine/animatedparam_wrapper.cpp:779
#7  0x00007ffff6f1d52c in PyEval_EvalFrameEx () from /opt/Natron2/bin/../lib/libpython2.7.so.1.0
#8  0x00007ffff6f1ed38 in PyEval_EvalCodeEx () from /opt/Natron2/bin/../lib/libpython2.7.so.1.0
#9  0x00007ffff6f1ef49 in PyEval_EvalCode () from /opt/Natron2/bin/../lib/libpython2.7.so.1.0
#10 0x00007ffff6f40df6 in PyRun_StringFlags () from /opt/Natron2/bin/../lib/libpython2.7.so.1.0
#11 0x00000000008b7453 in Natron::Python::interpretPythonScript(std::string const&, std::string*, std::string*) () at AppManager.cpp:3992
#12 0x0000000000810c62 in Natron::ScriptEditor::onExecScriptClicked() () at /usr/include/QtCore/qatomic_x86_64.h:134
#13 0x00000000008175ac in Natron::ScriptEditor::keyPressEvent(QKeyEvent*) () at ScriptEditor.cpp:541
#14 0x00007ffff522f062 in QWidget::event(QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#15 0x00007ffff51dfbcc in QApplicationPrivate::notify_helper(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#16 0x00007ffff51e6e03 in QApplication::notify(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#17 0x00007ffff4950cbe in QCoreApplication::notifyInternal(QObject*, QEvent*) () from /opt/Natron2/bin/../lib/libQtCore.so.4
---Type <return> to continue, or q <return> to quit---
#18 0x00007ffff5279ecf in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#19 0x00007ffff527a230 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#20 0x00007ffff5255f5e in QApplication::x11ProcessEvent(_XEvent*) () from /opt/Natron2/bin/../lib/libQtGui.so.4
#21 0x00007ffff527c993 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#22 0x00007ffff07e0597 in g_main_context_dispatch () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#23 0x00007ffff07e07a8 in ?? () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#24 0x00007ffff07e082c in g_main_context_iteration () from /opt/Natron2/bin/../lib/libglib-2.0.so.0
#25 0x00007ffff497c87e in QEventDispatcherGlib::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#26 0x00007ffff527caf7 in ?? () from /opt/Natron2/bin/../lib/libQtGui.so.4
#27 0x00007ffff494f244 in QEventLoop::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#28 0x00007ffff494f556 in QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) () from /opt/Natron2/bin/../lib/libQtCore.so.4
#29 0x00007ffff49549fc in QCoreApplication::exec() () from /opt/Natron2/bin/../lib/libQtCore.so.4
#30 0x00000000008b4a85 in Natron::AppManager::exec (this=<optimized out>) at /usr/include/QtCore/qcoreapplication.h:118
#31 0x000000000058aa2a in Natron::GuiApplicationManager::initGui(Natron::CLArgs const&) () at GuiApplicationManager.cpp:966
#32 0x00000000008c5dea in Natron::AppManager::loadInternal(Natron::CLArgs const&) () at /usr/include/QtCore/qdebug.h:93
#33 0x00000000008c63b0 in Natron::AppManager::loadFromArgs(Natron::CLArgs const&) () at AppManager.cpp:519
#34 0x00000000008c661f in Natron::AppManager::load (this=this@entry=0x7fffffffe510, argc=<optimized out>, argv=argv@entry=0x7fffffffe638, cl=...) at AppManager.cpp:536
#35 0x000000000057580f in main () at NatronApp_main.cpp:127
---Type <return> to continue, or q <return> to quit---
#36 0x00007ffff349d505 in __libc_start_main () from /lib64/libc.so.6
#37 0x00000000005849d4 in _start () at moc_GuiApplicationManager.cpp:96
