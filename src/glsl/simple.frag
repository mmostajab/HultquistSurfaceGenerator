#version 430 core

in VS_OUT {
	vec4 color;
} fs_in;

out vec4 out_color;

void main() {
	out_color = fs_in.color;
}