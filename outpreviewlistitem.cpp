#include "outpreviewlistitem.h"

bool OutPreviewListItem::operator<(const QTreeWidgetItem &other) const
{
    int sortCol = treeWidget()->sortColumn();
    if(sortCol == 0 || sortCol == 2) {
        // sort by slices
        return data(sortCol, Qt::UserRole).toInt() < other.data(sortCol, Qt::UserRole).toInt();
    }
    if(sortCol == 1) {
        // sort by size
        return data(1, Qt::UserRole).toULongLong() < other.data(1, Qt::UserRole).toULongLong();
    }
    // unknown column
    return text(sortCol) < other.text(sortCol);
}
