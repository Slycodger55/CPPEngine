#version 450 core
layout(location = 0) in vec3 Pos;

void main()
{
	gl_Position = vec4(Pos, 1);
}