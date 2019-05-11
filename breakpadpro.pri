SUBDIRS += \
BreakpadClient \
CrashReporter \
CrashReporterCLI

# what subproject depends on others
CrashReporter.depends = BreakpadClient
CrashReporterCLI.depends = BreakpadClient

