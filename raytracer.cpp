// Template for GP1, version 
// IGAD/NHTV - Jacco Bikker - 2006-2013

#include "template.h"
#include "raytracer.h"
#include "surface.h"
#include "bvh.h"
#include "twister.h"

#define MAXDEPTH 3
#define BVHVIEW 0
#define ANTIALIAS 1
using namespace Tmpl8;

extern Surface* screen;
extern float3 buffer[SCRWIDTH * SCRHEIGHT];
Scene* _scene;
BVHNode* rootNode;
Twister twist;
Material Test(float3(1.0f, 1.0f, 0.0f), 1.0f, .0f, .0f, 0.0f);

// == WORLD VISUALIZATION HELPERS =============================================
//    ---------------------------
#define WXMIN	-5.0f	// left edge of visible world
#define WYMIN	-5.0f	// top edge of visible world
#define WXMAX	 5.0f	// right edge of visible world
#define WYMAX	 5.0f	// bottom edge of visible world
#define SX(x) ((((x)-(WXMIN))/((WXMAX)-(WXMIN)))*(SCRWIDTH/2)+SCRWIDTH/2)
#define SY(y) ((((-y)-(WYMIN))/((WYMAX)-(WYMIN)))*SCRHEIGHT) // trust me

void Line2D( float x1, float y1, float x2, float y2, unsigned int c )
{
	x1 = SX( x1 ), y1 = SY( y1 ), x2 = SX( x2 ), y2 = SY( y2 );
	screen->ClipTo( SCRWIDTH / 2 + 2, 0, SCRWIDTH - 1, SCRHEIGHT - 1 );
	screen->Line( x1, y1, x2, y2, c );
}
void Plot2D( float x1, float y1, unsigned int c )
{
	screen->Plot( (int)SX( x1 ), (int)SY( y1 ), c );
}
unsigned int ConvertFPColor( float3 c )
{
	int red = (int)(min( 1.0f, c.x ) * 255.0f);
	int green = (int)(min( 1.0f, c.y ) * 255.0f);
	int blue = (int)(min( 1.0f, c.z ) * 255.0f);
	return (red << 16) + (green << 8) + blue;
}

float3 ColorToFloat3( unsigned int p )
{
	return float3( ((p >> 16) & 0xFF) / 255.0f , ((p >> 8) & 0xFF) / 255.0f, (p & 0xFF) / 255.0f  );
}


// == Texture class ===============================================================
//    ---------
Texture::Texture(char* a_file)
{
	m_Surface = new Surface(a_file);
	m_Width = m_Surface->GetWidth();
	m_Height = m_Surface->GetHeight();
	m_Pitch = m_Surface->GetPitch();
}

// == Ray class ===============================================================
//    ---------
void Ray::Draw2D()
{
	Line2D( O.x, O.z, O.x + 10000 * D.x, O.z + 10000 + D.z, 0x111111 );
	Line2D( O.x, O.z, O.x + D.x, O.z + D.z, 0x666666 );	
	Line2D( O.x, O.z, O.x + t * D.x, O.z + t * D.z, 0xaaaa00 );	
	Plot2D( O.x + t * D.x, O.z + t * D.z, 0xff0000 );
}


// == Triangle class ==========================================================
//    --------------
Triangle::Triangle( float3 _V0, float3 _V1, float3 _V2, Material* mat ) : v0( _V0 ), v1( _V1 ), v2( _V2 )
{
	// set triangle vertices and calculate its normal
	const float3 e1 = v1 - v0, e2 = v2 - v0;
	material = mat;
	N = Normalize( Cross( e1, e2 ) );
	VN[0] = N;
	VN[1] = N;
	VN[2] = N;
}

void Triangle::Intersect( Ray& _Ray ) 
{
	// find the intersection of a ray and a triangle
	float t = 1e34; // todo: find intersection distance
	const float3 e1 = v1 - v0;
	const float3 e2 = v2 - v0;
	const bool glass = (material->density < 1.0f);
	const float3 pvec = Cross(_Ray.D, e2);
	const float det = Dot(e1, pvec);
	float u, v;

	if(glass)
	{
		if( det > -EPSILON && det < EPSILON)
			return;
		const float inv_det = 1.0f / det;
		const float3 tvec = _Ray.O - v0;
		u = Dot(tvec, pvec) * inv_det;
		if( u < 0.0f || u > 1.0f) return;
		const float3 qvec = Cross(tvec, e1);
		v = Dot(_Ray.D, qvec) * inv_det;
		if(v < 0.0f || u + v > 1.0f) return;
		t = Dot(e2, qvec) * inv_det;
	}
	else
	{
		if(det < EPSILON)
			return;
	
		const float3 tvec = _Ray.O - v0;
		u = Dot(tvec, pvec);
		if( u < 0 || u > det )
			return;
		const float3 qvec = Cross(tvec, e1);
		v = Dot(_Ray.D, qvec);
		if( v < 0 || v + u > det )
			return;
		t = Dot(e2, qvec);
		const float inv_det = 1.0f / det;
		t *= inv_det;
		u *= inv_det;
		v *= inv_det;
	}
	
	float3 _N;
	const float f = 1.0f - u - v;
	_N.x = VN[0].x * f + VN[1].x * u + VN[2].x * v;
	_N.y = VN[0].y * f + VN[1].y * u + VN[2].y * v;
	_N.z = VN[0].z * f + VN[1].z * u + VN[2].z * v;

	float2 _UV;
	_UV.x = UV[0].x * f + UV[1].x * u + UV[2].x * v;
	_UV.y = UV[0].y * f + UV[1].y * u + UV[2].y * v;
	if (t < _Ray.t && t >= 0)
	{
		_Ray.t = t;
		_Ray.intersection.N = _N;
		_Ray.intersection.pos = _Ray.O + _Ray.D * t;
		_Ray.intersection.material = material;
		_Ray.intersection.u = _UV.x;
		_Ray.intersection.v = _UV.y;

		 //printf("%f, %f, %f\n", material->color.x, material->color.y, material->color.z);
	}
}

void Triangle::Draw2D()
{
	// draw triangle edges
	Line2D( v0.x, v0.z, v1.x, v1.z, 0xffffff );
	Line2D( v1.x, v1.z, v2.x, v2.z, 0xffffff );
	Line2D( v2.x, v2.z, v0.x, v0.z, 0xffffff );
}

float3 Triangle::GetCenter()
{
	return (v0 + v1 + v2) / 3;
}

// == Camera class ============================================================
//    ------------
void Camera::Draw2D()
{
	// draw view frustum
	Line2D( pos.x, pos.z, p1.x, p1.z, 0xaa );
	Line2D( pos.x, pos.z, p2.x, p2.z, 0xaa );
	Line2D( p1.x, p1.z, p2.x, p2.z, 0xaa );
}

void Camera::Set( float3 _Pos, float _xrot, float _yrot )
{
	// set position and view direction, then calculate screen corners

	pos = pos + Normalize(forward) * _Pos.z + _Pos.y * float3(0,1,0) + _Pos.x * Cross(forward, up);

	rotation = rotation + float3(_xrot, _yrot,0);
	p1 = float3( -0.5f, -0.5f, 1.0f ) ;
	p2 = float3( 0.5f, -0.5f, 1.0f ) ; 
	p3 = float3( 0.5f,  0.5f, 1.0f ) ; 
	p4 = float3( -0.5f,  0.5f, 1.0f ) ;

	matrix t;
	mat.RotateY(rotation.y);
	t.RotateX(rotation.x);
	mat.Concatenate(t);
	mat.Translate(pos);	

	p1 = mat.Transform(p1);
	p2 = mat.Transform(p2);
	p3 = mat.Transform(p3);
	p4 = mat.Transform(p4);

	forward = float3(mat[2], mat[6], mat[10]);
	up = float3(mat[1], mat[5], mat[9]);
	right = float3(mat[0], mat[4], mat[8]);
}

void Camera::GenerateRay( Ray& _Ray, int _X, int _Y )
{
	const float rx = twist.Rand() * m_Apterture - m_Apterture*.5f;
	const float ry = twist.Rand() * m_Apterture - m_Apterture*.5f;
	const float rz = twist.Rand() * m_Apterture - m_Apterture*.5f;

	_Ray.t = 1e34;
	const float x = (float)_X / (SCRWIDTH / 2);
	const float y = (float)_Y / SCRHEIGHT;
	float3 P = p1 + (p2 - p1) * x + (p4 - p1) * y;
	_Ray.O = pos + (float3(rx * right.x, ry * up.y, 0) ) ;
	float3 aim = m_focalDepth * (P - pos) + pos ;
	_Ray.D =   aim - _Ray.O ;
	
	//Plot2D( aim.x , aim.z , 0xffffff );
}

// == Scene class =============================================================
//    -----------
Scene::Scene()
{
	// scene initialization
	// Material: Texture/Color, DIFF, REFL, SPEC, ABSORB
	_scene = this;

	lightList = new Light*[16];
	lightCount = 0;

	//lightList[lightCount++] = new Light( float3(    0,  -5,  -0   ), float3( 1.0f, 1.0f, 1.0f ), 10.0f, 0 );
	lightList[lightCount++] = new Light( float3(    -5,  -3,  0   ), float3( 1.0f, 1.0f, 1.0f ), 10.0f, 1.0f );
	//lightList[lightCount++] = new Light( float3(    5,  -3,  -10  ), float3( 1.0f, 0.0f, 0.0f ), 5.0f, 1.0f );
	lightList[lightCount++] = new Light( float3(    -5,  -3,  -30  ), float3( 1.0f, 1.0f, 1.0f ), 5.0f, 1.0f );
	//lightList[lightCount++] = new Light( float3(    -2,  -3,  -3   ), float3( 1.0f, 0.0f, 0.0f ) );

	Material* floormat = new Material(float3(1,1,1), 1.0f, 0.5f, .0f, 0);
	Material* green = new Material(float3(0.0f, 1.0f, 0.0f), 1.0f, 0.0f, .0f, 0);

	//m_PrimList.push_back(	new Triangle(float3(30,2,-30), float3(30,2,30), float3(-30,2,30),	floormat));
	//m_PrimList.push_back(	new Triangle(float3(-30,2,-30), float3(30,2,-30), float3(-30,2,30), floormat));

	ObjLoader loader;
	//loader.LoadObj("assets/frog.obj", float3(-3,-.7f,-10), new Material(float3(.5f, 1.0f, .5f), 1.0f, 0.0f, .4f,0.0f));
	
	//loader.LoadObj("assets/california.obj", float3(5,0.3f,0), new Material(float3(1.0f, .5f, 0), .1f, .5f, .2f, 1));
	//loader.LoadObj("assets/sphere.obj", float3(2,0,-30), new Material(float3(0.0f, 0, 0), 1.0f, .5f, 0, 1.00f));
	loader.LoadObj("assets/asdf.obj", float3(1,.5f,-0), new Material(float3(1.0f, 0.0f, 0.0f), .1f, 1.0f, 0, 1.00f));
	loader.LoadObj("assets/sphere_small.obj", float3(0,.5f,-0), new Material(float3(1.0f, .0f, .0f), 1.0f, .0f, 0, .50f));
	loader.LoadObj("assets/sphere_small.obj", float3(0,.5f,-1), new Material(float3(1.0f, 0.0f, 1.0f), 1.0f, .5f, 0.2f, .50f));
	loader.LoadObj("assets/sphere_small.obj", float3(-1,.5f,-2), new Material(float3(0.0f, .0f, .0f), 1.0f, .0f, 0, 0.10f));
	//
	//loader.LoadObj("assets/sphere.obj", float3(5,-.5f,3.0f), new Material(float3(1.0f, 1, 1), 1.0f, .50f, 0, 1.0f));
	////loader.LoadObj("assets/sphere_big.obj", float3(8,0,-30), new Material(float3(1.0f, 0, 0), .4f, 1.00f, 0.5f, 0.0f));
	//
	//loader.LoadObj("assets/plane.obj", float3(2,1,2), new Material(new Texture("assets/woodwall.jpg"), .5f, 0.0f, 0.0f, 1.0f));
	loader.LoadObj("assets/stone.obj", float3(7.5f,1.5f,-0), new Material(new Texture("assets/stone.jpg"), .5f, 0.0f, 0.0f, 1.0f));
	loader.LoadObj("assets/stone.obj", float3(0,1.5f,-0), new Material(new Texture("assets/stone.jpg"), .5f, 0.0f, 0.0f, 1.0f));
	loader.LoadObj("assets/wall1.obj", float3(0,0,-5), new Material(new Texture("assets/woodwall.jpg"), .5f, 0.0f, 0.0f, 1.0f));
	loader.LoadObj("assets/wall2.obj", float3(0,0,5), new Material(new Texture("assets/woodwall.jpg"), .5f, 0.0f, 0.0f, 1.0f));


	
	loader.LoadObj("assets/german_shepard/dog.obj", float3(-2,0.8f,0), new Material(new Texture("assets/german_shepard/body_d.tga"), .5f, 0.0f, .0f, 1.0f));

	rootNode = new BVHNode(0);
	rootNode->triList = m_PrimList;
	rootNode->GenerateBounds();
	if(rootNode->triList.size() > MAXTRIS) rootNode->Split();

}
void Scene::Intersect( Ray& _Ray )
{
	_Ray.invD = 1/_Ray.D;
	_Ray.nodecount = 0;
	rootNode->Traverse(_Ray);
}

void Scene::Draw2D()
{
	// arrows
	const int cx = SCRWIDTH / 2, w = SCRWIDTH / 2, y2 = SCRHEIGHT - 1;
	screen->ClipTo( 0, 0, SCRWIDTH - 1, SCRHEIGHT - 1 );
	screen->Line( cx, 0, cx, y2, 0xffffff );
	screen->Line( cx + 9, y2 - 6, cx + 9, y2 - 40, 0xffffff );
	screen->Line( cx + 9, y2 - 40, cx + 6, y2 - 37, 0xffffff );
	screen->Line( cx + 9, y2 - 40, cx + 12, y2 - 37, 0xffffff );
	screen->Line( cx + 9, y2 - 6, cx + 43, y2 - 6, 0xffffff );
	screen->Line( cx + 43, y2 - 6, cx + 40, y2 - 3, 0xffffff );
	screen->Line( cx + 43, y2 - 6, cx + 40, y2 - 9, 0xffffff );
	screen->Print( "z", cx + 7, y2 - 50, 0xffffff );
	screen->Print( "x", cx + 48, y2 - 8, 0xffffff );
	// 2D viewport
	Line2D( 0, -10000, 0, 10000, 0x004400 );
	Line2D( -10000, 0, 10000, 0, 0x004400 );
	char t[128];	
	sprintf( t, "%4.1f", WXMIN );
	screen->Print( t, SCRWIDTH / 2 + 2, (int)SY( 0 ) - 7, 0x006600 );
	sprintf( t, "%4.1f", WXMAX );
	screen->Print( t, SCRWIDTH - 30, (int)SY( 0 ) - 7, 0x006600 );
	sprintf( t, "%4.1f", WYMAX );
	screen->Print( t, (int)SX( 0 ) + 2, 2, 0x006600 );
	sprintf( t, "%4.1f", WYMIN );
	screen->Print( t, (int)SX( 0 ) + 2, SCRHEIGHT - 8, 0x006600 );
	for ( unsigned int i = 0; i < m_PrimList.size(); i++ ) m_PrimList[i]->Draw2D();
	for ( int i = 0; i < lightCount; i++ ) lightList[i]->Draw2D();
	//if(rootNode)rootNode->Draw2D();

}

// == Renderer class ==========================================================
//    -----------
Renderer::Renderer()
{
	camera.forward = float3(0,0,1);
	camera.Set( float3( 0, 0, -4 ), 5.0f,0 );
	m_blendCount = 0;
}
Intersection::Intersection()
{
	material = &Test; 
}

float Schlick(const float3& N, float3& D, float n1, float n2)
{
	float cosI = -Dot(N, Normalize(D));
	float r0;
				
	r0 = (n1 - n2) / (n1 + n2);
	r0 *= r0;

	if(n1 > n2) 
	{
		const float n = n1 / n2;
		const float sinT2 = n * n * (1.0f - cosI * cosI);
		if(sinT2 > 1.0f) return 1.0f;
		cosI = sqrtf(1.0f - sinT2);
	}
	const float x = 1.0f - cosI;
	return (r0 + (1.0f - r0) * x * x * x * x * x);
}

float3 Renderer::Trace( Ray& _Ray, int _Depth )
{
	scene.Intersect(_Ray);
	if(BVHVIEW)
	{
		const float c = 10.0f / _Ray.nodecount;
		return float3(c,c,c);
	}
	float3 color = float3(0,0,0);
	if(_Ray.t < 1e34f)
	{
		Ray shadowRay;
		shadowRay.shadowRay = true;
		const float3 N = _Ray.intersection.N;
		float3 mcolor;
		mcolor = _Ray.intersection.material->color;

		if(_Ray.intersection.material->m_Texture != nullptr)
		{
			Texture* texture = _Ray.intersection.material->m_Texture;
			mcolor = ColorToFloat3(texture->m_Surface->GetBuffer()[
				( unsigned int )( _Ray.intersection.u * _Ray.intersection.material->m_Texture->m_Width ) +
				( unsigned int )( _Ray.intersection.v * _Ray.intersection.material->m_Texture->m_Height ) *
				( unsigned int )( _Ray.intersection.material->m_Texture->m_Pitch) ] );
		}

		for(int i = 0; i <_scene->lightCount; i++)
		{
			Light* light = _scene->lightList[i];
			shadowRay.t = 1e34;
			shadowRay.D = Normalize((light->pos + float3(twist.Rand() * light->m_Size * 2,twist.Rand() * light->m_Size * 2,twist.Rand() * light->m_Size * 2) ) - _Ray.intersection.pos);
			float dot = Dot(_Ray.intersection.N, shadowRay.D);
			float I = light->m_Intensity;
			
			// Shadows
			if(dot > 0  && _Ray.intersection.material->density >= 1.0f) // Check if face is not facing away from the light
			{
				float diffuse = 0;
				shadowRay.O = _Ray.intersection.pos + shadowRay.D * EPSILON;
				scene.Intersect(shadowRay);
				if(!(shadowRay.t < Length(light->pos - _Ray.intersection.pos) )) // If length between intersection and light is smaller than the distance of ray origin and light, there is an object between
				{
					if(light->m_Size > 0.0f) // Draw area light if sphere intersects
					{
						// find the intersection of a ray and a sphere
						float t = 1e34;
						float a = Dot( _Ray.D, _Ray.D );
						float b = 2 * Dot( _Ray.D, _Ray.O - light->pos );
						float c = Dot( light->pos, light->pos ) + Dot( _Ray.O, _Ray.O ) - 2 * Dot( light->pos, _Ray.O ) - light->m_Size * light->m_Size;
						float d = b * b - 4 * a * c;
						if (d > 0)	
						{
							float t1 = (-b - sqrtf( d )) / (2 * a);
							float t2 = (-b + sqrtf( d )) / (2 * a);
							if ((t1 < t) && (t1 >= 0)) { t = t1; }
							if ((t2 < t) && (t2 >= 0)) { t = t2; }
							if (t < _Ray.t)
							return light->color;
						}
					}
					diffuse = dot * _Ray.intersection.material->diffuse * (2.0f/(sqrt(SQRDISTANCE(light->pos, shadowRay.O))) );
				}
				 color += diffuse * mcolor * light->color * I;
			}
			if(m_blendCount < 5) return color;
			//Phong
			if(_Ray.intersection.material->spec > 0.0f)
			{
				float3 D = shadowRay.D - _Ray.D;
				float t = sqrtf( Dot(D,D) );
				if(t != 0)
				{
					D = 1.0f / t * D;
					t = MAX(Dot(D, _Ray.intersection.N), 0.0f);
					t =	_Ray.intersection.material->spec * powf(t, 20) * _Ray.intersection.material->reflection * 2;
					color += t * light->color ;
				}
			}
		}
		// Reflection
		if(_Ray.intersection.material->reflection >= 0.0f && _Depth < MAXDEPTH && _Ray.intersection.material->density >= 1.0f)
		{
			const float3 R = _Ray.D - 2.0f * Dot(_Ray.D, N) * N;
			Ray reflRay;
			reflRay.t = 1e34;
			reflRay.D = R;
			reflRay.O = _Ray.intersection.pos + reflRay.D * EPSILON;
			const float3 refl = Trace(reflRay, _Depth+1);
			color +=  _Ray.intersection.material->reflection* float3(refl.x *mcolor.x, refl.y * mcolor.y ,refl.z * mcolor.z);
		}
		// Refraction
		if(_Ray.intersection.material->density < 1.0f)
		{
			float cosI = -Dot(N,Normalize(_Ray.D));	
			float n, scale;
			bool refract = false;
			const float n1 = 1.0003f; // Index of air
			const float n2 = 1.52f;	// Index of glass
			float3 _N = N;
			if(cosI < 0) // Outgoing
			{
				n = n2/n1;
				_N = -1 * N;
				cosI = -Dot(_N, Normalize(_Ray.D));
			}
			else  // Ingoing
			{
				n = n1/n2;
			}
			const float sinT2 = n * n * (1.0f - cosI * cosI);
			if(sinT2 > 1.0f) return color;
			const float cosT = sqrtf(1.0f - sinT2);
			const float roulette = twist.Rand();
			const float schlick = Schlick(_N, _Ray.D, cosI < 0? n2 : n1, cosI < 0? n1 : n2);
			Ray R;
			if(roulette <= schlick) // reflect
			{
				R.D = _Ray.D - 2.0f * Dot(_Ray.D, _N) * _N;
				scale = 1.0f / schlick;
			}
			else if (roulette <= (1.0f - _Ray.intersection.material->reflection) + schlick) //Refract
			{
					R.D = n * Normalize(_Ray.D) + (n * cosI - cosT) * _N;
					refract = true;
			}
			R.t = 1e34;
			R.O = _Ray.intersection.pos + R.D * EPSILON;
		
			float3 c = Trace(R, _Depth+1);
			if(refract && cosI > 0)
			{
				const float3 absorption =  (_Ray.intersection.material->color) * ( _Ray.intersection.material->density) * - R.t ;
				const float3 transparency =  float3( expf(absorption.x),expf(absorption.y),expf(absorption.z));
				c *= transparency;
			}
			color =  c;
		}
	}
	else
	{
		for(unsigned int i = 0; i < scene.lightCount; i++)
		{
			Light* light = scene.lightList[i];
			if(light->m_Size > 0.0f)
			{
				// find the intersection of a ray and a sphere
				float a = Dot( _Ray.D, _Ray.D );
				float b = 2 * Dot( _Ray.D, _Ray.O - light->pos );
				float c = Dot( light->pos, light->pos ) + Dot( _Ray.O, _Ray.O ) - 2 * Dot( light->pos, _Ray.O ) - light->m_Size * light->m_Size;
				float d = b * b - 4 * a * c;
				if (d > 0)	return light->color;
			}
		}

		return float3(1,1,1);
	}
	return color;
}
Renderer::~Renderer() { }
void Renderer::Render( int _Y )
{
	for ( int x = 0; x < SCRWIDTH / 2; x++ )
	{
		Ray ray;
		float3 color;
		int _x = 0;
		int _y = 0;
		if(ANTIALIAS && m_blendCount < 128)
		{
			_x = twist.Rand()*4 - 2;
			_y = twist.Rand()*4 - 2;
		}

		camera.GenerateRay( ray, x + _x , _Y + _y );		
		color = Trace(ray, 0);
		
		//if ((_Y == (SCRHEIGHT / 2)) && ((x % 32) == 0)) ray.Draw2D();
		if(m_blendCount > 5) color += buffer[x + (_Y) * SCRWIDTH];
		buffer[x + _Y * SCRWIDTH] =  color ;
		if(m_blendCount > 5) color /= m_blendCount;
		screen->GetBuffer()[x + _Y * SCRWIDTH] = ConvertFPColor( color );
	}
}

Light::Light(float3 _Pos, float3 _Color, float _I, float _Size)
{
	pos = _Pos;
	color = _Color;
	m_Intensity = _I;
	m_Size = _Size;
}

void Light::Draw2D()
{
	Line2D(pos.x, pos.z - 0.1f, pos.x , pos.z + 0.1f, ConvertFPColor(color));
	Line2D(pos.x + 0.1f, pos.z, pos.x - 0.1f, pos.z, ConvertFPColor(color));


}

bool ObjLoader::LoadObj(char* a_file, float3 p, Material* mat)
{
	std::vector<float3> vertices;
	std::vector<int3> tris;
	std::vector<float3> normals;
	std::vector<int3> normals_index;
	std::vector<float2> uvs;
	std::vector<int3> uvs_index;
	static int triangleCount = 0;

	FILE* file = fopen(a_file, "r");
	if( file == NULL)
	{
		printf("Failed to open file\n");
		return false;
	}
	while ( 1 )
	{
		char lineHeader[128];
		int res = fscanf(file, "%s", lineHeader);
		if( res == EOF ) break;

		if(strcmp( lineHeader, "v" ) == 0)
		{
			float3 vtx;
			fscanf(file, "%f %f %f\n", &vtx.x, &vtx.y, &vtx.z);
			vtx.y *= -1;
			vertices.push_back(vtx);
		}
		else if(strcmp( lineHeader, "vn" ) == 0)
		{
			float3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			normal.y *= -1;
			normals.push_back(normal);
		}
		else if(strcmp (lineHeader, "vt") == 0)
		{
			float2 uv;
			fscanf(file, "%f %f %f\n", &uv.x, &uv.y);
			uv.y = 1.0f - uv.y;
			uvs.push_back(uv);
		}
		else if(strcmp( lineHeader, "f" ) == 0) // Read faces: 3  vertices, 3  normals (and uv)
		{
			int3 tri;
			int3 norms;
			int3 uv;
			fscanf(file, "%i/%i/%i %i/%i/%i %i/%i/%i\n", &tri.x, &uv.x, &norms.x, &tri.y, &uv.y, &norms.y, &tri.z, &uv.z, &norms.z);
			tris.push_back(tri);
			normals_index.push_back(norms);
			uvs_index.push_back(uv);
		}
		//else if(strcmp( lineHeader, "mtllib") == 0)
		//{
		//	char mtl[16];
		//	fscanf(file, "%s\n", &mtl);
		//	FILE* file2 = fopen(mtl, "r");
		//	char lineHeader2[128];
		//	if(strcmp (lineHeader2, "map_Kd") == 0)
		//	{
		//		char file3[16];
		//		fscanf(file2, "%c", &file3);
		//		
		//	}
		//	fclose( file2 );
		//}
		//else if(strcmp( lineHeader, "usemtl") == 0)
		//{
		//	char mtl[16];
		//	fscanf(file, "%c", &mtl);

		//}
	}

	fclose( file );

	for(unsigned int i = 0; i < tris.size(); i++)
	{
		const float3 p2 = vertices[tris[i].x-1] + p;
		const float3 p1 = vertices[tris[i].y-1] + p;
		const float3 p0 = vertices[tris[i].z-1] + p;
		Triangle* t = new Triangle(p0, p1, p2, mat);
		t->VN[2] = normals[normals_index[i].x-1];
		t->VN[1] = normals[normals_index[i].y-1];
		t->VN[0] = normals[normals_index[i].z-1];
		float3 N =  normals[normals_index[i].x-1] +  normals[normals_index[i].y-1] +  normals[normals_index[i].z-1];
		t->N = Normalize(N);
		t->UV[2] = uvs[uvs_index[i].x - 1];
		t->UV[1] = uvs[uvs_index[i].y - 1];
		t->UV[0] = uvs[uvs_index[i].z - 1];

		_scene->m_PrimList.push_back(t);
	}

	triangleCount += tris.size();

	return true;
}

// EOF
