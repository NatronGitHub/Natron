Fix bogus pointer comparison. Backport of upstream commit
https://github.com/qt/qttools/commit/7138c963f9d1258bc1b49cb4d63c3e2b7d0ccfda

--- tools/linguist/linguist/messagemodel.cpp.orig	2015-05-07 14:14:39 UTC
+++ tools/linguist/linguist/messagemodel.cpp
@@ -183,7 +183,7 @@ static int calcMergeScore(const DataMode
         if (ContextItem *c = one->findContext(oc->context())) {
             for (int j = 0; j < oc->messageCount(); ++j) {
                 MessageItem *m = oc->messageItem(j);
-                if (c->findMessage(m->text(), m->comment()) >= 0)
+                if (c->findMessage(m->text(), m->comment()))
                     ++inBoth;
             }
         }
