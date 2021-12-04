#include "sourcefileframe.h"
#include <QUrl>
#include <QMimeData>

void SourceFileFrame::dragEnterEvent(QDragEnterEvent* e)
{
    QStackedWidget::dragEnterEvent(e);

    if(e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void SourceFileFrame::dropEvent(QDropEvent* e)
{
    QStringList files;
    const auto& urls = e->mimeData()->urls();
    files.reserve(urls.size());
    for(const QUrl& url : urls) {
        QString file = url.toLocalFile();
        files.append(file);
    }
    if(!files.isEmpty()) {
        emit filesDropped(files);
    }
}
