#include "timelinevisualizer.h"

TimeLineVisualizer::TimeLineVisualizer(QGraphicsView *view) : QObject(view)
{
    this->view = view;
    scene = new QGraphicsScene(view);
    view->setScene(scene);
    view->setRenderHints(QPainter::Antialiasing);

    listLines = new QList<QGraphicsLineItem *>();

    // Scene background
    scene->setBackgroundBrush(QBrush(QColor::fromRgb(155, 209, 255)));

    front = LABEL_WIDTH + NODE_RADIUS + MARGIN * 5;

    connect(scene, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(onSceneRectChanged(QRectF)));
}

void TimeLineVisualizer::addSite(SiteInfo *site)
{
    int lineIndex = listLines->count();

    qreal x = MARGIN;
    qreal y = lineIndex * LINE_SPACE;

    // Label
    QGraphicsTextItem *label = new QGraphicsTextItem(site->name);
    label->setPos(x, y - 15);
    scene->addItem(label);

    // Node
    QGraphicsEllipseItem *node = new QGraphicsEllipseItem();
    node->setRect(x + LABEL_WIDTH, y - NODE_RADIUS / 2, NODE_RADIUS, NODE_RADIUS);
    node->setBrush(QBrush(Qt::green));
    QPen pen(Qt::blue);
    pen.setWidth(2);
    node->setPen(pen);
    scene->addItem(node);

    // Line
    QGraphicsLineItem *line = new QGraphicsLineItem();
    QPen penLine(QBrush(Qt::gray), 1, Qt::DashLine);
    line->setPen(penLine);
    line->setLine(x + LABEL_WIDTH + NODE_RADIUS + MARGIN, y, x + 1300, y);
    scene->addItem(line);

    listLines->append(line);
}

void TimeLineVisualizer::addTransitionEvent(int sender, int receiver, TimeLineVisualizer::TransitionType type)
{
    // Node Sender
    QGraphicsEllipseItem *nodeSender = new QGraphicsEllipseItem();
    nodeSender->setRect(front - NODE_RADIUS / 4, sender * LINE_SPACE - NODE_RADIUS / 4, NODE_RADIUS / 2, NODE_RADIUS / 2);
    nodeSender->setBrush(QBrush(Qt::yellow));
    QPen pen(Qt::magenta);
    pen.setWidth(1);
    nodeSender->setPen(pen);
    scene->addItem(nodeSender);

    front += EVENT_WIDTH;

    // Node Receiver
    QGraphicsEllipseItem *nodeReceievr = new QGraphicsEllipseItem();
    nodeReceievr->setRect(front - NODE_RADIUS / 4, receiver * LINE_SPACE - NODE_RADIUS / 4, NODE_RADIUS / 2, NODE_RADIUS / 2);
    nodeReceievr->setBrush(QBrush(Qt::yellow));
    nodeReceievr->setPen(pen);
    scene->addItem(nodeReceievr);

    front += EVENT_WIDTH / 2;

    // Arrow
    QGraphicsArrowItem *arrow = new QGraphicsArrowItem();
    if(type == REQUEST){
        arrow->setBrush(QBrush(QColor::fromRgb(5, 39, 150)));
        QPen penArrow(QColor::fromRgb(5, 39, 150));
        penArrow.setWidth(1);
        arrow->setPen(penArrow);
    } else {
        arrow->setBrush(QBrush(QColor::fromRgb(180, 25, 04)));
        QPen penArrow(QColor::fromRgb(180, 25, 04));
        penArrow.setWidth(1);
        arrow->setPen(penArrow);
    }
    qreal xStart = front - 3 * EVENT_WIDTH / 2;
    qreal yStart = sender * LINE_SPACE;
    qreal xEnd = front - EVENT_WIDTH / 2;
    qreal yEnd = receiver * LINE_SPACE;

    arrow->setEndPoints(xStart, yStart, xEnd, yEnd, NODE_RADIUS / 4, 10, 20);
    scene->addItem(arrow);

    // Length of this line
    qreal length = sqrt((yEnd - yStart) * (yEnd - yStart) + (xEnd - xStart) * (xEnd - xStart));

    // Middle point
    qreal midX = (xStart + xEnd) / 2;
    qreal midY = (yStart + yEnd) / 2;

    // Label
    QString labelText = type == REQUEST ? "Request" : "Token";
    QGraphicsTextItem *label = new QGraphicsTextItem(labelText);
    label->setScale(0.7);
    label->adjustSize();
    midX -= 3 * label->textWidth() / 4 * (midX - xStart) / length;
    midY -= 3 * label->textWidth() / 4 * (midY - yStart) / length;
    label->setPos(midX, midY);
    qreal angle = atan((yEnd - yStart ) / (xEnd - xStart));
    label->setRotation(angle * 180 / 3.14);
    scene->addItem(label);

    view->ensureVisible(nodeReceievr);
}

void TimeLineVisualizer::addCriticalSectionEvent(int site)
{
    // Node
    QGraphicsEllipseItem *node = new QGraphicsEllipseItem();
    node->setRect(front - NODE_RADIUS / 4, site * LINE_SPACE - NODE_RADIUS / 4, NODE_RADIUS / 2, NODE_RADIUS / 2);
    node->setBrush(QBrush(QColor::fromRgb(249, 129, 129)));
    QPen pen(Qt::magenta);
    pen.setWidth(1);
    node->setPen(pen);
    scene->addItem(node);

    // Label
    QGraphicsTextItem *label = new QGraphicsTextItem("CS");
    label->setScale(0.7);
    label->adjustSize();
    qreal x = front - label->textWidth() / 2 + MARGIN / 2 + 2;
    qreal y = site * LINE_SPACE - NODE_RADIUS / 4 - 3 * MARGIN - 2;
    label->setPos(x, y);
    scene->addItem(label);

    front += EVENT_WIDTH / 2;

    view->ensureVisible(node);
}

void TimeLineVisualizer::onSceneRectChanged(const QRectF &rect)
{
    foreach(QGraphicsLineItem *line, *listLines){
        qreal xStart = line->line().x1();
        qreal y = line->line().y1();
        qreal xEnd = rect.width();

        line->setLine(xStart, y, xEnd, y);
    }
    scene->invalidate();
}
