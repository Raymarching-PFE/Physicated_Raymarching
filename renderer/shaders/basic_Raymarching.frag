#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 fragUV;

const int MAX_STEPS = 128;
const float MAX_DIST = 100.0;
const float EPSILON = 0.001;

// Signed Distance torus
float sdTorus(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

// Fonction de raymarching
float rayMarch(vec3 ro, vec3 rd) {
    float dist = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + dist * rd;
        float d = sdTorus(p, vec2(1.0, 0.3));  // Rayon externe 1.0, rayon interne 0.3
        if (d < EPSILON) return dist;
        if (dist > MAX_DIST) break;
        dist += d;
    }
    return MAX_DIST;
}

// distance color
vec3 getColor(float d) {
    return mix(vec3(0.1, 0.2, 0.8), vec3(1.0, 0.6, 0.2), smoothstep(0.0, MAX_DIST, d));
}

void main() 
{
    vec2 uv = fragUV * 2.0 - 1.0;
    vec3 ro = vec3(0.0, 0.0, -3.0);
    vec3 rd = normalize(vec3(uv, 1.0));

    float d = rayMarch(ro, rd);
    vec3 color = getColor(d);
    
    outColor = vec4(color, 1.0);
    //outColor = vec4(1.0, 0.0, 0.0, 1.0);
}