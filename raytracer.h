// Template for GP1, version 1
// IGAD/NHTV - Jacco Bikker - 2006-2013
#include <vector>
#pragma once

namespace Tmpl8 {

//#define float3 float3a
// == class Material ==========================================================
//    --------------
class Surface;
class Plane;
class vector;
class Texture
{
public:
	Texture();
	Texture(char* a_file);
	Surface* m_Surface;
	int m_Pitch;
	int m_Width;
	int m_Height;
};


class Material
{
public:
	Material();
	Material(float3 _Color, float diff, float refl, float _spec, float _refr) : diffuse(diff), reflection(refl), spec(_spec), density(_refr) { color = _Color; m_Texture = nullptr; }
	Material(Texture* a_Texture, float diff, float refl, float _spec, float _refr) : diffuse(diff), reflection(refl), spec(_spec), density(_refr){ m_Texture = a_Texture; }
	
	float3 color;					// color of material
	float diffuse;
	float reflection;
	float density;
	float spec;
	Texture* m_Texture;
};

// == class Intersection ======================================================
//    ------------------
class Intersection
{
public:
	Intersection();
	float3 N;						// normal at intersection point
	Material* material;					// color at intersection point
	bool useT;		/// Use texc?
	float3 pos;
	float u, v;
};

// == class Ray ===============================================================
//    ---------
class Ray
{
public:
	// methods
	void Draw2D();
	// data members
	bool shadowRay;
	float3 O;						// ray origin
	float3 D;						// normalized ray direction
	float3 invD;					// inverted ray direction
	float t;						// distance of intersection along ray ( >= 0 )
	Intersection intersection;		// data for closest intersection point
	unsigned int nodecount;
	
};



// == class Triangle ==========================================================
//    --------------
class Triangle
{
public:
	// constructors
	Triangle() {}
	Triangle( float3 _V0, float3 _V1, float3 _V2, Material* mat );
	// methods
	void Intersect( Ray& _Ray );
	void Draw2D();
	float3 GetCenter();
	// data members
	float3 v0, v1, v2;				// triangle vertices
	float3 N;						// triangle normal
	float3 VN[3];
	float2 UV[3];

	Material* material;	
};

// == class Camera ============================================================
//    ------------
class Camera
{
public:
	// methods
	Camera() :  m_focalDepth(3.0f), m_Apterture(.03f) {}
	void Set( float3 _Pos, float _xrot, float _yrot );
	void GenerateRay( Ray& _Ray, int _X, int _Y );
	void Draw2D();
	// data members
	float3 pos;						// camera position
	float3 forward;						// normalized view direction
	float3 p1, p2, p3, p4;			// corners of screen plane
	float3 up;
	float3 right;
	float3 rotation;
	float m_focalDepth;
	float m_Apterture;
	matrix mat;
};

class Light
{
public:
	Light(float3 _Pos, float3 _Color, float _I, float _S);
	void Draw2D();
	float3 pos;
	float3 color;
	float m_Intensity;
	float m_Size; // area light
};

class ObjLoader
{
public:
	bool LoadObj(char* a_File, float3 p, Material* mat);
	Material* objMaterial;
};

// == class Scene =============================================================
//    -----------
class Scene
{
public:
	// constructor
	Scene();
	void Intersect( Ray& _Ray );
	void ShadowIntersect( Ray& _Ray );
	void Draw2D();
	// data members
	std::vector<Triangle*> m_PrimList;
	Light** lightList;
	int lightCount;
};

// == class Renderer ==========================================================
//    --------------
class Renderer
{
public:
	// constructor
	Renderer();
	~Renderer();
	// methods
	float3 Trace( Ray& _Ray, int _Depth );
	void Render( int _Y );
	int m_blendCount;
	// data members
	Scene scene;
	Camera camera;
};


};

// EOF