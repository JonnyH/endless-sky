/* SpriteShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SpriteShader.h"

#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

using namespace std;

namespace {
	Shader shader;
	GL::GLint scaleI;
	GL::GLint transformI;
	GL::GLint positionI;
	GL::GLint blurI;
	GL::GLint clipI;
	GL::GLint fadeI;
	GL::GLint texmatrixI;
	
	GL::GLuint vao;
	GL::GLuint vbo;

	static const GL::GLfloat SWIZZLE[9][4][4] = {
		{{1.0f, 0.0f, 0.0f, 0.0f},// red + yellow markings (republic)
		 {0.0f, 1.0f, 0.0f, 0.0f}, // R G B A
		 {0.0f, 0.0f, 1.0f, 0.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}},
		{{1.0f, 0.0f, 0.0f, 0.0f}, // red + magenta markings
		 {0.0f, 0.0f, 1.0f, 0.0f}, // R B G A
		 {0.0f, 1.0f, 0.0f, 0.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f, 0.0f}, // green + yellow (freeholders)
		 {1.0f, 0.0f, 0.0f, 0.0f}, // G R B A
		 {0.0f, 0.0f, 1.0f, 0.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}},
		{{0.0f, 0.0f, 1.0f, 0.0f}, // green + cyan
		 {1.0f, 0.0f, 0.0f, 0.0f}, // B R G A
		 {0.0f, 1.0f, 0.0f, 0.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f, 0.0f}, // blue + magenta (syndicate)
		 {0.0f, 0.0f, 1.0f, 0.0f}, // G B R A
		 {1.0f, 0.0f, 0.0f, 0.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}},
		{{0.0f, 0.0f, 1.0f, 0.0f}, // blue + cyan (merchant)
		 {0.0f, 1.0f, 0.0f, 0.0f}, // B G R A
		 {1.0f, 0.0f, 0.0f, 0.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f, 0.0f}, // red and black (pirate)
		 {0.0f, 0.0f, 1.0f, 0.0f}, // G B B A
		 {0.0f, 0.0f, 0.0f, 0.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}},
		{{0.0f, 0.0f, 1.0f, 0.0f}, // red only (cloaked)
		 {0.0f, 0.0f, 0.0f, 0.0f}, // B 0 0 A
		 {0.0f, 0.0f, 0.0f, 0.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}},
		{{0.0f, 0.0f, 0.0f, 0.0f}, // black only (outline)
		 {0.0f, 0.0f, 0.0f, 0.0f}, // 0 0 0 A
		 {0.0f, 0.0f, 0.0f, 0.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}},
	};
}



// Initialize the shaders.
void SpriteShader::Init()
{
	static const char *vertexCode =
		"uniform mat2 transform;\n"
		"uniform vec2 position;\n"
		"uniform vec2 scale;\n"
		"uniform vec2 blur;\n"
		"uniform float clip;\n"
		
		"attribute vec2 vert;\n"
		"varying vec2 fragTexCoord;\n"
		
		"void main() {\n"
		"  vec2 blurOff = 2.0 * vec2(vert.x * abs(blur.x), vert.y * abs(blur.y));\n"
		"  gl_Position = vec4((transform * (vert + blurOff) + position) * scale, 0.0, 1.0);\n"
		"  vec2 texCoord = vert + vec2(.5, .5);\n"
		"  fragTexCoord = vec2(texCoord.x, max(clip, texCoord.y)) + blurOff;\n"
		"}\n";

	static const char *fragmentCode =
		"uniform sampler2D tex0;\n"
		"uniform sampler2D tex1;\n"
		"uniform float fade;\n"
		"uniform vec2 blur;\n"
		"uniform mat4 tex_matrix;\n"
		"const int range = 5;\n"
		
		"varying vec2 fragTexCoord;\n"
		
		"void main() {\n"
		"  if(blur.x == 0.0 && blur.y == 0.0)\n"
		"  {\n"
		"    if(fade != 0.0)\n"
		"     gl_FragColor = tex_matrix * mix(texture2D(tex0, fragTexCoord), texture2D(tex1, fragTexCoord), fade);\n"
		"    else\n"
		"      gl_FragColor = tex_matrix * texture2D(tex0, fragTexCoord);\n"
		"    return;\n"
		"  }\n"
		"  const float divisor = float(range) * (float(range) + 2.0) + 1.0;\n"
		"  vec4 color = vec4(0., 0., 0., 0.);\n"
		"  for(int i = -range; i <= range; ++i)\n"
		"  {\n"
		"    float scale = (float(range) + 1.0 - abs(float(i))) / divisor;\n"
		"    vec2 coord = fragTexCoord + (blur * float(i)) / float(range);\n"
		"    if(fade != 0.0)\n"
		"      color += scale * mix(texture2D(tex0, coord), texture2D(tex1, coord), fade);\n"
		"    else\n"
		"      color += scale * texture2D(tex0, coord);\n"
		"  }\n"
		"  gl_FragColor = tex_matrix * color;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	transformI = shader.Uniform("transform");
	positionI = shader.Uniform("position");
	blurI = shader.Uniform("blur");
	clipI = shader.Uniform("clip");
	fadeI = shader.Uniform("fade");
	texmatrixI = shader.Uniform("tex_matrix");
	
	gl->UseProgram(shader.Object());
	gl->Uniform1i(shader.Uniform("tex0"), 0);
	gl->Uniform1i(shader.Uniform("tex1"), 1);
	gl->UseProgram(0);
	
	// Generate the vertex data for drawing sprites.
	gl->OES_vertex_array_object.GenVertexArrays(1, &vao);
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	gl->GenBuffers(1, &vbo);
	gl->BindBuffer(GL::ARRAY_BUFFER, vbo);
	
	GL::GLfloat vertexData[] = {
		-.5f, -.5f,
		-.5f,  .5f,
		 .5f, -.5f,
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



void SpriteShader::Draw(const Sprite *sprite, const Point &position, float zoom, int swizzle)
{
	if(!sprite)
		return;
	
	float pos[2] = {
		static_cast<float>(position.X()), static_cast<float>(position.Y())};
	float trans[4] = {sprite->Width() * zoom, 0.f, 0.f, sprite->Height() * zoom};
	
	Bind();
	Add(sprite->Texture(), 0, pos, trans, swizzle, 1.f, 0.f);
	Unbind();
}



void SpriteShader::Bind()
{
	gl->UseProgram(shader.Object());
	gl->OES_vertex_array_object.BindVertexArray(vao);
	gl->ActiveTexture(GL::TEXTURE0);
	
	GL::GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	gl->Uniform2fv(scaleI, 1, scale);
}



void SpriteShader::Add(uint32_t tex0, uint32_t tex1, const float position[2], const float transform[4], int swizzle, float clip, float fade, const float blur[2])
{
	gl->UniformMatrix2fv(transformI, 1, false, transform);
	gl->Uniform2fv(positionI, 1, position);
	gl->BindTexture(GL::TEXTURE_2D, tex0);
	
	if(fade && tex1)
	{
		gl->ActiveTexture(GL::TEXTURE1);
		gl->BindTexture(GL::TEXTURE_2D, tex1);
		gl->ActiveTexture(GL::TEXTURE0);
	}
	else
		fade = 0.f;
	
	// Set the color swizzle.
	gl->UniformMatrix4fv(texmatrixI, 1, GL::FALSE, &SWIZZLE[swizzle][0][0]);
	
	// Set the clipping.
	gl->Uniform1f(clipI, 1.f - clip);
	gl->Uniform1f(fadeI, fade);
	const float noBlur[2] = {0.f, 0.f};
	gl->Uniform2fv(blurI, 1, blur ? blur : noBlur);
	
	gl->DrawArrays(GL::TRIANGLE_STRIP, 0, 4);
}



void SpriteShader::Unbind()
{
	gl->OES_vertex_array_object.BindVertexArray(0);
	gl->UseProgram(0);
}
