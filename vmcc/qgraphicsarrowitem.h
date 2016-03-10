#ifndef QGRAPHICSARROWITEM_H
#define QGRAPHICSARROWITEM_H

#include <QGraphicsPolygonItem>
#include <QPainter>
#include <qmath.h>

class QGraphicsArrowItem : public QGraphicsPolygonItem
{
public:
    explicit QGraphicsArrowItem(QGraphicsItem *parent = 0):QGraphicsPolygonItem(parent){}
    void setEndPoints(qreal xStart, qreal yStart, qreal xEnd, qreal yEnd, qreal padding, qreal markerLength, qreal markerAngle){

        // Degree to Gradient
        markerAngle = markerAngle * 3.14 / 180.0;

        // Length of this line
        qreal length = sqrt((yEnd - yStart) * (yEnd - yStart) + (xEnd - xStart) * (xEnd - xStart));

        // Cut out from end points
        xStart += padding * (xEnd - xStart) / length;
        yStart += padding * (yEnd - yStart) / length;
        xEnd -= padding * (xEnd - xStart) / length;
        yEnd -= padding * (yEnd - yStart) / length;

        // Marker triangle base point
        qreal mBaseX = xEnd - markerLength * (xEnd - xStart) / length;
        qreal mBaseY = yEnd - markerLength * (yEnd - yStart) / length;

        // Triangle top point
        qreal mTopX = mBaseX + markerLength * (sin(markerAngle) / cos(markerAngle)) * (yEnd - yStart) / length;
        qreal mTopY = mBaseY - markerLength * (sin(markerAngle) / cos(markerAngle)) * (xEnd - xStart) / length;

        // Triangle bottom point
        qreal mBottomX = mBaseX + markerLength * (sin(-markerAngle) / cos(-markerAngle)) * (yEnd - yStart) / length;
        qreal mBottomY = mBaseY - markerLength * (sin(-markerAngle) / cos(-markerAngle)) * (xEnd - xStart) / length;

        QPolygonF polygon;
        polygon.append(QPointF(xStart, yStart));
        polygon.append(QPointF(xEnd, yEnd));
        polygon.append(QPointF(mTopX, mTopY));
        polygon.append(QPointF(mBottomX, mBottomY));
        polygon.append(QPointF(xEnd, yEnd));

        setPolygon(polygon);
    }
};

#endif // QGRAPHICSARROWITEM_H
