/*
 * filename :	triangle.h
 *
 * programmer :	Cao Jiayin
 */

#ifndef	SORT_TRIANGLE
#define	SORT_TRIANGLE

#include "primitive.h"

// pre-decleration
class	TriMesh;

//////////////////////////////////////////////////////////////////////////////////
//	definition of triangle
//	note: triangle is the only primitive supported by the system.
class	Triangle : public Primitive
{
// public method
public:
	// constructor
	// para 'trimesh' : the triangle mesh it belongs to
	// para 'index'   : the index buffer
	// para 'vb'      : the vertex buffer
	Triangle( const TriMesh* trimesh , const unsigned index );
	// destructor
	~Triangle(){}

	// check if the triangle is intersected with the ray
	// para 'r' : the ray to check
	// para 'intersect' : the result storing the intersection information
	//					  the intersection is an optimized versiion
	// result   : positive value if intersect
	bool	GetIntersect( const Ray& r , Intersection* intersect ) const;

	// get the bounding box of the triangle
	const BBox&	GetBBox();
};

#endif
