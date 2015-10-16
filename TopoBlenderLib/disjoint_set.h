#pragma once

// some stuff for portable hashes...
#ifdef WIN32
#ifndef __MINGW32__
#include <hash_map>
#include <hash_set>
#define STDEXT stdext
#else
#include <ext/hash_map>
#include <ext/hash_set>
#define STDEXT __gnu_cxx
#endif
#else
#include <unordered_map>
#include <unordered_set>

#if 0
#define STDEXT __gnu_cxx
// It's terrible but gnu's hash_map needs an instantiation of hash() for
// every key type! So we cast the pointer to void*
namespace __gnu_cxx{
	template <> class std::hash<void *>: private hash<unsigned long>{
	public:
		size_t operator()(const void *ptr) const { return std::hash<unsigned long>::operator()((unsigned long)ptr); }
	};
}
#endif
#define STDEXT std
#endif

#include <vector>
#include <assert.h>

/*!
* Given a set of elements, it is often useful to break them up or partition them into a number of separate, nonoverlapping groups. 
* A disjoint-set data structure is a data structure that keeps track of such a partitioning. See 
* <a href="http://en.wikipedia.org/wiki/Disjoint-set_data_structure">Diskoint-set data structure on Wikipedia </a> for more details.
*/
template<class OBJECT_TYPE>
class DisjointSet
{
	/*************************************************
	*		Inner class definitions
	**************************************************/
	struct DisjointSetNode
	{
		DisjointSetNode(OBJECT_TYPE *x) {obj=x; parent=obj; rank=0;}
		OBJECT_TYPE	*obj;
		OBJECT_TYPE *parent;
		int	rank;
	};

	typedef OBJECT_TYPE* ObjectPointer;
	typedef std::pair< ObjectPointer, int >	hPair;

	struct SimpleObjHashFunc{
		inline	size_t	operator ()(const ObjectPointer &p) const {return size_t(p);}
	};

#ifdef _MSC_VER
	STDEXT::hash_map< OBJECT_TYPE*, int > inserted_objects;
	typedef typename STDEXT::hash_map< ObjectPointer, int >::iterator	hIterator;
#else
	STDEXT::unordered_map< OBJECT_TYPE*, int, SimpleObjHashFunc > inserted_objects;
	typedef typename STDEXT::unordered_map< ObjectPointer, int, SimpleObjHashFunc  >::iterator	hIterator;
#endif

	typedef std::pair< hIterator, bool > hInsertResult;

public:
	/*!
	* Default constructor
	*/
	DisjointSet() {}

	/*!
	* Makes a group containing only a given element (a singleton).
	*/
	void MakeSet(OBJECT_TYPE *x)
	{
		int object_count		= int(inserted_objects.size());
		assert(inserted_objects.find(x)==inserted_objects.end()); //the map mustn't already contain the object x
		nodes.push_back(DisjointSetNode(x));
		inserted_objects.insert( hPair(x,object_count) );
	}

	/*!
	* Combine or merge two groups into a single group.
	*/
	void Union(OBJECT_TYPE *x, OBJECT_TYPE *y)
	{
		OBJECT_TYPE *s0 = FindSet(x);
		OBJECT_TYPE *s1 = FindSet(y);
		Link(s0, s1);
	}

	/*!
	* Determine which group a particular element is in.
	*/
	OBJECT_TYPE* FindSet(OBJECT_TYPE *x)
	{
		hIterator pos = inserted_objects.find(x);
		assert(pos!=inserted_objects.end());
		DisjointSetNode *node = &nodes[pos->second];
		if (node->parent!=x)
			node->parent = FindSet(node->parent);
		return node->parent;
	}

private:
	/*
	*/
	void Link(OBJECT_TYPE *x, OBJECT_TYPE *y)
	{
		hIterator xPos = inserted_objects.find(x);
		hIterator yPos = inserted_objects.find(y);
		assert(xPos!=inserted_objects.end() && yPos!=inserted_objects.end());
		DisjointSetNode *xNode = &nodes[xPos->second];
		DisjointSetNode *yNode = &nodes[yPos->second];
		if (xNode->rank>yNode->rank)
			xNode->parent = y;
		else
		{
			yNode->parent = x;
			if (xNode->rank==yNode->rank)
				yNode->rank++;
		}
	}

protected:
	std::vector< DisjointSetNode >	nodes;
};
