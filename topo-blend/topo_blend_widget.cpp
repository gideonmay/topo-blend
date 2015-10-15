#include "topo-blend.h"
#include "topo_blend_widget.h"
#include "ui_topo_blend_widget.h"
#include "ui_animationWidget.h"
#include "landmarks_dialog.h"
#include "StarlabDrawArea.h"

#include "QuickMesh.h"
#include "QuickViewer.h"
#include "QuickGroup.h"
#include <QFileDialog>
#include <QDir>
#include <QListWidgetItem>
#include <QMatrix4x4>

#include "topo-blend.h"
#include "GraphCorresponder.h"
#include "TopoBlender.h"
#include "Scheduler.h"
#include "SchedulerWidget.h"

#include "graphs-manager.h"
#include "correspondence-manager.h"
#include "SynthesisManager.h"

QuickViewer * viewer = NULL;
QString cur_filename;

topo_blend_widget::topo_blend_widget(topoblend * topo_blend, QWidget *parent) : QWidget(parent), ui(new Ui::topo_blend_widget)
{
    ui->setupUi(this);
	ui->groupBox1->setVisible(false);

    this->tb = topo_blend;
	
	// Save / load graphs
    topo_blend->g_manager->connect(ui->loadButton, SIGNAL(clicked()), SLOT(loadModel()));
    topo_blend->g_manager->connect(ui->saveButton, SIGNAL(clicked()), SLOT(saveModel()));
    topo_blend->g_manager->connect(ui->clearButton, SIGNAL(clicked()), SLOT(clearGraphs()));
    topo_blend->g_manager->connect(ui->linksButton, SIGNAL(clicked()), SLOT(modifyModel()));
    topo_blend->g_manager->connect(ui->alignButton, SIGNAL(clicked()), SLOT(quickAlign()));
    topo_blend->g_manager->connect(ui->normalizeButton, SIGNAL(clicked()), SLOT(normalizeAllGraphs()));

	// Save / load entire jobs
	this->connect(ui->loadJobButton, SIGNAL(clicked()), SLOT(loadJob()));
	this->connect(ui->saveJobButton, SIGNAL(clicked()), SLOT(saveJob()));

	// Animation widget
	//topo_blend->connect(ui->button4, SIGNAL(clicked()), SLOT(currentExperiment()));
	//this->connect(ui->animationButton, SIGNAL(clicked()), SLOT(renderViewer()));

	// Main blending process
    this->connect(ui->blendButton, SIGNAL(clicked()), SLOT(doBlend()), Qt::UniqueConnection);

	// Correspondence
	this->connect(ui->computeCorrButton, SIGNAL(clicked()), SLOT(loadCorrespondenceModel()));

	// Grouping
	this->connect(ui->groupButton, SIGNAL(clicked()), SLOT(showGroupingDialog()));

	// Synthesis
    this->connect(ui->genSynthButton, SIGNAL(clicked()), SLOT(generateSynthesisData()), Qt::UniqueConnection);

    topo_blend->s_manager->connect(ui->saveSynthButton, SIGNAL(clicked()), SLOT(saveSynthesisData()));
    topo_blend->s_manager->connect(ui->loadSynthButton, SIGNAL(clicked()), SLOT(loadSynthesisData()));
    topo_blend->s_manager->connect(ui->reconstructButton, SIGNAL(clicked()), SLOT(reconstructXYZ()));
	topo_blend->s_manager->connect(ui->outputCloudButton, SIGNAL(clicked()), SLOT(outputXYZ()));
	topo_blend->s_manager->connect(ui->clearSynthButton, SIGNAL(clicked()), SLOT(clear()));
	topo_blend->s_manager->connect(ui->synthesisSamplesCount, SIGNAL(valueChanged(int)), SLOT(setSampleCount(int)));

	// Model manipulation
	this->connect(ui->normalizeModel, SIGNAL(clicked()), SLOT(normalizeModel()));
	this->connect(ui->bottomCenterModel, SIGNAL(clicked()), SLOT(bottomCenterModel()));
	this->connect(ui->moveModel, SIGNAL(clicked()), SLOT(moveModel()));
	this->connect(ui->rotateModel, SIGNAL(clicked()), SLOT(rotateModel()));
	this->connect(ui->scaleModel, SIGNAL(clicked()), SLOT(scaleModel()));
	this->connect(ui->exportAsOBJ, SIGNAL(clicked()), SLOT(exportAsOBJ()));

	// Manipulate parts
	this->connect(ui->reverseCurveButton, SIGNAL(clicked()), SLOT(reverseCurve()));

	// Populate list
	this->updatePartsList();
	this->connect(ui->refreshViewButton, SIGNAL(clicked()), SLOT(updatePartsList()));
	this->connect(ui->partsList, SIGNAL(itemSelectionChanged()), SLOT(updateVisualization()));

	// Visualization & default values
	this->connect(ui->vizButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)),SLOT(vizButtonClicked(QAbstractButton*)));
	tb->viz_params["showNodes"] = true;
	tb->viz_params["showMeshes"] = true;
	tb->viz_params["showSamples"] = true;

	this->connect(ui->splatSize, SIGNAL(valueChanged(double)), SLOT(splatSizeChanged(double)));
}

QWidget * topo_blend_widget::simpleWidget()
{
	return ui->simpleWidget;
}

topo_blend_widget::~topo_blend_widget()
{
    delete ui;
}

void topo_blend_widget::doBlend()
{
    tb->doBlend();
}

void topo_blend_widget::generateSynthesisData()
{
	// Tell synthesis manager about active blend process
	tb->s_manager->gcorr = tb->gcoor;
	tb->s_manager->scheduler = tb->scheduler;
	tb->s_manager->blender = tb->blender;

	tb->s_manager->generateSynthesisData();
}

QString topo_blend_widget::loadJobFileName()
{
    QString job_filename = QFileDialog::getOpenFileName(0, tr("Load Job"), tb->mainWindow()->settings()->getString("lastUsedDirectory"), tr("Job Files (*.job)"));
    if(job_filename.isEmpty()) return "";

    // Keep folder active
    QFileInfo fileInfo(job_filename);
    tb->mainWindow()->settings()->set( "lastUsedDirectory", fileInfo.absolutePath() );

    return job_filename;
}

void topo_blend_widget::loadJob()
{
    this->loadJobFile( loadJobFileName() );
}

void topo_blend_widget::loadJobFile(QString job_filename)
{
	QFile job_file( job_filename );

	if (!job_file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
	QFileInfo jobFileInfo(job_file.fileName());
	QTextStream in(&job_file);

	// Job file content
	QString sgFileName, tgFileName, correspondenceFileName, scheduleFileName;
	int samplesCount;
	double gdResolution, timeStep;
	int reconLevel, renderCount;

	QString curPath = jobFileInfo.absolutePath() + "/";

	sgFileName = in.readLine();
	tgFileName = in.readLine();
	correspondenceFileName = in.readLine();
	scheduleFileName = in.readLine();

	in >> samplesCount;
	in >> gdResolution >> timeStep;
	in >> reconLevel >> renderCount;

	// Load graphs
	tb->graphs.push_back( new Structure::Graph ( curPath + sgFileName ) );
	tb->graphs.push_back( new Structure::Graph ( curPath + tgFileName ) );
	tb->setSceneBounds();
	tb->updateDrawArea();

	// Load correspondence
	tb->gcoor = tb->c_manager->makeCorresponder();
	tb->gcoor->loadCorrespondences( curPath + correspondenceFileName );
	tb->gcoor->isReady = true;

	// Create TopoBlender
	tb->doBlend();

	// Load schedule
	tb->scheduler->loadSchedule( curPath + scheduleFileName );

	// Set parameters
	ui->synthesisSamplesCount->setValue( samplesCount );
	tb->scheduler->widget->setParams( gdResolution, timeStep, reconLevel, renderCount );

	// Sample meshes
	//tb->generateSynthesisData();

	// Load samples if any
	tb->s_manager->gcorr = tb->gcoor;
	tb->s_manager->scheduler = tb->scheduler;
	tb->s_manager->blender = tb->blender;
    tb->s_manager->loadSynthesisData( curPath );
}

void topo_blend_widget::saveJob()
{
	topo_blend_widget::saveJob( tb->gcoor, tb->scheduler, tb->blender, tb->s_manager );
}

void topo_blend_widget::saveJob(GraphCorresponder * gcoor, Scheduler * scheduler, TopoBlender * blender, SynthesisManager * s_manager)
{
	if(!scheduler) return;

	QString sGraphName = gcoor->sgName();
	QString tGraphName = gcoor->tgName();
	QString graph_names = ( sGraphName + "_" + tGraphName ) + ".job";
	QString job_filename = QFileDialog::getSaveFileName(0, tr("Save Job"), tb->mainWindow()->settings()->getString("lastUsedDirectory") + "/" + graph_names, tr("Job Files (*.job)"));

	QFile job_file( job_filename );
	if (!job_file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
	QFileInfo jobFileInfo(job_file.fileName());
	QTextStream out(&job_file);

	// Create folders
	QDir jobDir( jobFileInfo.absolutePath() );

	QString sDir = jobDir.path() + "/Source/";
	QString tDir = jobDir.path() + "/Target/";
	jobDir.mkdir( sDir ); 
	jobDir.mkdir( tDir );

	// Save source and target graphs
	QString sRelative = "Source/" + sGraphName + ".xml";
	QString sgFileName = jobDir.path() + "/" + sRelative;
	blender->sg->saveToFile( sgFileName );

	QString tRelative = "Target/" + tGraphName + ".xml";
	QString tgFileName = jobDir.path() + "/"  + tRelative;
	blender->tg->saveToFile( tgFileName );

	// Save correspondence file
	QString correspondRelative = "correspondence.txt";
	QString correspondenceFileName = jobDir.path() + "/" + correspondRelative;
	gcoor->saveCorrespondences( correspondenceFileName, true );

	// Save the scheduler
	QString scheduleRelative = "schedule.txt";
	QString scheduleFileName = jobDir.path() + "/" + scheduleRelative;
	scheduler->saveSchedule( scheduleFileName );

	// Save paths & parameters
	out << sRelative << "\n";
	out << tRelative << "\n";
	out << correspondRelative << "\n";
	out << scheduleRelative << "\n";

	// Synthesis
	out << s_manager->samplesCount << "\t";

	// Discretization 
	out << scheduler->widget->gdResolution() << "\t" << scheduler->widget->timeStep() << "\n";

	// Reconstruction
	out << scheduler->widget->reconLevel() << "\t" << scheduler->widget->renderCount() << "\n";

	job_file.close();

	// Save samples
    s_manager->saveSynthesisData( jobDir.path() + "/" );
}

void topo_blend_widget::showGroupingDialog()
{
	if(tb->graphs.isEmpty()) return;

	QuickGroup groupingDialog(tb->graphs.front());
	tb->connect(&groupingDialog, SIGNAL(updateView()), SLOT(updateDrawArea()));
	groupingDialog.exec();
}

void topo_blend_widget::renderViewer()
{
	QDialog * d = new QDialog;
	d->show();

    Ui::AnimationForm aniForm;
    aniForm.setupUi(d);

    viewer = new QuickViewer();
    aniForm.mainLayout->addWidget(viewer);
    viewer->makeCurrent();

    this->connect(aniForm.button, SIGNAL(clicked()), SLOT(renderAnimation()));
    this->connect(aniForm.loadButton, SIGNAL(clicked()), SLOT(loadAnimationModel()));
}

void topo_blend_widget::renderAnimation()
{
	QStringList filters, files;
	filters << "*.obj" << "*.off";
	files = QDir(".").entryList(filters);

	int nframes = 0;

	foreach(QString filename, files)
	{
		QString prefix = QFileInfo(cur_filename).baseName();

		if(!filename.startsWith(prefix.mid(0,3))) continue;

		viewer->m->load(filename, false, false);
		viewer->updateGL();
		viewer->saveSnapshot(filename + ".png");

		nframes++;
	}

	// Generate GIF using ImageMagick
	system(qPrintable( QString("convert	-delay %1	-loop 0		seq*.png	animation.gif").arg( nframes / 20 ) ));

	viewer->setFocus();
}

void topo_blend_widget::loadAnimationModel()
{
    viewer->m = new QuickMesh;
    cur_filename = QFileDialog::getOpenFileName(this, tr("Open Mesh"), "", tr("Mesh Files (*.obj *.off)"));
    viewer->m->load(cur_filename, false, false);

	qglviewer::Vec a = qglviewer::Vec(viewer->m->bbmin.x(), viewer->m->bbmin.y(),viewer->m->bbmin.z());
	qglviewer::Vec b = qglviewer::Vec(viewer->m->bbmax.x(), viewer->m->bbmax.y(),viewer->m->bbmax.z());
	viewer->setSceneBoundingBox(a,b);
	viewer->showEntireScene();

	viewer->setFocus();
}

void topo_blend_widget::loadCorrespondenceModel()
{
	if(tb->graphs.size() != 2) {
		qDebug() << "Please load at least 2 graphs.";
		return;
	}

	LandmarksDialog dialog(tb);
	dialog.exec();
}

void topo_blend_widget::vizButtonClicked(QAbstractButton* b)
{
	Q_UNUSED(b)

	tb->viz_params["showNodes"] = ui->showNodes->isChecked();
	tb->viz_params["showEdges"] = ui->showEdges->isChecked();
	tb->viz_params["showMeshes"] = ui->showMeshes->isChecked();
	tb->viz_params["showTasks"] = ui->showTasks->isChecked();
	tb->viz_params["showCtrlPts"] = ui->showCtrlPts->isChecked();
	tb->viz_params["showCurveFrames"] = ui->showCurveFrames->isChecked();
	tb->viz_params["showSamples"] = ui->showSamples->isChecked();
	tb->viz_params["isSplatRenderer"] = ui->isSplatsHQ->isChecked();
	tb->viz_params["splatSize"] = ui->splatSize->value();
 
	tb->updateDrawArea();
}

void topo_blend_widget::splatSizeChanged(double newSize)
{
	tb->viz_params["splatSize"] = newSize;

	tb->updateDrawArea();
}

void topo_blend_widget::toggleCheckOption( QString optionName )
{
	foreach (QAbstractButton *pButton, ui->vizButtonGroup->buttons()){
		if(pButton->objectName() == optionName){
			QCheckBox * checkBox = (QCheckBox *)pButton;
			checkBox->setChecked( !checkBox->isChecked() );
			vizButtonClicked(pButton);
			return;
		}
	}
}

void topo_blend_widget::setCheckOption( QString optionName, bool toValue )
{
	foreach (QAbstractButton *pButton, ui->vizButtonGroup->buttons()){
		if(pButton->objectName() == optionName){
			((QCheckBox *)pButton)->setChecked(toValue);
			vizButtonClicked(pButton);
			return;
		}
	}
}

int topo_blend_widget::synthesisSamplesCount()
{
	return ui->synthesisSamplesCount->value();
}

void topo_blend_widget::normalizeModel()
{
	if(!tb->graphs.size()) return;

	tb->graphs.back()->normalize();

	tb->setSceneBounds();
	tb->updateDrawArea();
}

void topo_blend_widget::bottomCenterModel()
{
	if(!tb->graphs.size()) return;

	tb->graphs.back()->moveBottomCenterToOrigin();
	tb->setSceneBounds();
	tb->updateDrawArea();
}

void topo_blend_widget::moveModel()
{
	if(!tb->graphs.size()) return;

	double s = 0.1;

	QMatrix4x4 mat;
	mat.translate( ui->movX->value() * s, ui->movY->value() * s, ui->movZ->value() * s );
	tb->graphs.back()->transform( mat );
	tb->setSceneBounds();
	tb->updateDrawArea();
}

void topo_blend_widget::rotateModel()
{
	if(!tb->graphs.size()) return;

	QMatrix4x4 mat;
	mat.rotate( ui->rotX->value(), QVector3D(1,0,0) );
	mat.rotate( ui->rotY->value(), QVector3D(0,1,0) );
	mat.rotate( ui->rotZ->value(), QVector3D(0,0,1) );
	tb->graphs.back()->transform( mat );
	tb->setSceneBounds();
	tb->updateDrawArea();
}

void topo_blend_widget::scaleModel()
{
	if(!tb->graphs.size()) return;

	QMatrix4x4 mat;
	if( ui->isUniformScale->isChecked() )
	{
		double s = qMax(ui->scaleX->value(),qMax(ui->scaleY->value(),ui->scaleZ->value()));
		mat.scale( s );
	}
	else
	{
		mat.scale( ui->scaleX->value(), ui->scaleY->value(), ui->scaleZ->value() );
	}
	tb->graphs.back()->transform( mat );
	tb->setSceneBounds();
	tb->updateDrawArea();
}

void topo_blend_widget::exportAsOBJ()
{
	if(!tb->graphs.size()) return;

	Structure::Graph * g = tb->graphs.back();

	QString filename = QFileDialog::getSaveFileName(0, tr("Export OBJ"), 
		g->property["name"].toString().replace(".xml",".obj"), tr("OBJ Files (*.obj)"));
	
	if(filename.isEmpty()) return;

	g->exportAsOBJ( filename );
}

void topo_blend_widget::reverseCurve()
{
	if(!tb->graphs.size()) return;
	Structure::Graph * g = tb->graphs.back(); 

	foreach(QListWidgetItem * item, ui->partsList->selectedItems())
	{
		Structure::Node * n = g->getNode(item->text());
		if(n->type() == Structure::CURVE)
		{
			Structure::Curve * curve = (Structure::Curve*) n;

			// Flip the selected curve
			Array1D_Vector3 ctrlPoint = curve->controlPoints();
			Array1D_Real ctrlWeight = curve->controlWeights();
			std::reverse(ctrlPoint.begin(), ctrlPoint.end());
			std::reverse(ctrlWeight.begin(), ctrlWeight.end());

			NURBS::NURBSCurved newCurve(ctrlPoint, ctrlWeight);
			curve->curve = newCurve;

			// Update the coordinates of its links
			foreach( Structure::Link * l, g->getEdges(n->id) )
			{
				l->setCoord(n->id, inverseCoords( l->getCoord(n->id) ));
			}
		}

		if(n->type() == Structure::SHEET)
		{
			Structure::Sheet * sheet = (Structure::Sheet*) n;

			Array2D_Vector3 ctrlPoint = transpose<Vector3>(sheet->surface.mCtrlPoint);
			Array2D_Real ctrlWeight = transpose<Scalar>(sheet->surface.mCtrlWeight);

			std::reverse(ctrlPoint.begin(), ctrlPoint.end());
			std::reverse(ctrlWeight.begin(), ctrlWeight.end());

			int degree = 3;
			NURBS::NURBSRectangled newSurface(ctrlPoint, ctrlWeight, degree, degree, false, false, true, true); 
			sheet->surface = newSurface;

			// Update the coordinates of its links
			foreach( Structure::Link * l, g->getEdges(n->id) )
			{
				Array1D_Vector4d oldCoord = l->getCoord(n->id), newCoord;
				foreach(Vector4d c, oldCoord) newCoord.push_back(Vector4d(1- c[1], c[0], c[2], c[3]));
				l->setCoord(n->id, newCoord);
			}
		}
	}

	tb->updateDrawArea();
}

void topo_blend_widget::updatePartsList()
{
	if(!tb->graphs.size()) return;
	Structure::Graph * g = tb->graphs.back();

	// Populate list
	foreach(Structure::Node * n, g->nodes) 
		ui->partsList->addItem(new QListWidgetItem(n->id));
}

void topo_blend_widget::updateVisualization()
{
	if(!tb->graphs.size()) return;
	Structure::Graph * g = tb->graphs.back();

	g->setVisPropertyAll("glow", false);

	foreach(QListWidgetItem * item, ui->partsList->selectedItems())
	{
		Structure::Node * n = g->getNode(item->text());
		n->vis_property["glow"] = true;
	}

	tb->updateDrawArea();
}

void topo_blend_widget::setSynthSamplesCount( int samplesCount )
{
	ui->synthesisSamplesCount->setValue( samplesCount );
}

void topo_blend_widget::addTab( QWidget * widget )
{
	ui->tabWidget->insertTab(1, widget, widget->windowTitle());
}
