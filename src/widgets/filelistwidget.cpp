#include "filelistwidget.h"
#include <QMouseEvent>
#include <QDebug>

FileListWidget::FileListWidget(QWidget *parent)
    : QListWidget(parent)
{
    connect(this, &QListWidget::itemClicked, this, &FileListWidget::onItemClicked);
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

void FileListWidget::onItemClicked(QListWidgetItem *current)
{
    const QString filePath = current->data(Qt::UserRole).toString();
    emit selectedFileChanged(filePath);
}
