#version 450

const int MAX_SPHERE_NUMBER = 8;

layout(set = 0, binding = 0, std140) uniform UniformBufferObject
{
    float time;
    vec4 cameraPos;
    vec4 cameraFront;
    vec4 spheresArray[MAX_SPHERE_NUMBER]; // .xyz = center, .w = radius
    ivec4 sphereInfo; // x = number of spheres
} ubo;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 fragUV;

const int MAX_STEPS = 128;
const float MAX_DIST = 5.0;
const float EPSILON = 0.001;
const int MAX_RECURSION_DEPTH = 3;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

struct Material
{
    vec3 color;
    float reflectivity;
};

float smoothMin(float d1, float d2, float k)
{
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h);
}

Ray generateRay(vec2 uv)
{
    vec3 forward = normalize(ubo.cameraFront.xyz);
    if (length(forward) < 0.001 || isnan(forward.x))
        forward = vec3(0.0, 0.0, -1.0);

    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    vec3 up = normalize(cross(right, forward));
    float aspectRatio = 16.0 / 9.0;
    vec3 rayDir = normalize(forward + uv.x * right * aspectRatio + uv.y * up);

    return Ray(ubo.cameraPos.xyz, rayDir);
}

float sphereSDF(vec3 p, vec3 center, float radius)
{
    return length(p - center) - radius;
}

float sceneSDF(vec3 p, out Material material)
{
    float minDist = MAX_DIST;
    float k = 0.5;

    for (int i = 0; i < ubo.sphereInfo.x; ++i)
    {
        vec3 center = ubo.spheresArray[i].xyz;
        float radius = ubo.spheresArray[i].w;
        float d = sphereSDF(p, center, radius);
        minDist = smoothMin(minDist, d, k);
    }

    material.color = vec3(1.0);
    material.reflectivity = 0.0;
    return minDist;
}

float rayMarch(Ray ray, out Material material)
{
    float dist = 0.0;
    for (int i = 0; i < MAX_STEPS; ++i)
    {
        vec3 p = ray.origin + ray.direction * dist;
        float d = sceneSDF(p, material);
        if (d < EPSILON)
            return dist;
        if (isnan(d) || isinf(d))
            break;
        dist += d;
        if (dist > MAX_DIST)
            break;
    }
    return -1.0;
}

vec3 getNormal(vec3 p)
{
    vec2 e = vec2(EPSILON, 0.0);
    Material dummyMaterial;
    return normalize(vec3(
        sceneSDF(p + e.xyy, dummyMaterial) - sceneSDF(p - e.xyy, dummyMaterial),
        sceneSDF(p + e.yxy, dummyMaterial) - sceneSDF(p - e.yxy, dummyMaterial),
        sceneSDF(p + e.yyx, dummyMaterial) - sceneSDF(p - e.yyx, dummyMaterial)
    ));
}

vec3 getColor(Ray ray, vec3 p, Material material)
{
    vec3 color = vec3(0.0);
    vec3 attenuation = vec3(1.0);

    for (int depth = 0; depth < MAX_RECURSION_DEPTH; ++depth)
    {
        vec3 normal = getNormal(p);
        vec3 lightDir = normalize(vec3(1.0, -1.0, -1.0));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * material.color;

        color += attenuation * diffuse;

        vec3 reflectDir = reflect(ray.direction, normal);
        ray = Ray(p + reflectDir * EPSILON, reflectDir);
        float reflectDist = rayMarch(ray, material);
        if (reflectDist < 0.0)
            break;

        p = ray.origin + ray.direction * reflectDist;
        attenuation *= material.reflectivity;
    }

    return color;
}

void main()
{
    vec2 uv = fragUV * 2.0 - 1.0;
    Ray ray = generateRay(uv);
    Material material;
    float dist = rayMarch(ray, material);

    if (dist > 0.0)
    {
        vec3 p = ray.origin + ray.direction * dist;
        vec3 color = getColor(ray, p, material);
        outColor = vec4(color, 1.0);
    }
    else
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
