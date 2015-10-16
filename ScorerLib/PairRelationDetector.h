#pragma once

#include "RelationDetector.h"

class PairRelationDetector : public RelationDetector
{
public:
	class PairModifier
	{
	public:
		PairModifier(int pl):pointLevel_(pl),maxAllowDeviation_(0.0){};

		int pointLevel_;
		double maxAllowDeviation_;
		//QString type_;
	};
	class ConnectedPairModifier : public PairModifier
	{
	public:
		double operator() (Structure::Node *n1, Eigen::MatrixXd& m1, Structure::Node *n2, Eigen::MatrixXd& m2);
		ConnectedPairModifier(Structure::Graph *graph, double gd, bool bUseLink, int pl):PairModifier(pl), normalizeCoef_(gd), graph_(graph),bUseLink_(bUseLink)//, logStream_(logStream)
		{};

		double normalizeCoef_;
		Structure::Graph *graph_;
		bool bUseLink_;
		//QTextStream& logStream_;
	};
	class TransPairModifier : public PairModifier
	{
	public:
		double operator() (Structure::Node *n1, Eigen::MatrixXd& m1, Structure::Node *n2, Eigen::MatrixXd& m2);
		TransPairModifier(int pl):PairModifier(pl){};
	};
	class RefPairModifier : public PairModifier
	{
	public:
		double operator() (Structure::Node *n1, Eigen::MatrixXd& m1, Structure::Node *n2, Eigen::MatrixXd& m2);
		RefPairModifier(int pl):PairModifier(pl){};
	};
	
	
	////////////////////////////////////////////////////
public:    
	PairRelationDetector(Structure::Graph* g, int ith, double normalizeCoef, bool bUseLink=false, bool bModifyDeviation=false, int logLevel=0);

    // if bSource_, g is the target shape (graph_ is source shape). else g is the source shape
	void detect(Structure::Graph* g, QVector<PART_LANDMARK> &corres);	
	void detectConnectedPairs(Structure::Graph* g, QVector<PART_LANDMARK> &corres);	
	void detectOtherPairs(Structure::Graph* g, QVector<PART_LANDMARK> &corres);	
private:
	template<class Function>
	void modifyPairsDegree(QVector<PairRelation>& pairs, Function fn, Structure::Graph* g, QVector<PART_LANDMARK> &corres, QString title)
	{
		if( logLevel_ >0 )
		{
			logStream_ << "\n*************** " << title << " *************\n";
		}

		int num(0);
		for ( int i = 0; i < pairs.size(); ++i)
		{
			PairRelation& prb = pairs[i];
			prb.tag = true;
			std::vector<Structure::Node*> nodes1 = findNodesInST(prb.n1->id, g, corres, !bSource_);
			std::vector<Structure::Node*> nodes2 = findNodesInST(prb.n2->id, g, corres, !bSource_);
			fn.maxAllowDeviation_ = 0.0;

			double isExistinSorT(0.0);
			for ( int i1 = 0; i1 < (int) nodes1.size(); ++i1)
			{
				Eigen::MatrixXd m1 = node2matrix(nodes1[i1], pointLevel_);

				for ( int i2 = 0; i2 < (int) nodes2.size(); ++i2)
				{
					Eigen::MatrixXd m2 = node2matrix(nodes2[i2], pointLevel_);
					double min_dist = fn(nodes1[i1], m1, nodes2[i2], m2);
					isExistinSorT += min_dist;

					if (logLevel_>0) // && min_dist > prb.deviation 
					{
						logStream_ << num << "\n";
						if ( bSource_ )
						{
							logStream_ << "pair in source shape <" << prb.n1->id << ", " << prb.n2->id << "> with deviation: " << prb.deviation << "\n";
							logStream_ << "corresponds to a pair in target shape <" << nodes1[i1]->id << ", " << nodes2[i2]->id << ">: " << min_dist << "\n\n";
						}
						else
						{
							logStream_ << "pairs in target shape <" << prb.n1->id << ", " << prb.n2->id << "> with deviation: " << prb.deviation << "\n";
							logStream_ << "corresponds to a pair in source shape <" << nodes1[i1]->id << ", " << nodes2[i2]->id << ">: " << min_dist << "\n\n";
						}
						++num;
					}
					
				}
			}

			/////////
			if ( nodes1.size()*nodes2.size() == 0 || isExistinSorT == -double(nodes1.size()*nodes2.size()) )
			{				
				prb.tag = false;
			}
			else
			{
				prb.deviation = std::max(prb.deviation, fn.maxAllowDeviation_);				
			}
		}

		if( logLevel_ >0 )
		{
			logStream_ << "Total: " << pairs.size() << " pairs" << "\n\n";
			int num(0),k(0); double tmp(0.0); 

			if (bSource_)
				logStream_ << "Pairs from source & exist in target: \n";
			else
				logStream_ << "Pairs from target & exist in source: \n";

			for ( QVector<PairRelation>::iterator it = pairs.begin(); it != pairs.end(); ++it)
			{
				if ( it->tag)
				{
					if ( it->deviation > tmp)
					{
						tmp = it->deviation;
						k = num;
					}
					 ++num;

					logStream_ << num << ": " << *it << "\n";
				}
			}
			this->logStream_ << "pair " << k << " with max deviation: " << tmp << "\n\n";

			if( logLevel_ >1 )
			{
				if (bSource_)
					logStream_ << "Pairs from source & not exist in target: \n";
				else
					logStream_ << "Pairs from target & not exist in source: \n";

				for ( QVector<PairRelation>::iterator it = pairs.begin(); it != pairs.end(); ++it)
				{
					if ( !it->tag)
					{
						//if ( it->deviation > tmp)
						//{
						//	tmp = it->deviation;
						//	k = num;
						//}
						 ++num;

						logStream_ << num << ": " << *it << "\n";
					}
				}
			}
			
		}
	}
	bool has_trans_relation(int id1, int id2);
	bool has_ref_relation(int id1, int id2);
	void isParalOrthoCoplanar(int id1, int id2);
	//bool has_rot_relation(int id1, int id2);
	Eigen::MatrixXd& findCptsByNodeId(Structure::Node* node);

	void pushToOtherPairs(QVector<PairRelation>& pairs, QString type);
	QVector<PairRelation> transPairs_;
	QVector<PairRelation> refPairs_;

public:
	QVector<PairRelation> otherPairs_; // all pairs except connected pairs are save in it.
	QVector<PairRelation> connectedPairs_;
	
private:
	bool bSource_;
	bool bModifyDeviation_;
	bool bUseLink_;
	std::vector<Eigen::MatrixXd> nodesCpts_; 
    std::vector<Eigen::Vector3d> nodesCenter_;
	std::vector<double> nodesDiameter_;
};
