/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#include "ScriptTextEdit.h"

#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDropEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QPainter>
#include <QtCore/QRegExp>
#include <QtCore/QMimeData>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/Settings.h"
#include "Engine/EffectInstance.h"

#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobWidgetDnD.h" // KNOB_DND_MIME_DATA_KEY

NATRON_NAMESPACE_ENTER


struct PyHighLightRule
{
    PyHighLightRule(const QString &patternStr,
                    int n,
                    const QTextCharFormat &matchingFormat)
    {
        originalRuleStr = patternStr;
        pattern = QRegExp(patternStr);
        nth = n;
        format = matchingFormat;
    }

    QString originalRuleStr;
    QRegExp pattern;
    int nth;
    QTextCharFormat format;
};

struct PySyntaxHighlighterPrivate
{
    PySyntaxHighlighter* publicInterface;
    QStringList keywords;
    QStringList operators;
    QStringList braces;
    QHash<QString, QTextCharFormat> basicStyles;
    QList<PyHighLightRule> rules;
    QRegExp triSingleQuote;
    QRegExp triDoubleQuote;

    PySyntaxHighlighterPrivate(PySyntaxHighlighter* publicInterface)
        : publicInterface(publicInterface)
        , keywords()
        , operators()
        , braces()
        , basicStyles()
        , rules()
        , triSingleQuote()
        , triDoubleQuote()
    {
        keywords = QStringList() << QString::fromUtf8("and") << QString::fromUtf8("assert") << QString::fromUtf8("break") << QString::fromUtf8("class") << QString::fromUtf8("continue") << QString::fromUtf8("def") <<
                   QString::fromUtf8("del") << QString::fromUtf8("elif") << QString::fromUtf8("else") << QString::fromUtf8("except") << QString::fromUtf8("exec") << QString::fromUtf8("finally") <<
                   QString::fromUtf8("for") << QString::fromUtf8("from") << QString::fromUtf8("global") << QString::fromUtf8("if") << QString::fromUtf8("import") << QString::fromUtf8("in") <<
                   QString::fromUtf8("is") << QString::fromUtf8("lambda") << QString::fromUtf8("not") << QString::fromUtf8("or") << QString::fromUtf8("pass") << QString::fromUtf8("print") <<
                   QString::fromUtf8("raise") << QString::fromUtf8("return") << QString::fromUtf8("try") << QString::fromUtf8("while") << QString::fromUtf8("yield") <<
                   QString::fromUtf8("None") << QString::fromUtf8("True") << QString::fromUtf8("False");

        operators = QStringList() << QString::fromUtf8("=") <<
                    // Comparison
                    QString::fromUtf8("==") << QString::fromUtf8("!=") << QString::fromUtf8("<") << QString::fromUtf8("<=") << QString::fromUtf8(">") << QString::fromUtf8(">=") <<
                    // Arithmetic
                    QString::fromUtf8("\\+") << QString::fromUtf8("-") << QString::fromUtf8("\\*") << QString::fromUtf8("/") << QString::fromUtf8("//") << QString::fromUtf8("%") << QString::fromUtf8("\\*\\*") <<
                    // In-place
                    QString::fromUtf8("\\+=") << QString::fromUtf8("-=") << QString::fromUtf8("\\*=") << QString::fromUtf8("/=") << QString::fromUtf8("%=") <<
                    // Bitwise
                    QString::fromUtf8("\\^") << QString::fromUtf8("\\|") << QString::fromUtf8("&") << QString::fromUtf8("~") << QString::fromUtf8(">>") << QString::fromUtf8("<<");

        braces = QStringList() << QString::fromUtf8("\\{") << QString::fromUtf8("\\}") << QString::fromUtf8("\\(") << QString::fromUtf8("\\)") << QString::fromUtf8("\\[") << QString::fromUtf8("\\]");

        triSingleQuote.setPattern( QString::fromUtf8("'''") );
        triDoubleQuote.setPattern( QString::fromUtf8("\"\"\"") );

        reload();
    }

    void reload();

    void initializeRules();
    const QTextCharFormat getTextCharFormat( double r, double g, double b, const QString &style = QString() );
};

void
PySyntaxHighlighter::highlightBlock(const QString &text)
{
    for (QList<PyHighLightRule>::Iterator it = _imp->rules.begin(); it != _imp->rules.end(); ++it) {
        int idx = it->pattern.indexIn(text, 0);
        while (idx >= 0) {
            // Get index of Nth match
            idx = it->pattern.pos(it->nth);
            int length = it->pattern.cap(it->nth).length();
            setFormat(idx, length, it->format);
            idx = it->pattern.indexIn(text, idx + length);
        }
    }

    setCurrentBlockState(0);

    // Do multi-line strings
    bool isInMultiline = matchMultiline( text, _imp->triSingleQuote, 1, _imp->basicStyles.value( QString::fromUtf8( ("string2") ) ) );
    if (!isInMultiline) {
        bool stillMultiline = matchMultiline( text, _imp->triDoubleQuote, 2, _imp->basicStyles.value( QString::fromUtf8("string2") ) );
        Q_UNUSED(stillMultiline);
    }
}

void
PySyntaxHighlighterPrivate::initializeRules()
{
    rules.clear();
    for (QStringList::Iterator it = keywords.begin(); it != keywords.end(); ++it) {
        rules.append( PyHighLightRule( QString::fromUtf8("\\b%1\\b").arg(*it), 0, basicStyles.value( QString::fromUtf8( ("keyword") ) ) ) );
    }
    for (QStringList::Iterator it = operators.begin(); it != operators.end(); ++it) {
        rules.append( PyHighLightRule( QString::fromUtf8("%1").arg(*it), 0, basicStyles.value( QString::fromUtf8( ("operator") ) ) ) );
    }

    for (QStringList::Iterator it = braces.begin(); it != braces.end(); ++it) {
        rules.append( PyHighLightRule( QString::fromUtf8("%1").arg(*it), 0, basicStyles.value( QString::fromUtf8( ("brace") ) ) ) );
    }

    // 'self'
    rules.append( PyHighLightRule( QString::fromUtf8("\\bself\\b"), 0, basicStyles.value( QString::fromUtf8( ("self") ) ) ) );

    // Double-quoted string, possibly containing escape sequences
    rules.append( PyHighLightRule( QString::fromUtf8("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\""), 0, basicStyles.value( QString::fromUtf8("string") ) ) );

    // Single-quoted string, possibly containing escape sequences
    rules.append( PyHighLightRule( QString::fromUtf8("'[^'\\\\]*(\\\\.[^'\\\\]*)*'"), 0, basicStyles.value( QString::fromUtf8("string") ) ) );

    // 'def' followed by an identifier
    rules.append( PyHighLightRule( QString::fromUtf8("\\bdef\\b\\s*(\\w+)"), 1, basicStyles.value( QString::fromUtf8("defclass") ) ) );

    //  'class' followed by an identifier
    rules.append( PyHighLightRule( QString::fromUtf8("\\bclass\\b\\s*(\\w+)"), 1, basicStyles.value( QString::fromUtf8("defclass") ) ) );

    // From '#' until a newline
    rules.append( PyHighLightRule( QString::fromUtf8("#[^\\n]*"), 0, basicStyles.value( QString::fromUtf8("comment") ) ) );

    // Numeric literals
    rules.append( PyHighLightRule( QString::fromUtf8("\\b[+-]?[0-9]+[lL]?\\b"), 0, basicStyles.value( QString::fromUtf8("numbers") ) ) );
    rules.append( PyHighLightRule( QString::fromUtf8("\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b"), 0, basicStyles.value( QString::fromUtf8("numbers") ) ) );
    rules.append( PyHighLightRule( QString::fromUtf8("\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b"), 0, basicStyles.value( QString::fromUtf8("numbers") ) ) );
}

bool
PySyntaxHighlighter::matchMultiline(const QString &text,
                                    const QRegExp &delimiter,
                                    const int inState,
                                    const QTextCharFormat &style)
{
    int start = -1;
    int add = -1;
    int end = -1;
    int length = 0;

    // If inside triple-single quotes, start at 0
    // Otherwise, look for the delimiter on this line
    if (previousBlockState() == inState) {
        start = 0;
        add = 0;
    } else {
        start = delimiter.indexIn(text);
        // Move past this match
        add = delimiter.matchedLength();
    }

    // As long as there's a delimiter match on this line...
    while (start >= 0) {
        // Look for the ending delimiter
        end = delimiter.indexIn(text, start + add);
        // Ending delimiter on this line?
        if (end >= add) {
            length = end - start + add + delimiter.matchedLength();
            setCurrentBlockState(0);
        } else {
            // No= multi-line string
            setCurrentBlockState(inState);
            length = text.length() - start + add;
        }
        // Apply formatting and look for next
        setFormat(start, length, style);
        start = delimiter.indexIn(text, start + length);
    }
    // Return True if still inside a multi-line string, False otherwise
    if (currentBlockState() == inState) {
        return true;
    } else {
        return false;
    }
}

const QTextCharFormat
PySyntaxHighlighterPrivate::getTextCharFormat(double r,
                                              double g,
                                              double b,
                                              const QString &style)
{
    QTextCharFormat charFormat;
    QColor color;

    color.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
    charFormat.setForeground(color);
    if ( style.contains(QString::fromUtf8("bold"), Qt::CaseInsensitive) ) {
        charFormat.setFontWeight(QFont::Bold);
    }
    if ( style.contains(QString::fromUtf8("italic"), Qt::CaseInsensitive) ) {
        charFormat.setFontItalic(true);
    }

    return charFormat;
}

PySyntaxHighlighter::PySyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , _imp( new PySyntaxHighlighterPrivate(this) )
{
}

PySyntaxHighlighter::~PySyntaxHighlighter()
{
}

void
PySyntaxHighlighter::reload()
{
    _imp->reload();
}

void
PySyntaxHighlighterPrivate::reload()
{
    basicStyles.clear();

    double r, g, b;
    SettingsPtr s = appPTR->getCurrentSettings();
    s->getSEKeywordColor(&r, &g, &b);
    basicStyles.insert( QString::fromUtf8("keyword"), getTextCharFormat( r, g, b, QString::fromUtf8("bold") ) );

    s->getSEOperatorColor(&r, &g, &b);
    basicStyles.insert( QString::fromUtf8("operator"), getTextCharFormat(r, g, b) );

    s->getSEBraceColor(&r, &g, &b);
    basicStyles.insert( QString::fromUtf8("brace"), getTextCharFormat(r, g, b) );

    s->getSEDefClassColor(&r, &g, &b);
    basicStyles.insert( QString::fromUtf8("defclass"), getTextCharFormat( r, g, b, QString::fromUtf8("bold") ) );

    s->getSEStringsColor(&r, &g, &b);
    basicStyles.insert( QString::fromUtf8("string"), getTextCharFormat(r, g, b) );
    basicStyles.insert( QString::fromUtf8("string2"), getTextCharFormat(r, g, b) );

    s->getSECommentsColor(&r, &g, &b);
    basicStyles.insert( QString::fromUtf8("comment"), getTextCharFormat( r, g, b, QString::fromUtf8("italic") ) );

    s->getSESelfColor(&r, &g, &b);
    basicStyles.insert( QString::fromUtf8("self"), getTextCharFormat( r, g, b, QString::fromUtf8("bold,italic") ) );

    s->getSENumbersColor(&r, &g, &b);
    basicStyles.insert( QString::fromUtf8("numbers"), getTextCharFormat(r, g, b) );

    initializeRules();
}

InputScriptTextEdit::InputScriptTextEdit(Gui* gui,
                                         QWidget* parent)
    : QPlainTextEdit(parent)
    , _lineNumber( new LineNumberWidget(this) )
    , _gui(gui)
{
    QObject::connect( this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)) );
    QObject::connect( this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)) );
    QObject::connect( this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()) );

    _highlighter = new PySyntaxHighlighter( document() );
    setPlainText(QString::fromUtf8(""));
}

void
InputScriptTextEdit::reloadHighlighter()
{
    _highlighter->reload();
    _highlighter->rehighlight();
}

InputScriptTextEdit::~InputScriptTextEdit()
{
}

int
InputScriptTextEdit::getLineNumberAreaWidth()
{
    int digits = 1;
    int max = std::max( 1, blockCount() );

    //Get the max number of digits
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return 3 + fontMetrics().horizontalAdvance( QLatin1Char('9') ) * digits;
#else
    return 3 + fontMetrics().width( QLatin1Char('9') ) * digits;
#endif
}

void
InputScriptTextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();

    _lineNumber->setGeometry( QRect( cr.left(), cr.top(), getLineNumberAreaWidth(), cr.height() ) );
}

void
InputScriptTextEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(getLineNumberAreaWidth(), 0, 0, 0);
}

void
InputScriptTextEdit::updateLineNumberArea(const QRect &rect,
                                          int dy)
{
    if (dy) {
        _lineNumber->scroll(0, dy);
    } else {
        _lineNumber->update( 0, rect.y(), _lineNumber->width(), rect.height() );
    }
    if ( rect.contains( viewport()->rect() ) ) {
        updateLineNumberAreaWidth(0);
    }
}

void
InputScriptTextEdit::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if ( !isReadOnly() ) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(100, 100, 100);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void
InputScriptTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(_lineNumber);
    QColor fillColor;
    QColor txtColor;
    QColor selColor;
    {
        double r, g, b;
        appPTR->getCurrentSettings()->getRaisedColor(&r, &g, &b);
        fillColor.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        txtColor.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
        appPTR->getCurrentSettings()->getSelectionColor(&r, &g, &b);
        selColor.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
    }

    painter.fillRect(event->rect(), fillColor);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated( contentOffset() ).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while ( block.isValid() && top <= event->rect().bottom() ) {
        if ( block.isVisible() && ( bottom >= event->rect().top() ) ) {
            QString number = QString::number(blockNumber + 1);
            if ( blockNumber == textCursor().block().blockNumber() ) {
                painter.setPen(selColor);
            } else {
                painter.setPen(txtColor);
            }

            painter.drawText(0, top, _lineNumber->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void
InputScriptTextEdit::dragEnterEvent(QDragEnterEvent* e)
{
    QStringList formats = e->mimeData()->formats();

    if ( formats.contains( QLatin1String(KNOB_DND_MIME_DATA_KEY) ) ) {
        setCursor(Qt::DragCopyCursor);
        e->acceptProposedAction();
    }
}

void
InputScriptTextEdit::dragMoveEvent(QDragMoveEvent* e)
{
    if ( !acceptDrops() ) {
        return;
    }
    QStringList formats = e->mimeData()->formats();

    if ( formats.contains( QLatin1String(KNOB_DND_MIME_DATA_KEY) ) ) {
        e->acceptProposedAction();
    }
}

void
InputScriptTextEdit::dropEvent(QDropEvent* e)
{
    if ( !acceptDrops() ) {
        return;
    }
    QStringList formats = e->mimeData()->formats();
    if ( formats.contains( QLatin1String(KNOB_DND_MIME_DATA_KEY) ) ) {
        int cbDim;
        KnobIPtr fromKnob;
        QDrag* drag;
        _gui->getApp()->getKnobDnDData(&drag, &fromKnob, &cbDim);

        if (fromKnob) {
            EffectInstance* isEffect = dynamic_cast<EffectInstance*>( fromKnob->getHolder() );
            if (isEffect) {
                QString toAppend;
                toAppend.append( QString::fromUtf8( isEffect->getNode()->getFullyQualifiedName().c_str() ) );
                toAppend.append( QLatin1Char('.') );
                toAppend.append( QString::fromUtf8( fromKnob->getName().c_str() ) );
                toAppend.append( QString::fromUtf8(".get()") );
                if (fromKnob->getDimension() > 1) {
                    toAppend.append( QLatin1Char('[') );
                    if ( (cbDim == -1) || ( (cbDim == 0) && !fromKnob->getAllDimensionVisible() ) ) {
                        toAppend.append( QString::fromUtf8("dimension") );
                    } else {
                        toAppend.append( QString::number(cbDim) );
                    }
                    toAppend.append( QLatin1Char(']') );
                }
                appendPlainText(toAppend);
                e->acceptProposedAction();
            }
        }
    }
}

void
InputScriptTextEdit::enterEvent(QEvent* /*e*/)
{
    if ( acceptDrops() && (cursor().shape() != Qt::OpenHandCursor) ) {
        setCursor(Qt::OpenHandCursor);
    }
}

void
InputScriptTextEdit::leaveEvent(QEvent* /*e*/)
{
    if ( acceptDrops() && (cursor().shape() == Qt::OpenHandCursor) ) {
        setCursor(Qt::ArrowCursor);
    }
}

OutputScriptTextEdit::OutputScriptTextEdit(QWidget* parent)
    : QTextEdit(parent)
{
}

OutputScriptTextEdit::~OutputScriptTextEdit()
{
}

void
OutputScriptTextEdit::showEvent(QShowEvent* e)
{
    QTextEdit::showEvent(e);

    verticalScrollBar()->setValue( verticalScrollBar()->maximum() );
    Q_EMIT userScrollChanged(true);
}

void
OutputScriptTextEdit::scrollContentsBy(int dx,
                                       int dy)
{
    QTextEdit::scrollContentsBy(dx, dy);
    QScrollBar* sb = verticalScrollBar();
    int v = sb->value();
    int max = sb->maximum();
    Q_EMIT userScrollChanged(v == max);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ScriptTextEdit.cpp"
