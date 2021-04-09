#include "asmtextedit.h"

#include <QDebug>
#include <QJsonObject>
#include <QNetworkReply>
#include <QToolTip>
#include <qjsondocument.h>
#include <qsettings.h>

AsmTextEdit::AsmTextEdit(QWidget* parent)
    : QCodeEditor { parent }
{
    setMouseTracking(true);

    getHighlighter()->setCurrentLanguage(QSourceHighlite::QSourceHighliter::Language::CodeAsm);

    QSettings settings;
    QString fontSetting = settings.value(QStringLiteral("font")).toString();
    QFont font = fontSetting.isEmpty() ? QFontDatabase::systemFont(QFontDatabase::FixedFont) : fontSetting;
    font.setFixedPitch(true);
    font.setPointSize(settings.value(QStringLiteral("fontSize"), 12).toInt());

    setFont(font);
    setTabStopDistance(4 * QFontMetrics(font).horizontalAdvance(' '));

    //set background
    setStyleSheet(QStringLiteral("background-color: #272822;"));
}

void AsmTextEdit::mouseMoveEvent(QMouseEvent* event)
{
//    QTextCursor tc = cursorForPosition(event->pos());
//    tc.select(QTextCursor::WordUnderCursor);
//    const QString strWord = tc.selectedText();
//    if (strWord.isEmpty()) return;

    return QCodeEditor::mouseMoveEvent(event);
}

QString AsmTextEdit::getCurrentWordUnderCursor()
{
    const auto currentPosInWidget = mapFromGlobal(QCursor::pos());
    QTextCursor currentCursor = cursorForPosition(currentPosInWidget);
    currentCursor.select(QTextCursor::WordUnderCursor);
    auto currentWord = currentCursor.selectedText();
    return currentWord;
}
