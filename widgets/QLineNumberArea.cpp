// QCodeEditor
#include "QLineNumberArea.h"
#include "QCodeEditor.h"

// Qt
#include <QTextEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>

QLineNumberArea::QLineNumberArea(QCodeEditor* parent)
    : QWidget(parent)
    , m_codeEditParent(parent)
{

}

QSize QLineNumberArea::sizeHint() const
{
    if (m_codeEditParent == nullptr)
    {
        return QWidget::sizeHint();
    }

    // Calculating width
    int digits = 1;
    int max = qMax(1, m_codeEditParent->document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

#if QT_VERSION >= 0x050B00
    int space = 13 + m_codeEditParent->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
#else
    int space = 13 + m_codeEditParent->fontMetrics().width(QLatin1Char('9')) * digits;
#endif

    return {space, 0};
}

void QLineNumberArea::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    //    Clearing rect to update
    painter.fillRect(
        event->rect(),
        QColor("#404244"));

    auto blockNumber = m_codeEditParent->getFirstVisibleBlock();
    auto block       = m_codeEditParent->document()->findBlockByNumber(blockNumber);
    auto top         = (int) m_codeEditParent->document()->documentLayout()->blockBoundingRect(block).translated(0, -m_codeEditParent->verticalScrollBar()->value()).top();
    auto bottom = top + (int)m_codeEditParent->document()->documentLayout()->blockBoundingRect(block).height();

    auto currentLine = QColor("#eef067");
    auto otherLines = QColor("#a6a6a6");
    auto boldfont = m_codeEditParent->font();
    auto normalfont = m_codeEditParent->font();
    boldfont.setBold(true);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);

            auto isCurrentLine = m_codeEditParent->textCursor().blockNumber() == blockNumber;
            painter.setPen(isCurrentLine ? currentLine : otherLines);
            painter.setFont(isCurrentLine ? boldfont : normalfont);

            painter.drawText(
                -5,
                top,
                sizeHint().width(),
                m_codeEditParent->fontMetrics().height(),
                Qt::AlignRight,
                number
            );
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) m_codeEditParent->document()->documentLayout()->blockBoundingRect(block).height();
        ++blockNumber;
    }
}
