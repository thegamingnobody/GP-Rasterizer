#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <iostream>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		SDL_Surface* imageSurface = IMG_Load(path.c_str());
		Texture* texture{};
		if (imageSurface)
		{
			texture = new Texture{ imageSurface };
		}
		else
		{
			std::cout << "Texture not found\n";
		}

		return texture;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		uint8_t r{}, g{}, b{};

		const float u = std::clamp(uv.x, 0.0f, 1.0f);
		const float v = std::clamp(uv.y, 0.0f, 1.0f);
		const size_t px = static_cast<size_t>(u * m_pSurface->w);
		const size_t py = static_cast<size_t>(v * m_pSurface->h);

		SDL_GetRGB(m_pSurfacePixels[(py * m_pSurface->w) + px], m_pSurface->format, &r, &g, &b);

		const ColorRGB result{ r / 255.0f , g / 255.0f , b / 255.0f };
		return result;
	}
}