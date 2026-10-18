#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <cstdlib>
#include <algorithm>

// Simple 3D Vector utility
struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*(const Vector3& v) const { return Vector3(x * v.x, y * v.y, z * v.z); }
    Vector3 operator*(float f) const { return Vector3(x * f, y * f, z * f); }

    Vector3 normalize() const {
        float len = std::sqrt(x*x + y*y + z*z);
        return len > 0 ? Vector3(x/len, y/len, z/len) : Vector3(0,0,0);
    }
    float dot(const Vector3& v) const { return x*v.x + y*v.y + z*v.z; }
};

struct Ray {
    Vector3 origin, direction;
    Ray() {}
    Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d.normalize()) {}
};

//Setting up the materials
struct Material {
    Vector3 albedo;    //Base color
    Vector3 emission;  //Light output (RGB)
    bool is_emissive;
};

struct HitRecord {
    float t;
    Vector3 point;
    Vector3 normal;
    Material material;
};

//Defining the Torch geometry
struct AABB{
    Vector3 min;
    Vector3 max;
    Material material;

    bool hit(const Ray& ray, float t_min, float t_max, HitRecord& rec) const{
        //Standard AABB ray intersection algorithm (e.g., Smit's method)
        float t0x = (min.x - ray.origin.x) / ray.direction.x;
        float t1x = (max.x - ray.origin.x) / ray.direction.x;
        if (t0x > t1x) std::swap(t0x, t1x);

        float t0y = (min.y - ray.origin.y) / ray.direction.y;
        float t1y = (max.y - ray.origin.y) / ray.direction.y;
        if (t0y > t1y) std::swap(t0y, t1y);

        float t0z = (min.z - ray.origin.z) / ray.direction.z;
        float t1z = (max.z - ray.origin.z) / ray.direction.z;
        if (t0z > t1z) std::swap(t0z, t1z);

        float start = std::max({t0x, t0y, t0z});
        float end = std::min({t1x, t1y, t1z});

        if (start <= end && start < t_max && start > t_min) {
            rec.t = start;
            rec.point = ray.origin + ray.direction * start;

            // Simple normal calculation based on which face was hit
            Vector3 c = rec.point - (min + max) * 0.5f;
            Vector3 d = (min - max) * 0.5f;
            float bias = 1.001f;
            if (std::abs(c.x) >= std::abs(d.x) * bias) rec.normal = Vector3(c.x > 0 ? 1.f : -1.f, 0, 0);
            else if (std::abs(c.y) >= std::abs(d.y) * bias) 
                rec.normal = Vector3(0, c.y > 0 ? 1.f : -1.f, 0);
            else rec.normal = Vector3(0, 0, c.z > 0 ? 1.f : -1.f);

            rec.material = material;
            return true;
        }
        return false;
    }
};

// Generates a rough random vector for global illumination bounces
Vector3 random_on_hemisphere(const Vector3& normal) {
    float u = (float)rand() / RAND_MAX;
    float v = (float)rand() / RAND_MAX;
    float theta = u * 2.0f * 3.14159f;
    float phi = std::acos(2.0f * v - 1.0f);
    Vector3 rand_dir(std::sin(phi)*std::cos(theta), std::sin(phi)*std::sin(theta), std::cos(phi));
    if (rand_dir.dot(normal) < 0) return rand_dir * -1.0f;
    return rand_dir;
}

Vector3 trace_ray(const Ray& ray, const AABB* scene, int count, int depth) {
    if (depth <= 0) 
        return Vector3(0,0,0);

    HitRecord closest_rec;
    bool hit_anything = false;
    float closest_so_far = std::numeric_limits<float>::max();

    for (int i = 0; i < count; i++) {
        HitRecord rec;

        if (scene[i].hit(ray, 0.001f, closest_so_far, rec)) {
            hit_anything = true;
            closest_so_far = rec.t;
            closest_rec = rec;
        }
    }

    if (!hit_anything) {
        //Dark cave background color
        return Vector3(0.02f, 0.02f, 0.04f);
    }

    if (closest_rec.material.is_emissive) {
        return closest_rec.material.emission;
    }

    // Bounce ray
    Vector3 light_postion(0.0f, 0.30f, -0.325f);
    Vector3 to_light = light_postion - closest_rec.point;

    float distance_squared = to_light.dot(to_light);
    Vector3 light_dir = to_light.normalize();

    // Diffuse lighting : surfaces facing flame get bright
    float diffuse = std::max(
        0.0f, closest_rec.normal.dot(light_dir)
    );

    // Reduce light with distance
    float attenuation = 2.0f / (1.0f + distance_squared * 4.0f);

    // keep a little ambient light so wood remains visible
    float lighting = 0.12f + diffuse * attenuation;

    return closest_rec.material.albedo * lighting;
}

int main() {
    const int width = 400;
    const int height = 400;
    const int samples = 32;

    std::ofstream image("render.ppm");
    image << "P3\n" << width << " " << height << "\n255\n";

    // Set-up Materials
    // Creating the Torch components
    Material wood = { Vector3(0.4f, 0.25f, 0.15f), Vector3(0,0,0), false }; //Brown
    Material coal = { Vector3(0.15f, 0.15f, 0.15f), Vector3(0,0,0), false }; //Dark Grey
    Material flame = { Vector3(1.0f, 0.6f, 0.1f), Vector3(3.0f, 0.7f, 0.08f), true }; //Bright Orange Light
    Material stone = { Vector3(0.5f, 0.5f, 0.5f), Vector3(0,0,0), false }; //Background wall

    // Create scene array ( 1 stone wall, 1 torch handle, 1 torch tip)
    AABB scene[3];
    scene[0] = { Vector3(-5.0f, -5.0f, -2.0f), Vector3(5.0f, 5.0f, -1.5f), stone }; //Wall
    scene[1] = { Vector3(-0.15f, -0.6f, -0.5f), Vector3(0.15f, 0.2f, -0.2f), wood }; //Torch handle
    scene[2] = { Vector3(-0.15f, 0.2f, -0.5f), Vector3(0.15f, 0.5f, -0.2f), flame }; //Torch flame

    Vector3 camera_origin(0, 0, 1.5f);

    std::cout << "Rendering... Please wait.\n";

    for (int j = height - 1; j >= 0; --j) {
        for (int i = 0; i < width; ++i) {
            Vector3 pixel_color(0, 0, 0);
            for (int s = 0; s < samples; ++s) {
                float u = (float(i) + ((float) rand() / RAND_MAX)) / float(width);
                float v = (float(j) + ((float) rand() / RAND_MAX)) / float(height);

                // Screen coordinates mapping
                Vector3 screen_point(
                    (u - 0.5f) * 2.0f, 
                    (v - 0.5f) * 2.0f, 
                    0.5f
                );

                // Point the ray from the camera toward the pixel
                Ray ray(camera_origin, screen_point - camera_origin);

                pixel_color = pixel_color + trace_ray(ray, scene, 3, 3);
            }

            //Average samples and apply a rough gamma correction
            float r = std::sqrt(pixel_color.x / samples);
            float g = std::sqrt(pixel_color.y / samples);
            float b = std::sqrt(pixel_color.z / samples);

            int ir = static_cast<int>(255.99 * std::clamp(r, 0.0f, 1.0f));
            int ig = static_cast<int>(255.99 * std::clamp(g, 0.0f, 1.0f));
            int ib = static_cast<int>(255.99 * std::clamp(b, 0.0f, 1.0f));

            image << ir << " " << ig << " " << ib << "\n";
        }
    }
    std::cout << "Done! Saved to render.ppm\n";
    return 0;
}



