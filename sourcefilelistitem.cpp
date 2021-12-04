#include "sourcefilelistitem.h"
#include <QDateTime>
#include <QLocale>
#include "util.h"

SourceFileListItem* SourceFileListItem::create(QTreeWidgetItem *parent, const SourceFile& file)
{
    auto* item = new SourceFileListItem(parent, QStringList{
                                            file.fileName(),
                                            QLocale().toString(file.lastModified(), QLocale::ShortFormat),
                                            friendlySize(file.size())
                                        });
    item->setData(1, Qt::UserRole+1, file.lastModified());
    item->setData(2, Qt::UserRole+1, file.size());
    item->setTextAlignment(2, Qt::AlignRight);

    return item;
}
SourceFileListItem* SourceFileListItem::create(QTreeWidgetItem *parent, const QString& folder)
{
    auto* item = new SourceFileListItem(parent, QStringList{
                                            folder,
                                            "", ""
                                        });
    return item;
}

bool SourceFileListItem::operator<(const QTreeWidgetItem &other) const
{
    // always sort directories first
    if((childCount() > 0) != (other.childCount() > 0)) {
        return childCount() > 0; // true if this is a folder and the other isn't
    }

    int sortCol = treeWidget()->sortColumn();
    if(sortCol == 0 || childCount() > 0) {
        // sort by name
        return text(0).compare(other.text(0), Qt::CaseInsensitive) < 0;
    }
    if(sortCol == 1) {
        // sort by date
        return data(1, Qt::UserRole+1).toDateTime() < other.data(1, Qt::UserRole+1).toDateTime();
    }
    if(sortCol == 2) {
        // sort by size
        return data(2, Qt::UserRole+1).toULongLong() < other.data(2, Qt::UserRole+1).toULongLong();
    }
    // unknown column
    return text(sortCol) < other.text(sortCol);
}
