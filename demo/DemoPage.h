#pragma once
#include <QObject>

#include "DemoGlobal.h"
#include "Scene.h"

enum DemoPages{ SELECT_PAGE, MATCH_PAGE, CREATE_PAGE };

class DemoPage : public QObject{
    Q_OBJECT
public:
    DemoPage(Scene * scene, QString title = "") : s(scene)
    {
        visible = false;

        QFont titleFont("", 22);
        QGraphicsTextItem * titleItem = s->addText(title, titleFont);
        titleItem->setPos( (s->width()  * 0.50) - (titleItem->boundingRect().width() * 0.5), 15);
        titleItem->setDefaultTextColor(Qt::white);
        titleItem->setVisible(false);
        items.push_back(titleItem);

		this->connect(s, SIGNAL(keyUpEvent(QKeyEvent*)), SLOT(keyUp(QKeyEvent*)));
		s->connect(this, SIGNAL(update()), SLOT(update()));
    }

    PropertyMap property;
    bool isVisible() { return visible; }
	Scene * scene() { return s; }

protected:
    Scene * s;
    QVector<QGraphicsItem*> items;
    bool visible;

public slots:
    void hide()
    {
        this->visible = false;
        foreach(QGraphicsItem * item, items) item->hide();
		emit( becameHidden() );
    }

    void show()
    {
        this->visible = true;
        foreach(QGraphicsItem * item, items) item->show();
		emit( becameVisible() );
    }

	void keyUp(QKeyEvent* keyEvent){ emit( keyUpEvent(keyEvent) ); }

signals:
	void keyUpEvent(QKeyEvent*);
	void becameVisible();
	void becameHidden();
	void message(QString);
	void update();

	void showLogWindow();
};
