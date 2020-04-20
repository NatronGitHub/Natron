#0  0x00007ffff3573ba9 in syscall () from /lib64/libc.so.6
#1  0x00007ffff4855092 in ?? () from /opt/Natron2/bin/../lib/libQtCore.so.4
#2  0x00007ffff4850f58 in QMutex::lockInternal() () from /opt/Natron2/bin/../lib/libQtCore.so.4
#3  0x00000000006d04ca in lockInline (this=0x2a16410) at /usr/include/QtCore/qmutex.h:190
#4  QMutexLocker (m=0x2a16410, this=<synthetic pointer>) at /usr/include/QtCore/qmutex.h:109
#5  Natron::Gui::setGuiAboutToClose (this=0x2a1bee0, about=<optimized out>) at Gui.cpp:206
#6  0x00000000007085e3 in Natron::GuiAppInstance::aboutToQuit (this=0x29c9820) at GuiAppInstance.cpp:224
#7  0x00000000008b3998 in Natron::AppManager::afterQuitProcessingCallback(boost::shared_ptr<Natron::GenericWatcherCallerArgs> const&) () at AppManager.cpp:643
#8  0x00000000008b90ff in Natron::AppManager::quitNow(boost::shared_ptr<Natron::AppInstance> const&) () at /usr/include/boost169/boost/smart_ptr/detail/sp_counted_base_std_atomic.hpp:99
#9  0x000000000089fb08 in Natron::AppInstance::quitNow() () at /usr/include/boost169/boost/smart_ptr/detail/sp_counted_base_std_atomic.hpp:54
#10 0x00000000008b995a in Natron::AppManager::~AppManager() () at AppManager.cpp:572
#11 0x0000000000588ef0 in Natron::GuiApplicationManager::~GuiApplicationManager (this=0x7fffffffe510, __in_chrg=<optimized out>) at /usr/include/boost169/boost/smart_ptr/detail/sp_counted_base_std_atomic.hpp:109
#12 0x000000000057581a in main () at NatronApp_main.cpp:127
#13 0x00007ffff349d505 in __libc_start_main () from /lib64/libc.so.6
#14 0x00000000005849d4 in _start () at moc_GuiApplicationManager.cpp:96
