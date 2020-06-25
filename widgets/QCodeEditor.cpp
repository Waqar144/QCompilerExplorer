// QCodeEditor
#include "QCodeEditor.h"
#include "QFramedTextAttribute.h"
#include "QLineNumberArea.h"

#include "../QSourceHighlite/qsourcehighliter.h"

// Qt
#include <QAbstractItemView>
#include <QAbstractTextDocumentLayout>
#include <QCompleter>
#include <QCursor>
#include <QFontDatabase>
#include <QMimeData>
#include <QPaintEvent>
#include <QScrollBar>
#include <QShortcut>
#include <QTextBlock>
#include <QTextCharFormat>

QHash<QString, QSourceHighlite::QSourceHighliter::Language> QCodeEditor::_langStringToEnum = {
    { QLatin1String("c"), QSourceHighlite::QSourceHighliter::Language::CodeC },
    { QLatin1String("cpp"), QSourceHighlite::QSourceHighliter::Language::CodeCpp },
    { QLatin1String("cxx"), QSourceHighlite::QSourceHighliter::Language::CodeCpp },
    { QLatin1String("c++"), QSourceHighlite::QSourceHighliter::Language::CodeCpp },
    { QLatin1String("go"), QSourceHighlite::QSourceHighliter::Language::CodeGo },
    { QLatin1String("py"), QSourceHighlite::QSourceHighliter::Language::CodePython },
    { QLatin1String("python"), QSourceHighlite::QSourceHighliter::Language::CodePython },
    { QLatin1String("rust"), QSourceHighlite::QSourceHighliter::Language::CodeRust },
};

static QVector<QPair<QString, QString>> parentheses = {
    {"(", ")"},
    {"{", "}"},
    {"[", "]"},
    {"\"", "\""},
    {"'", "'"}
};

QCodeEditor::QCodeEditor(QWidget* widget)
    : QTextEdit(widget)
    , m_highlighter(nullptr)
    , m_lineNumberArea(new QLineNumberArea(this))
    , m_framedAttribute(new QFramedTextAttribute(this))
    , m_autoIndentation(true)
    , m_autoParentheses(true)
    , m_replaceTab(true)
    , m_tabReplace(QString(4, ' '))
{
    initDocumentLayoutHandlers();
    initFont();
    performConnections();

    m_highlighter = new QSourceHighlite::QSourceHighliter(document());
    m_highlighter->setTheme(QSourceHighlite::QSourceHighliter::Monokai);
    setStyleSheet("background-color: #272822;");

    //    setSyntaxStyle(QSyntaxStyle::defaultStyle());
}

void QCodeEditor::initDocumentLayoutHandlers()
{
    document()
        ->documentLayout()
        ->registerHandler(
            QFramedTextAttribute::type(),
            m_framedAttribute
        );
}

void QCodeEditor::initFont()
{
    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setFixedPitch(true);
    font.setPointSize(10);
    setTabStopDistance(4 * QFontMetrics(font).horizontalAdvance(' '));

    setFont(font);
}

void QCodeEditor::performConnections()
{
    connect(
        document(),
        &QTextDocument::blockCountChanged,
        this,
        &QCodeEditor::updateLineNumberAreaWidth
    );

    connect(
        verticalScrollBar(),
        &QScrollBar::valueChanged,
        [this](int){ m_lineNumberArea->update(); }
    );

    connect(
        this,
        &QTextEdit::cursorPositionChanged,
        this,
        &QCodeEditor::updateExtraSelection);

    connect(
        this,
        &QTextEdit::selectionChanged,
        this,
        &QCodeEditor::onSelectionChanged);
}

void QCodeEditor::initCodeLangs()
{
    using namespace QSourceHighlite;
    QCodeEditor::_langStringToEnum = QHash<QString, QSourceHighliter::Language> {
        { QLatin1String("c"), QSourceHighliter::Language::CodeC },
        { QLatin1String("cpp"), QSourceHighliter::Language::CodeCpp },
        { QLatin1String("cxx"), QSourceHighliter::Language::CodeCpp },
        { QLatin1String("c++"), QSourceHighliter::Language::CodeCpp },
        { QLatin1String("go"), QSourceHighliter::Language::CodeGo },
        { QLatin1String("py"), QSourceHighliter::Language::CodePython },
        { QLatin1String("python"), QSourceHighliter::Language::CodePython },
        { QLatin1String("rust"), QSourceHighliter::Language::CodeRust },
    };
}

void QCodeEditor::updateStyle()
{
    if (m_highlighter)
    {
        m_highlighter->rehighlight();
    }

    //    if (m_syntaxStyle)
    //    {
    //        auto currentPalette = palette();

    //        // Setting text format/color
    //        currentPalette.setColor(
    //            QPalette::ColorRole::Text,
    //            m_syntaxStyle->getFormat("Text").foreground().color()
    //        );

    //        // Setting common background
    //        currentPalette.setColor(
    //            QPalette::Base,
    //            m_syntaxStyle->getFormat("Text").background().color()
    //        );

    //        // Setting selection color
    //        currentPalette.setColor(
    //            QPalette::Highlight,
    //            m_syntaxStyle->getFormat("Selection").background().color()
    //        );

    //        setPalette(currentPalette);
    //    }

    updateExtraSelection();
}

void QCodeEditor::onSelectionChanged()
{
    auto selected = textCursor().selectedText();

    auto cursor = textCursor();

    // Cursor is null if setPlainText was called.
    if (cursor.isNull())
    {
        return;
    }

    cursor.movePosition(QTextCursor::MoveOperation::Left);
    cursor.select(QTextCursor::SelectionType::WordUnderCursor);

    QSignalBlocker blocker(this);
    m_framedAttribute->clear(cursor);

    if (selected.size() > 1 &&
        cursor.selectedText() == selected)
    {
        auto backup = textCursor();

        // Perform search selecting
        handleSelectionQuery(cursor);

        setTextCursor(backup);
    }
}

void QCodeEditor::resizeEvent(QResizeEvent* e)
{
    QTextEdit::resizeEvent(e);

    updateLineGeometry();
}

void QCodeEditor::updateLineGeometry()
{
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(),
              cr.top(),
              m_lineNumberArea->sizeHint().width(),
              cr.height()
        )
    );
}

void QCodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(m_lineNumberArea->sizeHint().width(), 0, 0, 0);
}

void QCodeEditor::updateLineNumberArea(const QRect& rect)
{
    m_lineNumberArea->update(
        0,
        rect.y(),
        m_lineNumberArea->sizeHint().width(),
        rect.height()
    );
    updateLineGeometry();

    if (rect.contains(viewport()->rect()))
    {
        updateLineNumberAreaWidth(0);
    }
}

void QCodeEditor::handleSelectionQuery(QTextCursor cursor)
{

    auto searchIterator = cursor;
    searchIterator.movePosition(QTextCursor::Start);
    searchIterator = document()->find(cursor.selectedText(), searchIterator);
    while (searchIterator.hasSelection())
    {
        m_framedAttribute->frame(searchIterator);

        searchIterator = document()->find(cursor.selectedText(), searchIterator);
    }
}

void QCodeEditor::updateExtraSelection()
{
    QList<QTextEdit::ExtraSelection> extra;

    highlightCurrentLine(extra);
    highlightParenthesis(extra);

    setExtraSelections(extra);
}

void QCodeEditor::highlightParenthesis(QList<QTextEdit::ExtraSelection>& extraSelection)
{
    auto currentSymbol = charUnderCursor();
    auto prevSymbol = charUnderCursor(-1);

    for (auto& pair : parentheses)
    {
        int direction;

        QChar counterSymbol;
        QChar activeSymbol;
        auto position = textCursor().position();

        if (pair.first == currentSymbol)
        {
            direction = 1;
            counterSymbol = pair.second[0];
            activeSymbol = currentSymbol;
        }
        else if (pair.second == prevSymbol)
        {
            direction = -1;
            counterSymbol = pair.first[0];
            activeSymbol = prevSymbol;
            position--;
        }
        else
        {
            continue;
        }

        auto counter = 1;

        while (counter != 0 &&
               position > 0 &&
               position < (document()->characterCount() - 1))
        {
            // Moving position
            position += direction;

            auto character = document()->characterAt(position);
            // Checking symbol under position
            if (character == activeSymbol)
            {
                ++counter;
            }
            else if (character == counterSymbol)
            {
                --counter;
            }
        }

        //        auto format = m_syntaxStyle->getFormat("Parentheses");

        // Found
        if (counter == 0) {
            ExtraSelection selection {};

            auto directionEnum = direction < 0 ? QTextCursor::MoveOperation::Left
                                               : QTextCursor::MoveOperation::Right;

            selection.format = QTextCharFormat();
            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            selection.cursor.movePosition(
                directionEnum,
                QTextCursor::MoveMode::MoveAnchor,
                std::abs(textCursor().position() - position)
            );

            selection.cursor.movePosition(
                QTextCursor::MoveOperation::Right,
                QTextCursor::MoveMode::KeepAnchor,
                1
            );

            extraSelection.append(selection);

            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            selection.cursor.movePosition(
                directionEnum,
                QTextCursor::MoveMode::KeepAnchor,
                1
            );

            extraSelection.append(selection);
        }

        break;
    }
}

void QCodeEditor::highlightCurrentLine(QList<QTextEdit::ExtraSelection>& extraSelection)
{
    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection {};

        QTextCharFormat fmt;
        fmt.setBackground(QColor(60, 60, 60));
        selection.format = fmt;
        selection.format.setForeground(QBrush());
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();

        extraSelection.append(selection);
    }
}

void QCodeEditor::paintEvent(QPaintEvent* e)
{
    updateLineNumberArea(e->rect());
    QTextEdit::paintEvent(e);
}

int QCodeEditor::getFirstVisibleBlock()
{
    // Detect the first block for which bounding rect - once translated
    // in absolute coordinated - is contained by the editor's text area

    // Costly way of doing but since "blockBoundingGeometry(...)" doesn't
    // exists for "QTextEdit"...

    QTextCursor curs = QTextCursor(document());
    curs.movePosition(QTextCursor::Start);
    for(int i=0; i < document()->blockCount(); ++i)
    {
        QTextBlock block = curs.block();

        QRect r1 = viewport()->geometry();
        QRect r2 = document()
            ->documentLayout()
            ->blockBoundingRect(block)
            .translated(
                viewport()->geometry().x(),
                viewport()->geometry().y() - verticalScrollBar()->sliderPosition()
            ).toRect();

        if (r1.intersects(r2))
        {
            return i;
        }

        curs.movePosition(QTextCursor::NextBlock);
    }

    return 0;
}

void QCodeEditor::keyPressEvent(QKeyEvent* e) {
#if QT_VERSION >= 0x050A00
  const int defaultIndent = tabStopDistance() / fontMetrics().averageCharWidth();
#else
  const int defaultIndent = tabStopWidth() / fontMetrics().averageCharWidth();
#endif

  bool completerSkip = false;

  if (!completerSkip) {
    if (m_replaceTab && e->key() == Qt::Key_Tab &&
        e->modifiers() == Qt::NoModifier) {
      insertPlainText(m_tabReplace);
      return;
    }

    // Auto indentation
    int indentationLevel = getIndentationSpaces();

#if QT_VERSION >= 0x050A00
    int tabCounts =
        indentationLevel * fontMetrics().averageCharWidth() / tabStopDistance();
#else
    int tabCounts =
        indentationLevel * fontMetrics().averageCharWidth() / tabStopWidth();
#endif

    // Have Qt Edior like behaviour, if {|} and enter is pressed indent the two
    // parenthesis
    if (m_autoIndentation && 
       (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
        charUnderCursor() == '}' && charUnderCursor(-1) == '{') 
    {
      int charsBack = 0;
      insertPlainText("\n");

      if (m_replaceTab)
        insertPlainText(QString(indentationLevel + defaultIndent, ' '));
      else
        insertPlainText(QString(tabCounts + 1, '\t'));

      insertPlainText("\n");
      charsBack++;

      if (m_replaceTab) 
      {
        insertPlainText(QString(indentationLevel, ' '));
        charsBack += indentationLevel;
      }
      else 
      {
        insertPlainText(QString(tabCounts, '\t'));
        charsBack += tabCounts;
      }

      while (charsBack--)
        moveCursor(QTextCursor::MoveOperation::Left);
      return;
    }

    // Shortcut for moving line to left
    if (m_replaceTab && e->key() == Qt::Key_Backtab) {
      indentationLevel = std::min(indentationLevel, m_tabReplace.size());

      auto cursor = textCursor();

      cursor.movePosition(QTextCursor::MoveOperation::StartOfLine);
      cursor.movePosition(QTextCursor::MoveOperation::Right,
                          QTextCursor::MoveMode::KeepAnchor, indentationLevel);

      cursor.removeSelectedText();
      return;
    }

    QTextEdit::keyPressEvent(e);

    if (m_autoIndentation && (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)) {
      if (m_replaceTab)
        insertPlainText(QString(indentationLevel, ' '));
      else
        insertPlainText(QString(tabCounts, '\t'));
    }

    if (m_autoParentheses) 
    {
      for (auto&& el : parentheses) 
      {
                // Inserting closed brace
                if (el.first == e->text()) 
                {
                  insertPlainText(el.second);
                  moveCursor(QTextCursor::MoveOperation::Left);
                  break;
                }

                // If it's close brace - check parentheses
                if (el.second == e->text())
                {
                    auto symbol = charUnderCursor();

                    if (symbol == el.second)
                    {
                        textCursor().deletePreviousChar();
                        moveCursor(QTextCursor::MoveOperation::Right);
                    }

                    break;
                }
            }
        }
    }

}

void QCodeEditor::setAutoIndentation(bool enabled)
{
    m_autoIndentation = enabled;
}

bool QCodeEditor::autoIndentation() const
{
    return m_autoIndentation;
}

void QCodeEditor::setAutoParentheses(bool enabled)
{
    m_autoParentheses = enabled;
}

bool QCodeEditor::autoParentheses() const
{
    return m_autoParentheses;
}

void QCodeEditor::setTabReplace(bool enabled)
{
    m_replaceTab = enabled;
}

void QCodeEditor::setCurrentLanguage(const QString& language)
{
    m_highlighter->setCurrentLanguage(QCodeEditor::_langStringToEnum.value(language));
}

bool QCodeEditor::tabReplace() const
{
    return m_replaceTab;
}

void QCodeEditor::setTabReplaceSize(int val)
{
    m_tabReplace.clear();

    m_tabReplace.fill(' ', val);
}

int QCodeEditor::tabReplaceSize() const
{
    return m_tabReplace.size();
}

void QCodeEditor::focusInEvent(QFocusEvent *e)
{
    QTextEdit::focusInEvent(e);
}

QChar QCodeEditor::charUnderCursor(int offset) const
{
    auto block = textCursor().blockNumber();
    auto index = textCursor().positionInBlock();
    auto text = document()->findBlockByNumber(block).text();

    index += offset;

    if (index < 0 || index >= text.size())
    {
        return {};
    }

    return text[index];
}

QString QCodeEditor::wordUnderCursor() const
{
    auto tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void QCodeEditor::insertFromMimeData(const QMimeData* source)
{
    insertPlainText(source->text());
}

int QCodeEditor::getIndentationSpaces()
{
    auto blockText = textCursor().block().text();

    int indentationLevel = 0;

    for (auto i = 0;
         i < blockText.size() && QString("\t ").contains(blockText[i]);
         ++i)
    {
        if (blockText[i] == ' ')
        {
            indentationLevel++;
        }
        else
        {
#if QT_VERSION >= 0x050A00
            indentationLevel += tabStopDistance() / fontMetrics().averageCharWidth();
#else
            indentationLevel += tabStopWidth() / fontMetrics().averageCharWidth();
#endif
        }
    }

    return indentationLevel;
}
