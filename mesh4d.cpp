#include "mesh4d.hpp"
#include "tesseract_program.hpp"
#include "GL.hpp"
#include "Scene.hpp"

#include <iostream>
#include <cstddef>
#include <set>
#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

void Mesh4D::init_gl() {
	glGenBuffers(1, &vbo);

	Position = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
	Color = Attrib(4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), offsetof(Vertex, color));

	/*** From MeshBuffer.cpp ***/

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Try to bind all attributes in this buffer:
	std::set< GLuint > bound;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	auto bind_attribute = [&](char const *name, Attrib const &attrib) {
		if (attrib.size == 0) return; //don't bind empty attribs
		GLint location = glGetAttribLocation(program, name);
		if (location == -1) {
			std::cerr << "WARNING: attribute '" << name << "' in 4d mesh buffer isn't active in program." << std::endl;
		} else {
			glVertexAttribPointer(location, attrib.size, attrib.type, attrib.normalized, attrib.stride, (GLbyte *)0 + attrib.offset);
			glEnableVertexAttribArray(location);
			bound.insert(location);
		}
	};
	bind_attribute("Position", Position);
	//bind_attribute("Normal", Normal);
	bind_attribute("Color", Color);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//Check that all active attributes were bound:
	GLint active = 0;
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &active);
	assert(active >= 0 && "Doesn't makes sense to have negative active attributes.");
	for (GLuint i = 0; i < GLuint(active); ++i) {
		GLchar name[100];
		GLint size = 0;
		GLenum type = 0;
		glGetActiveAttrib(program, i, 100, NULL, &size, &type, name);
		name[99] = '\0';
		GLint location = glGetAttribLocation(program, name);
		if (!bound.count(GLuint(location))) {
			throw std::runtime_error("ERROR: active attribute '" + std::string(name) + "' in program is not bound.");
		}
	}
}

Mesh4D::Mesh4D(std::vector<glm::vec4> raw_verts, std::vector<int> quads, std::vector<glm::u8vec4> quad_colors, GLuint program) : program(program) {
	tris = std::vector<int>((quads.size() / 4) * 2 * 3);

	for(int i = 0; i < quads.size() / 4; ++i) {
		int ti = 6 * i;
		int qi = 4 * i;
		tris[ti + 0] = quads[qi + 0];
		tris[ti + 1] = quads[qi + 1];
		tris[ti + 2] = quads[qi + 3];
		tris[ti + 3] = quads[qi + 1];
		tris[ti + 4] = quads[qi + 2];
		tris[ti + 5] = quads[qi + 3];
	}

	vertices = std::vector<glm::vec4>(tris.size());
	colors = std::vector<glm::u8vec4>(tris.size());

	for(int i = 0; i < tris.size(); ++i) {
		int og_quad = i / 6;
		vertices[i] = raw_verts[tris[i]];
		colors[i] = quad_colors[og_quad];
	}
	transformed_vertices = std::vector<glm::vec4>(vertices);
	projected_vertices = std::vector<Vertex>(vertices.size());

	init_gl();
}

Mesh4D::Mesh4D(Mesh4D &other) {
	vertices = std::vector<glm::vec4>(other.vertices);
	transformed_vertices = std::vector<glm::vec4>(other.transformed_vertices);
	colors = std::vector<glm::u8vec4>(other.colors);
	tris = std::vector<int>(other.tris);
	program = other.program;

	projected_vertices = std::vector<Vertex>(vertices.size());

	init_gl();
}

void Mesh4D::apply_perspective() {
	for(int x = 0; x < transformed_vertices.size(); ++x) {
		glm::vec4& cur_r4 = transformed_vertices[x];
		Vertex& vertex = projected_vertices[x];
		glm::vec3& cur_r3 = vertex.position;

		// Negate, because we consider the -w axis as the "look" axis here.
		double norm_factor = -1.0 / (cur_r4.w - camera_position_w);
		cur_r3.x = cur_r4.x * norm_factor;
		cur_r3.y = cur_r4.y * norm_factor;
		cur_r3.z = cur_r4.z * norm_factor;

		vertex.color = colors[x];
	}
}

void Mesh4D::upload_vertex_data() {
	static bool has_sent_debug = false;
	if(!has_sent_debug) {
		std::cout << "Sending first time hypercube verts to GPU:" << std::endl;
		for(int x = 0; x < projected_vertices.size(); x++) {
			auto p = projected_vertices[x].position;
			std::cout << "(" << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;
		}
		has_sent_debug = true;
	}
	
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(
		GL_ARRAY_BUFFER, 
		projected_vertices.size() * sizeof(Vertex), 
		projected_vertices.data(), 
		GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cur_vao_size = projected_vertices.size();
}

static inline void rotate_vertex(glm::vec4& p, RotationAxis4D axis, float rad) {
	double s = sin(rad);
	double c = cos(rad);

	glm::vec4 t = p;

	switch(axis) {
		case XY:
			t.x =  c * p.x +  s * p.y;
			t.y = -s * p.x +  c * p.y;
			break;
		case XZ:
			t.x =  c * p.x +  s * p.z;
			t.z = -s * p.x +  c * p.z;
			break;
		case XW:
			t.x =  c * p.x +  s * p.w;
			t.w = -s * p.x +  c * p.w;
			break;
		case YZ:
			t.y =  c * p.y +  s * p.z;
			t.z = -s * p.y +  c * p.z;
			break;
		case YW:
			t.y = c * p.y - s * p.w;
			t.w = s * p.y + c * p.w;
			break;
		case ZW:
			t.z = c * p.z - s * p.w;
			t.w = s * p.z + c * p.w;
			break;
	}

	p = t;
}

void Mesh4D::rotate(RotationAxis4D axis, float angle) {
	float rad = glm::radians(angle);
	rotate_vertex(reference, axis, rad);

	for(int x = 0; x < transformed_vertices.size(); ++x) {
		rotate_vertex(transformed_vertices[x], axis, rad);
	}
}

void Mesh4D::draw(Scene::Transform &t, glm::mat4 const &world_to_clip) const {
	glm::mat4 local_to_world = t.make_local_to_world();
	glm::mat4 mvp = world_to_clip * local_to_world;
	//glm::mat4x3 mv = glm::mat4x3(local_to_world);
	//glm::mat3 itmv = glm::inverse(glm::transpose(glm::mat3(mv)));

	glUseProgram(tesseract_program->program);
	if(tesseract_program->object_to_clip_mat4 != 1U)
		glUniformMatrix4fv(tesseract_program->object_to_clip_mat4, 1, GL_FALSE, glm::value_ptr(mvp));

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, cur_vao_size * 7);
}

void Mesh4D::draw(Scene::Transform &t, Scene::Camera const *camera) const {
	glm::mat4 world_to_camera = camera->transform->make_world_to_local();
	glm::mat4 world_to_clip = camera->make_projection() * world_to_camera;

	draw(t, world_to_clip);
}