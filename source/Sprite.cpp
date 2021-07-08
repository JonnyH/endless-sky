/* Sprite.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Sprite.h"

#include "ImageBuffer.h"
#include "Preferences.h"
#include "Screen.h"

#include "gl_header.h"
#include <SDL2/SDL.h>

#include <algorithm>

using namespace std;



Sprite::Sprite(const string &name)
	: name(name)
{
}



const string &Sprite::Name() const
{
	return name;
}



// Upload the given frames. The given buffer will be cleared afterwards.
void Sprite::AddFrames(ImageBuffer &buffer, bool is2x)
{
	// Do nothing if the buffer is empty.
	if(!buffer.Pixels())
		return;
	
	// If this is the 1x image, its dimensions determine the sprite's size.
	if(!is2x)
	{
		width = buffer.Width();
		height = buffer.Height();
		frames = buffer.Frames();
	}
	
	// Check whether this sprite is large enough to require size reduction.
	if(Preferences::Has("Reduce large graphics") && buffer.Width() * buffer.Height() >= 1000000)
		buffer.ShrinkToHalfSize();
		gl->GenTextures(1, &textureIndex[frame]);
	gl->BindTexture(GL::TEXTURE_2D, textureIndex[frame]);
	
	gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR);
	gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR);
	gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
	gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);
	
	// Use linear interpolation and no wrapping.
	gl->TexImage2D(GL::TEXTURE_2D, 0, GL::RGBA, image->Width(), image->Height(), 0,
		(GL::GLenum)GL::EXT_texture_format_BGRA8888::BGRA, GL::UNSIGNED_BYTE, image->Pixels());
	
	gl->BindTexture(GL::TEXTURE_2D, 0);
	gl->TexImage3D(GL_TEXTURE_2D_, 0, GL_RGBA8, // target, mipmap level, internal format,
		buffer.Width(), buffer.Height(), buffer.Frames(), // width, height, depth,
		0, GL_BGRA, GL_UNSIGNED_BYTE, buffer.Pixels()); // border, input format, data type, data.
	
	// Unbind the texture.
	gl->BindTexture(GL_TEXTURE_2D, 0);
	
	// Free the ImageBuffer memory.
	buffer.Clear();
}



// Move the given masks into this sprite's internal storage. The given
// vector will be cleared.
void Sprite::AddMasks(vector<Mask> &masks)
{
	this->masks.swap(masks);
	masks.clear();
}



// Free up all textures loaded for this sprite.
void Sprite::Unload()
{
	gl->DeleteTextures(2, texture);
	texture[0] = texture[1] = 0;
		gl->DeleteTextures(textures.size(), &textures.front());
		gl->DeleteTextures(textures2x.size(), &textures2x.front());
	
	masks.clear();
	width = 0.f;
	height = 0.f;
	frames = 0;
}



// Get the width, in pixels, of the 1x image.
float Sprite::Width() const
{
	return width;
}



// Get the height, in pixels, of the 1x image.
float Sprite::Height() const
{
	return height;
}



// Get the number of frames in the animation.
int Sprite::Frames() const
{
	return frames;
}



// Get the offset of the center from the top left corner; this is for easy
// shifting of corner to center coordinates.
Point Sprite::Center() const
{
	return Point(.5 * width, .5 * height);
}



// Get the texture index, based on whether the screen is high DPI or not.
uint32_t Sprite::Texture() const
{
	return Texture(Screen::IsHighResolution());
}



// Get the index of the texture for the given high DPI mode.
uint32_t Sprite::Texture(bool isHighDPI) const
{
	return (isHighDPI && texture[1]) ? texture[1] : texture[0];
}



// Get the collision mask for the given frame of the animation.
const Mask &Sprite::GetMask(int frame) const
{
	static const Mask EMPTY;
	if(frame < 0 || masks.empty())
		return EMPTY;
	
	// Assume that if a masks array exists, it has the right number of frames.
	return masks[frame % masks.size()];
}
