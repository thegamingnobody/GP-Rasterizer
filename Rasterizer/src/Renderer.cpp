//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	Render_W1_Part1();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

inline bool PointInTriangle(const Vector2& point, const Vector2& vertexOrigin, const Vector2& vertexNext)
{
	Vector2 eVector{ vertexNext - vertexOrigin };
	Vector2 vectorToP{ point - vertexOrigin };

	float crossProduct{ Vector2::Cross(eVector, vectorToP) };

	if (crossProduct < 0)
	{
		return false;
	}

	return true;
}


void Renderer::Render_W1_Part1()
{
	std::vector<Vector3> vertices_ndc
	{
		{  0.0f,  0.5f,  1.0f },
		{  0.5f, -0.5f,  1.0f },
		{ -0.5f, -0.5f,  1.0f }
	};

	std::vector<Vector2> vertices_ScreenSpace{};

	for (int i = 0; i < vertices_ndc.size(); i++)
	{
		Vector2 tempVector{ (vertices_ndc[i].x + 1) / 2 * m_Width, (1 - vertices_ndc[i].y) / 2 * m_Height };
		vertices_ScreenSpace.emplace_back(tempVector);
	}

	Vector2 v0v1		{};
	Vector2 v0v2		{};
	Vector2 pixel		{};
	ColorRGB finalColor	{};

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			finalColor = ColorRGB(1.0f, 1.0f, 1.0f);
			pixel = Vector2(px + 0.5f, py + 0.5f);

			if (!PointInTriangle(pixel, vertices_ScreenSpace[0], vertices_ScreenSpace[1]) or
				!PointInTriangle(pixel, vertices_ScreenSpace[1], vertices_ScreenSpace[2]) or
				!PointInTriangle(pixel, vertices_ScreenSpace[2], vertices_ScreenSpace[0]))
			{
				finalColor = ColorRGB(0.0f, 0.0f, 0.0f);
			}

			//Update Color in Buffer
			finalColor.MaxToOne();
			//finalColor.ToneMap();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
																  static_cast<uint8_t>(finalColor.r * 255),
																  static_cast<uint8_t>(finalColor.g * 255),
																  static_cast<uint8_t>(finalColor.b * 255));
		}
	}

}

