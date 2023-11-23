#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		float m_CameraMovementSpeed{ 10.0f };
		float m_CameraRotationSpeed{ 4.0f };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix

			//invViewMatrix = Matrix::CreateLookAtLH(origin, forward, up);

			auto translation = Matrix::CreateTranslation(origin);
			auto rotation = Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw);

			invViewMatrix = rotation * translation;
			viewMatrix = invViewMatrix.Inverse();

			forward = viewMatrix.TransformVector(-Vector3::UnitZ);
			forward.z *= -1;

			right = viewMatrix.TransformVector(Vector3::UnitX);
			right.Normalize();

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W3

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			//...
			const float movementSpeed{ 5.f * deltaTime };

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forward * movementSpeed;
			}

			if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward * movementSpeed;
			}

			if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= Vector3::Cross(up, forward) * movementSpeed;
			}

			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += Vector3::Cross(up, forward) * movementSpeed;
			}

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			const float rotationSpeed{ 1.5f / 180.0f };

			if (mouseState == SDL_BUTTON_LMASK)
			{
				origin -= forward * float(mouseY) * movementSpeed;
				totalYaw += mouseX * rotationSpeed;
			}

			if (mouseState == SDL_BUTTON_RMASK)
			{
				totalPitch += -mouseY * rotationSpeed;
				totalYaw += mouseX * rotationSpeed;
			}

			if (mouseState == SDL_BUTTON_X2)
			{
				origin.y -= mouseY * movementSpeed;
			}

			Matrix rotationMatrix = Matrix::CreateRotation(totalPitch, totalYaw, 0);
			forward = rotationMatrix.TransformVector(Vector3::UnitZ);
			forward.Normalize();

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
