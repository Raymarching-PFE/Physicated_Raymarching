#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject 
{
    vec3 cameraPos;
    vec3 cameraFront;
    vec3 cameraUp;
} ubo;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 fragUV;

const int MAX_STEPS = 128;
const float MAX_DIST = 5.0;
const float EPSILON = 0.001;

mat3 rotateX(float angle) 
{
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
        1.0, 0.0, 0.0,
        0.0, c, -s,
        0.0, s, c
    );
}

struct Ray 
{
    vec3 origin;
    vec3 direction;
};

Ray generateRay(vec2 uv) 
{
    vec3 right = normalize(cross(ubo.cameraFront, vec3(0.0, 1.0, 0.0)));
    vec3 up = normalize(cross(right, ubo.cameraFront));
    
    float fov = radians(90.0);
    float aspectRatio = 16.0 / 9.0;
    vec3 rayDir = normalize(ubo.cameraFront + uv.x * right * aspectRatio + uv.y * up);
    
    return Ray(ubo.cameraPos, rayDir);
}

//float sceneSDF(vec3 p) 
//{
//    vec3 torusPos = vec3(0.0, 0.0, 3.0);
//    vec2 t = vec2(1.0, 0.3);
//    vec2 q = vec2(length(p.xz - torusPos.xz) - t.x, p.y - torusPos.y);
//    return length(q) - t.y;
//}

float FirstsdTorus(vec3 p, vec2 t, float angle) 
{
    //p = rotateX(angle) * p;
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

float FirstRayMarch(vec3 ro, vec3 rd, float angle) 
{
    float dist = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) 
    {
        vec3 p = ro + dist * rd;
        float d = FirstsdTorus(p, vec2(1.0, 0.3), angle);
        if (d < EPSILON) 
            return dist;
        if (dist > MAX_DIST) 
            break;
        dist += d;
    }
    return MAX_DIST;
}

float rayMarch(Ray ray) 
{
    float distance = 0.0;
    for (int i = 0; i < 100; i++) 
    {
        vec3 p = ray.origin + ray.direction * distance;
        float d = FirstsdTorus(p, vec2(1.0, 0.3), 1.0);
        if (d < 0.001) 
            return distance;
        distance += d;
        if (distance > 50.0) 
            break;
    }
    return -1.0;
}

vec3 getColor(float d) 
{
    return mix(vec3(0.1, 0.2, 0.8), vec3(1.0, 0.6, 0.2), smoothstep(0.0, MAX_DIST, d));
}

void main() 
{
    vec2 uv = fragUV * 2.0 - 1.0;
    Ray ray = generateRay(uv);
    float dist = rayMarch(ray);
    
    if (dist > 0.0)
        outColor = vec4(1.0 - dist * 0.02, 0.3, 0.5, 1.0);
    else
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    //outColor = vec4(ubo.cameraPos.x, 0.0, 0.0, 1.0);
}
