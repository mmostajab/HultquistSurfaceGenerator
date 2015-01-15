#version 430 core

layout( location = 0 ) uniform mat4 proj_mat;
layout( location = 1 ) uniform mat4 view_mat;
layout( location = 2 ) uniform mat4 world_mat;
layout( location = 3 ) uniform vec3 light_dir;
layout( location = 4 ) uniform vec3 light_pos;

layout( location = 0 ) in vec3 position;

void main() {
	gl_Position = proj_mat * view_mat * vec4(position, 1.0f);
}