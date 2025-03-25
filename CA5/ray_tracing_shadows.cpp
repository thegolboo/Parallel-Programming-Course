%%writefile ray_tracing_shadows.cu
#include <iostream>
#include <fstream>
#include <cmath>
#include <curand_kernel.h>

#define M_PI 3.14159265358979323846f

struct Vec3 {
    float x, y, z;

    __host__ __device__ Vec3() : x(0), y(0), z(0) {}
    __host__ __device__ Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    __host__ __device__ Vec3 operator+(const Vec3& v) const {
        return Vec3(x + v.x, y + v.y, z + v.z);
    }
    __host__ __device__ Vec3 operator-(const Vec3& v) const {
        return Vec3(x - v.x, y - v.y, z - v.z);
    }
    __host__ __device__ Vec3 operator*(float t) const {
        return Vec3(x * t, y * t, z * t);
    }
    __host__ __device__ Vec3 operator/(float t) const {
        return Vec3(x / t, y / t, z / t);
    }
    __host__ __device__ float dot(const Vec3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }
    __host__ __device__ Vec3 cross(const Vec3& v) const {
        return Vec3(
                y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x
        );
    }
    __host__ __device__ float length() const {
        return sqrtf(x * x + y * y + z * z);
    }
    __host__ __device__ Vec3 normalize() const {
        float len = length();
        return *this / len;
    }
    __host__ __device__ Vec3 operator*(const Vec3& v) const {
        return Vec3(x * v.x, y * v.y, z * v.z);
    }

    __host__ __device__ friend Vec3 operator*(float t, const Vec3& v) {
        return Vec3(t * v.x, t * v.y, t * v.z);
    }
};

struct Ray {
    Vec3 origin;
    Vec3 direction;

    __host__ __device__ Ray() {}
    __host__ __device__ Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}

    __host__ __device__ Vec3 at(float t) const {
        return origin + direction * t;
    }
};

#define SPHERE 0
#define PLANE 1

struct Hittable {
    int type;        // Object type: SPHERE or PLANE
    Vec3 center;     // For sphere and plane (point on the plane)
    Vec3 normal;     // For plane normal (for PLANE type)
    float radius;    // For sphere
    Vec3 color;      // Material color
};

__device__ bool hitSphere(const Hittable& sphere, const Ray& r, float t_min, float t_max, float& t, Vec3& normal) {
    Vec3 oc = r.origin - sphere.center;
    float a = r.direction.dot(r.direction);
    float half_b = oc.dot(r.direction);
    float c = oc.dot(oc) - sphere.radius * sphere.radius;
    float discriminant = half_b * half_b - a * c;
    if (discriminant > 0) {
        float sqrt_d = sqrtf(discriminant);
        float root = (-half_b - sqrt_d) / a;
        if (root < t_max && root > t_min) {
            t = root;
            Vec3 hitPoint = r.at(t);
            normal = (hitPoint - sphere.center).normalize();
            return true;
        }
        root = (-half_b + sqrt_d) / a;
        if (root < t_max && root > t_min) {
            t = root;
            Vec3 hitPoint = r.at(t);
            normal = (hitPoint - sphere.center).normalize();
            return true;
        }
    }
    return false;
}

__device__ bool hitPlane(const Hittable& plane, const Ray& r, float t_min, float t_max, float& t, Vec3& normal) {
    float denom = plane.normal.dot(r.direction);
    if (fabsf(denom) > 1e-6f) { // Not parallel
        t = (plane.center - r.origin).dot(plane.normal) / denom;
        if (t < t_max && t > t_min) {
            normal = plane.normal;
            return true;
        }
    }
    return false;
}

__device__ bool isShadowed(const Vec3& point, const Vec3& light_pos, Hittable* objects, int num_objects) {
    Vec3 light_dir = (light_pos - point).normalize();
    Ray shadow_ray(point + light_dir * 0.001f, light_dir); // Offset to avoid self-intersection

    float t_min = 0.001f;
    float t_max = (light_pos - point).length();

    for (int i = 0; i < num_objects; ++i) {
        float t;
        Vec3 temp_normal;
        bool hit = false;

        if (objects[i].type == SPHERE) {
            hit = hitSphere(objects[i], shadow_ray, t_min, t_max, t, temp_normal);
        } else if (objects[i].type == PLANE) {
            hit = hitPlane(objects[i], shadow_ray, t_min, t_max, t, temp_normal);
        }

        if (hit) {
            return true; // Shadowed
        }
    }
    return false; // Not shadowed
}

__device__ Vec3 rayColor(const Ray& r, Hittable* objects, int num_objects, Vec3 light_pos) {
    float t_min = 0.001f;
    float t_max = 1e20f;
    float closest_t = t_max;
    Vec3 color(0, 0, 0);
    Vec3 normal;
    int hit_index = -1;

    // Find closest hit
    for (int i = 0; i < num_objects; ++i) {
        float t;
        Vec3 temp_normal;
        bool hit = false;

        if (objects[i].type == SPHERE) {
            hit = hitSphere(objects[i], r, t_min, closest_t, t, temp_normal);
        } else if (objects[i].type == PLANE) {
            hit = hitPlane(objects[i], r, t_min, closest_t, t, temp_normal);
        }

        if (hit) {
            closest_t = t;
            normal = temp_normal;
            color = objects[i].color;
            hit_index = i;
        }
    }

    if (hit_index >= 0) {
        Vec3 hit_point = r.at(closest_t);

        // Check if the point is in shadow
        bool shadowed = isShadowed(hit_point, light_pos, objects, num_objects);

        Vec3 light_dir = (light_pos - hit_point).normalize();
        float intensity = fmaxf(0.0f, normal.dot(light_dir));

        Vec3 ambient = 0.1f * color;
        Vec3 diffuse = shadowed ? Vec3(0, 0, 0) : intensity * color; // No diffuse if shadowed

        Vec3 result_color = ambient + diffuse;

        return result_color;
    }

    Vec3 unit_direction = r.direction.normalize();
    float t = 0.5f * (unit_direction.y + 1.0f);
    return (1.0f - t) * Vec3(1.0f, 1.0f, 1.0f) + t * Vec3(0.5f, 0.7f, 1.0f); // Sky gradient
}

__global__ void renderKernel(Vec3* pixels, int width, int height, Hittable* objects, int num_objects, Vec3 light_pos) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    int index = y * width + x;

    float aspect_ratio = float(width) / float(height);
    float viewport_height = 2.0f;       // Fixed viewport height
    float viewport_width = aspect_ratio * viewport_height;

    float focal_length = 1.0f;

    float u = (float(x) + 0.5f) / float(width);  // Add 0.5 for center of pixel
    float v = (float(y) + 0.5f) / float(height);

    float px = (u - 0.5f) * viewport_width;
    float py = (v - 0.5f) * viewport_height;

    Vec3 origin(0.0f, 0.0f, 0.0f);                 // Ray origin at the origin
    Vec3 direction(px, py, -focal_length);         // Ray direction towards the image plane
    direction = direction.normalize();             // Normalize the direction

    Ray r(origin, direction);

    Vec3 color = rayColor(r, objects, num_objects, light_pos);

    color = Vec3(sqrtf(color.x), sqrtf(color.y), sqrtf(color.z));
    pixels[index] = color;
}

void saveToPPM(const Vec3* pixels, int width, int height, const std::string& filename) {
    std::ofstream outFile(filename, std::ios::out | std::ios::binary);
    outFile << "P6\n" << width << " " << height << "\n255\n";

    for (int j = height - 1; j >= 0; --j) { // Flip the image vertically
        for (int i = 0; i < width; ++i) {
            int index = j * width + i;
            unsigned char r = static_cast<unsigned char>(255.99f * fminf(fmaxf(pixels[index].x, 0.0f), 1.0f));
            unsigned char g = static_cast<unsigned char>(255.99f * fminf(fmaxf(pixels[index].y, 0.0f), 1.0f));
            unsigned char b = static_cast<unsigned char>(255.99f * fminf(fmaxf(pixels[index].z, 0.0f), 1.0f));
            outFile << r << g << b;
        }
    }
    outFile.close();
}

int main() {
    const int width = 800;
    const int height = 600;
    size_t numPixels = width * height;
    Vec3* pixels;

    cudaMallocManaged(&pixels, numPixels * sizeof(Vec3));

    const int num_objects = 4;
    Hittable* objects;
    cudaMallocManaged(&objects, num_objects * sizeof(Hittable));

    objects[0].type = SPHERE;
    objects[0].center = Vec3(0.0f, 0.0f, -1.5f);
    objects[0].radius = 0.5f;
    objects[0].color = Vec3(0.8f, 0.1f, 0.1f); // Red

    objects[1].type = SPHERE;
    objects[1].center = Vec3(-1.0f, 0.0f, -2.0f);
    objects[1].radius = 0.5f;
    objects[1].color = Vec3(0.1f, 0.1f, 0.8f); // Blue

    objects[2].type = SPHERE;
    objects[2].center = Vec3(1.0f, 0.0f, -2.0f);
    objects[2].radius = 0.5f;
    objects[2].color = Vec3(0.1f, 0.8f, 0.1f); // Green

    objects[3].type = PLANE;
    objects[3].center = Vec3(0.0f, -0.5f, 0.0f); // Point on the plane
    objects[3].normal = Vec3(0.0f, 1.0f, 0.0f);  // Upward normal
    objects[3].color = Vec3(0.8f, 0.8f, 0.8f);   // Gray

    Vec3 light_pos = Vec3(5.0f, 5.0f, -5.0f);

    dim3 blockSize(16, 16);
    dim3 numBlocks((width + blockSize.x - 1) / blockSize.x,
                   (height + blockSize.y - 1) / blockSize.y);
    renderKernel<<<numBlocks, blockSize>>>(pixels, width, height, objects, num_objects, light_pos);
    cudaDeviceSynchronize();

    saveToPPM(pixels, width, height, "output.ppm");

    cudaFree(pixels);
    cudaFree(objects);

    return 0;
}