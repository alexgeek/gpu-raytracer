typedef struct {
    float4 origin;
    float4 dir;
    float4 col;
} Ray;

typedef struct {
    float4 diffuse_col;
    float diffuse;
    float4 specular_col;
    float specular;
    float4 pos;
    float4 normal;
    float4 scale;
} Primitive;

#define PRIM_TYPE(P) (int)((P).scale.w)
#define RADIUS(P) P.scale.x
#define SCALE(P) (float3)(P.scale.x, P.scale.y, P.scale.z)
#define PRIM_PLANE 1
#define PRIM_SPHERE 2
#define HIT 1
#define MISS 0
#define NONE -1

int ray_plane(Ray* ray, Primitive* prim, float* t) {
    // calculate dotproduct of ray and plane normal
    const float dp = dot(ray->dir, prim->normal);
    // ray orthogonal to plane
    if(dp == 0) return MISS;
    // calculate distance from camera
    const float d = dot(prim->normal, prim->pos - ray->origin) / dp;
    // if closer than previous
    if(d > 0 && d < *t) {
        *t = d;
        return HIT;
    }
    return MISS;
}

/**
 * http://www.vis.uky.edu/~ryang/teaching/cs535-2012spr/Lectures/13-RayTracing-II.pdf
 */
int ray_sphere(Ray* ray, Primitive* prim, float* t) {
    const float radius = prim->scale.x;
    // vector from origin to primitive
    const float4 v = prim->pos - ray->origin;
    // compute dotproduct of ray and v
    const float dp = dot(ray->dir, v);
    // b^2 -4ac
    const float det = dp*dp - dot(v, v) + radius*radius;
    // no solutions to quadratic formula
    if(det <= 0) return MISS;

    // solve for smaller t
    float d = (dp - sqrt(det));

    // if is t is less 0 then we have the wrong root
    if (d < 0 ) {
        // solve for the larger t
        d = (dp + sqrt(det));

        // intersection is in opposite direction to ray if t < 0
        if(d < 0) return MISS;
    }

    *t = d;

    return HIT;
}

int shade(Ray* ray, Primitive* prim, float4 intersection) {
        // add constant amount of ambient light
        ray->col += (float4)(0.1f, 0.1f, 0.1f, 1.0f);

        const float4 light_pos = (float4)(-3.0f, 4.0f, -1.0f, 0);
        const float4 light_col = (float4)(0, 0, 0.8f, 1.0f);

        // calculate direction of light
        const float4 light_dir = light_pos - intersection;

        // hack to get to primtive type from scale component
        const int prim_type = (int)prim->scale.w;
        if(prim_type == PRIM_SPHERE)
        {
            float radius = prim->scale.x;
            prim->normal = (intersection - prim->pos) * radius;
        }

        // normally normalised
        prim->normal = normalize(prim->normal);

        // calculate a multiplier to the surface colour dependent on viewing angle
        const float lambertian = max(dot(prim->normal, fast_normalize(light_dir)), 0.0f);

        // add diffuse shading
        ray->col += prim->diffuse * lambertian * prim->diffuse_col;

        // add specular highlights

        const float4 bisec = fast_normalize(light_dir + (prim->pos - intersection));

        // specular exponent
        const float alpha = 16.0f;

        const float dp2 = pow( max(dot(bisec, prim->normal), 0.0f), alpha);

        // temp hack to brighten up specular.
        ray->col += prim->diffuse * dp2 * prim->specular_col;

        // ray->col /= 2.0f;
}

int ray_trace(Ray* ray, Primitive* prims) {
    const int num_prims = 10;
    int hit = NONE;
    float t = MAXFLOAT; // far away

    // find ray primitive intersections
    for(int p = 0; p < num_prims; p++)
    {
        int prim_type = (int)(prims[p].scale.w);
        switch(prim_type)
        {
            case PRIM_PLANE:
                if(ray_plane(ray, &prims[p], &t)) {
                    hit = p;
                }

                break;
            case PRIM_SPHERE:
                if(ray_sphere(ray, &prims[p], &t)) hit = p;
                break;
        }
    }

    // no intersections
    if (hit == NONE) return 0;

    // calculate point of intersection
    const float4 intersection = ray->origin + t * ray->dir;

    // shade with prim at intersection point
    shade(ray, &prims[hit], intersection);

    return 0;
}

/**
 * Calculates position of a pixel in the scene.
 * Returns the aspect ratio.
 */
inline float calc_uv(float* u, float *v, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    const float ratio = (float)width / height;
    // calculate uv coordinates
    *u = ((x+0.5) - (width/2)) / (2 * width) * ratio;
    *v = ((y+0.5) - (height/2)) / (2 * height);
    return ratio;
}

/**
 * Calculate ray from uv coordinates and focal length.
 */
inline Ray calc_ray(float focal, float4 uv, float4 col) {
    Ray ray;
    ray.origin = (float4)(0, 0, -focal , 0);    // distance of camera from image plane
    ray.dir = fast_normalize(uv - ray.origin);  // from camera to image plane
    ray.col = col;
    return ray;
}

/**
 * Entry point.
 * Receives parameters and grants write only access to the OpenGL texture.
 */
__kernel void pixel_kernel(__write_only image2d_t img, unsigned int width, unsigned int height, float time)
{
    const unsigned int x = get_global_id(0);
    const unsigned int y = get_global_id(1);

    float u, v;
    calc_uv(&u, &v, x, y, width, height);

    // generate ray from camera position amd colour
    const Ray ray = calc_ray(0.95f, (float4)(u,v,0,0), (float4)(0, 0, 0, 1.0f));

    // setup planes
    Primitive prims[10];

    // CECECD (nice grey) floor
    prims[0].pos = (float4)(0,-.1f,0,0);
    prims[0].diffuse_col =  (float4)(206.0f, 206.0f, 205.0f, 255.0f) / 255.0f;
    prims[0].diffuse = 0.6f;
    prims[0].specular_col = (float4)(206.0f, 206.0f, 205.0f, 255.0f) / 255.0f;
    prims[0].specular = 0.2f;
    prims[0].scale = (float4)(1.0f, 1.0f, 1.0f, PRIM_PLANE);
    prims[0].normal = fast_normalize((float4)(0, 20.0f, -0.1f, 0));

    // 232323 (the new black) wall
    prims[1].pos = (float4)(0, 0, 50.0f, 0);
    prims[1].diffuse_col = (float4)(35.0f, 35.0f, 35.0f, 255.0f) / 255.0f;
    prims[1].diffuse = 0.8f;
    prims[1].specular_col = (float4)(30.0f, 30.0f, 30.0f, 255.0f) / 255.0f;
    prims[1].specular = 0.2f;
    prims[1].scale = (float4)(1.0f, 1.0f, 1.0f, PRIM_PLANE);
    prims[1].normal = normalize((float4)(0.2f, -0.2f, -0.9f, 0));

    // FF9A0C (sun drums) sphere
    prims[2].pos = (float4)(2.5f-time, 2.5f, 100.0f, 0);
    prims[2].diffuse_col =  (float4)(255.0f, 154.0f, 12.0f, 255.0f) / 255.0f;
    prims[2].diffuse = 0.7f;
    prims[2].specular_col = (float4)(24.0f, 185.0f, 209.0f, 255.0f) / 255.0f;
    prims[2].specular = 0.95f;
    prims[2].scale = (float4)(1.0f, 1.0f, 1.0f, PRIM_SPHERE);
    prims[2].normal = normalize((float4)(0, 0.1f, 1.0f, 0));

    // FA7339 (casa) sphere
    prims[3].pos = (float4)(5.0f*cos(time*10.0f), 0, 50.0f+10.0f*sin(time*10.0f), 0);
    prims[3].diffuse_col = (float4)(250.0f, 115.0f, 57.0f, 255.0f) / 255.0f;
    prims[3].diffuse = 0.7f;
    //prims[3].specular_col = (float4)(24.0f, 185.0f, 209.0f, 255.0f) / 255.0f;
    prims[3].specular_col = (float4)(255.0f, 185.0f, 209.0f, 255.0f) / 255.0f;
    prims[3].specular = 0.9f;
    prims[3].scale = (float4)(3.0f, 1.0f, 1.0f, PRIM_SPHERE);
    prims[3].normal = normalize((float4)(0, 0.1f, 1.0f, 0));

    int i = 4;
    for(;i<10;i++) {
        // 18CEDB (blue lagoon) spheres
        prims[i].pos = (float4)(-1.5f*i+15.0f, .5f, -2.5f*i+100.0f, 0);
        prims[i].diffuse_col = (float4)(24.0f, 200.0f, 213.0f, 255.0f) / 255.0f;
        prims[i].diffuse = 0.6f;
        prims[i].specular_col = (float4)(24.0f, 190.0f, 210.0f, 255.0f) / 255.0f;
        prims[i].specular = 1.0f;
        prims[i].scale = (float4)(1.0f, 1.0f, 1.0f, PRIM_SPHERE);
        prims[i].normal = normalize((float4)(0, 0.1f, 1.0f, 0));
    }

    ray_trace(&ray, &prims);

    float4 col = ray.col;
    col =  clamp(col, 0, 1.0f);

    // write pixel data to gpu
    write_imagef(img, (int2)(x, y), col);
}
