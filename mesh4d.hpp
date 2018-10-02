#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Scene.hpp"
#include "GL.hpp"

/*******
 * Attribution
 * 
 * Much of the math behind 4D object rendering was informed by the following
 * Unity Answers post:
 * https://answers.unity.com/questions/438251/creating-a-tesseract-a-4d-cube.html
 * ... and the code sample that was posted there by user WillNode:
 * http://tomkail.com/tesseract/Tesseract.cs
 * This Math Stackexchange post was also very helpful at defining 4D rotation
 * https://math.stackexchange.com/questions/1402362/rotation-in-4d
********/

// In 3 dimensions, we can say that we rotate "about an axis" because
// to rotate in 3d we fix exactly 1 dimension (leaving 2 for the rotation
// plane).  In 4 dimensions, we need to fix *two* dimensions to rotate,
// so we more accurately "rotate about a plane".
// 
// Trippy...
enum RotationAxis4D {
	XY, XZ, XW, YZ, YW, ZW
};

struct Mesh4D {
	// Logical objects (unprojected R4 space)
	std::vector<glm::vec4> vertices;
	std::vector<int> tris;
	std::vector<glm::vec4> transformed_vertices;
	glm::vec4 reference = glm::vec4(0.f,0.f,0.f,1.f); // used to compare against soln
	// Assume camera is looking down -w axis, with perspective projection wrt w
	float camera_position_w = 3;

	// OpenGL Rendering objects (projected R3 space)
	struct Vertex {
		glm::vec3 position;
		glm::u8vec4 color = glm::u8vec4(128, 128, 128, 128);
	};
	std::vector<Vertex> projected_vertices;
	size_t cur_vao_size;
	GLuint vao;
	GLuint vbo;
	GLuint program;

	// From MeshBuffer
	struct Attrib {
		GLint size = 0;
		GLenum type = 0;
		GLboolean normalized = GL_FALSE;
		GLsizei stride = 0;
		GLsizei offset = 0;

		Attrib() = default;
		Attrib(GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, GLsizei offset_)
		: size(size_), type(type_), normalized(normalized_), stride(stride_), offset(offset_) { }
	};

	Attrib Position;
	Attrib Color;

	Mesh4D(std::vector<glm::vec4> vertices, std::vector<int> quads, GLuint program);

	void rotate(RotationAxis4D axis, float angle);
	void apply_perspective();
	void upload_vertex_data();
	void reset_rotation() {
		transformed_vertices = std::vector<glm::vec4>(vertices);
		reference = glm::vec4(0,0,0,1);
	}
	void draw(Scene::Transform &t, glm::mat4 const &world_to_clip) const;
	void draw(Scene::Transform &t, Scene::Camera const *camera) const;
};