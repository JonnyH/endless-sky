/* PointerShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PointerShader.h"

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
	GL::GLint angleI;
	GL::GLint sizeI;
	GL::GLint offsetI;
	GL::GLint colorI;
	
	GL::GLuint vao;
	GL::GLuint vbo;
}



void PointerShader::Init()
{
	static const char *vertexCode =
		"uniform vec2 scale;\n"
		"uniform vec2 center;\n"
		"uniform vec2 angle;\n"
		"uniform vec2 size;\n"
		"uniform float offset;\n"
		
		"attribute vec2 vert;\n"
		"varying vec2 coord;\n"
		
		"void main() {\n"
		"  coord = vert * size.x;\n"
		"  vec2 base = center + angle * (offset - size.y * (vert.x + vert.y));\n"
		"  vec2 wing = vec2(angle.y, -angle.x) * (size.x * .5 * (vert.x - vert.y));\n"
		"  gl_Position = vec4((base + wing) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"uniform vec4 color;\n"
		"uniform vec2 size;\n"
		
		"varying vec2 coord;\n"
		
		"void main() {\n"
		"  float height = (coord.x + coord.y) / size.x;\n"
		"  float taper = height * height * height;\n"
		"  taper *= taper * .5 * size.x;\n"
		"  float alpha = clamp(.8 * min(coord.x, coord.y) - taper, 0.0, 1.0);\n"
		"  alpha *= clamp(1.8 * (1. - height), 0.0, 1.0);\n"
		"  gl_FragColor = color * alpha;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	centerI = shader.Uniform("center");
	angleI = shader.Uniform("angle");
	sizeI = shader.Uniform("size");
	offsetI = shader.Uniform("offset");
	colorI = shader.Uniform("color");
	
	// Generate the vertex data for drawing sprites.
	gl->OES_vertex_array_object.GenVertexArrays(1, &vao);
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	gl->GenBuffers(1, &vbo);
	gl->BindBuffer(GL::ARRAY_BUFFER, vbo);
	
	GL::GLfloat vertexData[] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
	};
	gl->BufferData(GL::ARRAY_BUFFER, sizeof(vertexData), vertexData, GL::STATIC_DRAW);
	
	gl->EnableVertexAttribArray(shader.Attrib("vert"));
	gl->VertexAttribPointer(shader.Attrib("vert"), 2, GL::FLOAT, GL::FALSE,
		2 * sizeof(GL::GLfloat), NULL);
	
	// unbind the VBO and VAO
	gl->BindBuffer(GL::ARRAY_BUFFER, 0);
	gl->OES_vertex_array_object.BindVertexArray(0);
}



void PointerShader::Draw(const Point &center, const Point &angle, float width, float height, float offset, const Color &color)
{
	Bind();
	
	Add(center, angle, width, height, offset, color);
	
	Unbind();
}


	
void PointerShader::Bind()
{
	if(!shader.Object())
		throw runtime_error("PointerShader: Bind() called before Init().");
	
	gl->UseProgram(shader.Object());
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	GL::GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	gl->Uniform2fv(scaleI, 1, scale);
}



void PointerShader::Add(const Point &center, const Point &angle, float width, float height, float offset, const Color &color)
{
	GL::GLfloat c[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	gl->Uniform2fv(centerI, 1, c);
	
	GL::GLfloat a[2] = {static_cast<float>(angle.X()), static_cast<float>(angle.Y())};
	gl->Uniform2fv(angleI, 1, a);
	
	GL::GLfloat size[2] = {width, height};
	gl->Uniform2fv(sizeI, 1, size);
	
	gl->Uniform1f(offsetI, offset);
	
	gl->Uniform4fv(colorI, 1, color.Get());
	
	gl->DrawArrays(GL::TRIANGLES, 0, 3);
}



void PointerShader::Unbind()
{
	gl->OES_vertex_array_object.BindVertexArray(0);
	gl->UseProgram(0);
}
