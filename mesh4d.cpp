#include "mesh4d.hpp"

#include <cstddef>
#include <glm/glm.hpp>

Mesh4D::Mesh4D(std::vector<glm::vec4> vertices, std::vector<int> edges, std::vector<int> tris) :
		vertices(vertices), edges(edges), tris(tris) {
	transformed_vertices = std::vector<glm::vec4>(vertices);
	projected_vertices = std::vector<Vertex>(vertices.size());

	glGenBuffers(1, &vbo);
	glGenBuffers(1, &vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(
		GL_ARRAY_BUFFER, 
		projected_vertices.size() * sizeof(Vertex), 
		projected_vertices.data(), 
		GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Position = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
	Color = Attrib(4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), offsetof(Vertex, color));
}

void Mesh4D::apply_perspective() {
	for(int x = 0; x < transformed_vertices.size(); ++x) {
		glm::vec4& cur_r4 = transformed_vertices[x];
		glm::vec3& cur_r3 = projected_vertices[x];

		double norm_factor = 1.0 / (cur_r4.w - camera_position_w);
		cur_r3.x = cur_r4.x * norm_factor;
		cur_r3.y = cur_r4.y * norm_factor;
		cur_r3.z = cur_r4.z * norm_factor;
	}
}

void Mesh4D::upload_vertex_data() {

}

static inline void rotate_vertex(glm::vec4& p, RotationAxis4D axis, float rad) {
	double s = sin(angle);
	double c = cos(angle);

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

}