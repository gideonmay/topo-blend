#pragma once

#include <QGraphicsObject>
#include <QMap>

#include "StructureGraph.h"

class BlendRenderItem : public QGraphicsObject {
	Q_OBJECT
public:
    BlendRenderItem(QPixmap pixmap);

	QPixmap pixmap;
	QMap<QString, QVariant> property;

    QRectF boundingRect() const { return pixmap.rect(); }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	bool isOnTop();

	Structure::Graph * graph();

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event);

};
