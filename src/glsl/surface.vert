#version 430 core

layout( location = 0 ) uniform mat4 proj_mat;
layout( location = 1 ) uniform mat4 view_mat;
layout( location = 2 ) uniform mat4 world_mat;
layout( location = 3 ) uniform vec3 light_dir;
layout( location = 4 ) uniform vec3 light_pos;

layout( location = 0 ) in vec3 position;
layout( location = 1 ) in vec3 color;
layout( location = 2 ) in vec3 normal;

out VS_OUT {
	vec4 pos;
	vec4 color;
	vec3 normal;
} vs_out;

void main() {
	vs_out.pos    = proj_mat * view_mat * world_mat * vec4(position, 1.0f);
	vs_out.color  = vec4(color, 1.0f);
	vs_out.normal = normalize(normal);
	gl_Position   = vs_out.pos;
}