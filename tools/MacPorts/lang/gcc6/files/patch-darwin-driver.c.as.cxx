--- gcc/config/darwin-driver.c.orig	2017-06-25 13:25:11.000000000 -0700
+++ gcc/config/darwin-driver.c	2017-06-25 13:25:48.000000000 -0700
@@ -299,10 +299,10 @@ darwin_driver_init (unsigned int *decode
   if (vers_string != NULL)
     {
       char *asm_major = NULL;
-      char *first_period = strchr(vers_string, '.');
+      const char *first_period = strchr(vers_string, '.');
       if (first_period != NULL)
 	{
-	  char *second_period = strchr(first_period+1, '.');
+	  const char *second_period = strchr(first_period+1, '.');
 	  if (second_period  != NULL)
 	    asm_major = xstrndup (vers_string, second_period-vers_string);
 	  else
