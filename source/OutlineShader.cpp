/* OutlineShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "OutlineShader.h"

#include "Color.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

using namespace std;

namespace {
	Shader shader;
	GL::GLint scaleI;
	GL::GLint offI;
	GL::GLint transformI;
	GL::GLint positionI;
	GL::GLint colorI;
	GL::GLint frameI;
	GL::GLint frameCountI;
	GL::GLint colorI;
	
	GL::GLuint vao;
	GL::GLuint vbo;
}



void OutlineShader::Init()
{
	static const char *vertexCode =
		"uniform vec2 scale;\n"
		"uniform vec2 position;\n"
		"uniform mat2 transform;\n"
		
		"attribute vec2 vert;\n"
		"attribute vec2 vertTexCoord;\n"
		"varying vec2 fragTexCoord;\n"
		
		"void main() {\n"
		"  fragTexCoord = vertTexCoord;\n"
		"  gl_Position = vec4((transform * vert + position) * scale, 0, 1);\n"
		"}\n";
	
	// The outline shader applies a Sobel filter to an image. The alpha channel
	// (i.e. the silhouette of the sprite) is given the most weight, but some
	// weight is also given to the RGB values so that there will be some detail
	// in the interior of the silhouette as well.
	
	// To reduce sampling error and bring out fine details, for every output
	// pixel the Sobel filter is actually applied in a 3x3 neighborhood and
	// averaged together. That neighborhood's scale is .618034 times the scale
	// of the Sobel neighborhood (i.e. the golden ratio) to minimize any
	// aliasing effects between the two.
	static const char *fragmentCode =
		"uniform sampler2DArray tex;\n"
		"uniform float frame = 0;\n"
		"uniform float frameCount = 0;\n"
		"uniform vec4 color = vec4(1, 1, 1, 1);\n"
		"uniform vec2 off;\n"
		"const vec4 weight = vec4(.4, .4, .4, 1.);\n"
		
		"attribute vec2 fragTexCoord;\n"
		
		"varying vec4 finalColor;\n"
		
		
		"  float sum = 0.0;\n"
		"  for(int dy = -1; dy <= 1; ++dy)\n"
		"  {\n"
		"    for(int dx = -1; dx <= 1; ++dx)\n"
		"    {\n"
		"      vec2 d = vec2(.618 * float(dx) * off.x, .618 * float(dy) * off.y);\n"
		"      float ae = texture2D(tex, d + vec2(tc.x - off.x, tc.y)).a;\n"
		"      float aw = texture2D(tex, d + vec2(tc.x + off.x, tc.y)).a;\n"
		"      float an = texture2D(tex, d + vec2(tc.x, tc.y - off.y)).a;\n"
		"      float as = texture2D(tex, d + vec2(tc.x, tc.y + off.y)).a;\n"
		"      float ane = texture2D(tex, d + vec2(tc.x - off.x, tc.y - off.y)).a;\n"
		"      float anw = texture2D(tex, d + vec2(tc.x + off.x, tc.y - off.y)).a;\n"
		"      float ase = texture2D(tex, d + vec2(tc.x - off.x, tc.y + off.y)).a;\n"
		"      float asw = texture2D(tex, d + vec2(tc.x + off.x, tc.y + off.y)).a;\n"
		"      float h = (ae * 2.0 + ane + ase) - (aw * 2.0 + anw + asw);\n"
		"      float v = (an * 2.0 + ane + anw) - (as * 2.0 + ase + asw);\n"
		"      sum += h * h + v * v;\n"
		"    }\n"
		"  }\n"
		"  gl_FragColor = color * sqrt(sum / 144.0);\n"
		"}\n"
		
		"void main() {\n"
		"  float first = floor(frame);\n"
		"  float second = mod(ceil(frame), frameCount);\n"
		"  float fade = frame - first;\n"
		"  float sum = mix(Sobel(first), Sobel(second), fade);\n"
		"  finalColor = color * sqrt(sum / 180);\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	offI = shader.Uniform("off");
	transformI = shader.Uniform("transform");
	positionI = shader.Uniform("position");
	frameI = shader.Uniform("frame");
	frameCountI = shader.Uniform("frameCount");
	colorI = shader.Uniform("color");
	
	gl->UseProgram(shader.Object());
	gl->Uniform1i(shader.Uniform("tex"), 0);
	gl->UseProgram(0);
	
	// Generate the vertex data for drawing sprites.
	gl->OES_vertex_array_object.GenVertexArrays(1, &vao);
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	gl->GenBuffers(1, &vbo);
	gl->BindBuffer(GL::ARRAY_BUFFER, vbo);
	
	GL::GLfloat vertexData[] = {
		-.5f, -.5f, 0.f, 0.f,
		 .5f, -.5f, 1.f, 0.f,
		-.5f,  .5f, 0.f, 1.f,
		 .5f,  .5f, 1.f, 1.f
	};
	gl->BufferData(GL::ARRAY_BUFFER, sizeof(vertexData), vertexData, GL::STATIC_DRAW);
	
	gl->EnableVertexAttribArray(shader.Attrib("vert"));
	gl->VertexAttribPointer(shader.Attrib("vert"), 2, GL::FLOAT, GL::FALSE,
		4 * sizeof(GL::GLfloat), NULL);
	
	gl->EnableVertexAttribArray(shader.Attrib("vertTexCoord"));
	gl->VertexAttribPointer(shader.Attrib("vertTexCoord"), 2, GL::FLOAT, GL::TRUE,
		4 * sizeof(GL::GLfloat), (const GL::GLvoid*)(2 * sizeof(GL::GLfloat)));
	
	// unbind the VBO and VAO
	gl->BindBuffer(GL::ARRAY_BUFFER, 0);
	gl->OES_vertex_array_object.BindVertexArray(0);
}



void OutlineShader::Draw(const Sprite *sprite, const Point &pos, const Point &size, const Color &color, const Point &unit, float frame)
{
	gl->UseProgram(shader.Object());
	gl->OES_vertex_array_object.BindVertexArray(vao);
	gl->ActiveTexture(GL::TEXTURE0);
	
	GL::GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	gl->Uniform2fv(scaleI, 1, scale);
	
	GL::GLfloat off[2] = {
		static_cast<float>(.5 / size.X()),
		static_cast<float>(.5 / size.Y())};
	gl->Uniform2fv(offI, 1, off);
	
	gl->Uniform1f(frameI, frame);
	gl->Uniform1f(frameCountI, sprite->Frames());
	
	Point uw = unit * size.X();
	Point uh = unit * size.Y();
	GL::GLfloat transform[4] = {
		static_cast<float>(-uw.Y()),
		static_cast<float>(uw.X()),
		static_cast<float>(-uh.X()),
		static_cast<float>(-uh.Y())
	};
	gl->UniformMatrix2fv(transformI, 1, false, transform);
	
	GL::GLfloat position[2] = {
		static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	gl->Uniform2fv(positionI, 1, position);
	
	gl->Uniform4fv(colorI, 1, color.Get());
	
	gl->BindTexture(GL_TEXTURE_2D, sprite->Texture(unit.Length() * Screen::Zoom() > 50.));
	
	gl->DrawArrays(GL::TRIANGLE_STRIP, 0, 4);
	
	gl->OES_vertex_array_object.BindVertexArray(0);
	gl->UseProgram(0);
}
