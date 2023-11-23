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

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { 0.0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	std::fill_n(m_pBackBufferPixels, m_Width * m_Height, 0);
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
		Vertex out{};
		out.color = vertices_in[index].color;
		out.position = m_Camera.viewMatrix.TransformPoint(vertices_in[index].position);

		out.position.x = out.position.x / (ratio * m_Camera.fov * out.position.z);
		out.position.y = out.position.y / (m_Camera.fov * out.position.z);

		out.position.x = (out.position.x + 1) / 2 * m_Width;
		out.position.y = (1 - out.position.y) / 2 * m_Height;

		vertices_out.push_back(out);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::Render_W1_Part1()
{
	
	std::vector<Vertex> vertices_ndc
	{
		{ {  0.0f,  2.0f,  0.0f }, colors::Red },
		{ {  1.5f, -1.0f,  0.0f }, colors::Red },
		{ { -1.5f, -1.0f,  0.0f }, colors::Red },
		{ {  0.0f,  4.0f,  2.0f }, colors::Red	 },
		{ {  3.0f, -2.0f,  2.0f }, colors::Green },
		{ { -3.0f, -2.0f,  2.0f }, colors::Blue  },

	};

	int			verticesNDCSize	{ int(vertices_ndc.size()) };
	float		totalWeight		{};
	Vector2		v0v1			{};
	Vector2		v0v2			{};
	Vector2		pixel			{};
	ColorRGB	finalColor		{};
	verticesNDCSize -= vertices_ndc.size() % 3;

	std::vector<Vertex> vertices_ScreenSpace{};
	//vertices_ScreenSpace.resize(verticesNDCSize);
	std::vector<float> vertices_weights{};
	vertices_weights.resize(verticesNDCSize);
	
	VertexTransformationFunction(vertices_ndc, vertices_ScreenSpace);

	//RENDER LOGIC
	for (int vertexIndex{}; vertexIndex < verticesNDCSize; vertexIndex += 3)
	{
		int minX = int(std::min({ vertices_ScreenSpace[vertexIndex + 0].position.x, vertices_ScreenSpace[vertexIndex + 1].position.x, vertices_ScreenSpace[vertexIndex + 2].position.x }));
		int minY = int(std::min({ vertices_ScreenSpace[vertexIndex + 0].position.y, vertices_ScreenSpace[vertexIndex + 1].position.y, vertices_ScreenSpace[vertexIndex + 2].position.y }));
		int maxX = int(std::max({ vertices_ScreenSpace[vertexIndex + 0].position.x, vertices_ScreenSpace[vertexIndex + 1].position.x, vertices_ScreenSpace[vertexIndex + 2].position.x }));
		int maxY = int(std::max({ vertices_ScreenSpace[vertexIndex + 0].position.y, vertices_ScreenSpace[vertexIndex + 1].position.y, vertices_ScreenSpace[vertexIndex + 2].position.y }));

		minX = std::clamp(minX, 0, m_Width);
		maxX = std::clamp(maxX, 0, m_Width);
		minY = std::clamp(minY, 0, m_Height);
		maxY = std::clamp(maxY, 0, m_Height);

		for (int px{minX}; px < maxX; ++px)
		{
			for (int py{minY}; py < maxY; ++py)
			{
				finalColor = ColorRGB(0.0f, 0.0f, 0.0f);
				pixel = Vector2(px + 0.5f, py + 0.5f);
				
				if (m_pDepthBufferPixels[px + (py * m_Height)] > vertices_ScreenSpace[vertexIndex].position.z)
				{
					vertices_weights[vertexIndex + 0] = Vector2::Cross(vertices_ScreenSpace[vertexIndex + 1].position.GetXY() - vertices_ScreenSpace[vertexIndex + 0].position.GetXY(), pixel - vertices_ScreenSpace[vertexIndex + 0].position.GetXY());
					vertices_weights[vertexIndex + 1] = Vector2::Cross(vertices_ScreenSpace[vertexIndex + 2].position.GetXY() - vertices_ScreenSpace[vertexIndex + 1].position.GetXY(), pixel - vertices_ScreenSpace[vertexIndex + 1].position.GetXY());
					vertices_weights[vertexIndex + 2] = Vector2::Cross(vertices_ScreenSpace[vertexIndex + 0].position.GetXY() - vertices_ScreenSpace[vertexIndex + 2].position.GetXY(), pixel - vertices_ScreenSpace[vertexIndex + 2].position.GetXY());

					if (vertices_weights[vertexIndex + 0] > 0 and vertices_weights[vertexIndex + 1] > 0 and vertices_weights[vertexIndex + 2] > 0 /*or
						vertices_weights[vertexIndex + 0] < 0 and vertices_weights[vertexIndex + 1] < 0 and vertices_weights[vertexIndex + 2] < 0*/)
					{
						totalWeight = vertices_weights[vertexIndex + 0] + vertices_weights[vertexIndex + 1] + vertices_weights[vertexIndex + 2];
						finalColor = ColorRGB(vertices_weights[vertexIndex + 0] * vertices_ScreenSpace[vertexIndex + 0].color + vertices_weights[vertexIndex + 1] * vertices_ScreenSpace[vertexIndex + 1].color + vertices_weights[vertexIndex + 2] * vertices_ScreenSpace[vertexIndex + 2].color)/ totalWeight;
						m_pDepthBufferPixels[px + (py * m_Height)] = vertices_ScreenSpace[vertexIndex].position.z;
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

	}

}

