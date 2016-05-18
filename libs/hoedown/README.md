Hoedown
=======

[![Build Status](https://travis-ci.org/hoedown/hoedown.png?branch=master)](https://travis-ci.org/hoedown/hoedown)

`Hoedown` is a revived fork of [Sundown](https://github.com/vmg/sundown),
the Markdown parser based on the original code of the
[Upskirt library](http://fossil.instinctive.eu/libupskirt/index)
by Natacha Porté.

Features
--------

*	**Fully standards compliant**

	`Hoedown` passes out of the box the official Markdown v1.0.0 and v1.0.3
	test suites, and has been extensively tested with additional corner cases
	to make sure its output is as sane as possible at all times.

*	**Massive extension support**

	`Hoedown` has optional support for several (unofficial) Markdown extensions,
	such as non-strict emphasis, fenced code blocks, tables, autolinks,
	strikethrough and more.

*	**UTF-8 aware**

	`Hoedown` is fully UTF-8 aware, both when parsing the source document and when
	generating the resulting (X)HTML code.

*	**Tested & Ready to be used on production**

	`Hoedown` has been extensively security audited, and includes protection against
	all possible DOS attacks (stack overflows, out of memory situations, malformed
	Markdown syntax...).

	We've worked very hard to make `Hoedown` never leak or crash under *any* input.

	**Warning**: `Hoedown` doesn't validate or post-process the HTML in Markdown documents.
	Unless you use `HTML_ESCAPE` or `HTML_SKIP`, you should strongly consider using a
	good post-processor in conjunction with Hoedown to prevent client-side attacks.

*	**Customizable renderers**

	`Hoedown` is not stuck with XHTML output: the Markdown parser of the library
	is decoupled from the renderer, so it's trivial to extend the library with
	custom renderers. A fully functional (X)HTML renderer is included.

*	**Optimized for speed**

	`Hoedown` is written in C, with a special emphasis on performance. When wrapped
	on a dynamic language such as Python or Ruby, it has shown to be up to 40
	times faster than other native alternatives.

*	**Zero-dependency**

	`Hoedown` is a zero-dependency library composed of some `.c` files and their
	headers. No dependencies, no bullshit. Only standard C99 that builds everywhere.

*	**Additional features**

	`Hoedown` comes with a fully functional implementation of SmartyPants,
	a separate autolinker, escaping utilities, buffers and stacks.

Bindings
--------

You can see a community-maintained list of `Hoedown` bindings at
[the wiki](https://github.com/hoedown/hoedown/wiki/Bindings). There is also a
[migration guide](https://github.com/hoedown/hoedown/wiki/Migration-Guide)
available for authors of Sundown bindings.

Help us
-------

`Hoedown` is all about security. If you find a (potential) security vulnerability in the
library, or a way to make it crash through malicious input, please report it to us by
emailing the private [Hoedown Security](mailto:hoedown-security@googlegroups.com)
mailing list. The `Hoedown` security team will review the vulnerability and work with you
to reproduce and resolve it.

Unicode character handling
--------------------------

Given that the Markdown spec makes no provision for Unicode character handling, `Hoedown`
takes a conservative approach towards deciding which extended characters trigger Markdown
features:

*	Punctuation characters outside of the U+007F codepoint are not handled as punctuation.
	They are considered as normal, in-word characters for word-boundary checks.

*	Whitespace characters outside of the U+007F codepoint are not considered as
	whitespace. They are considered as normal, in-word characters for word-boundary checks.

Install
-------

Just typing `make` will build `Hoedown` into a dynamic library and create the `hoedown`
and `smartypants` executables, which are command-line tools to render Markdown to HTML
and perform SmartyPants, respectively.

If you are using [CocoaPods](http://cocoapods.org), just add the line `pod 'hoedown'` to your Podfile and call `pod install`.

Or, if you prefer, you can just throw the files at `src` into your project.
