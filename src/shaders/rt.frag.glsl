#version 330 core
out vec4 FragColor;

in vec2 FragPos;

uniform float uRatio;
uniform vec3 uCamPos;
uniform mat4 uInvProj;
uniform mat4 uInvView;
uniform vec3 uSpheres[3];

#define SPHERE_RAD 0.75

struct Ray {
    vec3 origin;
    vec3 dir;
};

bool SphereIntersection(vec3 position, Ray ray) {
    float t = dot(position - ray.origin, ray.dir);
    vec3 p = ray.origin + ray.dir * t;

    float y = length(position - p);
    if (y < SPHERE_RAD) { 
        float x = sqrt(SPHERE_RAD * SPHERE_RAD - y * y);
        float t1 = t - x;
        if (t1 > 0) {
            return true;
        }
    }

    return false;
}

void main() {
    vec4 target = uInvProj * vec4(FragPos.xy, 1, 1);
    vec3 ray_dir = vec3(uInvView * vec4(normalize(vec3(target) / target.w), 0));
    Ray ray;
    ray.origin = uCamPos;
    ray.dir = ray_dir;

    vec3 result = vec3(0);
    for (int i = 0; i < 3; i++) {
        if (SphereIntersection(uSpheres[i], ray))
            result = vec3(1, 0, 0);
    }
    FragColor = vec4(result, 1.0);
}
