/* Shader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Shader.h"

#include "Files.h"

#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;



Shader::Shader(const char *vertex, const char *fragment)
{
	GL::GLuint vertexShader = Compile(vertex, GL::VERTEX_SHADER);
	GL::GLuint fragmentShader = Compile(fragment, GL::FRAGMENT_SHADER);
	
	program = gl->CreateProgram();
	if(!program)
		throw runtime_error("Creating OpenGL shader program failed.");
	
	gl->AttachShader(program, vertexShader);
	gl->AttachShader(program, fragmentShader);
	
	gl->LinkProgram(program);
	
	gl->DetachShader(program, vertexShader);
	gl->DetachShader(program, fragmentShader);
	
	GL::GLint status;
	gl->GetProgramiv(program, GL::LINK_STATUS, &status);
	if(status == GL::FALSE)
		throw runtime_error("Linking OpenGL shader program failed.");
}



GL::GLuint Shader::Object() const
{
	return program;
}



GL::GLint Shader::Attrib(const char *name) const
{
	GL::GLint attrib = gl->GetAttribLocation(program, name);
	if(attrib == -1)
		throw runtime_error("Attribute \"" + string(name) + "\" not found.");
	
	return attrib;
}



GL::GLint Shader::Uniform(const char *name) const
{
	GL::GLint uniform = gl->GetUniformLocation(program, name);
	if(uniform == -1)
		throw runtime_error("Uniform \"" + string(name) + "\" not found.");
	
	return uniform;
}



GL::GLuint Shader::Compile(const char *str, GL::GLenum type)
{
	GL::GLuint object = gl->CreateShader(type);
	if(!object)
		throw runtime_error("Shader creation failed.");
	
	static string prefix =
		"#version 100\n"
		"precision highp float;\n"
		"precision highp int;\n";
	size_t length = strlen(str);
	vector<GL::GLchar> text(prefix.length() + length + 1);
	memcpy(&text.front(), prefix.data(), prefix.length());
	memcpy(&text.front() + prefix.length(), str, length);
	text[prefix.length() + length] = '\0';
	
	const GL::GLchar *cText = &text.front();
	gl->ShaderSource(object, 1, &cText, nullptr);
	gl->CompileShader(object);
	
	GL::GLint status;
	gl->GetShaderiv(object, GL::COMPILE_STATUS, &status);
	if(status == GL::FALSE)
	{
		cerr << prefix;
		error += string(str, length);
		
		static const int SIZE = 4096;
		GL::GLchar message[SIZE];
		GL::GLsizei length;
		
		gl->GetShaderInfoLog(object, SIZE, &length, message);
		error += string(message, length);
		Files::LogError(error);
		throw runtime_error("Shader compilation failed.");
	}
	
	return object;
}
