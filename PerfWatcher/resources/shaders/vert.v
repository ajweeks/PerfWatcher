#version 400

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec4 in_Colour;

out vec4 ex_Color;

uniform mat4 viewProj;
uniform mat4 model;

void main()
{
    ex_Color = in_Colour;
    gl_Position = viewProj * model * vec4(in_Position, 1.0);
}
