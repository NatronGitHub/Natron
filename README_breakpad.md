Breakpad is an open source crash reporting system.

The breakpad-specific parts in the Natron source code are enabled by adding CONFIG+=enable-breakpad to the qmake options.

It consists in:

- `breakpadpro.pri`: breakpad-specific additions to Project.pro)
- `breakpadclient.pri`: included by global.pri
- `BreakpadClient`: the breakpad client library used by `CrashReporter` and `CrashReporterCLI`
- `CrashReporter`: the crash reporter executable (GUI version, monitors crashes by `Natron`)
- `CrashReporterCLI`: the crash reporter executable (command-line version, monitors crashes by `NatronRenderer`)
- `libs/google-breakpad`: a submodule that points to https://github.com/NatronGitHub/google-breakpad which is a fork of https://github.com/google/breakpad 


There are several issues with breakpad, see:

- https://github.com/NatronGitHub/Natron/issues/219
- https://github.com/NatronGitHub/Natron/issues/145
- https://github.com/NatronGitHub/Natron/issues/143

TODO:

- fork https://github.com/google/breakpad
- checkout the version from Sep 6, 2015 (https://github.com/google/breakpad/commit/2d450f312b4abf567f6fc82b6cb66d39fe64f08f, or maybe https://github.com/google/breakpad/commit/3f4d090d70c3f5fbeb9b6e646a079631f1ebf05b which is the last version on https://chromium.googlesource.com/external/google-breakpad)
- re-apply local changes, including MinGW fixes
- test
