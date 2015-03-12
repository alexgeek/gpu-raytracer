typedef struct {
    float4 col;
    float4 intensity;
    float4 pos;
    float4 normal;
} Plane;

typedef struct {
    float4 origin;
    float4 dir;
    float4 col;
} Ray;

int ray_plane(Ray* ray, Plane* plane, float* t) {
    float dp = dot(ray->dir, plane->normal);
    if(dp == 0) return 0;
    float d = dot(plane->normal, plane->pos - ray->origin) / dp;
    if(d > 0 && d < *t) *t = d;
    return *t >= 0;
}

int ray_trace(Ray* ray, Plane* planes) {
    int hit = -1;
    float t;
    for(int p = 0; p < 2; p++)
        if(ray_plane(ray, &planes[p], &t))
            hit = p;

    // no intersections
    if (hit == -1) return 0;
    // if intersected light
    // if( all(planes[hit].intensity != 0))
    if( length(planes[hit].intensity) > 0) {
        ray->col = planes[hit].intensity * planes[hit].col;
    } else {
        float4 intersection = ray->origin + (t * ray->dir);
        for(int p = 0; p < 10; p++)
        {
            // if its emitting light
            if( length(planes[p].intensity) > 0) {
                // vector between two planes
                float4 dp = dot(planes[p].pos - intersection, planes[p].normal);
                // rather than branch use min
                dp = min(0, dp);
                ray->col += dp * planes[p].col * planes[p].intensity;
            }
        }
    }
    return 0;
}

__kernel void sine_wave(__global float4* pos, unsigned int width, unsigned int height, float time)
{
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);

    // calculate uv coordinates
    float u = x / (float) width;
    float v = y / (float) height;

    // generate ray from camera position amd colour
    Ray ray;
    ray.origin = (float4)(0, 5.0f, -10.0f, 1.0f);
    ray.dir = (float4)(0, 0, 1.0f, 0);
    ray.col = (float4)(0, 0, 0, 1.0f);

    // setup planes
    Plane planes[2];
    planes[0].pos = (float4)(0,0,0,0);
    planes[0].col = (float4)(0.3f, 0.2f, 0.33f, 1);
    planes[0].intensity = (float4)(0,0,0,0);
    planes[0].normal = (float4)(0,1.0f,0,0);

    planes[1].pos = (float4)(0,10,0,0);
    planes[1].col = (float4)(0.7f,0.5f,0.33f,1.0f);
    planes[1].intensity = (float4)(0,0,0,0);
    planes[1].normal = (float4)(0,-1.0f,0,0);

    ray_trace(&ray, &planes);

    u = u*2.0f - 1.0f;
    v = v*2.0f - 1.0f;

    // calculate simple sine wave pattern
    float freq = 4.0f;
    float w = sin(u*freq + time) * cos(v*freq + time) * 0.5f;

    // write output vertex
    pos[y*width+x] = (float4)(u, w, v, 1.0f);
    pos[y*width+x] = (float4)(0.5f, 0.5f, 0.8f, 1.0f);


}

