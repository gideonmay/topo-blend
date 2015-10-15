#include <QFileDialog>
#include <QStack>

#include "nurbs_plugin_global.h"

#include "nurbs_plugin.h"
#include "NanoKdTree.h"
#include "StructureGraph.h"

#include "BoundaryFitting.h"

#include "interfaces/ModePluginDockWidget.h"

QVector<QColor> randColors;

//#include "LSCM.h"
//#include "LinABF.h"

QString groupID = "";
SurfaceMesh::SurfaceMeshModel * m = NULL;
RichParameterSet * mcf_params = NULL;

PointSoup pps,pps2;
SphereSoup spheres;
VectorSoup vs;
LineSegments lseg;

#include "PCA.h"
#include "SimilarSampling.h"
#include "GenericGraph.h"
#include "StructureGraph.h"

#define RADIANS(deg)    ((deg)/180.0 * M_PI)

void nurbs_plugin::create()
{
	if(widget) return;

	if(mesh()->name == "empty"){
		document()->deleteModel(document()->getModel("empty"));
	}	

	drawArea()->setRenderer( mesh(), "Transparent");
	drawArea()->camera()->setType(qglviewer::Camera::PERSPECTIVE);

	entireMesh = (SurfaceMeshModel*)document()->selectedModel();
	entirePoints = entireMesh->vertex_property<Vector3>("v:point");

	for(int i = 0; i < 10; i++)
		randColors.push_back(qRandomColor());

	loadGroupsFromOBJ();

	m = mesh();
	points = m->vertex_property<Vector3>("v:point");

	graph = new Structure::Graph;

	ModePluginDockWidget * dockwidget = new ModePluginDockWidget("NURBS plugin", mainWindow());
	widget = new NURBSTools(this);
	dockwidget->setWidget(widget);
	dockwidget->setWindowTitle(widget->windowTitle());
	mainWindow()->addDockWidget(Qt::RightDockWidgetArea,dockwidget);
	widget->fillList();

	mainWindow()->showMaximized();

	//buildSamples();
}

void nurbs_plugin::decorate()
{
	// Draw OBB
	//mesh_obb.draw();

	for(int i = 0; i < (int)curves.size(); i++)
	{
		NURBS::CurveDraw::draw( &curves[i], randColors[i%randColors.size()], true );
	}

	for(int i = 0; i < (int)rects.size(); i++)
	{
		NURBS::SurfaceDraw::draw( &rects[i], randColors[i%randColors.size()], true );
	}

	if(graph) graph->draw( this->drawArea() );

	ps.draw();
	vs.draw();
	lseg.draw();
	pps.draw();
	pps2.draw();

	spheres.draw();
}

void nurbs_plugin::clearAll()
{
	curves.clear();
	rects.clear();
}

void nurbs_plugin::saveAll()
{
	foreach(Structure::Node * n, graph->nodes)
	{
		QString nodeID = n->id;
		SurfaceMeshModel * nodeMesh = extractMesh( nodeID );

		// Assign sub-mesh to node
		n->property["mesh_filename"] = entireMesh->name + "/" + nodeID + ".obj";
		n->property["mesh"].setValue( QSharedPointer<SurfaceMeshModel>(nodeMesh) );
	}

	QString folderPath = QFileDialog::getExistingDirectory();
	QDir::setCurrent( folderPath );
	
	QString filename = folderPath + "/" + entireMesh->name + ".xml";
	graph->saveToFile( filename );
}

bool nurbs_plugin::keyPressEvent( QKeyEvent* event )
{
	bool used = false;

	if(event->key() == Qt::Key_E)
	{
		NURBS::NURBSCurved & c = curves.back();

		NURBS::NURBSCurved newCurve = NURBS::NURBSCurved::createCurveFromPoints( c.simpleRefine(1) );

		if(curves.size() == 1)
			curves.push_back( newCurve );
		else
			c = newCurve;

		used = true;
	}

	if(event->key() == Qt::Key_R)
	{
		NURBS::NURBSCurved & c = curves.back();

		c = NURBS::NURBSCurved::createCurveFromPoints( c.removeKnots(2) ) ;

		used = true;
	}

	if(event->key() == Qt::Key_Q)
	{
		NURBS::NURBSRectangled & r = rects.back();

		r = NURBS::NURBSRectangled::createSheetFromPoints( r.midPointRefined() );

		used = true;
	}

	if(event->key() == Qt::Key_W)
	{
		//QElapsedTimer timer; timer.start();

		//LinABF linabf(m);
		//mainWindow()->setStatusBarMessage(QString("LinABF time = %1 ms").arg(timer.elapsed()),512);

		//linabf.applyUVTom;

		//used = true;
	}

	if(event->key() == Qt::Key_Z)
	{
		NURBS::NURBSRectangled & r = rects.back();

		r = NURBS::NURBSRectangled::createSheetFromPoints( r.simpleRefine(1,0) );

		used = true;
	}

	if(event->key() == Qt::Key_X)
	{
		NURBS::NURBSRectangled & r = rects.back();

		r = NURBS::NURBSRectangled::createSheetFromPoints( r.simpleRefine(1,1) );

		used = true;
	}

	if(event->key() == Qt::Key_C)
	{
		NURBS::NURBSRectangled & r = rects.back();

		r = NURBS::NURBSRectangled::createSheetFromPoints( r.simpleRemove(r.mNumUCtrlPoints * 0.5,0) );

		used = true;
	}

	if(event->key() == Qt::Key_V)
	{
		NURBS::NURBSRectangled & r = rects.back();

		r = NURBS::NURBSRectangled::createSheetFromPoints( r.simpleRemove(r.mNumVCtrlPoints * 0.5,1) );

		used = true;
	}

	if(event->key() == Qt::Key_Space)
	{
		buildSamples();
		used = true;
	}

	drawArea()->updateGL();

	return used;
}

void nurbs_plugin::buildSamples()
{
	// Curve example
	int degree = 3;
	std::vector<Vector3d> cps;
	int steps = 10;
	double theta = M_PI * 5 / steps;
	double r = M_PI;

	for(int i = 0; i <= steps; i++){
		double x = (double(i) / steps) * r;
		double y = sin(i * theta) * r * 0.25;

		cps.push_back(Vector3d(x - r * 0.5, y, cos(i*theta)));
	}

	std::vector<Scalar> weight(cps.size(), 1.0);

	curves.push_back(NURBS::NURBSCurved(cps, weight, degree, false, true));

	// Rectangular surface
	double w = 1;
	double l = 2;

	int width = w * 5;
	int length = l * 5;

	std::vector< std::vector<Vector3d> > cpts( width, std::vector<Vector3d>(length) );
	std::vector< std::vector<Scalar> > weights( width, std::vector<Scalar>(length) );

	double omega = M_PI * 3 / qMin(width, length);

	Vector3d surface_pos(0,1,0);

	for(int i = 0; i < width; i++)
		for(int j = 0; j < length; j++){
			double x = double(i) / width;
			double y = double(j) / length;

			double delta = sqrt(x*x + y*y) * 0.5;

			cpts[i][j] = Vector3d(surface_pos.x() + x * w, surface_pos.y() + y * l, delta + sin(j * omega) * qMin(width, length) * 0.02);
		}

	rects.push_back( NURBS::NURBSRectangled(cpts, weights, degree, degree, false, false, true, true) );
}

void nurbs_plugin::prepareSkeletonize()
{
	// Remove visualization
	ps.clear();

	m->update_face_normals();
	m->update_vertex_normals();
	m->updateBoundingBox();

	drawArea()->setRenderer( m, "Wireframe");
	document()->setSelectedModel( m );

	// Voxelize
	if( widget->isVoxelize() )
	{
		FilterPlugin * voxResamplePlugin = pluginManager()->getFilter("Voxel Resampler");
		RichParameterSet * resample_params = new RichParameterSet;
		voxResamplePlugin->initParameters( resample_params );
		resample_params->setValue("voxel_scale", float( widget->voxelParamter() ));
		voxResamplePlugin->applyFilter( resample_params );
	}

	// Remesh
	if( widget->isRemesh() )
	{
		FilterPlugin * remeshPlugin = pluginManager()->getFilter("Isotropic Remesher");
		RichParameterSet * remesh_params = new RichParameterSet;
		remeshPlugin->initParameters( remesh_params );
		float edge_threshold = float(widget->remeshParamter() * m->bbox().diagonal().norm());

		if(widget->isProtectSmallFeatures()){
			remesh_params->setValue("keep_shortedges", true);
			edge_threshold *= 0.5;
		}

		remesh_params->setValue("edgelength_TH", edge_threshold);

		remeshPlugin->applyFilter( remesh_params );
	}

	// Compute MAT
	FilterPlugin * matPlugin = pluginManager()->getFilter("Voronoi based MAT");
	RichParameterSet * mat_params = new RichParameterSet;
	matPlugin->initParameters( mat_params );
	matPlugin->applyFilter( mat_params );
}

void nurbs_plugin::stepSkeletonizeMesh()
{	
	FilterPlugin * mcfPlugin = pluginManager()->getFilter("MCF Skeletonization");

	if(!mcf_params){
		mcf_params = new RichParameterSet;
		mcfPlugin->initParameters( mcf_params );

		// Custom MCF parameters
		if( !widget->isUseMedial() ) 
			mcf_params->setValue("omega_P_0", 0.0f);
	}
	mcfPlugin->applyFilter( mcf_params );
	 
	//drawArea()->setRenderer(m,"Flat Wire");
	//drawArea()->updateGL(); // may cause crashing of OpenGL
}

void nurbs_plugin::drawWithNames()
{
	// Faces
	foreach( const Face f, entireMesh->faces() )
	{
		// Collect points
		QVector<Vector3> pnts; 
		Surface_mesh::Vertex_around_face_circulator vit = entireMesh->vertices(f),vend=vit;
		do{ pnts.push_back(entirePoints[vit]); } while(++vit != vend);

		glPushName(f.idx());
		glBegin(GL_TRIANGLES);
		foreach(Vector3 p, pnts) glVector3(p);
		glEnd();
		glPopName();
	}
}


SurfaceMeshModel * nurbs_plugin::extractMesh( QString gid )
{
	SurfaceMeshModel * subMesh = NULL;

	QVector<int> part = groupFaces[gid];

	// Create copy of sub-part
	subMesh = new SurfaceMeshModel(groupID + ".obj", groupID);
	QSet<int> vertSet;
	QMap<Vertex,Vertex> vmap;
	foreach(int fidx, part){
		Surface_mesh::Vertex_around_face_circulator vit = entireMesh->vertices(Face(fidx)),vend=vit;
		do{ vertSet.insert(Vertex(vit).idx()); } while(++vit != vend);
	}
	foreach(int vidx, vertSet){
		vmap[Vertex(vidx)] = Vertex(vmap.size());
		subMesh->add_vertex( entirePoints[Vertex(vidx)] );
	}
	foreach(int fidx, part){
		std::vector<Vertex> pnts; 
		Surface_mesh::Vertex_around_face_circulator vit = entireMesh->vertices(Face(fidx)),vend=vit;
		do{ pnts.push_back(vmap[vit]); } while(++vit != vend);
		subMesh->add_face(pnts);
	}
	subMesh->updateBoundingBox();
	subMesh->isVisible = true;

	return subMesh;
}

bool nurbs_plugin::postSelection( const QPoint& point )
{
	Q_UNUSED(point);
	int selectedID = drawArea()->selectedName();
	if (selectedID == -1){
		ps.clear();
		document()->setSelectedModel(entireMesh);
        return false;
	}
	qDebug() << "Selected ID is " << selectedID;

	Vertex selectedVertex = entireMesh->vertices( Surface_mesh::Face(selectedID) );

	QString gid = faceGroup[entireMesh->face(entireMesh->halfedge(selectedVertex)).idx()];
	selectGroup(gid);

    return true;
}

void nurbs_plugin::selectGroup(QString gid)
{
	groupID = gid;
	m = extractMesh( groupID );
	QVector<int> part = groupFaces[gid];

	// Draw selection
	ps.clear();
	foreach(int fidx, part)
	{
		// Collect points
		QVector<QVector3> pnts; 
		Surface_mesh::Vertex_around_face_circulator vit = entireMesh->vertices(Face(fidx)),vend=vit;
		do{ pnts.push_back(entirePoints[vit]); } while(++vit != vend);

		ps.addPoly(pnts, QColor(255,0,0,100));
	}
}

void nurbs_plugin::loadGroupsFromOBJ()
{
	// Read obj file
	QFile file(mesh()->path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
	QTextStream inF(&file);
	int fidx = 0;
	while( !inF.atEnd() ){
		QString line = inF.readLine();
		if(!line.size()) continue;
		if((line.at(0).unicode() & 0xFF) == 'g'){
			QStringList groupLine = line.split(" ");
			QString gid = QString::number(groupFaces.size());
			if(groupLine.size()) gid = groupLine.at(1);
			while(true){
				QString line = inF.readLine();
				if(!line.startsWith("f")) break;
				groupFaces[gid].push_back(fidx);
				faceGroup[fidx] = gid;
				fidx++;
			}
		}
	}
}

Vector3d nurbs_plugin::pole( Vector3d center, double radius, SurfaceMeshModel * part )
{
	Vector3VertexProperty partPoints = part->vertex_property<Vector3>("v:point");

	std::vector<Vector3d> samples;
	QVector<Face> ifaces;
	Vector3d p;

	// intersect sphere with part, collect faces
	foreach( Face f, part->faces() ){
		QVector<Vector3d> pnts; 
		Surface_mesh::Vertex_around_face_circulator vit = part->vertices(Face(f)),vend=vit;
		do{ pnts.push_back(partPoints[vit]); } while(++vit != vend);

		if(TestSphereTriangle(center, radius, pnts[0], pnts[1], pnts[2], p)){
			ifaces.push_back(f);
			foreach(Vector3d s, sampleEdgeTri(pnts[0], pnts[1], pnts[2]))
				samples.push_back(s);
		}
	}

	Vector3d a,b,c;
	return PCA::mainAxis(samples,a,b,c).normalized();
}

std::vector<Vector3d> nurbs_plugin::sampleEdgeTri( Vector3d a, Vector3d b, Vector3d c )
{
	std::vector<Vector3d> samples;
	samples.push_back(a);samples.push_back(b);samples.push_back(c);
	samples.push_back((a+b) * 0.5);
	samples.push_back((b+c) * 0.5);
	samples.push_back((c+a) * 0.5);
	return samples;
}

double nurbs_plugin::minAngle(Face f, SurfaceMeshModel * ofMesh)
{
	double minAngle(DBL_MAX);
	Vector3VertexProperty pts = ofMesh->vertex_property<Vector3>("v:point");

	SurfaceMesh::Model::Halfedge_around_face_circulator h(ofMesh, f), eend = h;
	do{ 
		Vector3 a = pts[ofMesh->to_vertex(h)];
		Vector3 b = pts[ofMesh->from_vertex(h)];
		Vector3 c = pts[ofMesh->to_vertex(ofMesh->next_halfedge(h))];

		double d = dot((b-a).normalized(), (c-a).normalized());
		double angle = acos(qRanged(-1.0, d, 1.0));

		minAngle = qMin(angle, minAngle);
	} while(++h != eend);

	return minAngle;
}

void nurbs_plugin::convertToCurve()
{
	if(!m) return;

	document()->addModel( m );

	prepareSkeletonize(); 

	double theta = 15.0; // degrees

	for(int i = 0; i < 30; i++){
		stepSkeletonizeMesh();
	
		// Decide weather or not to keep contracting based on angle of faces
		bool isDone = true;
		foreach( Face f, m->faces() ){
			if( minAngle(f, m) > RADIANS(theta) )
				isDone = false;
		}

		if(isDone) break;
	}

	// Clear parameters
	mcf_params = NULL;

	foreach(Vertex v, m->vertices()) if(m->is_isolated(v)) m->remove_vertex(v);
	m->garbage_collection();

	// Overwrite any existing extracted nodes for selected part
	if(graph->getNode(groupID)) graph->removeNode(graph->getNode(groupID)->id);

	graph->addNode( new Structure::Curve(curveFit( m ), groupID) );

	// Clean up
	ps.clear();
	document()->setSelectedModel(entireMesh);
	document()->deleteModel(m);
	drawArea()->clear();
	
	m = NULL;

	drawArea()->updateGL();
}

void nurbs_plugin::convertToSheet()
{
	if(!m) return;

	document()->addModel( m );

	prepareSkeletonize(); 

	for(int i = 0; i < widget->contractIterations(); i++)
	{
		stepSkeletonizeMesh();
	}

	// Clear parameters
	mcf_params = NULL;

	// Clean up
	foreach(Vertex v, m->vertices()) if(m->is_isolated(v)) m->remove_vertex(v);
	m->garbage_collection();

	// Overwrite any existing extracted nodes for selected part
	if(graph->getNode(groupID)) graph->removeNode(graph->getNode(groupID)->id);

	graph->addNode( new Structure::Sheet(surfaceFit( m ), groupID) );

	document()->setSelectedModel( entireMesh );
	document()->deleteModel(m);
	drawArea()->clear();

	m = NULL;

	drawArea()->updateGL();
}

NURBS::NURBSCurved nurbs_plugin::curveFit( SurfaceMeshModel * part )
{
	NURBS::NURBSCurved fittedCurve;

	Vector3VertexProperty partPoints = part->vertex_property<Vector3>("v:point");

	GenericGraphs::Graph<int,double> g;
	SurfaceMeshHelper h(part);
	ScalarEdgeProperty elen = h.computeEdgeLengths();

	foreach(Edge e, part->edges()){
		Vertex v0 = part->vertex(e, 0);
		Vertex v1 = part->vertex(e, 1);
		g.AddEdge(v0.idx(), v1.idx(), elen[e]);
	}

	// Find initial furthest point
	g.DijkstraComputePaths(0);
	double max_dist = -DBL_MAX;
	int idxA = 0;
	for(int i = 0; i < (int)part->n_vertices(); i++){
		if(g.min_distance[i] > max_dist){
			max_dist = qMax(max_dist, g.min_distance[i]);
			idxA = i;
		}
	}

	// Find two furthest points
	g.DijkstraComputePaths(idxA);
	max_dist = -DBL_MAX;
	int idxB = 0;
	for(int i = 0; i < (int)part->n_vertices(); i++){
		if(g.min_distance[i] > max_dist){
			max_dist = qMax(max_dist, g.min_distance[i]);
			idxB = i;
		}
	}

	std::list<int> path = g.DijkstraShortestPath(idxA,idxB);

	// Check for loop case
	double r = 0.025 * part->bbox().diagonal().norm();
	QVector<int> pathA, pathB;
	foreach(int vi, path) pathA.push_back(vi);
	Vertex centerPath ( pathA[pathA.size() / 2] );

	foreach( Face f, part->faces() ){
		QVector<Vertex> vidx; 
		Surface_mesh::Vertex_around_face_circulator vit = part->vertices(Face(f)),vend=vit;
		do{ vidx.push_back(vit); } while(++vit != vend);
		Vector3d cp(0,0,0);
		if( TestSphereTriangle(partPoints[centerPath], r, partPoints[vidx[0]], partPoints[vidx[1]], partPoints[vidx[2]], cp) ){
			int v0 = vidx[0].idx();
			int v1 = vidx[1].idx();
			int v2 = vidx[2].idx();
			g.SetEdgeWeight(v0,v1,DBL_MAX);
			g.SetEdgeWeight(v1,v2,DBL_MAX);
			g.SetEdgeWeight(v2,v0,DBL_MAX);
		}
	}

	foreach(int vi, g.DijkstraShortestPath(idxB,idxA)) pathB.push_back(vi);

	QVector<int> finalPath = pathA;

	// We have a loop
	if(pathB.size() > 0.1 * pathA.size()) 
		finalPath += pathB;

	std::vector<Vector3d> polyLine;
	for(int i = 0; i < (int)finalPath.size(); i++)
		polyLine.push_back( partPoints[Vertex(finalPath[i])] );

	polyLine = refineByResolution(polyLine, r);
	//polyLine = smoothPolyline(polyLine, 1);

	return NURBS::NURBSCurved::createCurveFromPoints(polyLine);
}

std::vector<Vertex> nurbs_plugin::collectRings(SurfaceMeshModel * part, Vertex v, size_t min_nb)
{
	std::vector<Vertex> all;
	std::vector<Vertex> current_ring, next_ring;
	SurfaceMeshModel::Vertex_property<int> visited_map = part->vertex_property<int>("v:visit_map",-1);

	//initialize
	visited_map[v] = 0;
	current_ring.push_back(v);
	all.push_back(v);

	int i = 1;

	while ( (all.size() < min_nb) &&  (current_ring.size() != 0) ){
		// collect i-th ring
		std::vector<Vertex>::iterator it = current_ring.begin(), ite = current_ring.end();

		for(;it != ite; it++){
			// push neighbors of 
			SurfaceMeshModel::Halfedge_around_vertex_circulator hedgeb = part->halfedges(*it), hedgee = hedgeb;
			do{
				Vertex vj = part->to_vertex(hedgeb);

				if (visited_map[vj] == -1){
					visited_map[vj] = i;
					next_ring.push_back(vj);
					all.push_back(vj);
				}

				++hedgeb;
			} while(hedgeb != hedgee);
		}

		//next round must be launched from p_next_ring...
		current_ring = next_ring;
		next_ring.clear();

		i++;
	}

	//clean up
	part->remove_vertex_property(visited_map);

	return all;
}

NURBS::NURBSRectangled nurbs_plugin::surfaceFit( SurfaceMeshModel * part )
{
	points = part->vertex_property<Vector3>("v:point");
	
	/// Pick a side by clustering normals

	// 1) Find edge with flat dihedral angle
	SurfaceMeshModel::Vertex_property<double> vals = part->vertex_property<double>("v:vals",0);
	foreach(Vertex v, part->vertices()){
		double sum = 0.0;
		foreach(Vertex v, collectRings(part,v,12)){
			foreach(Halfedge h, part->onering_hedges(v)){
				sum += abs(calc_dihedral_angle( part, h ));
			}
		}
		vals[v] = sum;
	}

	double minSum = DBL_MAX;
	Vertex minVert;
	foreach(Vertex v, part->vertices()){
		if(vals[v] < minSum){
			minSum = vals[v];
			minVert = v;
		}
	}
	Halfedge startEdge = part->halfedge(minVert);

	// 2) Grow region by comparing difference of adjacent dihedral angles
	double angleThreshold = deg_to_rad( 40.0 );

	SurfaceMesh::Model::Vertex_property<bool> vvisited = part->add_vertex_property<bool>("v:visited", false);

	QStack<SurfaceMesh::Model::Vertex> to_visit;
	to_visit.push( part->to_vertex(startEdge) );

	while(!to_visit.empty())
	{
		Vertex cur_v = to_visit.pop();
		if( vvisited[cur_v] ) continue;
		vvisited[cur_v] = true;

		// Sum of angles around
		double sumAngles = 0.0;
		foreach(Halfedge hj, part->onering_hedges(cur_v)){
			sumAngles += abs(calc_dihedral_angle( part, hj ));
		}

		foreach(Halfedge hj, part->onering_hedges(cur_v))
		{
			Vertex vj = part->to_vertex(hj);
			if(sumAngles < angleThreshold)
				to_visit.push(vj);
			else
				vvisited[vj];
		}
	}

	// Get filtered inner vertices of selected side
	int shrink_count = 2;
	std::set<Vertex> inner;
	std::set<Vertex> border;
	for(int i = 0; i < shrink_count; i++)
	{
		std::set<Vertex> all_points;
		foreach(Vertex v, part->vertices())
			if(vvisited[v]) all_points.insert(v);

		border.clear();
		foreach(Vertex v, part->vertices()){
			if(vvisited[v]){
				foreach(Halfedge hj, part->onering_hedges(v)){
					Vertex vj = part->to_vertex(hj);
					if(!vvisited[vj])
						border.insert(vj);
				}
			}
		}

		inner.clear();
		std::set_difference(all_points.begin(), all_points.end(), border.begin(), border.end(),
			std::inserter(inner, inner.end()));

		// Shrink one level
		foreach(Vertex vv, border){
			foreach(Halfedge hj, part->onering_hedges(vv))
				vvisited[ part->to_vertex(hj) ] = false;
		}
	}

	SurfaceMesh::Model * submesh = NULL;

	bool isOpen = false;
	foreach(Vertex v, part->vertices()){
		if(part->is_boundary(v)){
			isOpen = true;
			break;
		}
	}

	if(!isOpen)
	{
		// Collect inner faces
		std::set<Face> innerFaces;
		std::set<Vertex> inFacesVerts;
		foreach(Vertex v, inner)
		{
			foreach(Halfedge hj, part->onering_hedges(v)){
				Face f = part->face(hj);
				innerFaces.insert(f);
				Surface_mesh::Vertex_around_face_circulator vit = part->vertices(f),vend=vit;
				do{ inFacesVerts.insert( Vertex(vit) ); } while(++vit != vend);
			}
		}

		// Create sub-mesh
		submesh = new SurfaceMesh::Model("SideFlat.obj","SideFlat");

		// Add vertices
		std::map<Vertex,Vertex> vmap;
		foreach(Vertex v, inFacesVerts){
			vmap[ v ] = Vertex(vmap.size());
			submesh->add_vertex( points[v] );
		}

		// Add faces
		foreach(Face f, innerFaces){
			std::vector<Vertex> verts;
			Surface_mesh::Vertex_around_face_circulator vit = part->vertices(f),vend=vit;
			do{ verts.push_back( Vertex(vit) ); } while(++vit != vend);
			submesh->add_triangle( vmap[verts[0]], vmap[verts[1]], vmap[verts[2]] );
		}
	}
	else
	{
		submesh = part;
	}

	{
		//ModifiedButterfly subdiv;
		//subdiv.subdivide((*(Surface_mesh*)submesh),1);
	}

	
	submesh->isVisible = false;

	if(part != submesh) document()->addModel( submesh );
	Vector3VertexProperty sub_points = submesh->vertex_property<Vector3>("v:point");

	// Smoothing
	{
		int numIteration = 3;
		bool protectBorders = true;

		Surface_mesh::Vertex_property<Point> newPositions = submesh->vertex_property<Point>("v:new_point",Vector3(0,0,0));
		Surface_mesh::Vertex_around_vertex_circulator vvit, vvend;

		// This method uses the basic equal weights Laplacian operator
		for(int iteration = 0; iteration < numIteration; iteration++)
		{
			Surface_mesh::Vertex_iterator vit, vend = submesh->vertices_end();

			// Original positions, for boundary
			for(vit = submesh->vertices_begin(); vit != vend; ++vit)
				newPositions[vit] = sub_points[vit];

			// Compute Laplacian
			for(vit = submesh->vertices_begin(); vit != vend; ++vit)
			{
				if(!protectBorders || (protectBorders && !submesh->is_boundary(vit)))
				{
					newPositions[vit] = Point(0,0,0);

					// Sum up neighbors
					vvit = vvend = submesh->vertices(vit);
					do{ newPositions[vit] += sub_points[vvit]; } while(++vvit != vvend);

					// Average it
					newPositions[vit] /= submesh->valence(vit);
				}
			}

			// Set vertices to final position
			for(vit = submesh->vertices_begin(); vit != vend; ++vit)
				sub_points[vit] = newPositions[vit];
		}

		submesh->remove_vertex_property(newPositions);
	}

	/// ==================
	// Fit rectangle

	BoundaryFitting bf( (SurfaceMeshModel*)submesh, widget->uCount(), widget->vCount() );

	/// ==================
	// Debug fitting

	if( false )
	{
		PointSoup * ps = new PointSoup;
		foreach(Vertex v, submesh->vertices())
		{
			//if(submesh->is_boundary(v))
			//	ps->addPoint( sub_points[v], qtJetColorMap(bf.vals[v]) );
		}
		drawArea()->addRenderObject(ps);

        FaceBarycenterHelper helper(submesh);
        Vector3FaceProperty fcenter = helper.compute();
		foreach(Face f, submesh->faces()){
			//if(submesh->is_boundary(f))
			vs.addVector(fcenter[f],bf.fgradient[f]);
		}

		drawArea()->addRenderObject( new LineSegments(bf.ls) );

		foreach(Vertex v, submesh->vertices()){
			//if(submesh->is_boundary(v))
			//	vs.addVector(sub_points[v],bf.vdirection[v]);
			//vs.addVector(sub_points[v],bf.vgradient[v]);
		}

		for(int i = 0; i < (int)bf.debugPoints2.size(); i++)
		{
			double t = double(i) / (bf.debugPoints2.size()-1);
			pps.addPoint(bf.debugPoints2[i], QColor(0,255 * t,50));
		}

		for(int i = 0; i < (int)bf.debugPoints.size(); i++)
		{
			//double t = double(i) / (bf.debugPoints.size()-1);
			//pps.addPoint(bf.debugPoints[i], QColor(255 * t,0,0));
		}

		for(int i = 1; i < (int)bf.debugPoints.size(); i++)
		{
			lseg.addLine( bf.debugPoints[i-1], bf.debugPoints[i], QColor(255,255,255,100) );
		}

		if(bf.corners.size())
		{
			PointSoup * ppp = new PointSoup(20);
			ppp->addPoint(bf.corners[0], QColor(255,0,0));
			ppp->addPoint(bf.corners[1], QColor(0,255,0));
			ppp->addPoint(bf.corners[2], QColor(0,0,255));
			ppp->addPoint(bf.corners[3], QColor(0,255,255));
			drawArea()->addRenderObject(ppp);
		}

		if(bf.main_edges.size())
		{
			PointSoup * ppe = new PointSoup(10);
			foreach(Vector3d p, bf.main_edges[0]) ppe->addPoint(p, QColor(255,0,0));
			foreach(Vector3d p, bf.main_edges[1]) ppe->addPoint(p, QColor(0,255,0));
			foreach(Vector3d p, bf.main_edges[2]) ppe->addPoint(p, QColor(0,0,255));
			foreach(Vector3d p, bf.main_edges[3]) ppe->addPoint(p, QColor(0,255,255));
			drawArea()->addRenderObject(ppe);
		}
	}
	/// ==================

	Array2D_Vector3 cp = bf.lines;
	//cp.clear(); // debug

	if(!cp.size()) return NURBS::NURBSRectangled::createSheet(Vector3d(0,0,0),Vector3d(0.01, 0.01, 0.01));

	Array2D_Real cw(cp.size(), Array1D_Real(cp.front().size(), 1.0));
	int degree = 3;
	return NURBS::NURBSRectangled(cp,cw,degree,degree,false,false,true,true);

	document()->deleteModel(m);

	// debug
	//return NURBS::NURBSRectangled::createSheet(Vector3d(0,0,0),Vector3d(0.01));
}

void nurbs_plugin::experiment()
{
	NanoKdTree tree;
	Vector3VertexProperty mesh_points = mesh()->get_vertex_property<Vector3>(VPOINT);

	foreach(Vertex v, mesh()->vertices())
	{
		tree.addPoint( mesh_points[v] );
	}

	tree.build();

	KDResults matches;
	tree.ball_search(Vector3d(0,0,0), 0.1, matches);

	drawArea()->deleteAllRenderObjects();

	foreach(KDResultPair r, matches)
	{
		drawArea()->drawPoint(mesh_points[Vertex(r.first)]);
	}
}

void nurbs_plugin::updateDrawArea()
{
	drawArea()->update();
}

void nurbs_plugin::loadGraph()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(0, "Open Model",
		mainWindow()->settings()->getString("lastUsedDirectory"), "Model Files (*.xml)");
	if(fileNames.isEmpty()) return;

	graph = new Structure::Graph(fileNames.front());
}

// XXX TODO Fix plugin
// Q_EXPORT_PLUGIN (nurbs_plugin)
