#ifndef GRAPHVISUALIZER_H
#define GRAPHVISUALIZER_H

#include <QList>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>

#include "siteinfo.h"

#define NODE_RADIUS 24
#define VSPACE 75
#define HSPACE 80
#define LABEL_SPACE 23

class GraphVisualizer
{
public:
    GraphVisualizer(QGraphicsView *view);
    void addSite(SiteInfo *site);
    void invalidateScene();

private:
    QGraphicsEllipseItem *root;
    QGraphicsPixmapItem *token;
    QList<QGraphicsEllipseItem *> *listItems;
    QList<QGraphicsPathItem *> *listEdges;
    QList<QGraphicsTextItem *> *listLabels;
    QList<SiteInfo *> *listSites;
    QGraphicsScene *scene;
};

#endif // GRAPHVISUALIZER_H
