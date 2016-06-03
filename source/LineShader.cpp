/* LineShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "LineShader.h"

#include "Color.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	Shader shader;
	GL::GLint scaleI;
	GL::GLint startI;
	GL::GLint lengthI;
	GL::GLint widthI;
	GL::GLint colorI;
	
	GL::GLuint vao;
	GL::GLuint vbo;
}



void LineShader::Init()
{
	static const char *vertexCode =
		"uniform vec2 scale;\n"
		"uniform vec2 start;\n"
		"uniform vec2 len;\n"
		"uniform vec2 width;\n"
		
		"attribute vec2 vert;\n"
		"varying vec2 tpos;\n"
		"varying float tscale;\n"
		
		"void main() {\n"
		"  tpos = vert;\n"
		"  tscale = length(len);\n"
		"  gl_Position = vec4((start + vert.x * len + vert.y * width) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"uniform vec4 color;\n"
		
		"varying vec2 tpos;\n"
		"varying float tscale;\n"
		
		"void main() {\n"
		"  float alpha = min(tscale - abs(tpos.x * (2.0 * tscale) - tscale), 1.0 - abs(tpos.y));\n"
		"  gl_FragColor = color * alpha;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	startI = shader.Uniform("start");
	lengthI = shader.Uniform("len");
	widthI = shader.Uniform("width");
	colorI = shader.Uniform("color");
	
	// Generate the vertex data for drawing sprites.
	gl->OES_vertex_array_object.GenVertexArrays(1, &vao);
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	gl->GenBuffers(1, &vbo);
	gl->BindBuffer(GL::ARRAY_BUFFER, vbo);
	
	GL::GLfloat vertexData[] = {
		0.f, -1.f,
		1.f, -1.f,
		0.f,  1.f,
		1.f,  1.f
	};
	gl->BufferData(GL::ARRAY_BUFFER, sizeof(vertexData), vertexData, GL::STATIC_DRAW);
	
	gl->EnableVertexAttribArray(shader.Attrib("vert"));
	gl->VertexAttribPointer(shader.Attrib("vert"), 2, GL::FLOAT, GL::FALSE,
		2 * sizeof(GL::GLfloat), NULL);
	
	// unbind the VBO and VAO
	gl->BindBuffer(GL::ARRAY_BUFFER, 0);
	gl->OES_vertex_array_object.BindVertexArray(0);
}



void LineShader::Draw(const Point &from, const Point &to, float width, const Color &color)
{
	if(!shader.Object())
		throw runtime_error("LineShader: Draw() called before Init().");
	
	gl->UseProgram(shader.Object());
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	GL::GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	gl->Uniform2fv(scaleI, 1, scale);
	
	GL::GLfloat start[2] = {static_cast<float>(from.X()), static_cast<float>(from.Y())};
	gl->Uniform2fv(startI, 1, start);
	
	Point v = to - from;
	Point u = v.Unit() * width;
	GL::GLfloat length[2] = {static_cast<float>(v.X()), static_cast<float>(v.Y())};
	gl->Uniform2fv(lengthI, 1, length);
	
	GL::GLfloat w[2] = {static_cast<float>(u.Y()), static_cast<float>(-u.X())};
	gl->Uniform2fv(widthI, 1, w);
	
	gl->Uniform4fv(colorI, 1, color.Get());
	
	gl->DrawArrays(GL::TRIANGLE_STRIP, 0, 4);
	
	gl->OES_vertex_array_object.BindVertexArray(0);
	gl->UseProgram(0);
}
