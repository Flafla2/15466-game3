#include "GameMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "check_fb.hpp" //helper for checking currently bound OpenGL framebuffer
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "load_save_png.hpp"
#include "texture_program.hpp"
#include "tesseract_program.hpp"
#include "depth_program.hpp"
#include "mesh4d.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <cstdlib>
#include <random>

MLoad< Mesh4D > hypercube(LoadTagDefault, [](){
	glm::vec4 hypercube_vertices_[] = {
		glm::vec4(-1, -1, -1, -1), // 0
		glm::vec4(+1, -1, -1, -1), // 1
		glm::vec4(-1, +1, -1, -1), // 2
		glm::vec4(+1, +1, -1, -1), // 3
		glm::vec4(-1, -1, +1, -1), // 4
		glm::vec4(+1, -1, +1, -1), // 5
		glm::vec4(-1, +1, +1, -1), // 6
		glm::vec4(+1, +1, +1, -1), // 7
		glm::vec4(-1, -1, -1, +1), // 8
		glm::vec4(+1, -1, -1, +1), // 9
		glm::vec4(-1, +1, -1, +1), // 10
		glm::vec4(+1, +1, -1, +1), // 11
		glm::vec4(-1, -1, +1, +1), // 12
		glm::vec4(+1, -1, +1, +1), // 13
		glm::vec4(-1, +1, +1, +1), // 14
		glm::vec4(+1, +1, +1, +1)  // 15
	};
	int hypercube_faces_[] = {
		0, 1, 3, 2,
		4, 5, 7, 6,
		8, 9, 11, 10,
		12, 13, 15, 14,

		9, 13, 15, 11,
		8, 12, 14, 10,
		1, 5, 7, 3,
		0, 4, 6, 2,

		10, 11, 15, 14,
		8, 9, 13, 12,
		2, 3, 7, 6,
		0, 1, 5, 4,

		8, 9, 1, 0,
		9, 1, 5, 13,
		13, 5, 4, 12,
		12, 8, 0, 4,

		10, 2, 3, 11,
		11, 3, 7, 15,
		15, 7, 6, 14,
		14, 6, 2, 10,

		10, 2, 0, 8,
		11, 3, 1, 9,
		15, 7, 5, 13,
		14, 6, 4, 12
	};
	// Coloring scheme: color opposite faces with
	// complementary colors, and color the "fins"
	// connecting w = -1 to w = 1 a feint white.
	//
	// This strategy is inspired by the game FEZ, which
	// features a lovable companion character, a hypercube
	// called Dot, which has a similar coloring scheme.
	glm::u8vec4 hypercube_colors_[] = {
		glm::u8vec4(0, 0, 255, 128),
		glm::u8vec4(0, 0, 255, 128),
		glm::u8vec4(255, 255, 0, 128),
		glm::u8vec4(255, 255, 0, 128),

		glm::u8vec4(255, 0, 0, 128),
		glm::u8vec4(255, 0, 0, 128),
		glm::u8vec4(0, 255, 255, 128),
		glm::u8vec4(0, 255, 255, 128),

		glm::u8vec4(0, 255, 0, 128),
		glm::u8vec4(0, 255, 0, 128),
		glm::u8vec4(255, 0, 255, 128),
		glm::u8vec4(255, 0, 255, 128),

		glm::u8vec4(255, 255, 255, 80),
		glm::u8vec4(255, 255, 255, 80),
		glm::u8vec4(255, 255, 255, 80),
		glm::u8vec4(255, 255, 255, 80),

		glm::u8vec4(255, 255, 255, 80),
		glm::u8vec4(255, 255, 255, 80),
		glm::u8vec4(255, 255, 255, 80),
		glm::u8vec4(255, 255, 255, 80),

		glm::u8vec4(255, 255, 255, 80),
		glm::u8vec4(255, 255, 255, 80),
		glm::u8vec4(255, 255, 255, 80),
		glm::u8vec4(255, 255, 255, 80)
	};
	std::vector<glm::vec4> hypercube_vertices(hypercube_vertices_, hypercube_vertices_ + 16);
	std::vector<int> hypercube_faces(hypercube_faces_, hypercube_faces_ + (24 * 4));
	std::vector<glm::u8vec4> hypercube_colors(hypercube_colors_, hypercube_colors_ + (24 * 4));
	return new Mesh4D(hypercube_vertices, hypercube_faces, hypercube_colors, tesseract_program->program);
});

MLoad <Mesh4D> reference_hypercube(LoadTagDefault, [](){
	return new Mesh4D(*hypercube);
});

Load< MeshBuffer > meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("vignette.pnct"));
});

Load< GLuint > meshes_for_texture_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(texture_program->program));
});

Load< GLuint > meshes_for_depth_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(depth_program->program));
});

//used for fullscreen passes:
Load< GLuint > empty_vao(LoadTagDefault, [](){
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindVertexArray(0);
	return new GLuint(vao);
});

Load< GLuint > blur_program(LoadTagDefault, [](){
	GLuint program = compile_program(
		//this draws a triangle that covers the entire screen:
		"#version 330\n"
		"void main() {\n"
		"	gl_Position = vec4(4 * (gl_VertexID & 1) - 1,  2 * (gl_VertexID & 2) - 1, 0.0, 1.0);\n"
		"}\n"
		,
		//NOTE on reading screen texture:
		//texelFetch() gives direct pixel access with integer coordinates, but accessing out-of-bounds pixel is undefined:
		//	vec4 color = texelFetch(tex, ivec2(gl_FragCoord.xy), 0);
		//texture() requires using [0,1] coordinates, but handles out-of-bounds more gracefully (using wrap settings of underlying texture):
		//	vec4 color = texture(tex, gl_FragCoord.xy / textureSize(tex,0));

		"#version 330\n"
		"uniform sampler2D tex;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	vec2 at = (gl_FragCoord.xy - 0.5 * textureSize(tex, 0)) / textureSize(tex, 0).y;\n"
		//make blur amount more near the edges and less in the middle:
		"	float amt = 0.3 * (0.01 * textureSize(tex,0).y) * max(0.0,(length(at) - 0.5)/0.2);\n"
		//pick a vector to move in for blur using function inspired by:
		//https://stackoverflow.com/questions/12964279/whats-the-origin-of-this-glsl-rand-one-liner
		"	vec2 ofs = amt * normalize(vec2(\n"
		"		fract(dot(gl_FragCoord.xy ,vec2(12.9898,78.233))),\n"
		"		fract(dot(gl_FragCoord.xy ,vec2(96.3869,-27.5796)))\n"
		"	));\n"
		//do a four-pixel average to blur:
		"	vec4 blur =\n"
		"		+ 0.25 * texture(tex, (gl_FragCoord.xy + vec2(ofs.x,ofs.y)) / textureSize(tex, 0))\n"
		"		+ 0.25 * texture(tex, (gl_FragCoord.xy + vec2(-ofs.y,ofs.x)) / textureSize(tex, 0))\n"
		"		+ 0.25 * texture(tex, (gl_FragCoord.xy + vec2(-ofs.x,-ofs.y)) / textureSize(tex, 0))\n"
		"		+ 0.25 * texture(tex, (gl_FragCoord.xy + vec2(ofs.y,-ofs.x)) / textureSize(tex, 0))\n"
		"	;\n"
		"	fragColor = vec4(blur.rgb, 1.0);\n" //blur;\n"
		"}\n"
	);

	glUseProgram(program);

	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	glUseProgram(0);

	return new GLuint(program);
});


GLuint load_texture(std::string const &filename) {
	glm::uvec2 size;
	std::vector< glm::u8vec4 > data;
	load_png(filename, &size, &data, LowerLeftOrigin);

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();

	return tex;
}

Load< GLuint > wood_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/wood.png")));
});

Load< GLuint > marble_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/marble.png")));
});

Load< GLuint > white_tex(LoadTagDefault, [](){
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glm::u8vec4 white(0xff, 0xff, 0xff, 0xff);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, glm::value_ptr(white));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	return new GLuint(tex);
});


Scene::Transform *win_text_transform = nullptr;
Scene::Transform *lose_text_transform = nullptr;
Scene::Transform *camera_parent_transform = nullptr;
Scene::Camera *camera = nullptr;
Scene::Transform *spot_parent_transform = nullptr;
Scene::Lamp *spot = nullptr;

Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;

	//pre-build some program info (material) blocks to assign to each object:
	Scene::Object::ProgramInfo texture_program_info;
	texture_program_info.program = texture_program->program;
	texture_program_info.vao = *meshes_for_texture_program;
	texture_program_info.mvp_mat4  = texture_program->object_to_clip_mat4;
	texture_program_info.mv_mat4x3 = texture_program->object_to_light_mat4x3;
	texture_program_info.itmv_mat3 = texture_program->normal_to_light_mat3;

	Scene::Object::ProgramInfo depth_program_info;
	depth_program_info.program = depth_program->program;
	depth_program_info.vao = *meshes_for_depth_program;
	depth_program_info.mvp_mat4  = depth_program->object_to_clip_mat4;


	//load transform hierarchy:
	ret->load(data_path("vignette.scene"), [&](Scene &s, Scene::Transform *t, std::string const &m){
		Scene::Object *obj = s.new_object(t);

		obj->programs[Scene::Object::ProgramTypeDefault] = texture_program_info;
		if (t->name == "Platform") {
			obj->programs[Scene::Object::ProgramTypeDefault].textures[0] = *wood_tex;
		} else if (t->name == "Pedestal" || t->name == "PedestalTarget") {
			obj->programs[Scene::Object::ProgramTypeDefault].textures[0] = *marble_tex;
		} else {
			obj->programs[Scene::Object::ProgramTypeDefault].textures[0] = *white_tex;
		}

		obj->programs[Scene::Object::ProgramTypeShadow] = depth_program_info;

		MeshBuffer::Mesh const &mesh = meshes->lookup(m);
		obj->programs[Scene::Object::ProgramTypeDefault].start = mesh.start;
		obj->programs[Scene::Object::ProgramTypeDefault].count = mesh.count;

		obj->programs[Scene::Object::ProgramTypeShadow].start = mesh.start;
		obj->programs[Scene::Object::ProgramTypeShadow].count = mesh.count;
	});

	//look up camera parent transform:
	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
		if (t->name == "CameraParent") {
			if (camera_parent_transform) throw std::runtime_error("Multiple 'CameraParent' transforms in scene.");
			camera_parent_transform = t;
		}
		if (t->name == "SpotParent") {
			if (spot_parent_transform) throw std::runtime_error("Multiple 'SpotParent' transforms in scene.");
			spot_parent_transform = t;
		}
		if(t->name == "WinText") {
			if (win_text_transform) throw std::runtime_error("Multiple 'WinText' transforms in scene.");
			win_text_transform = t;
		}

		if(t->name == "LoseText") {
			if (lose_text_transform) throw std::runtime_error("Multiple 'LoseText' transforms in scene.");
			lose_text_transform = t;
		}

	}
	if (!camera_parent_transform) throw std::runtime_error("No 'CameraParent' transform in scene.");
	if (!spot_parent_transform) throw std::runtime_error("No 'SpotParent' transform in scene.");

	//look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");

	//look up the spotlight:
	for (Scene::Lamp *l = ret->first_lamp; l != nullptr; l = l->alloc_next) {
		if (l->transform->name == "Spot") {
			if (spot) throw std::runtime_error("Multiple 'Spot' objects in scene.");
			if (l->type != Scene::Lamp::Spot) throw std::runtime_error("Lamp 'Spot' is not a spotlight.");
			spot = l;
		}
	}
	if (!spot) throw std::runtime_error("No 'Spot' spotlight in scene.");

	return ret;
});

GameMode::GameMode() {
	hypercube->apply_perspective();
	hypercube->upload_vertex_data();
	reference_hypercube->apply_perspective();
	reference_hypercube->upload_vertex_data();

	hypercube_transform.scale = glm::vec3(1, 1, 1);
	hypercube_transform.position = glm::vec3(0, -1.5f, 1.5);

	ref_hypercube_transform.position = glm::vec3(0, 1.5f, 1.5);

	regenerate_target_rotations();
	reapply_target_rotations();

	hypercube->apply_perspective();
	hypercube->upload_vertex_data();
	reference_hypercube->apply_perspective();
	reference_hypercube->upload_vertex_data();

	win_text_transform->position.z = -5;
	lose_text_transform->position.z = -5;
}

GameMode::~GameMode() {
}

void GameMode::regenerate_target_rotations() {
	target_rotations.clear();
	// On first challenge, only do one rotation for difficulty
	// scaling.
	static bool generated_before = false;

	for(int x = 0; x < (generated_before ? 2 : 1); ++x) {
		int r = std::rand() % 3;
		RotationAxis4D axis;
		if(r == 0) axis = XW;
		else if (r == 1) axis = YW;
		else axis = ZW; // r == 2
		double r2 = ((double)std::rand()) / ((double)RAND_MAX);
		float angle = 180.f * r2;
		target_rotations.push_front(Rotation4D(axis, angle));
	}
	generated_before = true;
}

void GameMode::reapply_target_rotations() {
	reference_hypercube->reset_rotation();
	for(auto rot : target_rotations) {
		reference_hypercube->rotate(rot.axis, rot.angle);
	}
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_SPACE) {
			// check
			if(display_timer <= 0) {
				float d = glm::dot(hypercube->reference, reference_hypercube->reference);
				if(d > 0.9f) {
					std::cout << "GOT IT!" << std::endl;
					current_display = WIN;
				} else {
					std::cout << "NOT GOT IT!" << std::endl;
					current_display = LOSE;
				}
				display_timer = 1.0;
			}
			
		} else if (evt.key.keysym.sym == SDLK_BACKSPACE) {
			// reset
			hypercube->reset_rotation();
			reapply_target_rotations();

			hypercube->apply_perspective();
			hypercube->upload_vertex_data();
			reference_hypercube->apply_perspective();
			reference_hypercube->upload_vertex_data();
		}
	}

	if (evt.type == SDL_MOUSEMOTION) {
		if (evt.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			camera_spin += 5.0f * evt.motion.xrel / float(window_size.x);
			return true;
		}
		if (evt.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
			spot_spin += 5.0f * evt.motion.xrel / float(window_size.x);
			return true;
		}

	}

	return false;
}

void GameMode::update(float elapsed) {
	camera_parent_transform->rotation = glm::angleAxis(camera_spin, glm::vec3(0.0f, 0.0f, 1.0f));
	spot_parent_transform->rotation = glm::angleAxis(spot_spin, glm::vec3(0.0f, 0.0f, 1.0f));

	const Uint8* key_state = SDL_GetKeyboardState(NULL);

	bool must_update = key_state[SDL_SCANCODE_Q] 
					|| key_state[SDL_SCANCODE_W]
					|| key_state[SDL_SCANCODE_E]
					|| key_state[SDL_SCANCODE_R]
					|| key_state[SDL_SCANCODE_A]
					|| key_state[SDL_SCANCODE_S]
					|| key_state[SDL_SCANCODE_D]
					|| key_state[SDL_SCANCODE_F]
					|| key_state[SDL_SCANCODE_Z]
					|| key_state[SDL_SCANCODE_X]
					|| key_state[SDL_SCANCODE_C]
					|| key_state[SDL_SCANCODE_V];

	if(key_state[SDL_SCANCODE_W]) {
		hypercube->rotate(XY, 45.f * elapsed);
		reference_hypercube->rotate(XY, 45.f * elapsed);
	}
	if(key_state[SDL_SCANCODE_Q]) {
		hypercube->rotate(XY, -45.f * elapsed);
		reference_hypercube->rotate(XY, -45.f * elapsed);
	}

	if(key_state[SDL_SCANCODE_R]) {
		hypercube->rotate(XZ, 45.f * elapsed);
		reference_hypercube->rotate(XZ, 45.f * elapsed);
	}
	if(key_state[SDL_SCANCODE_E]) {
		hypercube->rotate(XZ, -45.f * elapsed);
		reference_hypercube->rotate(XZ, -45.f * elapsed);
	}

	if(key_state[SDL_SCANCODE_S])
		hypercube->rotate(XW, 45.f * elapsed);
	if(key_state[SDL_SCANCODE_A])
		hypercube->rotate(XW, -45.f * elapsed);

	if(key_state[SDL_SCANCODE_F]) {
		hypercube->rotate(YZ, 45.f * elapsed);
		reference_hypercube->rotate(YZ, 45.f * elapsed);
	}
	if(key_state[SDL_SCANCODE_D]) {
		hypercube->rotate(YZ, -45.f * elapsed);
		reference_hypercube->rotate(YZ, -45.f * elapsed);
	}

	if(key_state[SDL_SCANCODE_X])
		hypercube->rotate(YW, 45.f * elapsed);
	if(key_state[SDL_SCANCODE_Z])
		hypercube->rotate(YW, -45.f * elapsed);

	if(key_state[SDL_SCANCODE_V])
		hypercube->rotate(ZW, 45.f * elapsed);
	if(key_state[SDL_SCANCODE_C])
		hypercube->rotate(ZW, -45.f * elapsed);

	if(display_timer < 0 && current_display != NONE) {
		win_text_transform->position.z = -5;
		lose_text_transform->position.z = -5;

		if(current_display == WIN) {
			hypercube->reset_rotation();
			regenerate_target_rotations();
			reapply_target_rotations();
			must_update = true;
		}
		current_display = NONE;
	}
	if(display_timer >= 0) {
		if(current_display == WIN) {
			win_text_transform->position.z = -.1f;
			lose_text_transform->position.z = -5;
		} else if(current_display == LOSE) {
			win_text_transform->position.z = -5;
			lose_text_transform->position.z = -.1f;
		} else {
			win_text_transform->position.z = -5;
			lose_text_transform->position.z = -5;
		}
	} else {
		win_text_transform->position.z = -5;
		lose_text_transform->position.z = -5;
	}

	if(must_update) {
		hypercube->apply_perspective();
		hypercube->upload_vertex_data();
		reference_hypercube->apply_perspective();
		reference_hypercube->upload_vertex_data();
	}

	if(display_timer > 0)
		display_timer -= elapsed;
}

//GameMode will render to some offscreen framebuffer(s).
//This code allocates and resizes them as needed:
struct Framebuffers {
	glm::uvec2 size = glm::uvec2(0,0); //remember the size of the framebuffer

	//This framebuffer is used for fullscreen effects:
	GLuint color_tex = 0;
	GLuint depth_rb = 0;
	GLuint fb = 0;

	//This framebuffer is used for shadow maps:
	glm::uvec2 shadow_size = glm::uvec2(0,0);
	GLuint shadow_color_tex = 0; //DEBUG
	GLuint shadow_depth_tex = 0;
	GLuint shadow_fb = 0;

	void allocate(glm::uvec2 const &new_size, glm::uvec2 const &new_shadow_size) {
		//allocate full-screen framebuffer:
		if (size != new_size) {
			size = new_size;

			if (color_tex == 0) glGenTextures(1, &color_tex);
			glBindTexture(GL_TEXTURE_2D, color_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
	
			if (depth_rb == 0) glGenRenderbuffers(1, &depth_rb);
			glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.x, size.y);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
	
			if (fb == 0) glGenFramebuffers(1, &fb);
			glBindFramebuffer(GL_FRAMEBUFFER, fb);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
			check_fb();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			GL_ERRORS();
		}

		//allocate shadow map framebuffer:
		if (shadow_size != new_shadow_size) {
			shadow_size = new_shadow_size;

			if (shadow_color_tex == 0) glGenTextures(1, &shadow_color_tex);
			glBindTexture(GL_TEXTURE_2D, shadow_color_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadow_size.x, shadow_size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);


			if (shadow_depth_tex == 0) glGenTextures(1, &shadow_depth_tex);
			glBindTexture(GL_TEXTURE_2D, shadow_depth_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadow_size.x, shadow_size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
	
			if (shadow_fb == 0) glGenFramebuffers(1, &shadow_fb);
			glBindFramebuffer(GL_FRAMEBUFFER, shadow_fb);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadow_color_tex, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_depth_tex, 0);
			check_fb();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			GL_ERRORS();
		}
	}
} fbs;

void GameMode::draw(glm::uvec2 const &drawable_size) {
	fbs.allocate(drawable_size, glm::uvec2(512, 512));

	//Draw scene to shadow map for spotlight:
	glBindFramebuffer(GL_FRAMEBUFFER, fbs.shadow_fb);
	glViewport(0,0,fbs.shadow_size.x, fbs.shadow_size.y);

	glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	//render only back faces to shadow map (prevent shadow speckles on fronts of objects):
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

	scene->draw(spot, Scene::Object::ProgramTypeShadow);

	glDisable(GL_CULL_FACE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GL_ERRORS();



	//Draw scene to off-screen framebuffer:
	glBindFramebuffer(GL_FRAMEBUFFER, fbs.fb);
	glViewport(0,0,drawable_size.x, drawable_size.y);

	camera->aspect = drawable_size.x / float(drawable_size.y);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up light positions:
	glUseProgram(texture_program->program);

	//don't use distant directional light at all (color == 0):
	glUniform3fv(texture_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
	glUniform3fv(texture_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(0.0f, 0.0f,-1.0f))));
	//use hemisphere light for subtle ambient light:
	glUniform3fv(texture_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
	glUniform3fv(texture_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 1.0f)));

	glm::mat4 world_to_spot =
		//This matrix converts from the spotlight's clip space ([-1,1]^3) into depth map texture coordinates ([0,1]^2) and depth map Z values ([0,1]):
		glm::mat4(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.5f+0.00001f /* <-- bias */, 1.0f
		)
		//this is the world-to-clip matrix used when rendering the shadow map:
		* spot->make_projection() * spot->transform->make_world_to_local();

	glUniformMatrix4fv(texture_program->light_to_spot_mat4, 1, GL_FALSE, glm::value_ptr(world_to_spot));

	glm::mat4 spot_to_world = spot->transform->make_local_to_world();
	glUniform3fv(texture_program->spot_position_vec3, 1, glm::value_ptr(glm::vec3(spot_to_world[3])));
	glUniform3fv(texture_program->spot_direction_vec3, 1, glm::value_ptr(-glm::vec3(spot_to_world[2])));
	glUniform3fv(texture_program->spot_color_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));

	glm::vec2 spot_outer_inner = glm::vec2(std::cos(0.5f * spot->fov), std::cos(0.85f * 0.5f * spot->fov));
	glUniform2fv(texture_program->spot_outer_inner_vec2, 1, glm::value_ptr(spot_outer_inner));

	//This code binds texture index 1 to the shadow map:
	// (note that this is a bit brittle -- it depends on none of the objects in the scene having a texture of index 1 set in their material data; otherwise scene::draw would unbind this texture):
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fbs.shadow_depth_tex);
	//The shadow_depth_tex must have these parameters set to be used as a sampler2DShadow in the shader:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	//NOTE: however, these are parameters of the texture object, not the binding point, so there is no need to set them *each frame*. I'm doing it here so that you are likely to see that they are being set.
	glActiveTexture(GL_TEXTURE0);

	scene->draw(camera);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	glDisable(GL_DEPTH_TEST);
	
	reference_hypercube->draw(ref_hypercube_transform, camera);
	hypercube->draw(hypercube_transform, camera);
	
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GL_ERRORS();


	//Copy scene from color buffer to screen, performing post-processing effects:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbs.color_tex);
	glUseProgram(*blur_program);
	glBindVertexArray(*empty_vao);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
