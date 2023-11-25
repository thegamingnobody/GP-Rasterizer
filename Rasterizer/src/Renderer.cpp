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

	m_Texture = m_Texture->LoadFromFile("Resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
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

	Render_W7();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
	float ratio{ static_cast<float>(m_Width) / m_Height };

	for (int index = 0; index < m_TrianglesVertexIndices.size(); index++)
	{
		Vertex out{};
		out.color = vertices_in[m_TrianglesVertexIndices[index]].color;
		out.uv = vertices_in[m_TrianglesVertexIndices[index]].uv;
		out.position = m_Camera.viewMatrix.TransformPoint(vertices_in[m_TrianglesVertexIndices[index]].position);

		out.position.x = out.position.x / ((ratio) * m_Camera.fov * out.position.z);
		out.position.y = out.position.y / (m_Camera.fov * out.position.z);

		out.position.x = ((out.position.x + 1) / 2) * static_cast<float>(m_Width);
		out.position.y = ((1 - out.position.y) / 2) * static_cast<float>(m_Height);

		vertices_out.push_back(out);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::InitializeTriangles(std::vector<Vertex>& verticesNDC, std::vector<int>& trianglesVertexIndices)
{
	verticesNDC.clear();
	trianglesVertexIndices.clear();
#pragma region W6

	//verticesNDC.push_back({ {  0.0f,  2.0f,  0.0f }, colors::Red });
	//verticesNDC.push_back({ {  1.5f, -1.0f,  0.0f }, colors::Red });
	//verticesNDC.push_back({ { -1.5f, -1.0f,  0.0f }, colors::Red });

	//verticesNDC.push_back({ {  0.0f,  4.0f,  2.0f }, colors::Red   });
	//verticesNDC.push_back({ {  3.0f, -2.0f,  2.0f }, colors::Green });
	//verticesNDC.push_back({ { -3.0f, -2.0f,  2.0f }, colors::Blue  });

	//trianglesVertexIndices.push_back(0);
	//trianglesVertexIndices.push_back(1);
	//trianglesVertexIndices.push_back(2);
	//trianglesVertexIndices.push_back(2);
	//trianglesVertexIndices.push_back(3);
	//trianglesVertexIndices.push_back(3);
	//trianglesVertexIndices.push_back(4);
	//trianglesVertexIndices.push_back(5);
#pragma endregion

#pragma region W7
	verticesNDC.push_back({ {-3,  3, -2}, colors::White, { 0.0f,  0.0f} }) ;
	verticesNDC.push_back({ { 0,  3, -2}, colors::White, { 0.5f,  0.0f} });
	verticesNDC.push_back({ { 3,  3, -2}, colors::White, { 1.0f,  0.0f} });
	verticesNDC.push_back({ {-3,  0, -2}, colors::White, { 0.0f,  0.5f} }) ;
	verticesNDC.push_back({ { 0,  0, -2}, colors::White, { 0.5f,  0.5f} }) ;
	verticesNDC.push_back({ { 3,  0, -2}, colors::White, { 1.0f,  0.5f} });
	verticesNDC.push_back({ {-3, -3, -2}, colors::White, { 0.0f,  1.0f} });
	verticesNDC.push_back({ { 0, -3, -2}, colors::White, { 0.5f,  1.0f} });
	verticesNDC.push_back({ { 3, -3, -2}, colors::White, { 1.0f,  1.0f} });

	trianglesVertexIndices.push_back(3);
	trianglesVertexIndices.push_back(0); 
	trianglesVertexIndices.push_back(4);
	trianglesVertexIndices.push_back(1);	
	trianglesVertexIndices.push_back(5);
	trianglesVertexIndices.push_back(2);

	trianglesVertexIndices.push_back(2);
	trianglesVertexIndices.push_back(6);

	trianglesVertexIndices.push_back(6);
	trianglesVertexIndices.push_back(3);
	trianglesVertexIndices.push_back(7);
	trianglesVertexIndices.push_back(4);
	trianglesVertexIndices.push_back(8);
	trianglesVertexIndices.push_back(5);

#pragma endregion
}

void Renderer::Render_W7()
{
	InitializeTriangles(m_VerticesNDC, m_TrianglesVertexIndices);

	float		totalWeight		{};
	Vector2		v0v1			{};
	Vector2		v0v2			{};
	Vector2		pixel			{};
	ColorRGB	finalColor		{};
	int swapOddVertices1		{};
	int swapOddVertices2		{};

	std::vector<Vertex> vertices_ScreenSpace{};
	std::vector<float> vertices_weights{};
	vertices_weights.resize(m_TrianglesVertexIndices.size());
	
	VertexTransformationFunction(m_VerticesNDC, vertices_ScreenSpace);

	//RENDER LOGIC
	for (int vertexIndex{}; vertexIndex < (m_TrianglesVertexIndices.size()-2); vertexIndex++)
	{
		if (m_TrianglesVertexIndices[vertexIndex + 0] == m_TrianglesVertexIndices[vertexIndex + 1] or m_TrianglesVertexIndices[vertexIndex + 0] == m_TrianglesVertexIndices[vertexIndex + 2] or m_TrianglesVertexIndices[vertexIndex + 1] == m_TrianglesVertexIndices[vertexIndex + 2])
		{
			continue;
		}

		int minX = int(std::min({ vertices_ScreenSpace[vertexIndex + 0].position.x, vertices_ScreenSpace[vertexIndex + 1].position.x, vertices_ScreenSpace[vertexIndex + 2].position.x }));
		int minY = int(std::min({ vertices_ScreenSpace[vertexIndex + 0].position.y, vertices_ScreenSpace[vertexIndex + 1].position.y, vertices_ScreenSpace[vertexIndex + 2].position.y }));
		int maxX = int(std::max({ vertices_ScreenSpace[vertexIndex + 0].position.x, vertices_ScreenSpace[vertexIndex + 1].position.x, vertices_ScreenSpace[vertexIndex + 2].position.x }));
		int maxY = int(std::max({ vertices_ScreenSpace[vertexIndex + 0].position.y, vertices_ScreenSpace[vertexIndex + 1].position.y, vertices_ScreenSpace[vertexIndex + 2].position.y }));

		minX = std::clamp(minX, 0, m_Width) - 5;
		maxX = std::clamp(maxX, 0, m_Width) + 5;
		minY = std::clamp(minY, 0, m_Height) - 5;
		maxY = std::clamp(maxY, 0, m_Height) + 5;

		if (vertexIndex & 1)
		{
			swapOddVertices1 = 2;
			swapOddVertices2 = 1;
		}
		else
		{
			swapOddVertices1 = 1;
			swapOddVertices2 = 2; 
		}

		for (int px{minX}; px < maxX; ++px)
		{
			for (int py{minY}; py < maxY; ++py)
			{
				finalColor = ColorRGB(0.0f, 0.0f, 0.0f);
				pixel = Vector2(px + 0.5f, py + 0.5f);
				
				if (m_pDepthBufferPixels[px + (py * m_Width)] > vertices_ScreenSpace[vertexIndex].position.z)
				{
					vertices_weights[vertexIndex + 0]				 = Vector2::Cross(vertices_ScreenSpace[vertexIndex + swapOddVertices1].position.GetXY() - vertices_ScreenSpace[vertexIndex + 0].position.GetXY(),				 pixel - vertices_ScreenSpace[vertexIndex + 0].position.GetXY());
					vertices_weights[vertexIndex + swapOddVertices1] = Vector2::Cross(vertices_ScreenSpace[vertexIndex + swapOddVertices2].position.GetXY() - vertices_ScreenSpace[vertexIndex + swapOddVertices1].position.GetXY(), pixel - vertices_ScreenSpace[vertexIndex + swapOddVertices1].position.GetXY());
					vertices_weights[vertexIndex + swapOddVertices2] = Vector2::Cross(vertices_ScreenSpace[vertexIndex + 0].position.GetXY()				- vertices_ScreenSpace[vertexIndex + swapOddVertices2].position.GetXY(), pixel - vertices_ScreenSpace[vertexIndex + swapOddVertices2].position.GetXY());

					if (vertices_weights[vertexIndex + 0] < 0 or vertices_weights[vertexIndex + 1] < 0 or vertices_weights[vertexIndex + 2] < 0) continue;

					totalWeight = vertices_weights[vertexIndex + 0] + vertices_weights[vertexIndex + 1] + vertices_weights[vertexIndex + 2];

					vertices_weights[vertexIndex + 0]				 = vertices_weights[vertexIndex + 0]				/ totalWeight;
					vertices_weights[vertexIndex + swapOddVertices1] = vertices_weights[vertexIndex + swapOddVertices1] / totalWeight;
					vertices_weights[vertexIndex + swapOddVertices2] = vertices_weights[vertexIndex + swapOddVertices2] / totalWeight;
					totalWeight	  = vertices_weights[vertexIndex + 0] + vertices_weights[vertexIndex + 1] + vertices_weights[vertexIndex + 2];

					float interpolatedZ = 1 / ((vertices_weights[vertexIndex + 0] / vertices_ScreenSpace[vertexIndex + swapOddVertices2].position.z) + (vertices_weights[vertexIndex + swapOddVertices1] / vertices_ScreenSpace[vertexIndex + 0].position.z) + (vertices_weights[vertexIndex + swapOddVertices2] / vertices_ScreenSpace[vertexIndex + swapOddVertices1].position.z));
					if (m_Texture)
					{
						Vector2 interpolatedUV = (((vertices_ScreenSpace[vertexIndex + 0].uv				/ vertices_ScreenSpace[vertexIndex + 0].position.z)					* vertices_weights[vertexIndex + swapOddVertices1])	+
												  ((vertices_ScreenSpace[vertexIndex + swapOddVertices1].uv	/ vertices_ScreenSpace[vertexIndex + swapOddVertices1].position.z)	* vertices_weights[vertexIndex + swapOddVertices2]) +
												  ((vertices_ScreenSpace[vertexIndex + swapOddVertices2].uv	/ vertices_ScreenSpace[vertexIndex + swapOddVertices2].position.z)	* vertices_weights[vertexIndex + 0])) * interpolatedZ;
						float test{ interpolatedUV.x };
						float test2{ interpolatedUV.y };
						finalColor = m_Texture->Sample(interpolatedUV);
					}
					else
					{
						finalColor = ColorRGB(vertices_weights[vertexIndex + 0] * vertices_ScreenSpace[vertexIndex + 0].color + vertices_weights[vertexIndex + 1] * vertices_ScreenSpace[vertexIndex + 1].color + vertices_weights[vertexIndex + 2] * vertices_ScreenSpace[vertexIndex + 2].color);
					}

					//m_pDepthBufferPixels[px + (py * m_Width)] = vertices_ScreenSpace[vertexIndex].position.z;
					m_pDepthBufferPixels[px + (py * m_Width)] = interpolatedZ;


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