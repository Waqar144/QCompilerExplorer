#include "filelistwidget.h"
#include <QMouseEvent>
#include <QDebug>

FileListWidget::FileListWidget(QWidget *parent)
    : QListWidget(parent)
{
}


void FileListWidget::mousePressEvent(QMouseEvent *event)
{
    auto item = itemAt(event->pos());
    if (item == nullptr) {
        this->clearSelection();
        this->setCurrentItem(nullptr);
    } else {
        return QListWidget::mousePressEvent(event);
    }
}
