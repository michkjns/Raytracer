// Template for GP1, version 1
// IGAD/NHTV - Jacco Bikker - 2006-2013

#include "string.h"
#include "game.h"
#include "surface.h"
#include "stdlib.h"
#include <Windows.h>
#include "template.h"
#include "raytracer.h"
#include "jobmanager.h"

using namespace Tmpl8;

Surface* screen = 0;
float3 buffer[SCRWIDTH * SCRHEIGHT];
Renderer renderer;
JobManager* jobmanager = JobManager::CreateManager(NUMTHREADS);

bool wdown = false;
bool sdown = false;
bool adown = false;
bool ddown = false;
bool keydown = false;


bool render = true;

void Game::Init()
{
	screen = new Surface( SCRWIDTH, SCRHEIGHT );
	screen->InitCharset();
	jobmanager->StartThreads();
	srand(time(0));


}

void RenderLine(void* param)
{
	int y = (int) param;
	renderer.Render( y );
}

void Game::Tick( float a_DT )
{
	//if(renderer.m_blendCount == 0)
	//{
	//	for(unsigned int x = 0; x < SCRWIDTH/2; x++) for(unsigned int y = 0; y < SCRHEIGHT; y++)
	//		buffer[x + y * SCRWIDTH] = float3(0,0,0);
	//}
	renderer.m_blendCount ++;
	//screen->Clear( 0 );
	for(int i = 0; i < SCRHEIGHT; i++)
	{
		jobmanager->AddJobb(&RenderLine, (void*) i);
	}

	screen->Bar(SCRWIDTH/2, 1, SCRWIDTH-1, SCRHEIGHT-1, 0);
	renderer.scene.Draw2D();
	renderer.camera.Draw2D();
	
	screen->ClipTo( SCRWIDTH / 2, 0, SCRWIDTH - 1, SCRHEIGHT - 1 );
	screen->ClipTo( 0, 0, SCRWIDTH / 2, SCRHEIGHT - 1 );
	screen->CopyTo( m_Surface, 0, 0 );
	jobmanager->Wait();
	if(GetAsyncKeyState(VK_UP))
	{
		renderer.camera.Set(float3(0,0,0), -1.5f,0);
		
	}
	if(GetAsyncKeyState(VK_DOWN))
	{
		renderer.camera.Set(float3(0,0,0), 1.5f,0);
		
	}
	if(GetAsyncKeyState(VK_LEFT))
	{
		renderer.camera.Set(float3(0,0,0),0, 1.5f);
	}
	if(GetAsyncKeyState(VK_RIGHT))
	{
		renderer.camera.Set(float3(0,0,0),0, -1.5f);
	}
	
	if(GetAsyncKeyState(VK_SPACE))
	{
		renderer.camera.Set(float3(0,-.1f,0),0, 0);
	}
	if(GetAsyncKeyState(VK_CONTROL))
	{
		renderer.camera.Set(float3(0,.1f,0),0, 0);
	}

	if(wdown) renderer.camera.Set(float3(0,0,0.1f),0, 0);
	if(sdown) renderer.camera.Set(float3(0,0,-0.1f),0, 0);
	if(adown) renderer.camera.Set(float3(.1f,0,0), 0,0);
	if(ddown) renderer.camera.Set(float3(-.1f,0,0), 0,0);

	if(keydown)
	{
		renderer.m_blendCount = 0;
	}
	
}

void Game::KeyDown(unsigned int c)
{
	//printf("%i\n", c);

	if(c == 17) wdown = true;
	if(c == 16)
	{
		renderer.camera.m_focalDepth -= .3f;
		printf("focal depth set to: %f\n", renderer.camera.m_focalDepth);
	}
	if(c == 18)
	{
		renderer.camera.m_focalDepth += .3f;
		printf("focal depth set to: %f\n", renderer.camera.m_focalDepth);
	}
	if(c == 31)  sdown = true;
	if(c == 30)  adown = true;
	if(c == 32)  ddown = true;
	keydown = true;
}

void Game::KeyUp(unsigned int c)
{
	
	if(c == 17) wdown = false;
	if(c == 30)  adown = false;
	if(c == 32)  ddown = false;
	if(c == 31)  sdown = false;
	if(!wdown && !adown && !ddown && !sdown) keydown = false;

	renderer.m_blendCount = 0;
}

