#include "ECSRegistry.h"

#include "imgui/imgui.h"

using namespace ECS;

void ECS::CameraController::init(Transform& transform)
{
	updateRotation(transform);
}

void ECS::CameraController::update(Transform& transform, InputManager& input, float dt)
{
	if (input.IsMouseDown(MouseButton::Right))
	{
		glm::vec2 delta = input.GetMouseDelta() * mouseSensitivity;
		yaw += delta.x;
		pitch += delta.y;
		pitch = glm::clamp(pitch, -89.0f, 89.0f);

		updateRotation(transform);
	}

	glm::vec3 forward = transform.rotation * glm::vec3(0, 0, -1);
	glm::vec3 right = transform.rotation * glm::vec3(1, 0, 0);

	float velocity = movementSpeed * dt;

	if (input.IsKeyDown(Key::W)) transform.position += forward * velocity;
	if (input.IsKeyDown(Key::S)) transform.position -= forward * velocity;
	if (input.IsKeyDown(Key::A)) transform.position -= right * velocity;
	if (input.IsKeyDown(Key::D)) transform.position += right * velocity;

	transform.dirty = true;
}

void ECS::CameraController::updateRotation(Transform& transform)
{
	float cy = cos(glm::radians(yaw));
	float sy = sin(glm::radians(yaw));
	float cp = cos(glm::radians(pitch));
	float sp = sin(glm::radians(pitch));

	glm::vec3 trueDirection;
	trueDirection.x = cy * cp;
	trueDirection.y = sp;
	trueDirection.z = sy * cp;
	trueDirection = glm::normalize(trueDirection);

	transform.rotation = glm::rotation(glm::vec3(0, 0, -1), trueDirection);
	transform.dirty = true;
}

void ECS::DebugSystem::update(ECSRegistry& registry)
{
	ImGui::Begin("Scene");

	for (auto& e : registry.getEntities())
	{
		if (registry.hasComponent<Hierarchy>(e))
		{
			auto& h = registry.getComponent<Hierarchy>(e);
			if (h.parent != INVALID_ENTITY) continue;
		}
		drawEntityNode(registry, e);
	}

	ImGui::End();
}

void ECS::DebugSystem::drawEntityNode(ECSRegistry& registry, Entity e)
{
	ImGui::PushID(e);

	ImGui::PopID();
}
