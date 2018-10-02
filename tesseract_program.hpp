#include "GL.hpp"
#include "Load.hpp"

struct TesseractProgram {
	GLuint program = 0;

	GLuint object_to_clip_mat4 = -1U;

	TesseractProgram();
};

extern Load< TesseractProgram > tesseract_program;