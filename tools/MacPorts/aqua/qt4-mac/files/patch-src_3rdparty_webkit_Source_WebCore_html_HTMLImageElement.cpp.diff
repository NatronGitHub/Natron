------------------------------------------------------------------------
r113848 | abecsi@webkit.org | 2012-04-11 11:23:19 +0000 (Wed, 11 Apr 2012) | 27 lines

Fix the build with gcc 4.7.0
https://bugs.webkit.org/show_bug.cgi?id=83584
[...]
* html/HTMLImageElement.cpp:
(WebCore::HTMLImageElement::createForJSConstructor): Fails because of -Werror=extra
[...]

--- src/3rdparty/webkit/Source/WebCore/html/HTMLImageElement.cpp.orig	2015-05-07 14:14:47 UTC
+++ src/3rdparty/webkit/Source/WebCore/html/HTMLImageElement.cpp
@@ -74,7 +74,7 @@ PassRefPtr<HTMLImageElement> HTMLImageEl
     RefPtr<HTMLImageElement> image = adoptRef(new HTMLImageElement(imgTag, document));
     if (optionalWidth)
         image->setWidth(*optionalWidth);
-    if (optionalHeight > 0)
+    if (optionalHeight)
         image->setHeight(*optionalHeight);
     return image.release();
 }
