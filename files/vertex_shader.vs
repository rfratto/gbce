#version 330 core 

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 uv; 

out vec2 UV; 

void main() {
	gl_Position.xyz = vertex;
	gl_Position.w = 1.0;

	UV = uv; 
}
