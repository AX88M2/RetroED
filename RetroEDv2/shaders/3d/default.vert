#version 330 core
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_UV;

out vec4 ex_color;
out vec2 ex_UV;

uniform mat4 model;
uniform mat4 view;       
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
    ex_color    = in_color;
    ex_UV       = in_UV;
}