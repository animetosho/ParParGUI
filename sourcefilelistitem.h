#ifndef SOURCEFILELISTITEM_H
#define SOURCEFILELISTITEM_H

#include <QTreeWidgetItem>
#include "sourcefile.h"

class SourceFileListItem : public QTreeWidgetItem
{
    SourceFileListItem(QTreeWidgetItem *tree) : QTreeWidgetItem(tree) {}
    SourceFileListItem(QTreeWidgetItem *parent, const QStringList & strings) : QTreeWidgetItem(parent, strings) {}
public:
    static SourceFileListItem* create(QTreeWidgetItem *parent, const SourceFile &file);
    static SourceFileListItem* create(QTreeWidgetItem *parent, const QString &folder);

    bool operator< (const QTreeWidgetItem &other) const override;
};

#endif // SOURCEFILELISTITEM_H
