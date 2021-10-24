This directory contains formulas for [Homebrew](https://brew.sh), the macOS package manager.

Although it is recommended to use [MacPorts](https://www.macports.org/) rather than Homebrew to build portable binaries, homebrew may still be useful for debugging.

These formulas contain recipes to build software that is not included anymore in Homebrew, and should be copied to `/usr/local/Homebrew/Library/Taps/homebrew/homebrew-core/Formula` before installing.

- `opencolorio@1` is a keg-only formula for OpenColorIO 1.1.1, corresponding to `git checkout f772cb9a399726fd5f3ba859c8f315988afb3d60 Formula/openimageio.rb` from homebrew-core.
- `python@2` is a formula for Python 2.17, which is still used as the scripting language for Natron, mainly because Natron still requires Qt 4 with PySide. This corresponds to the version that can be obtained from the homebrew-core repository by `git checkout 3a877e3525d93cfeb076fc57579bdd589defc585 Formula/python@2.rb`
- `seexpr@2.rb` is a keg-only formula for SeExpr 2.11, because the SeExpr plugins were not updated to compile with SeExpr 33, which was a major API change. If the `qt@4` formula is installed, it may be necessary to `brew unlink qt@4` before building this recipe, and `brew link qt@4` one the recipe is built. This corresponds to `git checkout 4abcbc52a544c293f548b0373867d90d4587fd73 Formula/seexpr.rb` from homebrew-core.

It is recommended to install these formulas with the `--build-from-source` option.


