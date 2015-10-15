#ifndef LANDMARKS_DIALOG_H
#define LANDMARKS_DIALOG_H

#include <QDialog>
#include "topo-blend.h"

namespace Ui {
class LandmarksDialog;
}

class LandmarksDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit LandmarksDialog(topoblend *topo_blender, QWidget *parent = 0);
    ~LandmarksDialog();

private:
	topoblend *tb;
	GraphCorresponder *gcorr;
	Ui::LandmarksDialog *ui;

public:
	Structure::Graph * sg();
	Structure::Graph * tg();
	bool sIsLandmark(int i);
	bool tIsLandmark(int i);
	Structure::Node* sNode(int i);
	Structure::Node* tNode(int i);

	void updatePointLandmarksList();

	void updateSList();
	void updateTList();
	void updateLandmarksTableWidget();
	void updateLandmarksTab();

	void updateCorrTableWidget();	
	void updateCorrTab();

	void updateAlignmentTab();

public slots:
	// Reload everything
	void reload();

	void updateAll();

	// Point Landmarks
	void addPointLandmark();
	void removePointLandmark();
	void visualizeCurrentPointLandmark();
	void visualizeAllPointLandmarks();

	// Part Landmarks
	void sClearSelections();
	void tClearSelections();
	void visualizeSelections();
	void visualizeLandmark();

	void addLandmark();
	void removeLandmark();

	void saveLandmarks();
	void loadLandmarks();
	void loadCorrespondenceFile();

	// Alignment
	void showAABB(int state);
	void normalize();
	void rotateGraph();
	void scaleGraph(int slider);
	void basicAlign();

	/// Correspondences
	void prepareMatrices();

	void setSpatialWeight(double w);
	void setStructuralWeight(double w);
	void setSizeWeight(double w);
	void setOrientationWeight(double w);
	void computeDisM();

	void setScoreThreshold(double tau);
	void computePartToPartCorrespondences();

	void alignAllNodes();
	void computeCorrespondences();
	void visualizeCorr(int row, int column);

	void visualizePart2PartDistance(int id);
};

#endif // LANDMARKS_DIALOG_H
