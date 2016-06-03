/* Font.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Font.h"

#include "Color.h"
#include "ImageBuffer.h"
#include "Point.h"
#include "Screen.h"

#include <cmath>
#include <cstring>

using namespace std;

namespace {
	static bool showUnderlines = false;
	
	static const char *vertexCode =
		// "scale" maps pixel coordinates to GL coordinates (-1 to 1).
		"uniform vec2 scale;\n"
		// The (x, y) coordinates of the top left corner of the glyph.
		"uniform vec2 position;\n"
		// The glyph to draw. (ASCII value - 32).
		"uniform int glyph;\n"
		// Aspect ratio of rendered glyph (unity by default).
		"uniform float aspect;\n"
		
		// Inputs from the VBO.
		"attribute vec2 vert;\n"
		"attribute vec2 corner;\n"
		
		// Output to the fragment shader.
		"varying vec2 texCoord;\n"
		
		// Pick the proper glyph out of the texture.
		"void main() {\n"
		"  texCoord = vec2((float(glyph) + corner.x) / 98.f, corner.y);\n"
		"  gl_Position = vec4((aspect * vert.x + position.x) * scale.x, (vert.y + position.y) * scale.y, 0, 1);\n"
		"}\n";
	
	static const char *fragmentCode =
		// The user must supply a texture and a color (white by default).
		"uniform sampler2D tex;\n"
		"uniform vec4 color;\n"
		
		// This comes from the vertex shader.
		"varying vec2 texCoord;\n"
		
		// Multiply the texture by the user-specified color (including alpha).
		"void main() {\n"
		"  gl_FragColor = texture2D(tex, texCoord).a * color;\n"
		"}\n";
	
	static const int KERN = 2;
}



Font::Font()
	: texture(0), vao(0), vbo(0), colorI(0), scaleI(0), glyphI(0), aspectI(0),
	  positionI(0), height(0), space(0), screenWidth(0), screenHeight(0)
{
}



Font::Font(const string &imagePath)
	: Font()
{
	Load(imagePath);
}



void Font::Load(const string &imagePath)
{
	// Load the texture.
	ImageBuffer *image = ImageBuffer::Read(imagePath);
	if(!image)
		return;
	
	LoadTexture(image);
	CalculateAdvances(image);
	SetUpShader(image->Width() / GLYPHS, image->Height());
	
	delete image;
}



void Font::Draw(const string &str, const Point &point, const Color &color) const
{
	DrawAliased(str, round(point.X()), round(point.Y()), color);
}



void Font::DrawAliased(const std::string &str, double x, double y, const Color &color) const
{
	gl->UseProgram(shader.Object());
	gl->ActiveTexture(GL::TEXTURE0);
	gl->BindTexture(GL::TEXTURE_2D, texture);
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	gl->Uniform4fv(colorI, 1, color.Get());
	
	// Update the scale, only if the screen size has changed.
	if(Screen::Width() != screenWidth || Screen::Height() != screenHeight)
	{
		screenWidth = Screen::Width();
		screenHeight = Screen::Height();
		GL::GLfloat scale[2] = {2.f / screenWidth, -2.f / screenHeight};
		gl->Uniform2fv(scaleI, 1, scale);
	}
	
	GL::GLfloat textPos[2] = {
		static_cast<float>(x - 1.),
		static_cast<float>(y)};
	int previous = 0;
	bool isAfterSpace = true;
	bool underlineChar = false;
	const int underscoreGlyph = max(0, min(GLYPHS - 1, '_' - 32));
	
	for(char c : str)
	{
		if(c == '_')
		{
			underlineChar = showUnderlines;
			continue;
		}
		
		int glyph = Glyph(c, isAfterSpace);
		if(c != '"' && c != '\'')
			isAfterSpace = !glyph;
		if(!glyph)
		{
			textPos[0] += space;
			continue;
		}
		
		gl->Uniform1i(glyphI, glyph);
		gl->Uniform1f(aspectI, 1.f);
		
		textPos[0] += advance[previous * GLYPHS + glyph] + KERN;
		gl->Uniform2fv(positionI, 1, textPos);
		
		gl->DrawArrays(GL::TRIANGLE_STRIP, 0, 4);
		
		if(underlineChar)
		{
			gl->Uniform1i(glyphI, underscoreGlyph);
			gl->Uniform1f(aspectI, static_cast<float>(advance[glyph * GLYPHS] + KERN)
				/ (advance[underscoreGlyph * GLYPHS] + KERN));
			
			gl->Uniform2fv(positionI, 1, textPos);
			
			gl->DrawArrays(GL::TRIANGLE_STRIP, 0, 4);
			underlineChar = false;
		}
		
		previous = glyph;
	}
}



int Font::Width(const string &str, char after) const
{
	return Width(str.c_str(), after);
}



int Font::Width(const char *str, char after) const
{
	int width = 0;
	int previous = 0;
	bool isAfterSpace = true;
	
	for( ; *str; ++str)
	{
		if(*str == '_')
			continue;
		
		int glyph = Glyph(*str, isAfterSpace);
		if(*str != '"' && *str != '\'')
			isAfterSpace = !glyph;
		if(!glyph)
			width += space;
		else
		{
			width += advance[previous * GLYPHS + glyph] + KERN;
			previous = glyph;
		}
	}
	width += advance[previous * GLYPHS + max(0, min(GLYPHS - 1, after - 32))];
	
	return width;
}



int Font::Height() const
{
	return height;
}



int Font::Space() const
{
	return space;
}



void Font::ShowUnderlines(bool show)
{
	showUnderlines = show;
}



int Font::Glyph(char c, bool isAfterSpace)
{
	// Curly quotes.
	if(c == '\'' && isAfterSpace)
		return 96;
	if(c == '"' && isAfterSpace)
		return 97;
	
	return max(0, min(GLYPHS - 3, c - 32));
}



void Font::LoadTexture(ImageBuffer *image)
{
	gl->GenTextures(1, &texture);
	gl->BindTexture(GL::TEXTURE_2D, texture);
	
	gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR);
	gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR);
	gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
	gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);
	
	gl->TexImage2D(GL::TEXTURE_2D, 0, GL::RGBA, image->Width(), image->Height(), 0,
		(GL::GLenum)GL::EXT_texture_format_BGRA8888::BGRA, GL::UNSIGNED_BYTE, image->Pixels());
}



void Font::CalculateAdvances(ImageBuffer *image)
{
	// Get the format and size of the surface.
	int width = image->Width() / GLYPHS;
	height = image->Height();
	unsigned mask = 0xFF000000;
	unsigned half = 0xC0000000;
	int pitch = image->Width();
	
	// advance[previous * GLYPHS + next] is the x advance for each glyph pair.
	// There is no advance if the previous value is 0, i.e. we are at the very
	// start of a string.
	memset(advance, 0, GLYPHS * sizeof(advance[0]));
	for(int previous = 1; previous < GLYPHS; ++previous)
		for(int next = 0; next < GLYPHS; ++next)
		{
			int maxD = 0;
			int glyphWidth = 0;
			uint32_t *begin = reinterpret_cast<uint32_t *>(image->Pixels());
			for(int y = 0; y < height; ++y)
			{
				// Find the last non-empty pixel in the previous glyph.
				uint32_t *pend = begin + previous * width;
				uint32_t *pit = pend + width;
				while(pit != pend && (*--pit & mask) < half) {}
				int distance = (pit - pend) + 1;
				glyphWidth = max(distance, glyphWidth);
				
				// Special case: if "next" is zero (i.e. end of line of text),
				// calculate the full width of this character. Otherwise:
				if(next)
				{
					// Find the first non-empty pixel in this glyph.
					uint32_t *nit = begin + next * width;
					uint32_t *nend = nit + width;
					while(nit != nend && (*nit++ & mask) < half) {}
					
					// How far apart do you want these glyphs drawn? If drawn at
					// an advance of "width", there would be:
					// pend + width - pit   <- pixels after the previous glyph.
					// nit - (nend - width) <- pixels before the next glyph.
					// So for zero kerning distance, you would want:
					distance += 1 - (nit - (nend - width));
				}
				maxD = max(maxD, distance);
				
				// Update the pointer to point to the beginning of the next row.
				begin += pitch;
			}
			// This is a fudge factor to avoid over-kerning, especially for the
			// underscore and for glyph combinations like AV.
			advance[previous * GLYPHS + next] = max(maxD, glyphWidth - 4) / 2;
		}
	
	// Set the space size based on the character width.
	width /= 2;
	height /= 2;
	space = (width + 3) / 6 + 1;
}



void Font::SetUpShader(float glyphW, float glyphH)
{
	glyphW *= .5f;
	glyphH *= .5f;
	
	shader = Shader(vertexCode, fragmentCode);
	gl->UseProgram(shader.Object());
	
	// Create the VAO and VBO.
	gl->OES_vertex_array_object.GenVertexArrays(1, &vao);
	gl->OES_vertex_array_object.BindVertexArray(vao);
	
	gl->GenBuffers(1, &vbo);
	gl->BindBuffer(GL::ARRAY_BUFFER, vbo);
	
	GL::GLfloat vertices[] = {
		   0.f,    0.f, 0.f, 0.f,
		   0.f, glyphH, 0.f, 1.f,
		glyphW,    0.f, 1.f, 0.f,
		glyphW, glyphH, 1.f, 1.f
	};
	gl->BufferData(GL::ARRAY_BUFFER, sizeof(vertices), vertices, GL::STATIC_DRAW);
	
	// connect the xy to the "vert" attribute of the vertex shader
	gl->EnableVertexAttribArray(shader.Attrib("vert"));
	gl->VertexAttribPointer(shader.Attrib("vert"), 2, GL::FLOAT, GL::FALSE,
		4 * sizeof(GL::GLfloat), NULL);
	
	gl->EnableVertexAttribArray(shader.Attrib("corner"));
	gl->VertexAttribPointer(shader.Attrib("corner"), 2, GL::FLOAT, GL::FALSE,
		4 * sizeof(GL::GLfloat), (const GL::GLvoid*)(2 * sizeof(GL::GLfloat)));
	
	// We must update the screen size next time we draw.
	screenWidth = 0;
	screenHeight = 0;
	
	// The texture always comes from texture unit 0.
	gl->Uniform1i(shader.Uniform("tex"), 0);

	colorI = shader.Uniform("color");
	scaleI = shader.Uniform("scale");
	glyphI = shader.Uniform("glyph");
	aspectI = shader.Uniform("aspect");
	positionI = shader.Uniform("position");
}
