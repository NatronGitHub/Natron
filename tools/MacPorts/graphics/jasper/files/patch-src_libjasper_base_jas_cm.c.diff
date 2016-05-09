--- src/libjasper/base/jas_cm.c.orig	Sun Feb  8 17:34:40 2004
+++ src/libjasper/base/jas_cm.c	Mon Mar  1 22:15:08 2004
@@ -453,7 +453,7 @@
 int jas_cmxform_apply(jas_cmxform_t *xform, jas_cmpixmap_t *in, jas_cmpixmap_t *out)
 {
 	jas_cmcmptfmt_t *fmt;
-	jas_cmreal_t buf[2][APPLYBUFSIZ];
+	static jas_cmreal_t buf[2][APPLYBUFSIZ];
 	jas_cmpxformseq_t *pxformseq;
 	int i;
 	int j;
