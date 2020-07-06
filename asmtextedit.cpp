#include "asmtextedit.h"

#include "compilerservice.h"

#include <QDebug>
#include <QJsonObject>
#include <QNetworkReply>
#include <QToolTip>
#include <qjsondocument.h>
#include <qsettings.h>

AsmTextEdit::AsmTextEdit(QWidget* parent)
    : QPlainTextEdit { parent }
    , m_highlighter { new AsmHighlighter { document() } }
{
    setMouseTracking(true);

    QSettings settings;
    QString fontSetting = settings.value("font").toString();
    QFont font = fontSetting.isEmpty() ? QFontDatabase::systemFont(QFontDatabase::FixedFont) : fontSetting;
    font.setFixedPitch(true);
    font.setPointSize(settings.value("fontSize", 12).toInt());

    setFont(font);
    setTabStopDistance(4 * QFontMetrics(font).horizontalAdvance(' '));

    //set background
    setStyleSheet("background-color: #272822; color: #e3e2d6;");
}

void AsmTextEdit::mouseMoveEvent(QMouseEvent* event)
{
    QTextCursor tc = cursorForPosition(event->pos());
    tc.select(QTextCursor::WordUnderCursor);
    QString strWord = tc.selectedText();
    auto gpos = mapToGlobal(event->pos());

    QNetworkReply* reply = CompileSvc::instance()->tooltipRequest(strWord);
    connect(reply, &QNetworkReply::readyRead, this, [=]() {
        QJsonObject doc = QJsonDocument::fromJson(reply->readAll()).object();
        auto resultObj = doc.value("result").toObject();
        auto value = resultObj.value("tooltip").toString();
        QString tooltip = value;
        QToolTip::showText(gpos, value, this);
    });
}
