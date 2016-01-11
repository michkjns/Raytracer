#include "bvh.h"
#include "raytracer.h"
#include "surface.h"
#include <algorithm>

using namespace Tmpl8;

extern Surface* screen;
#define WXMIN	-5.0f	// left edge of visible world
#define WYMIN	-5.0f	// top edge of visible world
#define WXMAX	 5.0f	// right edge of visible world
#define WYMAX	 5.0f	// bottom edge of visible world
#define SX(x) ((((x)-(WXMIN))/((WXMAX)-(WXMIN)))*(SCRWIDTH/2)+SCRWIDTH/2)
#define SY(y) ((((-y)-(WYMIN))/((WYMAX)-(WYMIN)))*SCRHEIGHT) // trust me

void _Line2D( float x1, float y1, float x2, float y2, unsigned int c )
{
	x1 = SX( x1 ), y1 = SY( y1 ), x2 = SX( x2 ), y2 = SY( y2 );
	screen->ClipTo( SCRWIDTH / 2 + 2, 0, SCRWIDTH - 1, SCRHEIGHT - 1 );
	screen->Line( x1, y1, x2, y2, c );
}

void BVHNode::Draw2D()
{
	if(m_isLeaf)
	{
		_Line2D(m_Box.min.x, m_Box.min.z, m_Box.max.x, m_Box.min.z, 0x00ff00);
		_Line2D(m_Box.min.x, m_Box.max.z, m_Box.max.x, m_Box.max.z, 0x00ff00);
		_Line2D(m_Box.max.x, m_Box.min.z, m_Box.max.x, m_Box.max.z, 0x00ff00);
		_Line2D(m_Box.min.x, m_Box.min.z, m_Box.min.x, m_Box.max.z, 0x00ff00);
	}
	else
	{
		//_Line2D(m_Box.min.x, m_Box.min.z, m_Box.max.x, m_Box.min.z, 0x00ffff);
		//_Line2D(m_Box.min.x, m_Box.max.z, m_Box.max.x, m_Box.max.z, 0x00ffff);
		//_Line2D(m_Box.max.x, m_Box.min.z, m_Box.max.x, m_Box.max.z, 0x00ffff);
		//_Line2D(m_Box.min.x, m_Box.min.z, m_Box.min.x, m_Box.max.z, 0x00ffff);

		m_Left->Draw2D();
		m_Right->Draw2D();
	}
}

void BVHNode::Split()
{
	if(m_depth > MAXBOXES * .5f) return;
	m_isLeaf = false;

	const float dx = abs(m_Box.min.x - m_Box.max.x);	
	const float dy = abs(m_Box.min.y - m_Box.max.y);	
	const float dz = abs(m_Box.min.z - m_Box.max.z);	
	const float3 s =  float3(dx,dy,dz);
	int bestAxis = 0;
	if(s.y >= s.x )	bestAxis = 1;
	if(s.z >= s.y) bestAxis = 2;

	const float half = (m_Box.min[bestAxis] + m_Box.max[bestAxis]) *.5f;

	m_Left  = new BVHNode(m_depth+1);
	m_Right = new BVHNode(m_depth+1);
	
	for ( unsigned int i = 0; i < triList.size(); i++ ) 
	{ 
		if ( triList[i]->GetCenter()[bestAxis] < half ) 
		{ 
			m_Left->triList.push_back( triList[i] );
		} 
		else 
		{ 
			m_Right->triList.push_back( triList[i] ); 
		}
	}

	m_Left->m_Box = m_Box;
	m_Right->m_Box = m_Box;
	m_Left->m_Box.max[bestAxis] = half;
	m_Right->m_Box.min[bestAxis] = half;

	if(m_Left->triList.size() > MAXTRIS) m_Left->Split();
	if(m_Right->triList.size() > MAXTRIS) m_Right->Split();
	m_Left->GenerateBounds();
	m_Right->GenerateBounds();
}

void BVHNode::GenerateBounds()
{

	float sx, sy, sz, bx, by, bz;
	sx = sy = sz = 1e34;
	bx = by = bz = -1e34;
	for(unsigned int i = 0; i < triList.size(); i++)
	{
		Triangle* t = triList[i];
		sx = min(t->v0.x, sx);
		sy = min(t->v0.y, sy);
		sz = min(t->v0.z, sz);

		bx = max(t->v0.x, bx);
		by = max(t->v0.y, by);
		bz = max(t->v0.z, bz);

		sx = min(t->v1.x, sx);
		sy = min(t->v1.y, sy);
		sz = min(t->v1.z, sz);

		bx = max(t->v1.x, bx);
		by = max(t->v1.y, by);
		bz = max(t->v1.z, bz);

		sx = min(t->v2.x, sx);
		sy = min(t->v2.y, sy);
		sz = min(t->v2.z, sz);

		bx = max(t->v2.x, bx);
		by = max(t->v2.y, by);
		bz = max(t->v2.z, bz);
	}

	m_Box.min.x = sx;
	m_Box.min.y = sy;

	m_Box.max.x = bx;
	m_Box.max.y = by;

	m_Box.min.z = sz;
	m_Box.max.z = bz;
}

BVHNode::BVHNode(std::vector<Triangle*> _List, int depth) : m_Left(0), m_Right(0), triList(_List), m_isLeaf(true), m_depth(depth)
{  

}

bool BVHNode::Intersect(Ray& _Ray)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
 
	if ( _Ray.D.x >= 0 ) 
	{
			tmin = ( m_Box.min.x - _Ray.O.x ) * _Ray.invD.x;
			tmax = ( m_Box.max.x - _Ray.O.x ) * _Ray.invD.x;
	}                                                                
	else 
	{
			tmin = ( m_Box.max.x - _Ray.O.x ) * _Ray.invD.x;
			tmax = ( m_Box.min.x - _Ray.O.x ) * _Ray.invD.x;
	}                                                                  
                                                                           
	if ( _Ray.D.y >= 0 )
	{                     
			tymin = ( m_Box.min.y - _Ray.O.y ) * _Ray.invD.y;
			tymax = ( m_Box.max.y - _Ray.O.y ) * _Ray.invD.y;
	}                                                                          
	else 
	{                                                             
			tymin = ( m_Box.max.y - _Ray.O.y ) * _Ray.invD.y;
			tymax = ( m_Box.min.y - _Ray.O.y ) * _Ray.invD.y;
	}
 
	if ( ( tmin > tymax ) || ( tymin > tmax ) )
			return false;
 
	if ( tymin > tmin )
			tmin = tymin;
	if ( tymax < tmax )
			tmax = tymax;
 
	if ( _Ray.D.z >= 0 ) 
	{
			tzmin = ( m_Box.min.z - _Ray.O.z ) * _Ray.invD.z;
			tzmax = ( m_Box.max.z - _Ray.O.z ) * _Ray.invD.z;
	}
	else 
	{
			tzmin = ( m_Box.max.z - _Ray.O.z ) * _Ray.invD.z;
			tzmax = ( m_Box.min.z - _Ray.O.z ) * _Ray.invD.z;
	}
 
	if ( ( tmin > tzmax ) || ( tzmin > tmax) )
			return false;
 
	if ( tzmin > tmin )
			tmin = tzmin;
	if ( tzmax < tmax )
			tmax = tzmax;
       

	return ( ( tmin <= 1e34 ) && ( tmax > 0 ) );
}

void BVHNode::Traverse(Ray& _Ray)
{
	if(Intersect(_Ray))
	{
		_Ray.nodecount++;
		if(m_isLeaf)
		{
			for( unsigned int i = 0; i < triList.size(); i++)
			{
				triList[i]->Intersect(_Ray);
			}
		}
		else 
		{
			  m_Right->Traverse(_Ray);
			  m_Left->Traverse(_Ray);
			
		}
	}
	return;
}

// EOF
