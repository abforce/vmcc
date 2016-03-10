#ifndef TIMELINEVISUALIZER_H
#define TIMELINEVISUALIZER_H

#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <qmath.h>
#include "qgraphicsarrowitem.h"
#include "siteinfo.h"

#define NODE_RADIUS 24
#define LINE_SPACE 50
#define MARGIN 5
#define LABEL_WIDTH 50
#define EVENT_WIDTH 30

class TimeLineVisualizer : public QObject
{
    Q_OBJECT

public:
    enum TransitionType {
        REQUEST, TOKEN
    };

    TimeLineVisualizer(QGraphicsView *view);
    void addSite(SiteInfo *site);
    void addTransitionEvent(int sender, int receiver, TransitionType type);
    void addRequestEvent(int sender, int receiver);
    void addTokenEvent(int sender, int receiver);
    void addCriticalSectionEvent(int site);

private:
    QGraphicsView *view;
    QGraphicsScene *scene;
    QList<QGraphicsLineItem *> *listLines;
    qreal front;

private slots:
    void onSceneRectChanged(const QRectF &rect);
};

#endif // TIMELINEVISUALIZER_H
