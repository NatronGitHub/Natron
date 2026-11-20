/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <QString>

#include "Engine/Markdown.h"
#include "Engine/Utils.h" // convertFromPlainTextToMarkdown

// Markdown::convert2html() (md4c, CommonMark with the extensions Natron enables)
// on fixed inputs. These tests compare the exact HTML, so that an update of
// libs/md4c that changes the rendering shows up here.

NATRON_NAMESPACE_USING

static QString
md2html(const char* markdown)
{
    return Markdown::convert2html( QString::fromUtf8(markdown) );
}

TEST(Markdown, Empty)
{
    EXPECT_EQ( QString(), md2html("") );
}

TEST(Markdown, Table)
{
    EXPECT_EQ( QString::fromUtf8("<table>\n"
                                 "<thead>\n"
                                 "<tr>\n"
                                 "<th>Input</th>\n"
                                 "<th align=\"center\">Description</th>\n"
                                 "<th align=\"right\">Optional</th>\n"
                                 "</tr>\n"
                                 "</thead>\n"
                                 "<tbody>\n"
                                 "<tr>\n"
                                 "<td>Source</td>\n"
                                 "<td align=\"center\">The image</td>\n"
                                 "<td align=\"right\">No</td>\n"
                                 "</tr>\n"
                                 "</tbody>\n"
                                 "</table>\n"),
               md2html("Input | Description | Optional\n"
                       "--- | :---: | ---:\n"
                       "Source | The image | No\n") );
}

TEST(Markdown, Footnote)
{
    EXPECT_EQ( QString::fromUtf8("<p>Text<sup><a href=\"#fn-1\" id=\"fnref-1-1\">1</a></sup>.</p>\n"
                                 "<section class=\"footnotes\">\n"
                                 "<ol>\n"
                                 "<li id=\"fn-1\">\n"
                                 "The note.<a href=\"#fnref-1-1\" class=\"footnote-backref\">&#8617;</a>\n"
                                 "</li>\n"
                                 "</ol>\n"
                                 "</section>\n"),
               md2html("Text[^1].\n"
                       "\n"
                       "[^1]: The note.\n") );
}

TEST(Markdown, FencedCode)
{
    EXPECT_EQ( QString::fromUtf8("<pre><code class=\"language-python\">print(&quot;&lt;a&gt;&quot;)\n"
                                 "</code></pre>\n"),
               md2html("```python\n"
                       "print(\"<a>\")\n"
                       "```\n") );
}

// indented code blocks are enabled, as they were with hoedown
TEST(Markdown, IndentedCode)
{
    EXPECT_EQ( QString::fromUtf8("<p>Para</p>\n"
                                 "<pre><code>code &lt;x&gt;\n"
                                 "</code></pre>\n"),
               md2html("Para\n"
                       "\n"
                       "    code <x>\n") );
}

TEST(Markdown, Links)
{
    EXPECT_EQ( QString::fromUtf8("<p>See <a href=\"https://natrongithub.github.io/\">https://natrongithub.github.io/</a>"
                                 " and <a href=\"https://github.com/NatronGitHub/Natron\">https://github.com/NatronGitHub/Natron</a>,"
                                 " <a href=\"http://www.example.com\">www.example.com</a>,"
                                 " or <a href=\"mailto:foo@example.com\">foo@example.com</a>.\n"
                                 "<a href=\"https://natron.fr\" title=\"title\">Natron</a></p>\n"),
               md2html("See <https://natrongithub.github.io/> and https://github.com/NatronGitHub/Natron,"
                       " www.example.com, or foo@example.com.\n"
                       "[Natron](https://natron.fr \"title\")\n") );
}

// an URL with a ':' in its path (as in the Merge plug-in description) is a link
// (it was not with md4c 0.6.0)
TEST(Markdown, AutolinkWithColonInPath)
{
    EXPECT_EQ( QString::fromUtf8("<ul>\n<li>&quot;Merge&quot; by Martin Constable "
                                 "<a href=\"https://web.archive.org/web/20220627015903/http://www.opticalenquiry.com/nuke/index.php?title=Merge\">"
                                 "https://web.archive.org/web/20220627015903/http://www.opticalenquiry.com/nuke/index.php?title=Merge</a></li>\n</ul>\n"),
               md2html("- \"Merge\" by Martin Constable https://web.archive.org/web/20220627015903/http://www.opticalenquiry.com/nuke/index.php?title=Merge\n") );
}

// LaTeX math spans use the MathJax delimiters \(...\) and \[...\], as with hoedown
TEST(Markdown, Math)
{
    EXPECT_EQ( QString::fromUtf8("<p>Inline \\(a^2+b^2\\) and display \\[\\sum_i x_i\\].</p>\n"),
               md2html("Inline $a^2+b^2$ and display $$\\sum_i x_i$$.\n") );
    // an <x-equation> element written as raw HTML is left as is
    EXPECT_EQ( QString::fromUtf8("<p><x-equation-foo>a</x-equation-foo> \\(b\\)</p>\n"),
               md2html("<x-equation-foo>a</x-equation-foo> $b$\n") );
}

// raw HTML is passed through, as with hoedown
TEST(Markdown, RawHtml)
{
    EXPECT_EQ( QString::fromUtf8("<p><b>bold</b> and <span style=\"color:red\">red</span></p>\n"
                                 "<div class=\"x\">\n"
                                 "block\n"
                                 "</div>\n"),
               md2html("<b>bold</b> and <span style=\"color:red\">red</span>\n"
                       "\n"
                       "<div class=\"x\">\n"
                       "block\n"
                       "</div>\n") );
}

TEST(Markdown, Unicode)
{
    EXPECT_EQ( QString::fromUtf8("<p>Unicode: \xc3\xa9 \xc3\xbc \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e \xe2\x80\x94 \xe2\x80\x9cquotes\xe2\x80\x9d &amp; \xc2\xa9 \xe2\x9c\x93</p>\n"),
               md2html("Unicode: \xc3\xa9 \xc3\xbc \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e \xe2\x80\x94 \xe2\x80\x9cquotes\xe2\x80\x9d &amp; &copy; \xe2\x9c\x93\n") );
}

TEST(Markdown, NestedLists)
{
    EXPECT_EQ( QString::fromUtf8("<ul>\n"
                                 "<li>one</li>\n"
                                 "<li>two<ol>\n"
                                 "<li>sub a</li>\n"
                                 "<li>sub b</li>\n"
                                 "</ol>\n"
                                 "</li>\n"
                                 "<li>three</li>\n"
                                 "</ul>\n"),
               md2html("- one\n"
                       "- two\n"
                       "    1. sub a\n"
                       "    2. sub b\n"
                       "- three\n") );
}

// no underline extension: _text_ is emphasis and __text__ is strong, as in CommonMark
TEST(Markdown, Spans)
{
    EXPECT_EQ( QString::fromUtf8("<p><del>del</del> <em>em</em> <em>em</em> <strong>strong</strong> <strong>strong</strong>"
                                 " <mark>hl</mark> <sup>sup</sup> foo_bar_baz</p>\n"),
               md2html("~~del~~ _em_ *em* **strong** __strong__ ==hl== ^sup^ foo_bar_baz\n") );
}

// plain text (the hints of parameters without Markdown hints, in the generated
// documentation) is rendered verbatim, including the characters of the extensions
TEST(Markdown, PlainTextToMarkdown)
{
    const QString plain = QString::fromUtf8("2^n by 2^n, file.~1~, $1 to $9, a == b, *x* [y](z) <b>");
    EXPECT_EQ( QString::fromUtf8("<p>2^n by 2^n, file.~1~, $1 to $9, a == b, *x* [y](z) &lt;b&gt;</p>\n"),
               Markdown::convert2html( convertFromPlainTextToMarkdown(plain, true, false) ) );
    // in a table cell
    EXPECT_EQ( QString::fromUtf8("<table>\n<thead>\n<tr>\n<th>A</th>\n<th>B</th>\n</tr>\n</thead>\n<tbody>\n<tr>\n"
                                 "<td>2^n by 2^n, file.~1~, $1 to $9, a == b, *x* [y](z) &lt;b&gt;</td>\n<td>c</td>\n"
                                 "</tr>\n</tbody>\n</table>\n"),
               Markdown::convert2html( QString::fromUtf8("A | B\n--- | ---\n") + convertFromPlainTextToMarkdown(plain, true, true) + QString::fromUtf8(" | c\n") ) );
}

TEST(Markdown, Headers)
{
    EXPECT_EQ( QString::fromUtf8("<h1>Title</h1>\n"
                                 "<p>#NoSpace</p>\n"
                                 "<h2>Sub</h2>\n"),
               md2html("# Title\n"
                       "\n"
                       "#NoSpace\n"
                       "\n"
                       "Sub\n"
                       "---\n") );
}

TEST(Markdown, ParseCustomLinksForHTML)
{
    EXPECT_EQ( QString::fromUtf8("see |html::/plugins/a.html| here"),
               Markdown::parseCustomLinksForHTML( QString::fromUtf8("see |html::/plugins/a.html||rst::a| here") ) );
}

// the Inputs and Controls tables of the node documentation get their class and toggle
TEST(Markdown, FixNodeHTML)
{
    QString html = Markdown::fixNodeHTML( md2html("Inputs\n"
                                                  "------\n"
                                                  "\n"
                                                  "Input | Description\n"
                                                  "--- | ---\n"
                                                  "Source | The image\n"
                                                  "\n"
                                                  "Controls\n"
                                                  "--------\n"
                                                  "\n"
                                                  "Parameter | Function\n"
                                                  "--- | ---\n"
                                                  "Size | The size\n") );
    EXPECT_TRUE( html.startsWith( QString::fromUtf8("<h2>Inputs <span class=\"showHideTable\">(<a class=\"toggleInputTable\" href=\"#\">+/-</a>)</span></h2>\n"
                                                    "<table class=\"inputTable\">\n") ) ) << html.toStdString();
    EXPECT_TRUE( html.contains( QString::fromUtf8("<h2>Controls <span class=\"showHideTable\">(<a class=\"toggleControlTable\" href=\"#\">+/-</a>)</span></h2>\n"
                                                  "<table class=\"controlTable\">\n") ) ) << html.toStdString();
}

// the headers of the Preferences documentation get an id
TEST(Markdown, FixSettingsHTML)
{
    EXPECT_EQ( QString::fromUtf8("<h2 id=\"general-settings\">General Settings</h2><p>Text</p>"),
               Markdown::fixSettingsHTML( md2html("General Settings\n"
                                                  "----------------\n"
                                                  "\n"
                                                  "Text\n") ) );
}
