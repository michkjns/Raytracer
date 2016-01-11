#include <vector>
#include "template.h"
#include "raytracer.h"
#pragma once


namespace Tmpl8 {

#define MAXTRIS 5
#define MAXBOXES 500


struct aabb
{
public:
	float3 min, max;
};

class BVHNode
{
public:
	BVHNode(std::vector<Triangle*> _List, int depth) ;
	BVHNode(unsigned int depth) : m_depth(depth), m_Left(nullptr), m_Right(nullptr), m_isLeaf(true) {} 
	void Split();
	bool Intersect(Ray& ray);
	void Traverse(Ray& ray);
	void GenerateBounds();
	void Draw2D();
	aabb m_Box;

	
	bool m_isLeaf;
	int m_depth;
	std::vector<Triangle*> triList;
	BVHNode* m_Left, *m_Right;
};

}


// EOF