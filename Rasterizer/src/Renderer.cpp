//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"
#include <iostream>

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
	m_Camera.Initialize(45.0f, { 0.0f, 5.0f, -64.0f }, (static_cast<float>(m_Width) / m_Height));

	m_Mesh = new Mesh();

	m_ModelYRotation = 0.0f;

	m_DiffuseTexture	= m_DiffuseTexture->LoadFromFile("Resources/vehicle_diffuse.png");
	m_NormalsTexture	= m_NormalsTexture->LoadFromFile("Resources/vehicle_normal.png");
	m_SpecularTexture   = m_SpecularTexture->LoadFromFile("Resources/vehicle_specular.png");
	m_GlossinessTexture = m_GlossinessTexture->LoadFromFile("Resources/vehicle_gloss.png");

	Utils::ParseOBJ("Resources/vehicle.obj", m_Mesh->vertices, m_Mesh->indices);
	m_Mesh->vertices_out.resize(m_Mesh->vertices.size());
	m_Mesh->primitiveTopology = PrimitiveTopology::TriangleList;

	m_Mesh->isVertex_outInScreenSpace.resize(m_Mesh->vertices.size());
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_SpecularTexture;
	delete m_GlossinessTexture;
	delete m_NormalsTexture;
	delete m_DiffuseTexture;
	delete m_Mesh;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	std::fill_n(m_pBackBufferPixels, m_Width * m_Height, 0);

	if (m_IsRotating)
	{

		float rotateSpeed{ 1.0f };
		m_ModelYRotation += rotateSpeed * pTimer->GetElapsed();
		m_Mesh->RotateY(m_ModelYRotation);
	}

	m_Mesh->Update();
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

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
	Vertex_Out out{};
	Vertex currentVertex{};

	Matrix finalMatrix = m_Mesh->worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

	for (int index = 0; index < vertices_in.size(); index++)
	{
		currentVertex = vertices_in[index];

		out.color			= currentVertex.color;
		out.uv				= currentVertex.uv;
		out.position		= finalMatrix.TransformPoint(currentVertex.position.ToPoint4());
		out.normal			= m_Mesh->worldMatrix.TransformVector(currentVertex.normal.ToVector4());
		out.tangent			= m_Mesh->worldMatrix.TransformVector(currentVertex.tangent.ToVector4());
		out.viewDirection	= out.position - m_Camera.origin.ToPoint4();

		out.position.x /= out.position.w;
		out.position.y /= out.position.w;
		out.position.z /= out.position.w;

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
	float		interpolatedW	{};
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
			if (CheckCulling(vertexIndex))
			{
				continue;
			}

			ConvertToScreenSpace(vertexIndex);
			RenderList(vertexIndex, interpolatedZ, interpolatedW, totalWeight, pixel, interpolatedUV, finalColor, swapOddVertices1, swapOddVertices2, pixelIndex, vertices_weights);
		}
		break;
	case PrimitiveTopology::TriangleStrip:
		for (int vertexIndex{}; vertexIndex < (m_Mesh->indices.size() - 2); vertexIndex++)
		{
			if (CheckCulling(vertexIndex))
			{
				continue;
			}

			ConvertToScreenSpace(vertexIndex);
			RenderStrip(vertexIndex, interpolatedZ, interpolatedW, totalWeight, pixel, interpolatedUV, finalColor, swapOddVertices1, swapOddVertices2, pixelIndex, vertices_weights, true);
		}
		break;
	}

	for (int i = 0; i < m_Mesh->isVertex_outInScreenSpace.size(); i++)
	{
		m_Mesh->isVertex_outInScreenSpace[i] = false;
	}
}

void Renderer::RenderList(int vertexIndex, float interpolatedZ, float interpolatedW, float totalWeight, Vector2& pixel, Vector2& interpolatedUV, ColorRGB& finalColor, int swapOddVertices1, int swapOddVertices2, int pixelIndex, std::vector<float>& vertices_weights)
{
	RenderStrip(vertexIndex, interpolatedZ, interpolatedW, totalWeight, pixel, interpolatedUV, finalColor, swapOddVertices1, swapOddVertices2, pixelIndex, vertices_weights, false);
}

void Renderer::RenderStrip(int vertexIndex, float interpolatedZ, float interpolatedW, float totalWeight, Vector2& pixel, Vector2& interpolatedUV, ColorRGB& finalColor, int swapOddVertices1, int swapOddVertices2, int pixelIndex, std::vector<float>& vertices_weights, bool isStrip = true)
{
	int minX{}, maxX{}, minY{}, maxY{};
	
	CalculateBoundingBox(minX, maxX, minY, maxY, vertexIndex);

	//these are used to swap the orientation of triangles in the strip to all face the correct side
	//if these are not used and + 1 or + 2 is written istead, that should mean that the order/position of the numbers doesn't really matter
	//+ 0 is written purely for clarity
	if (vertexIndex & 1 and isStrip)
	{
		swapOddVertices1 = 1;
		swapOddVertices2 = 2;
	}
	else
	{
		swapOddVertices1 = 2;
		swapOddVertices2 = 1;
	}

	for (int px{ minX }; px < maxX; ++px)
	{
		for (int py{ minY }; py < maxY; ++py)
		{
			if (m_ShowBoundingBox == false)
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
					totalWeight = vertices_weights[vertexIndex + 0] +
						vertices_weights[vertexIndex + 1] +
						vertices_weights[vertexIndex + 2];

					vertices_weights[vertexIndex + 0] /= totalWeight;
					vertices_weights[vertexIndex + swapOddVertices1] /= totalWeight;
					vertices_weights[vertexIndex + swapOddVertices2] /= totalWeight;

					totalWeight = vertices_weights[vertexIndex + 0] +
						vertices_weights[vertexIndex + 1] +
						vertices_weights[vertexIndex + 2];

					interpolatedZ = 1 / ((vertices_weights[vertexIndex + 0] / m_Mesh->vertices_out[vertexIndex + swapOddVertices2].position.z) +
						(vertices_weights[vertexIndex + swapOddVertices1] / m_Mesh->vertices_out[vertexIndex + 0].position.z) +
						(vertices_weights[vertexIndex + swapOddVertices2] / m_Mesh->vertices_out[vertexIndex + swapOddVertices1].position.z));

					interpolatedW = 1 / ((vertices_weights[vertexIndex + 0] / m_Mesh->vertices_out[vertexIndex + swapOddVertices2].position.w) +
						(vertices_weights[vertexIndex + swapOddVertices1] / m_Mesh->vertices_out[vertexIndex + 0].position.w) +
						(vertices_weights[vertexIndex + swapOddVertices2] / m_Mesh->vertices_out[vertexIndex + swapOddVertices1].position.w));

					if (m_pDepthBufferPixels[pixelIndex] < interpolatedZ)
					{
						continue;
					}

					m_pDepthBufferPixels[pixelIndex] = interpolatedZ;

					interpolatedUV = (((m_Mesh->vertices_out[vertexIndex + 0].uv / m_Mesh->vertices_out[vertexIndex + 0].position.w) * vertices_weights[vertexIndex + swapOddVertices1]) +
						((m_Mesh->vertices_out[vertexIndex + swapOddVertices1].uv / m_Mesh->vertices_out[vertexIndex + swapOddVertices1].position.w) * vertices_weights[vertexIndex + swapOddVertices2]) +
						((m_Mesh->vertices_out[vertexIndex + swapOddVertices2].uv / m_Mesh->vertices_out[vertexIndex + swapOddVertices2].position.w) * vertices_weights[vertexIndex + 0])) * interpolatedW;

					float remap{ DepthRemap(interpolatedZ, 0.9975f, 1.0f) };
					finalColor = ColorRGB(remap, remap, remap);

					Vertex_Out vertexToShade{};
					vertexToShade.position.x = static_cast<float>(px);
					vertexToShade.position.y = static_cast<float>(py);
					vertexToShade.position.z = interpolatedZ;
					vertexToShade.position.w = interpolatedW;
					vertexToShade.color = finalColor;
					vertexToShade.uv = interpolatedUV;
					vertexToShade.normal = ((m_Mesh->vertices_out[vertexIndex + 0].normal * vertices_weights[vertexIndex + swapOddVertices1]) +
						(m_Mesh->vertices_out[vertexIndex + swapOddVertices1].normal * vertices_weights[vertexIndex + swapOddVertices2]) +
						(m_Mesh->vertices_out[vertexIndex + swapOddVertices2].normal * vertices_weights[vertexIndex + 0])) / 3;


					vertexToShade.tangent = ((m_Mesh->vertices_out[vertexIndex + 0].tangent * vertices_weights[vertexIndex + swapOddVertices1]) +
						(m_Mesh->vertices_out[vertexIndex + swapOddVertices1].tangent * vertices_weights[vertexIndex + swapOddVertices2]) +
						(m_Mesh->vertices_out[vertexIndex + swapOddVertices2].tangent * vertices_weights[vertexIndex + 0])) / 3;

					vertexToShade.viewDirection = ((m_Mesh->vertices_out[vertexIndex + 0].viewDirection * vertices_weights[vertexIndex + swapOddVertices1]) +
						(m_Mesh->vertices_out[vertexIndex + swapOddVertices1].viewDirection * vertices_weights[vertexIndex + swapOddVertices2]) +
						(m_Mesh->vertices_out[vertexIndex + swapOddVertices2].viewDirection * vertices_weights[vertexIndex + 0])) / 3;
					vertexToShade.viewDirection.Normalize();

					finalColor = PixelShading(vertexToShade);

					//Update Color in Buffer
					finalColor.MaxToOne();
					//finalColor.ToneMap();

					m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
			else
			{
				pixelIndex = px + (py * m_Width);

				finalColor = colors::White;

				m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}
}

bool Renderer::CheckCulling(const int vertexIndex)
{
	const int frustumOffset{ 1 };

	//check for frustum
	for (int i = 0; i < 3; i++)
	{
		if (m_Mesh->isVertex_outInScreenSpace[vertexIndex + i] == false)
		{
			if (m_Mesh->vertices_out[vertexIndex + i].position.x < -frustumOffset or m_Mesh->vertices_out[vertexIndex + i].position.x > frustumOffset or
				m_Mesh->vertices_out[vertexIndex + i].position.y < -frustumOffset or m_Mesh->vertices_out[vertexIndex + i].position.y > frustumOffset or
				m_Mesh->vertices_out[vertexIndex + i].position.z < 0			  or m_Mesh->vertices_out[vertexIndex + i].position.z > frustumOffset)
			{
				return true;
			}
		}
	}

	if (m_Mesh->vertices_out[vertexIndex].position.w < 0 or m_Mesh->vertices_out[vertexIndex + 1].position.w < 0 or m_Mesh->vertices_out[vertexIndex + 2].position.w < 0)
	{
		return true;
	}

	//check if 2 vertices in triangle are the same => not a triangle => skip
	if (m_Mesh->indices[vertexIndex + 0] == m_Mesh->indices[vertexIndex + 1] or 
		m_Mesh->indices[vertexIndex + 0] == m_Mesh->indices[vertexIndex + 2] or 
		m_Mesh->indices[vertexIndex + 1] == m_Mesh->indices[vertexIndex + 2])
	{
		return true;
	}
	
	return false;
}

void Renderer::CalculateBoundingBox(int& minX, int& maxX, int& minY, int& maxY, const int vertexIndex)
{
	minX = int(std::min({ m_Mesh->vertices_out[vertexIndex + 0].position.x, m_Mesh->vertices_out[vertexIndex + 1].position.x, m_Mesh->vertices_out[vertexIndex + 2].position.x }));
	minY = int(std::min({ m_Mesh->vertices_out[vertexIndex + 0].position.y, m_Mesh->vertices_out[vertexIndex + 1].position.y, m_Mesh->vertices_out[vertexIndex + 2].position.y }));
	maxX = int(std::max({ m_Mesh->vertices_out[vertexIndex + 0].position.x, m_Mesh->vertices_out[vertexIndex + 1].position.x, m_Mesh->vertices_out[vertexIndex + 2].position.x }));
	maxY = int(std::max({ m_Mesh->vertices_out[vertexIndex + 0].position.y, m_Mesh->vertices_out[vertexIndex + 1].position.y, m_Mesh->vertices_out[vertexIndex + 2].position.y }));

	int offset{ 1 };
	minX = std::clamp(minX, offset, m_Width - offset)  - offset;
	maxX = std::clamp(maxX, offset, m_Width - offset)  + offset;
	minY = std::clamp(minY, offset, m_Height - offset) - offset;
	maxY = std::clamp(maxY, offset, m_Height - offset) + offset;

}

void Renderer::ConvertToScreenSpace(const int vertexIndex)
{
	for (int i = 0; i < 3; i++)
	{
		if (m_Mesh->isVertex_outInScreenSpace[vertexIndex + i] == false)
		{
			m_Mesh->vertices_out[vertexIndex + i].position.x = ((m_Mesh->vertices_out[vertexIndex + i].position.x + 1) / 2) * float(m_Width);
			m_Mesh->vertices_out[vertexIndex + i].position.y = ((1 - m_Mesh->vertices_out[vertexIndex + i].position.y) / 2) * float(m_Height);
			m_Mesh->isVertex_outInScreenSpace[vertexIndex + i] = true;
		}
	}
}

float Renderer::DepthRemap(const float value, const float fromMin, const float fromMax)
{
	float normalizedValue{ (value - fromMin) / (fromMax - fromMin) };
	if (normalizedValue < 0.0f)
	{
		normalizedValue = 0.0f;
	}

	return normalizedValue;
}

void Renderer::ToggleDepthBufferVisuals()
{
	m_ShowDepthBuffer = !m_ShowDepthBuffer;
	std::cout << "Show depth buffer: " << std::boolalpha << m_ShowDepthBuffer << "\n";
}
void Renderer::ToggleUseNormalMap()
{
	m_UseNormalMap = !m_UseNormalMap;
	std::cout << "Using Normal Map: " << std::boolalpha << m_UseNormalMap << "\n";
}
void Renderer::ToggleRotation()
{
	m_IsRotating = !m_IsRotating;
	std::cout << "Is Rotating: " << std::boolalpha << m_IsRotating << "\n";
}
void Renderer::ToggleShadingMode()
{
	switch (m_ShadingMode)
	{
	case Renderer::ShadingMode::ObservedArea:
		std::cout << "Shading mode: Diffuse\n";
		m_ShadingMode = ShadingMode::Diffuse; 
		break;
	case Renderer::ShadingMode::Diffuse:
		std::cout << "Shading mode: Specular\n";
		m_ShadingMode = ShadingMode::Specular; 
		break;
	case Renderer::ShadingMode::Specular:
		std::cout << "Shading mode: Combined\n";
		m_ShadingMode = ShadingMode::Combined; 
		break;
	case Renderer::ShadingMode::Combined:
		std::cout << "Shading mode: Observed Area\n";
		m_ShadingMode = ShadingMode::ObservedArea;
		break;
	}
}
void Renderer::ToggleShowBoudingBox()
{
	m_ShowBoundingBox = !m_ShowBoundingBox;
	std::cout << "Show Bounding Box: " << std::boolalpha << m_ShowBoundingBox << "\n";
}


ColorRGB Renderer::PixelShading(const Vertex_Out& v)
{
	ColorRGB result{ v.color };

	float shininess{ 25.0f };
	float observedArea{}; 
	float KD{ 7.0f };
	float diffuseReflectance{ 2.0f };
	Vector3 normal{ v.normal }; 
	Vector3 lightDirection{ 0.577f, -0.577f, 0.577f };
	ColorRGB phong{};
	ColorRGB lambert{};
	ColorRGB ambient{ 0.03f, 0.03f, 0.03f };

	//calculate normal
	if (m_UseNormalMap)
	{
		Vector3 biNormaal{ Vector3::Cross(v.normal, v.tangent) };
		Matrix tangentSpaceAxis{ Matrix(v.tangent, biNormaal, v.normal, Vector3::Zero) };

		normal = Vector3(m_NormalsTexture->Sample(v.uv));
		normal = (2.0f * normal) - Vector3(1.0f, 1.0f, 1.0f);

		normal = tangentSpaceAxis.TransformVector(normal);

	}
	normal.Normalize();

	if (m_ShowDepthBuffer == false)
	{
		observedArea = CalculateOA(normal, lightDirection);
		switch (m_ShadingMode)
		{
		case Renderer::ShadingMode::ObservedArea:
			result *= ColorRGB(observedArea, observedArea, observedArea);
			break;

		case Renderer::ShadingMode::Diffuse:
			lambert = CalculateDiffuse(diffuseReflectance, v.uv);
			result *= lambert * observedArea;
			break;

		case Renderer::ShadingMode::Specular:
			phong = CalculatePhong(normal, lightDirection, v.viewDirection, v.uv, shininess);
			result *= phong * observedArea;
			break;

		case Renderer::ShadingMode::Combined:
			lambert = CalculateDiffuse(diffuseReflectance, v.uv);
			phong = CalculatePhong(normal, lightDirection, v.viewDirection, v.uv, shininess);
			result *= (lambert + phong) * observedArea;
			break;
		}

	}

	result += ambient;

	return result;
}

float Renderer::CalculateOA(const Vector3& normal, const Vector3& lightDirection)
{
	float result = Vector3::Dot(normal, -lightDirection);
	if (result < 0.0f)
	{
		result = 0.0f;
	}

	return result;
}
ColorRGB Renderer::CalculateDiffuse(const float reflectance, const Vector2& uv)
{
	ColorRGB diffuseColor = m_DiffuseTexture->Sample(uv);
	diffuseColor *= reflectance;

	return diffuseColor;
}
ColorRGB Renderer::CalculatePhong(const Vector3& normal, const Vector3& lightDirection, const Vector3& viewDirection, const Vector2& uv, const float shininess)
{
	Vector3 reflect{ Vector3::Reflect(lightDirection, normal).Normalized() };
	float angle{ Vector3::Dot(-reflect, viewDirection) };
	if (angle >= 0.0f)
	{
		float phongExponent{ m_GlossinessTexture->Sample(uv).r * shininess }; 

		float specularReflectCoeficient{ m_SpecularTexture->Sample(uv).r }; 

		float phong{ specularReflectCoeficient * powf(angle, phongExponent) };	

		return ColorRGB(phong, phong, phong);
	}

	return colors::Black;
}