#ifndef FILELISTWIDGET_H
#define FILELISTWIDGET_H

#include <QListWidget>

class FileListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit FileListWidget(QWidget* parent = nullptr);

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent *event) override;
private slots:
    void onItemClicked(QListWidgetItem *current);

signals:
    void selectedFileChanged(const QString& filePath);
};

#endif // FILELISTWIDGET_H
