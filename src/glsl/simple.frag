#version 430 core

layout(location = 3) uniform vec3 light_dir;
layout(location = 4) uniform vec3 light_pos;

out vec4 out_color;

void main() {
	// out_color = fs_in.color * abs(dot(normalize(light_dir), normalize(fs_in.normal)));
	out_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}