#ifndef SOURCEFILEFRAME_H
#define SOURCEFILEFRAME_H

#include <QStackedWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QStringList>

class SourceFileFrame : public QStackedWidget
{
    Q_OBJECT

public:
    SourceFileFrame(QWidget *parent = nullptr) : QStackedWidget(parent) {}

protected:
    void dragEnterEvent(QDragEnterEvent* e);
    void dropEvent(QDropEvent* e);

signals:
    void filesDropped(QStringList files);
};

#endif // SOURCEFILEFRAME_H
