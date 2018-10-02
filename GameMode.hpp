#pragma once

#include "Scene.hpp"
#include "Mode.hpp"
#include "mesh4d.hpp"

#include "MeshBuffer.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <list>

// The 'GameMode' mode is the main gameplay mode:

struct GameMode : public Mode {
	GameMode();
	virtual ~GameMode();

	//handle_event is called when new mouse or keyboard events are received:
	// (note that this might be many times per frame or never)
	//The function should return 'true' if it handled the event.
	virtual bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;

	//update is called at the start of a new frame, after events are handled:
	virtual void update(float elapsed) override;

	//draw is called after update:
	virtual void draw(glm::uvec2 const &drawable_size) override;

	float camera_spin = 0.0f;
	float spot_spin = 0.0f;

	enum DisplayState {
		NONE, WIN, LOSE
	};

	DisplayState current_display = NONE;
	float display_timer = 0;

	Scene::Transform hypercube_transform;
	Scene::Transform ref_hypercube_transform;

	struct Rotation4D {
		RotationAxis4D axis;
		float angle;

		Rotation4D(RotationAxis4D axis, float angle) : axis(axis), angle(angle) {}
	};
	std::list<Rotation4D> target_rotations;

	void regenerate_target_rotations();
	void reapply_target_rotations();
};
