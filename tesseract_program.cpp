#include "tesseract_program.hpp"

#include "compile_program.hpp"
#include "gl_errors.hpp"

TesseractProgram::TesseractProgram() {
	program = compile_program(
		"#version 330\n"
		"uniform mat4 object_to_clip;\n"
		"layout(location=0) in vec4 Position;\n" //note: layout keyword used to make sure that the location-0 attribute is always bound to something
		"in vec4 Color;\n"
		"out vec4 color;\n"
		"void main() {\n"
		"	gl_Position = object_to_clip * Position;\n"
		"	color = Color;\n"
		"}\n"
		,
		"#version 330\n"
		"in vec4 color;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = color;\n"
		"}\n"
	);

	object_to_clip_mat4 = glGetUniformLocation(program, "object_to_clip");

	GL_ERRORS();
}

Load< TesseractProgram > tesseract_program(LoadTagInit, [](){
	return new TesseractProgram();
});
