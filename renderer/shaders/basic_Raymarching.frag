#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 fragUV;

const int MAX_STEPS = 128;
const float MAX_DIST = 5.0;
const float EPSILON = 0.001;

mat3 rotateX(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
        1.0, 0.0, 0.0,
        0.0, c, -s,
        0.0, s, c
    );
}

float sdTorus(vec3 p, vec2 t, float angle) {
    p = rotateX(angle) * p;
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

float rayMarch(vec3 ro, vec3 rd, float angle) {
    float dist = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + dist * rd;
        float d = sdTorus(p, vec2(1.0, 0.3), angle);
        if (d < EPSILON) return dist;
        if (dist > MAX_DIST) break;
        dist += d;
    }
    return MAX_DIST;
}

vec3 getColor(float d) {
    return mix(vec3(0.1, 0.2, 0.8), vec3(1.0, 0.6, 0.2), smoothstep(0.0, MAX_DIST, d));
}

void main() 
{
    vec2 uv = fragUV * 2.0 - 1.0;
    vec3 ro = vec3(0.0, 0.0, -3.0);
    vec3 rd = normalize(vec3(uv, 1.0));

    float time = 2.0 * gl_FragCoord.x * 0.001 + gl_FragCoord.y * 0.001;
    float d = rayMarch(ro, rd, time);
    
    vec3 color = getColor(d);
    outColor = vec4(color, 1.0);
}
