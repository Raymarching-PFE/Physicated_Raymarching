#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D img_output;

void main() {
    outColor = texture(img_output, uv);
}
