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

			forward = viewMatrix.TransformVector(Vector3::UnitZ);
			forward.Normalize();

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

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			const float camSpeedDelta{ m_CameraMovementSpeed * deltaTime };
			int forwardModifier{};
			int rightModifier{};
			int upModifier{};

			if (pKeyboardState[SDL_SCANCODE_W])
			{
				forwardModifier = 1;
			}
			if (pKeyboardState[SDL_SCANCODE_S])
			{
				forwardModifier = -1;
			}
			if (pKeyboardState[SDL_SCANCODE_A])
			{
				rightModifier = -1;
			}
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				rightModifier = 1;
			}
			if (pKeyboardState[SDL_SCANCODE_SPACE])
			{
				upModifier = 1;
			}
			if (pKeyboardState[SDL_SCANCODE_LSHIFT])
			{
				upModifier = -1;
			}

			origin += camSpeedDelta * forward * static_cast<float>(forwardModifier);
			origin += camSpeedDelta * right * static_cast<float>(rightModifier);
			origin += camSpeedDelta * up * static_cast<float>(upModifier);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			const float camRotateDelta{ m_CameraRotationSpeed * deltaTime };

			if (mouseState == SDL_BUTTON_LMASK)
			{
				if (mouseX < 0)
				{
					totalYaw += camRotateDelta;
				}
				if (mouseX > 0)
				{
					totalYaw -= camRotateDelta;
				}
				if (mouseY < 0)
				{
					origin += camSpeedDelta * forward;
				}
				if (mouseY > 0)
				{
					origin += camSpeedDelta * (-forward);
				}
			}
			if (mouseState == SDL_BUTTON_RMASK)
			{
				if (mouseX < 0)
				{
					totalYaw -= camRotateDelta;
				}
				if (mouseX > 0)
				{
					totalYaw += camRotateDelta;
				}
				if (mouseY < 0)
				{
					totalPitch += camRotateDelta;
				}
				if (mouseY > 0)
				{
					totalPitch -= camRotateDelta;
				}
			}
			if (mouseState == SDL_BUTTON_LMASK + SDL_BUTTON_RMASK)
			{
				if (mouseY < 0)
				{
					origin += camSpeedDelta * up;
				}
				if (mouseY > 0)
				{
					origin += camSpeedDelta * -up;
				}
			}

			//todo: W2
			CalculateViewMatrix();

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
