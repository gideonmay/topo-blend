#pragma once
#include <QStack>
#include <qglviewer/camera.h>
#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QPropertyAnimation>

#include "StructureGraph.h"
#include "DemoGlobal.h"

Q_DECLARE_METATYPE(QVector< Vector3 >)
Q_DECLARE_METATYPE(QVector< QVector<Vector3> >)

class GraphItem : public QGraphicsObject{
    Q_OBJECT
    Q_PROPERTY(QRectF geometry READ boundingRect WRITE setGeometry NOTIFY geometryChanged)
public:
	GraphItem(Structure::Graph *graph, QRectF region, qglviewer::Camera * camera);
	~GraphItem();

    Structure::Graph* g;
	QString name;
	SphereSoup marker;

    void setGeometry(QRectF newGeometry);
    QRectF boundingRect() const { return m_geometry; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QRectF setCamera();

    void draw3D();
	void pick(int x, int y, int pickMode);

    QRectF popState();
    void pushState();
    QPropertyAnimation * animateTo(QRectF newRect, int duration = 200);

	struct HitResult{
		GraphItem * item;
		QString nodeID;
		Vector3 ipoint;
		QPoint mousePos;
		int faceIDX;
		int hitMode;
		HitResult(GraphItem * item, QString nodeID, Vector3 ipoint, QPoint mousePos, int faceIDX, int hitMode) :
		 item(item), nodeID(nodeID), ipoint(ipoint), mousePos(mousePos), faceIDX(faceIDX), hitMode(hitMode) {}
	};

private:
    QRectF m_geometry;
    QStack<QRectF> states;
	qglviewer::Camera * camera;

	// DEBUG:
	PointSoup ps1, ps2;
	VectorSoup vs1, vs2;
	SphereSoup ss1, ss2;

public slots:
    void geometryHasChanged();

signals:
    void geometryChanged();
	void pickResult( PropertyMap );
	void hit( GraphItem::HitResult );
	void miss();
};
