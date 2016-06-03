/* FillShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FillShader.h"

#include "Color.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	Shader shader;
	GL::GLint scaleI;
	GL::GLint centerI;
	GL::GLint sizeI;
	GL::GLint colorI;
	
	GL::GLuint vao;
	GL::GLuint vbo;
}



void FillShader::Init()
{
	static const char *vertexCode =
		"uniform vec2 scale;\n"
		"uniform vec2 center;\n"
		"uniform vec2 size;\n"
		
		"attribute vec2 vert;\n"
		
		"void main() {\n"
		"  gl_Position = vec4((center + vert * size) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"uniform vec4 color;\n"
		
		"void main() {\n"
		"  gl_FragColor = color;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	centerI = shader.Uniform("center");
	sizeI = shader.Uniform("size");
	colorI = shader.Uniform("color");
	
	// Generate the vertex data for drawing sprites.
	gl->OES_vertex_array_object.GenVertexArrays(1, &vao);
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	gl->GenBuffers(1, &vbo);
	gl->BindBuffer(GL::ARRAY_BUFFER, vbo);
	
	GL::GLfloat vertexData[] = {
		-.5f, -.5f,
		 .5f, -.5f,
		-.5f,  .5f,
		 .5f,  .5f
	};
	gl->BufferData(GL::ARRAY_BUFFER, sizeof(vertexData), vertexData, GL::STATIC_DRAW);
	
	gl->EnableVertexAttribArray(shader.Attrib("vert"));
	gl->VertexAttribPointer(shader.Attrib("vert"), 2, GL::FLOAT, GL::FALSE,
		2 * sizeof(GL::GLfloat), NULL);
	
	// unbind the VBO and VAO
	gl->BindBuffer(GL::ARRAY_BUFFER, 0);
	gl->OES_vertex_array_object.BindVertexArray(0);
}



void FillShader::Fill(const Point &center, const Point &size, const Color &color)
{
	if(!shader.Object())
		throw runtime_error("FillShader: Draw() called before Init().");
	
	gl->UseProgram(shader.Object());
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	GL::GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	gl->Uniform2fv(scaleI, 1, scale);
	
	GL::GLfloat centerV[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	gl->Uniform2fv(centerI, 1, centerV);
	
	GL::GLfloat sizeV[2] = {static_cast<float>(size.X()), static_cast<float>(size.Y())};
	gl->Uniform2fv(sizeI, 1, sizeV);
	
	gl->Uniform4fv(colorI, 1, color.Get());
	
	gl->DrawArrays(GL::TRIANGLE_STRIP, 0, 4);
	
	gl->OES_vertex_array_object.BindVertexArray(0);
	gl->UseProgram(0);
}
