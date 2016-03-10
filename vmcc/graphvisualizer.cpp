#include "graphvisualizer.h"

GraphVisualizer::GraphVisualizer(QGraphicsView *view)
{
    listItems = new QList<QGraphicsEllipseItem *>();
    listLabels = new QList<QGraphicsTextItem *>();
    listEdges = new QList<QGraphicsPathItem *>();
    listSites = new QList<SiteInfo *>();
    scene = new QGraphicsScene(view);
    view->setScene(scene);
    view->setRenderHints(QPainter::Antialiasing);

    // Root node
    root = new QGraphicsEllipseItem();
    root->setRect(- NODE_RADIUS / 2, - VSPACE - NODE_RADIUS / 2, NODE_RADIUS, NODE_RADIUS);
    QPen pen(QColor::fromRgb(67, 0, 124));
    pen.setWidth(2);
    root->setPen(pen);
    root->setBrush(QColor::fromRgb(255, 114, 0));
    scene->addItem(root);

    // Root node label
    QGraphicsTextItem *rootLabel = new QGraphicsTextItem("VMCC");
    rootLabel->adjustSize();
    rootLabel->setPos(root->rect().x() + NODE_RADIUS / 2 - rootLabel->textWidth() / 2, root->rect().y() - LABEL_SPACE);
    scene->addItem(rootLabel);

    // Token pixmap
    token = new QGraphicsPixmapItem();
    QPixmap pixmap = QPixmap("token.png");
    token->setPixmap(pixmap);
    token->setZValue(1);
    token->setVisible(false);
    scene->addItem(token);

    // Scene background
    scene->setBackgroundBrush(QBrush(QColor::fromRgb(155, 209, 255)));
}

void GraphVisualizer::addSite(SiteInfo *site)
{
    // Node
    QGraphicsEllipseItem *item = new QGraphicsEllipseItem();
    item->setBrush(QBrush(Qt::green));
    QPen pen(Qt::blue);
    pen.setWidth(2);
    item->setPen(pen);
    scene->addItem(item);

    // Edge
    QGraphicsPathItem *edge = new QGraphicsPathItem();
    QPen pen2(Qt::red);
    pen2.setWidth(2);
    edge->setPen(pen2);
    edge->setZValue(-1);
    scene->addItem(edge);

    // Label
    QGraphicsTextItem *label = new QGraphicsTextItem(site->name);
    label->adjustSize();
    scene->addItem(label);

    listSites->append(site);
    listItems->append(item);
    listEdges->append(edge);
    listLabels->append(label);
    invalidateScene();
}

void GraphVisualizer::invalidateScene()
{
    int count = listItems->size();
    for(int i = 0; i < count; i += 1){
        // Node
        qreal diff = (i - (count - 1) / 2.0);
        qreal x = diff * HSPACE - NODE_RADIUS / 2;
        qreal y = - NODE_RADIUS / 2;
        listItems->at(i)->setRect(x, y, NODE_RADIUS, NODE_RADIUS);

        // Edge
        QPainterPath path;
        path.moveTo(0, -VSPACE + NODE_RADIUS / 2);
        path.cubicTo(0, -VSPACE / 2, diff * HSPACE, -VSPACE / 2, diff * HSPACE, y);
        listEdges->at(i)->setPath(path);

        // Label
        QGraphicsTextItem *label = listLabels->at(i);
        label->setPos(x + NODE_RADIUS / 2 - label->textWidth() / 2, y + LABEL_SPACE / 1.2);

        // Token
        if(listSites->at(i)->holder){
            token->setVisible(true);
            token->setPos(x + 5, y + 4);
        }
    }

    scene->invalidate();
}
