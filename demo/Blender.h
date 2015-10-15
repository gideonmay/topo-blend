#pragma once
#include <QElapsedTimer>
#include "DemoPage.h"
#include "GraphCorresponder.h"
#include "TopoBlender.h"
#include "Scheduler.h"

// Forward Declare
class ProgressItem; class SynthesisManager;
class BlendPathRenderer; class BlendRenderItem; class BlendPathSubButton;
class BlendPathWidget;
class PathEvaluator;

// Blend path container
struct BlendPath{
	Structure::Graph * source;
	Structure::Graph * target;
	GraphCorresponder * gcorr;
	QSharedPointer<Scheduler> scheduler;
	QSharedPointer<TopoBlender> blender;
};

class Blender : public DemoPage
{
    Q_OBJECT
public:
    explicit Blender(Scene * scene, QString title);
    void setupBlendPathItems();

	int numSuggestions, numInBetweens, numSchedules;
	bool isFiltering;
	bool isUniformPath;

	int itemHeight;
	int graphItemWidth;
	int blendPathWidth, blendPathHeight;

	int resultsPage;
	QVector<ScheduleType> allSchedules;

    QGraphicsProxyWidget *prevButton, *nextButton;

	QSharedPointer<Scheduler> m_scheduler;
	QSharedPointer<TopoBlender> m_blender;
	QVector< BlendPath > blendPaths;
	QList< QSharedPointer<Scheduler> > jobs;

	QSharedPointer<SynthesisManager> s_manager;

	friend class Session;
	friend class BlendPathRenderer;
	friend class BlendPathSubButton;
	friend class PathEvaluator;

signals:
    void blendPathsReady();
    void blendPathsDone();
	void blendStarted();
	void blendDone();
    void blendFinished();

	void exportShape(QString name, PropertyMap data);

public slots:
    void show();
    void hide();
	void keyReleased(QKeyEvent* keyEvent);
	void mousePress(QGraphicsSceneMouseEvent* mouseEvent);
	void cleanUp();
	void clearResults();

	void setGraphCorresponder(GraphCorresponder *);
	void preparePaths();
	void schedulePaths( const QSharedPointer<Scheduler> & scheduler, const QSharedPointer<TopoBlender> & blender );
	void pathProgressChanged();
	void synthDataReady();
	void computeBlendPaths();
	void computeBlendPathsThread();

	void blenderDone();
	void blendResultDone(QGraphicsItem* item);
	void blenderAllResultsDone();

	void showPrevResults();
	void showNextResults();
	void showResultsPage();

	QVector<BlendRenderItem *> selectedInBetween();
	void clearSelectedInBetween();
	void exportSelected();
	void saveJob();
	QWidget * viewer();

	void previewItem(BlendRenderItem*);

	void filterStateChanged(int state);

	void emitMessage(QString);

private:
	QVector< QGraphicsProxyWidget* > blendPathsWidgets;
	QVector< QVector< QSharedPointer<BlendRenderItem> > > resultItems;
	QVector< QVector< QSharedPointer<BlendPathSubButton> > > blendSubItems;
	QVector< QGraphicsItem* > auxItems;
	
	PathEvaluator * pathsEval;

	bool isSample;
	bool isFinished;

    void computePath(const int &index);

	void addBlendSubItem(double x, double y, double w, double h, int i, int j);

	GraphCorresponder * m_gcorr;
	BlendPathRenderer * renderer;
	BlendPathRenderer * resultViewer;
	ProgressItem * progress;

	QElapsedTimer pathsTimer;
	QElapsedTimer synthTimer;
	QElapsedTimer blendTimer;
	QElapsedTimer renderTimer;
};
