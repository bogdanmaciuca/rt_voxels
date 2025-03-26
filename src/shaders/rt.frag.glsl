#version 430 core
out vec4 FragColor;

in vec2 FragPos;

#define PALETTE_SIZE 256
#define MAX_STEPS 32

layout (std430, binding = 0) buffer scene {
    ivec3 size;
    vec4 palette[PALETTE_SIZE];
    uint voxel_data[];
};

uniform vec3 uCamPos;
uniform mat4 uInvProj;
uniform mat4 uInvView;

// - divide by 4 to get index into i32's
// - shift left by 8 * (3 - idx % 4)
// - apply mask to only get a single byte (right-most byte)
uint ByteAt(uint idx) {
    return (voxel_data[idx >> 2u] >> ((idx & 3u) << 3u)) & 255u;
}

uint CoordIdx(uint x, uint y, uint z) {
    return z * size.x * size.y + y * size.x + x;
}

uint ByteAt(uint x, uint y, uint z) {
    return ByteAt(CoordIdx(x, y, z));
}

struct Ray {
    vec3 origin;
    vec3 direction;
};

// Deepseek
vec4 MarchRay(Ray ray) {
    ivec3 map = ivec3(ray.origin);
    ivec3 stepAmount;
    vec3 tDelta = abs(1.0 / ray.direction);
    vec3 tMax;
    uint voxel = 0;
    int side;

    if (ray.direction.x < 0) {
        stepAmount.x = -1;
        tMax.x = (ray.origin.x - map.x) * tDelta.x;
    }
    else if (ray.direction.x > 0) {
        stepAmount.x = 1;
        tMax.x = (map.x + 1.0 - ray.origin.x) * tDelta.x;
    }
    else {
        stepAmount.x = 0;
        tMax.x = 0;
    }

    if (ray.direction.y < 0) {
        stepAmount.y = -1;
        tMax.y = (ray.origin.y - map.y) * tDelta.y;
    }
    else if (ray.direction.y > 0) {
        stepAmount.y = 1;
        tMax.y = (map.y + 1.0 - ray.origin.y) * tDelta.y;
    }
    else {
        stepAmount.y = 0;
        tMax.y = 0;
    }

    if (ray.direction.z < 0) {
        stepAmount.z = -1;
        tMax.z = (ray.origin.z - map.z) * tDelta.z;
    }
    else if (ray.direction.z > 0) {
        stepAmount.z = 1;
        tMax.z = (map.z + 1.0 - ray.origin.z) * tDelta.z;
    }
    else {
        stepAmount.z = 0;
        tMax.z = 0;
    }

    do {
        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                map.x += stepAmount.x;
                if (map.x >= size.x || map.x < 0)
                    break;
                tMax.x += tDelta.x;
                side = 0;
            }
            else {
                map.z += stepAmount.z;
                if (map.z >= size.z || map.z < 0)
                    break;
                tMax.z += tDelta.z;
                side = 1;
            }
        }
        else {
            if (tMax.y < tMax.z) {
                map.y += stepAmount.y;
                if (map.y >= size.y || map.y < 0)
                    break;
                tMax.y += tDelta.y;
                side = 2;
            }
            else {
                map.z += stepAmount.z;
                if (map.z >= size.z || map.z < 0)
                    break;
                tMax.z += tDelta.z;
                side = 1;
            }
        }
        voxel = ByteAt(map.x, map.y, map.z);
    } while (voxel == 0);
    return palette[voxel];
}
void main() {
    vec4 target = uInvProj * vec4(FragPos.xy, 1, 1);
    vec3 ray_dir = vec3(uInvView * vec4(normalize(vec3(target) / target.w), 0));
    Ray ray;
    ray.origin = uCamPos;
    ray.direction = ray_dir;

    vec4 result = MarchRay(ray);

    FragColor = result;
}
