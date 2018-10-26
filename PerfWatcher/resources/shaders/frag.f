#version 400

in vec4 ex_Color;

uniform vec4 ColourMultiplier = vec4(1.0,1.0,1.0,1.0);

out vec4 fragmentColor;

void main()
{
	fragmentColor = ex_Color * ColourMultiplier;
}
