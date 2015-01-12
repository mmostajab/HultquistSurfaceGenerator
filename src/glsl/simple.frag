#version 430 core

layout(location = 3) uniform vec3 light_dir;
layout(location = 4) uniform vec3 light_pos;

in VS_OUT {
	vec4 pos;
	vec4 color;
	vec3 normal;
} fs_in;

out vec4 out_color;

void main() {
	out_color = fs_in.color * abs(dot(normalize(light_dir), normalize(fs_in.normal)));
}