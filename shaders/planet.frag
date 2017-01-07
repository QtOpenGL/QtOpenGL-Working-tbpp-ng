R"(
#version 330 core

in vec3 nowColor;
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

void main()
{
    fragColor = brightColor = vec4(nowColor, 1.0);
}
)"
