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

		Camera(const Vector3& _origin, float _fovAngle, float aspectRatio):
			origin{_origin},
			fovAngle{_fovAngle},
			ratio{aspectRatio}
		{
		}

		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float fovCheck{ fov };
		float ratio{};
		float nearPlane{ 0.1f };
		float farPlane{ 100.0f };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{0};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};
		Matrix worldViewProjectionMatrix{};

		float m_CameraMovementSpeed{ 20.0f };
		float m_CameraRotationSpeed{ 1.5f };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f}, float aspectRatio = 1.0f)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			ratio = aspectRatio;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix

			//invViewMatrix = Matrix::CreateLookAtLH(origin, forward, up);

			Matrix translation = Matrix::CreateTranslation(origin);
			Matrix rotation = Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw);

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

			projectionMatrix = { { 1 / (ratio * fov),	0,		 0,												0 },
								 { 0,					1 / fov, 0,												0 },
								 { 0,					0,		 farPlane / (farPlane-nearPlane),				1 },
								 { 0,					0,		 -(farPlane*nearPlane) / (farPlane-nearPlane),	0 }};

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			//...
			const float movementSpeed{ m_CameraMovementSpeed * deltaTime };

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

			if (pKeyboardState[SDL_SCANCODE_SPACE])
			{
				origin += up * movementSpeed;
			}

			if (pKeyboardState[SDL_SCANCODE_LSHIFT])
			{
				origin -= up * movementSpeed;
			}

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			const float rotationSpeed{ m_CameraRotationSpeed / 180.0f };

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
			if (fovCheck != fov)
			{
				fovCheck = fov;
				CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
			}

		}
	};
}
