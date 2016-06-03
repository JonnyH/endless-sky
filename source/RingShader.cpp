/* RingShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "RingShader.h"

#include "Color.h"
#include "pi.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	Shader shader;
	GL::GLint scaleI;
	GL::GLint positionI;
	GL::GLint radiusI;
	GL::GLint widthI;
	GL::GLint angleI;
	GL::GLint startAngleI;
	GL::GLint dashI;
	GL::GLint colorI;
	
	GL::GLuint vao;
	GL::GLuint vbo;
}



void RingShader::Init()
{
	static const char *vertexCode =
		"uniform vec2 scale;\n"
		"uniform vec2 position;\n"
		"uniform float radius;\n"
		"uniform float width;\n"
		
		"attribute vec2 vert;\n"
		"varying vec2 coord;\n"
		
		"void main() {\n"
		"  coord = (radius + width) * vert;\n"
		"  gl_Position = vec4((coord + position) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"uniform vec4 color;\n"
		"uniform float radius;\n"
		"uniform float width;\n"
		"uniform float angle;\n"
		"uniform float startAngle;\n"
		"uniform float dash;\n"
		"const float pi = 3.1415926535897932384626433832795;\n"
		
		"varying vec2 coord;\n"
		
		"void main() {\n"
		"  float arc = mod(atan(coord.x, coord.y) + pi + startAngle, 2.0 * pi);\n"
		"  float arcFalloff = 1.0 - min(2.0 * pi - arc, arc - angle) * radius;\n"
		"  if(dash != 0.0)\n"
		"  {\n"
		"    arc = mod(arc, dash);\n"
		"    arcFalloff = min(arcFalloff, min(arc, dash - arc) * radius);\n"
		"  }\n"
		"  float len = length(coord);\n"
		"  float lenFalloff = width - abs(len - radius);\n"
		"  float alpha = clamp(min(arcFalloff, lenFalloff), 0.0, 1.0);\n"
		"  gl_FragColor = color * alpha;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	positionI = shader.Uniform("position");
	radiusI = shader.Uniform("radius");
	widthI = shader.Uniform("width");
	angleI = shader.Uniform("angle");
	startAngleI = shader.Uniform("startAngle");
	dashI = shader.Uniform("dash");
	colorI = shader.Uniform("color");
	
	// Generate the vertex data for drawing sprites.
	gl->OES_vertex_array_object.GenVertexArrays(1, &vao);
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	gl->GenBuffers(1, &vbo);
	gl->BindBuffer(GL::ARRAY_BUFFER, vbo);
	
	GL::GLfloat vertexData[] = {
		-1.f, -1.f,
		-1.f,  1.f,
		 1.f, -1.f,
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



void RingShader::Draw(const Point &pos, float out, float in, const Color &color)
{
	float width = .5f * (1.f + out - in) ;
	Draw(pos, out - width, width, 1.f, color);
}



void RingShader::Draw(const Point &pos, float radius, float width, float fraction, const Color &color, float dash, float startAngle)
{
	Bind();
	
	Add(pos, radius, width, fraction, color, dash, startAngle);
	
	Unbind();
}



void RingShader::Bind()
{
	if(!shader.Object())
		throw runtime_error("RingShader: Bind() called before Init().");
	
	gl->UseProgram(shader.Object());
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	GL::GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	gl->Uniform2fv(scaleI, 1, scale);
}



void RingShader::Add(const Point &pos, float out, float in, const Color &color)
{
	float width = .5f * (1.f + out - in) ;
	Add(pos, out - width, width, 1.f, color);
}



void RingShader::Add(const Point &pos, float radius, float width, float fraction, const Color &color, float dash, float startAngle)
{
	GL::GLfloat position[2] = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	gl->Uniform2fv(positionI, 1, position);
	
	gl->Uniform1f(radiusI, radius);
	gl->Uniform1f(widthI, width);
	gl->Uniform1f(angleI, fraction * 2. * PI);
	gl->Uniform1f(startAngleI, startAngle * TO_RAD);
	gl->Uniform1f(dashI, dash ? 2. * PI / dash : 0.);
	
	gl->Uniform4fv(colorI, 1, color.Get());
	
	gl->DrawArrays(GL::TRIANGLE_STRIP, 0, 4);
}



void RingShader::Unbind()
{
	gl->OES_vertex_array_object.BindVertexArray(0);
	gl->UseProgram(0);
}
