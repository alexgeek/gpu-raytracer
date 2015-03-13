typedef struct {
    float4 origin;
    float4 dir;
    float4 col;
} Ray;

typedef struct {
    float4 col;
    float4 pos;
    float4 normal;
    float4 scale;
} Primitive;

#define PRIM_TYPE(P) (int)((P).scale.w)
#define RADIUS(P) P.scale.x
#define SCALE(P) (float3)(P.scala.x, P.scale.y, P.scale.z)
#define PRIM_PLANE 1
#define PRIM_SPHERE 2

int ray_plane(Ray* ray, Primitive* prim, float* t) {
    float dp = dot(ray->dir, prim->normal);
    if(dp == 0) return 0;
    float d = dot(prim->normal, prim->pos - ray->origin) / dp;
    if(d > 0 && d < *t) *t = d;
    return *t >= 0;
}

/**
 * http://www.vis.uky.edu/~ryang/teaching/cs535-2012spr/Lectures/13-RayTracing-II.pdf
 */
int ray_sphere(Ray* ray, Primitive* prim, float* t) {
    float radius = prim->scale.x;
    // vector from origin to primitive
    float4 v = prim->pos - ray->origin;
    // compute dotproduct of ray and v
    float dp = dot(ray->dir, v);
    // b^2 -4ac
    float det = dp*dp - dot(v, v) + radius*radius;
    // no solutions to quadratic formula
    if(det <= 0) return false;

    // solve for smaller t
    *t = (dp - sqrt(det));

    // if is t is less 0 then we have the wrong root
    if ( *t < 0 ) {
        // solve for the larger t
        *t = (dp + sqrt(det));

        // intersection is in opposite direction to ray if t < 0
        if(*t < 0) return false;
    }

    return true;
}

int ray_trace(Ray* ray, Primitive* prims) {
    const int num_prims = 6;
    int hit = -1;
    float t = MAXFLOAT;
    for(int p = 0; p < num_prims; p++)
    {
        int prim_type = (int)(prims[p].scale.w);
        switch(prim_type)
        {
            case PRIM_PLANE:
                if(ray_plane(ray, &prims[p], &t)) hit = p;
                break;
            case PRIM_SPHERE:
                if(ray_sphere(ray, &prims[p], &t)) hit = p;
                break;
        }
    }

    // no intersections
    if (hit == -1) return 0;

    // get the primitive we intersected
    Primitive* prim = prims + hit;

    // calculate point of intersection
    float4 intersection = ray->origin + t * ray->dir;

    // hack to get to primtive type from scale component
    int prim_type = (int)prim->scale.x;
    if(prim_type == PRIM_SPHERE)
    {
        float radius = prim->scale.x;
        prim->normal = (intersection - prim->pos) * radius;
    }
    prim->normal = normalize(prim->normal);
    float dp = dot(prim->normal, prim->pos - intersection);
    //if(dp > 0) ray->col += dp * prims[hit].col;
    dp = max(0.0f, dp);
    ray->col += dp * prims[hit].col;

    return 0;
}

__kernel void sine_wave(__write_only image2d_t img, unsigned int width, unsigned int height, float time)
{
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    float ratio = (float)width / height;

    // calculate uv coordinates
    float u = ((x+0.5) - (width/2)) / (2.0f * width) * ratio;
    float v = ((y+0.5) - (height/2)) / (2.0f * height) ;

    // generate ray from camera position amd colour
    Ray ray;
    ray.origin = -(float4)(0, 0, 0.95f , 0);
    ray.dir = fast_normalize((float4)(u, v, 0, 0) - ray.origin);
    ray.col = (float4)(0, 0, 0, 1.0f);

    // setup planes
    Primitive prims[6];

    // dark purple floor
    prims[0].pos = (float4)(0,0,0,0);
    prims[0].col = (float4)(0.2f, 0, 0.2f, 1.0f);
    prims[0].scale = (float4)(1.0f, 1.0f, 1.0f, PRIM_PLANE);
    prims[0].normal = normalize((float4)(0,1.0f,0,0));

    // green back wall1
    prims[1].pos = (float4)(0, 0, 100.0f, 0);
    prims[1].col = (float4)(0,0,0,0); // (float4)(0.1f, 0.8f, 0.33f, 1.0f);
    prims[1].scale = (float4)(1.0f, 1.0f, 1.0f, PRIM_PLANE);
    prims[1].normal = normalize((float4)(0, 0.1f, -1.0f, 0));

    // blue sphere
    prims[2].pos = (float4)(2.5f-time, 2.5f, 100.0f, 0);
    prims[2].col = (float4)(0, (sin(time)+1.0f)/2.0f, 0.9f, 1.0f);
    prims[2].scale = (float4)(1.0f, 1.0f, 1.0f, PRIM_SPHERE);
    prims[2].normal = normalize((float4)(0, 0.1f, 1.0f, 0));

    // red sphere
    prims[3].pos = (float4)(5.0f, 0, 100.0f, 0);
    prims[3].col = (float4)(0.5f, 0, 0, 1.0f);
    prims[3].scale = (float4)(3.0f, 1.0f, 1.0f, PRIM_SPHERE);
    prims[3].normal = normalize((float4)(0, 0.1f, 1.0f, 0));

    // red sphere
    prims[4].pos = (float4)(-2.5f, 2.5f, 100.0f, 0);
    prims[4].col = (float4)(0, 0.7f, 0, 1.0f);
    prims[4].scale = (float4)(1.0f, 1.0f, 1.0f, PRIM_SPHERE);
    prims[4].normal = normalize((float4)(0, 0.1f, 1.0f, 0));

    // yellow sphere
    prims[5].pos = (float4)(-5.0f, 0, 100.0f, 0);
    prims[5].col = (float4)(0.5f, 0.5f, 0, 1.0f);
    prims[5].scale = (float4)(4.0f, 1.0f, 1.0f, PRIM_SPHERE);
    prims[5].normal = normalize((float4)(0, 0.1f, 1.0f, 0));

    ray_trace(&ray, &prims);

    float4 col = ray.col;
    //float4 col = (float4)(u, v, 0, 1.0f);
    col =  clamp(col, 0, 1.0f); // clamp [0, 1]
    write_imagef(img, (int2)(x, y), col);
}

