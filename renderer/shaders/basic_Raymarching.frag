#version 450

const int MAX_SPHERE_NUMBER = 4;

layout(set = 0, binding = 0) uniform UniformBufferObject 
{
    float time;
    vec3 cameraPos;
    vec3 cameraFront;
    vec4 spheresArray[MAX_SPHERE_NUMBER];
    int sphereNumber;

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

Ray generateRay(vec2 uv) 
{
    vec3 right = normalize(cross(ubo.cameraFront, vec3(0.0, 1.0, 0.0)));
    vec3 up = normalize(cross(right, ubo.cameraFront));
    
    float fov = radians(90.0);
    float aspectRatio = 16.0 / 9.0;

    vec3 rayDir = normalize(ubo.cameraFront + uv.x * right * aspectRatio + uv.y * up);
    
    return Ray(ubo.cameraPos, rayDir);
}

float smoothMin(float torus, float b, float k) 
{
    float h = max(k - abs(torus - b), 0.0) / k;
    return min(torus, b) - h * h * h * k * (1.0 / 6.0);
}

float sphereSDF(vec3 p, vec3 center, float radius)
{
    return length(p - center) - radius;
}

float torusSDF(vec3 p, vec2 t)
{
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}


float sceneSDF(vec3 p, out Material material) 
{
    float spheresDistArray[MAX_SPHERE_NUMBER];
    for(int i = 0; i < ubo.sphereNumber; i++)
    {
        spheresDistArray[i] = sphereSDF(p, vec3(ubo.spheresArray[i].xyz), 0.5);
    }
    
    float k = 0.5; // Constante de lissage
    
    float dist[MAX_SPHERE_NUMBER];
    
    for (int i = 0; i < ubo.sphereNumber; i++)
    {
        for (int j = i + 1; j < ubo.sphereNumber; j++)
        {
            dist[i] = smoothMin(spheresDistArray[i], spheresDistArray[j], k);
        }
    }

    material.color = vec3(1.0, 1.0, 1.0);
    material.reflectivity = 0;

    float minValue = dist[0];
    for (int i = 1; i < ubo.sphereNumber; i++)
    {
        minValue = min(minValue, dist[i]);
    }
    return minValue;
}

float rayMarch(Ray ray, out Material material) 
{
    float distance = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) 
    {
        vec3 p = ray.origin + ray.direction * distance;
        float d = sceneSDF(p, material);
        if (d < EPSILON) 
            return distance;
        distance += d;
        if (distance > MAX_DIST) 
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

    for (int depth = 0; depth < MAX_RECURSION_DEPTH; depth++)
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
