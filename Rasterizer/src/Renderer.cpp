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
	m_Camera.Initialize(60.f, { 0.0f,.0f,-10.f }, (static_cast<float>(m_Width) / m_Height));

	m_Texture = m_Texture->LoadFromFile("Resources/tuktuk.png");

	m_Mesh = new Mesh();

	//InitializeTriangles(m_Mesh->vertices, m_Mesh->indices);
	Utils::ParseOBJ("Resources/tuktuk.obj", m_Mesh->vertices, m_Mesh->indices);
	m_Mesh->primitiveTopology = PrimitiveTopology::TriangleList;

	const Vector3 translation{ 0.0f, -5.0f, 10.0f };
	m_Mesh->Translate(translation);

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

	m_Mesh->Update();
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

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const
{
	//Todo > W1 Projection Stage
	float ratio{ float(m_Width) / m_Height };
	float fovTimesZ{};
	Vertex_Out out{};

	Matrix finalMatrix = m_Mesh->worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

	vertices_out.resize(m_Mesh->indices.size());

	for (int index = 0; index < m_Mesh->indices.size(); index++)
	{
		out.color = vertices_in[m_Mesh->indices[index]].color;
		out.uv = vertices_in[m_Mesh->indices[index]].uv;
		out.position = finalMatrix.TransformPoint(vertices_in[m_Mesh->indices[index]].position.ToPoint4());

		fovTimesZ = m_Camera.fov * out.position.z;

		out.position.x = out.position.x / out.position.z;
		out.position.y = out.position.y / out.position.z;

		out.position.x = ((out.position.x + 1) / 2) * float(m_Width);
		out.position.y = ((1 - out.position.y) / 2) * float(m_Height);

		vertices_out[index] = out;
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::InitializeTriangles(std::vector<Vertex>& verticesNDC, std::vector<uint32_t>& trianglesVertexIndices)
{
#pragma region W7
	m_Mesh->vertices.push_back({ {-3,  3, -2}, colors::White, { 0.0f,  0.0f} });
	m_Mesh->vertices.push_back({ { 0,  3, -2}, colors::White, { 0.5f,  0.0f} });
	m_Mesh->vertices.push_back({ { 3,  3, -2}, colors::White, { 1.0f,  0.0f} });
	m_Mesh->vertices.push_back({ {-3,  0, -2}, colors::White, { 0.0f,  0.5f} });
	m_Mesh->vertices.push_back({ { 0,  0, -2}, colors::White, { 0.5f,  0.5f} });
	m_Mesh->vertices.push_back({ { 3,  0, -2}, colors::White, { 1.0f,  0.5f} });
	m_Mesh->vertices.push_back({ {-3, -3, -2}, colors::White, { 0.0f,  1.0f} });
	m_Mesh->vertices.push_back({ { 0, -3, -2}, colors::White, { 0.5f,  1.0f} });
	m_Mesh->vertices.push_back({ { 3, -3, -2}, colors::White, { 1.0f,  1.0f} });

	m_Mesh->indices.push_back(3);
	m_Mesh->indices.push_back(0); 
	m_Mesh->indices.push_back(4);
	m_Mesh->indices.push_back(1);	
	m_Mesh->indices.push_back(5);
	m_Mesh->indices.push_back(2);

	m_Mesh->indices.push_back(2);
	m_Mesh->indices.push_back(6);

	m_Mesh->indices.push_back(6);
	m_Mesh->indices.push_back(3);
	m_Mesh->indices.push_back(7);
	m_Mesh->indices.push_back(4);
	m_Mesh->indices.push_back(8);
	m_Mesh->indices.push_back(5);

#pragma endregion

	m_Mesh->vertices_out.resize(m_Mesh->indices.size());
}

void Renderer::Render_W7()
{
	float		interpolatedZ	{};
	float		totalWeight		{};
	Vector2		pixel			{};
	Vector2		interpolatedUV	{};
	ColorRGB	finalColor		{};
	int swapOddVertices1		{};
	int swapOddVertices2		{};
	int pixelIndex				{};

	std::vector<float> vertices_weights{};
	vertices_weights.resize(m_Mesh->indices.size());
	
	VertexTransformationFunction(m_Mesh->vertices, m_Mesh->vertices_out);

	//RENDER LOGIC
	switch (m_Mesh->primitiveTopology)
	{
	case PrimitiveTopology::TriangleList:
		for (int vertexIndex{}; vertexIndex < m_Mesh->indices.size(); vertexIndex+=3)
		{
			if (m_Mesh->vertices_out[vertexIndex].position.w < 0 or m_Mesh->vertices_out[vertexIndex + 1].position.w < 0 or m_Mesh->vertices_out[vertexIndex + 2].position.w < 0)
			{
				continue;
			}
			RenderList(vertexIndex, interpolatedZ, totalWeight, pixel, interpolatedUV, finalColor, swapOddVertices1, swapOddVertices2, pixelIndex, vertices_weights);
		}
		break;
	case PrimitiveTopology::TriangleStrip:
		for (int vertexIndex{}; vertexIndex < (m_Mesh->indices.size() - 2); vertexIndex++)
		{
			if (m_Mesh->vertices_out[vertexIndex].position.w < 0 or m_Mesh->vertices_out[vertexIndex + 1].position.w < 0 or m_Mesh->vertices_out[vertexIndex + 2].position.w < 0)
			{
				continue;
			}
			RenderStrip(vertexIndex, interpolatedZ, totalWeight, pixel, interpolatedUV, finalColor, swapOddVertices1, swapOddVertices2, pixelIndex, vertices_weights, true);
		}
		break;
	}

}

void Renderer::RenderList(int vertexIndex, float interpolatedZ, float totalWeight, Vector2& pixel, Vector2& interpolatedUV, ColorRGB& finalColor, int swapOddVertices1, int swapOddVertices2, int pixelIndex, std::vector<float>& vertices_weights)
{
	RenderStrip(vertexIndex, interpolatedZ, totalWeight, pixel, interpolatedUV, finalColor, swapOddVertices1, swapOddVertices2, pixelIndex, vertices_weights, false);
}

void Renderer::RenderStrip(int vertexIndex, float interpolatedZ, float totalWeight, Vector2& pixel, Vector2& interpolatedUV, ColorRGB& finalColor, int swapOddVertices1, int swapOddVertices2, int pixelIndex, std::vector<float>& vertices_weights, bool isStrip = true)
{
	if (m_Mesh->indices[vertexIndex + 0] == m_Mesh->indices[vertexIndex + 1] or m_Mesh->indices[vertexIndex + 0] == m_Mesh->indices[vertexIndex + 2] or m_Mesh->indices[vertexIndex + 1] == m_Mesh->indices[vertexIndex + 2])
	{
		return;
	}

	int minX = int(std::min({ m_Mesh->vertices_out[vertexIndex + 0].position.x, m_Mesh->vertices_out[vertexIndex + 1].position.x, m_Mesh->vertices_out[vertexIndex + 2].position.x }));
	int minY = int(std::min({ m_Mesh->vertices_out[vertexIndex + 0].position.y, m_Mesh->vertices_out[vertexIndex + 1].position.y, m_Mesh->vertices_out[vertexIndex + 2].position.y }));
	int maxX = int(std::max({ m_Mesh->vertices_out[vertexIndex + 0].position.x, m_Mesh->vertices_out[vertexIndex + 1].position.x, m_Mesh->vertices_out[vertexIndex + 2].position.x }));
	int maxY = int(std::max({ m_Mesh->vertices_out[vertexIndex + 0].position.y, m_Mesh->vertices_out[vertexIndex + 1].position.y, m_Mesh->vertices_out[vertexIndex + 2].position.y }));

	int offset{ 5 };
	minX = std::clamp(minX, offset, m_Width - offset) - 5;
	maxX = std::clamp(maxX, offset, m_Width - offset) + 5;
	minY = std::clamp(minY, offset, m_Height - offset) - 5;
	maxY = std::clamp(maxY, offset, m_Height - offset) + 5;

	//these are used to swap the orientation of triangles in the strip to all face the correct side
	//if these are not used and + 1 or + 2 is written istead, that should mean that the order/position of the numbers doesn't really matter
	//+ 0 is written purely for clarity
	if (vertexIndex & 1 and isStrip)
	{
		swapOddVertices1 = 2;
		swapOddVertices2 = 1;
	}
	else
	{
		swapOddVertices1 = 1;
		swapOddVertices2 = 2;
	}

	for (int px{ minX }; px < maxX; ++px)
	{
		for (int py{ minY }; py < maxY; ++py)
		{
			finalColor = ColorRGB(0.0f, 0.0f, 0.0f);
			pixel = Vector2(px + 0.5f, py + 0.5f);
			pixelIndex = px + (py * m_Width);

			if (m_pDepthBufferPixels[pixelIndex] > m_Mesh->vertices_out[vertexIndex].position.z)
			{
				//initial weight calculation
				vertices_weights[vertexIndex + 0] = Vector2::Cross(m_Mesh->vertices_out[vertexIndex + swapOddVertices1].position.GetXY() - m_Mesh->vertices_out[vertexIndex + 0].position.GetXY(), pixel - m_Mesh->vertices_out[vertexIndex + 0].position.GetXY());
				vertices_weights[vertexIndex + swapOddVertices1] = Vector2::Cross(m_Mesh->vertices_out[vertexIndex + swapOddVertices2].position.GetXY() - m_Mesh->vertices_out[vertexIndex + swapOddVertices1].position.GetXY(), pixel - m_Mesh->vertices_out[vertexIndex + swapOddVertices1].position.GetXY());
				vertices_weights[vertexIndex + swapOddVertices2] = Vector2::Cross(m_Mesh->vertices_out[vertexIndex + 0].position.GetXY() - m_Mesh->vertices_out[vertexIndex + swapOddVertices2].position.GetXY(), pixel - m_Mesh->vertices_out[vertexIndex + swapOddVertices2].position.GetXY());

				if (vertices_weights[vertexIndex + 0] < 0 or vertices_weights[vertexIndex + 1] < 0 or vertices_weights[vertexIndex + 2] < 0) continue;

				//normalize weights and calculate total weight
				totalWeight = vertices_weights[vertexIndex + 0] + vertices_weights[vertexIndex + 1] + vertices_weights[vertexIndex + 2];
				vertices_weights[vertexIndex + 0] /= totalWeight;
				vertices_weights[vertexIndex + swapOddVertices1] /= totalWeight;
				vertices_weights[vertexIndex + swapOddVertices2] /= totalWeight;
				totalWeight = vertices_weights[vertexIndex + 0] + vertices_weights[vertexIndex + 1] + vertices_weights[vertexIndex + 2];

				//if there is a texture, display is, else display the vertex colors
				if (m_Texture)
				{
					interpolatedZ = 1 / ((vertices_weights[vertexIndex + 0] / m_Mesh->vertices_out[vertexIndex + swapOddVertices2].position.z) + (vertices_weights[vertexIndex + swapOddVertices1] / m_Mesh->vertices_out[vertexIndex + 0].position.z) + (vertices_weights[vertexIndex + swapOddVertices2] / m_Mesh->vertices_out[vertexIndex + swapOddVertices1].position.z));					
					if (interpolatedZ >= 0.0f and interpolatedZ <= 1.0f)
					{
						interpolatedUV = (((m_Mesh->vertices_out[vertexIndex + 0].uv / m_Mesh->vertices_out[vertexIndex + 0].position.z) * vertices_weights[vertexIndex + swapOddVertices1]) +
							((m_Mesh->vertices_out[vertexIndex + swapOddVertices1].uv / m_Mesh->vertices_out[vertexIndex + swapOddVertices1].position.z) * vertices_weights[vertexIndex + swapOddVertices2]) +
							((m_Mesh->vertices_out[vertexIndex + swapOddVertices2].uv / m_Mesh->vertices_out[vertexIndex + swapOddVertices2].position.z) * vertices_weights[vertexIndex + 0])) * interpolatedZ;
						//if (interpolatedUV.x < 0.0f or interpolatedUV.x > 1.0f or interpolatedUV.y < 0.0f or interpolatedUV.y > 1.0f)
						//{
						//	continue;
						//}
						finalColor = m_Texture->Sample(interpolatedUV);
					}
				}
				else
				{
					finalColor = ColorRGB(vertices_weights[vertexIndex + 0] * m_Mesh->vertices_out[vertexIndex + 0].color + vertices_weights[vertexIndex + 1] * m_Mesh->vertices_out[vertexIndex + 1].color + vertices_weights[vertexIndex + 2] * m_Mesh->vertices_out[vertexIndex + 2].color);
				}

				if (AreEqual(interpolatedZ, 0))
				{
					m_pDepthBufferPixels[pixelIndex] = m_Mesh->vertices_out[vertexIndex].position.z;
				}
				else
				{
					m_pDepthBufferPixels[pixelIndex] = interpolatedZ;
				}

				//Update Color in Buffer
				finalColor.MaxToOne();
				//finalColor.ToneMap();

				m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}

}