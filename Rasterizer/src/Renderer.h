#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"

#include <memory>

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	struct Vertex_Out;
	class Timer;
	class Scene;
	enum class PrimitiveTopology;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const;

		void Render_W7();

		void InitializeTriangles(std::vector<Vertex>& verticesNDC, std::vector<uint32_t>& trianglesVertexIndices);

		void RenderStrip(int vertexIndex, float interpolatedZ, float interpolatedW, float totalWeight, Vector2& pixel, Vector2& interpolatedUV, ColorRGB& finalColor, int swapOddVertices1, int swapOddVertices2, int pixelIndex, std::vector<float>& vertices_weights, bool isStrip);
		void RenderList(int vertexIndex, float interpolatedZ, float interpolatedW, float totalWeight, Vector2& pixel, Vector2& interpolatedUV, ColorRGB& finalColor, int swapOddVertices1, int swapOddVertices2, int pixelIndex, std::vector<float>& vertices_weights);
		
		bool CheckCulling(const int vertexIndex);

		void CalculateBoundingBox(int& minX, int& maxX, int& minY, int& maxY, const int vertexIndex);

		void ConvertToScreenSpace(const int vertexIndex);

		float DepthRemap(const float value, const float fromMin, const float fromMax);

		void ToggleDepthBufferVisuals();
		void ToggleUseNormalMap();
		void ToggleRotation();
		void ToggleShadingMode();

		ColorRGB PixelShading(const Vertex_Out& v);

		float CalculateOA(const Vector3& normal, const Vector3& lightDirection);
		ColorRGB CalculateDiffuse(const float reflectance, const Vector2& uv);
		ColorRGB CalculatePhong(const Vector3& normal, const Vector3& lightDirection, const Vector3& viewDirection, const Vector2& uv, const float shininess);
	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		Texture* m_DiffuseTexture;
		Texture* m_NormalsTexture;
		Texture* m_GlossinessTexture;
		Texture* m_SpecularTexture;

		Mesh* m_Mesh = nullptr;
		float m_ModelYRotation{};

		bool m_ShowDepthBuffer = false;
		bool m_UseNormalMap = true;
		bool m_IsRotating = false;

		enum class ShadingMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined
		};
		ShadingMode m_ShadingMode{ ShadingMode::Combined };
	};
}
