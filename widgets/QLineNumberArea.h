#pragma once

// Qt
#include <QWidget> // Required for inheritance

class QCodeEditor;
class QSyntaxStyle;

/**
 * @brief Class, that describes line number area widget.
 */
class QLineNumberArea : public QWidget
{
    Q_OBJECT

public:

    /**
     * @brief Constructor.
     * @param parent Pointer to parent QTextEdit widget.
     */
    explicit QLineNumberArea(QCodeEditor* parent=nullptr);

    // Disable copying
    QLineNumberArea(const QLineNumberArea&) = delete;
    QLineNumberArea& operator=(const QLineNumberArea&) = delete;

    /**
     * @brief Overridden method for getting line number area
     * size.
     */
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:

    QCodeEditor* m_codeEditParent;

};

