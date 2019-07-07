--- src/3rdparty/webkit/Source//JavaScriptCore/bytecompiler/BytecodeGenerator.cpp.orig	2019-07-06 01:50:33.000000000 +0200
+++ src/3rdparty/webkit/Source//JavaScriptCore/bytecompiler/BytecodeGenerator.cpp	2019-07-06 01:52:48.000000000 +0200
@@ -2058,7 +2058,7 @@
 {
     m_usesExceptions = true;
 #if ENABLE(JIT)
-    HandlerInfo info = { start->bind(0, 0), end->bind(0, 0), instructions().size(), m_dynamicScopeDepth + m_baseScopeDepth, CodeLocationLabel() };
+    HandlerInfo info = { static_cast<uint32_t>(start->bind(0, 0)), static_cast<uint32_t>(end->bind(0, 0)), static_cast<uint32_t>(instructions().size()), static_cast<uint32_t>(m_dynamicScopeDepth + m_baseScopeDepth), CodeLocationLabel() };
 #else
     HandlerInfo info = { start->bind(0, 0), end->bind(0, 0), instructions().size(), m_dynamicScopeDepth + m_baseScopeDepth };
 #endif
@@ -2107,7 +2107,7 @@
 
 void BytecodeGenerator::beginSwitch(RegisterID* scrutineeRegister, SwitchInfo::SwitchType type)
 {
-    SwitchInfo info = { instructions().size(), type };
+    SwitchInfo info = { static_cast<uint32_t>(instructions().size()), type };
     switch (type) {
         case SwitchInfo::SwitchImmediate:
             emitOpcode(op_switch_imm);
