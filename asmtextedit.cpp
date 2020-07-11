#include "asmtextedit.h"

#include "compilerservice.h"

#include <QDebug>
#include <QJsonObject>
#include <QNetworkReply>
#include <QToolTip>
#include <qjsondocument.h>
#include <qsettings.h>

#include <cxxabi.h>

AsmTextEdit::AsmTextEdit(QWidget* parent)
    : QCodeEditor { parent }
{
    setMouseTracking(true);

    getHighlighter()->setCurrentLanguage(QSourceHighlite::QSourceHighliter::Language::CodeAsm);

    QSettings settings;
    QString fontSetting = settings.value("font").toString();
    QFont font = fontSetting.isEmpty() ? QFontDatabase::systemFont(QFontDatabase::FixedFont) : fontSetting;
    font.setFixedPitch(true);
    font.setPointSize(settings.value("fontSize", 12).toInt());

    setFont(font);
    setTabStopDistance(4 * QFontMetrics(font).horizontalAdvance(' '));

    //set background
    setStyleSheet("background-color: #272822;");
}

void AsmTextEdit::setText(QString text)
{
    //look for _Z

    int next = 0;
    int last = 0;
    while (next != -1) {
        next = text.indexOf("_Z", last);
        //get token
        if (next != -1) {
            int token = text.indexOf(QRegExp(":|,|@|\\[\\s|\\n"), next + 1);
            int len = token - next;
            QStringRef tok = text.midRef(next, len);
            qDebug () << "Current token: " << tok;
            int status = 0;
            char* name = abi::__cxa_demangle(tok.toUtf8().constData(), 0, 0, &status);
            if (status != 0) {
                qDebug () << "Demangling of: " << tok << " failed, status: " << status;
            }
            QString qName{name};
            qName.prepend(' ');
            qDebug () << qName;
            text.replace(next, len, qName);
            free((void*)name);
        }
    }

    setPlainText(text);
}

void AsmTextEdit::mouseMoveEvent(QMouseEvent* event)
{
    QTextCursor tc = cursorForPosition(event->pos());
    tc.select(QTextCursor::WordUnderCursor);
    QString strWord = tc.selectedText();
    if (strWord.isEmpty()) return;
    auto gpos = mapToGlobal(event->pos());

    QNetworkReply* reply = CompileSvc::instance()->tooltipRequest(strWord);
    connect(reply, &QNetworkReply::readyRead, this, [=]() {
        QJsonObject doc = QJsonDocument::fromJson(reply->readAll()).object();
        auto resultObj = doc.value("result").toObject();
        auto value = resultObj.value("tooltip").toString();
        QString tooltip = value;
        QToolTip::showText(gpos, value, this);
    });
    return QCodeEditor::mouseMoveEvent(event);
}
