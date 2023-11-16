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
	m_Camera.Initialize(60.f, { 1.0f,.0f,-10.f });
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
	int ratio{ m_Width / m_Height };

	for (int index = 0; index < vertices_in.size(); index++)
	{
		vertices_out[index].position = m_Camera.viewMatrix.TransformPoint(vertices_in[index].position);

		vertices_out[index].position.x = vertices_out[index].position.x / (ratio * m_Camera.fov * vertices_out[index].position.z);
		vertices_out[index].position.y = vertices_out[index].position.y / (m_Camera.fov * vertices_out[index].position.z);

		vertices_out[index].position.x = (vertices_out[index].position.x + 1) / 2 * m_Width;
		vertices_out[index].position.y = (1 - vertices_out[index].position.y) / 2 * m_Height;
	}
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
	std::vector<Vertex> vertices_ndc
	{
		{ {  0.0f,  0.5f,  1.0f }, colors::White },
		{ {  0.5f, -0.5f,  1.0f }, colors::White },
		{ { -0.5f, -0.5f,  1.0f }, colors::White }
	};

	std::vector<Vertex> vertices_ScreenSpace{};
	vertices_ScreenSpace.resize(3);
	
	VertexTransformationFunction(vertices_ndc, vertices_ScreenSpace);

	Vector2 v0v1		{};
	Vector2 v0v2		{};
	Vector2 pixel		{};
	ColorRGB finalColor	{};

	//int minX = std::min({ vertices_ScreenSpace[0].x, vertices_ScreenSpace[1].x, vertices_ScreenSpace[2].x });
	//int minY = std::min({ vertices_ScreenSpace[0].y, vertices_ScreenSpace[1].y, vertices_ScreenSpace[2].y });
	//int maxX = std::max({ vertices_ScreenSpace[0].x, vertices_ScreenSpace[1].x, vertices_ScreenSpace[2].x });
	//int maxY = std::max({ vertices_ScreenSpace[0].y, vertices_ScreenSpace[1].y, vertices_ScreenSpace[2].y });

	//RENDER LOGIC
	for (int px{0}; px < m_Width; ++px)
	{
		for (int py{0}; py < m_Height; ++py)
		{
			finalColor = ColorRGB(1.0f, 1.0f, 1.0f);
			pixel = Vector2(px + 0.5f, py + 0.5f);

			if (!PointInTriangle(pixel, vertices_ScreenSpace[0].position.GetXY(), vertices_ScreenSpace[1].position.GetXY()) or
				!PointInTriangle(pixel, vertices_ScreenSpace[1].position.GetXY(), vertices_ScreenSpace[2].position.GetXY()) or
				!PointInTriangle(pixel, vertices_ScreenSpace[2].position.GetXY(), vertices_ScreenSpace[0].position.GetXY()))
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

