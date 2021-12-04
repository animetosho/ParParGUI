#ifndef OUTPREVIEWLISTITEM_H
#define OUTPREVIEWLISTITEM_H

#include <QTreeWidgetItem>

class OutPreviewListItem : public QTreeWidgetItem
{
public:
    OutPreviewListItem(QTreeWidgetItem *tree) : QTreeWidgetItem(tree) {}
    OutPreviewListItem(QTreeWidgetItem *parent, const QStringList & strings) : QTreeWidgetItem(parent, strings) {}

    bool operator< (const QTreeWidgetItem &other) const override;
};

#endif // OUTPREVIEWLISTITEM_H
