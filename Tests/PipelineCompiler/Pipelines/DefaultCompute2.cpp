
#ifdef __cplusplus
#include "Engine/PipelineCompiler/Pipelines/ComputePipeline.h"

using namespace PipelineCompiler;

DECL_PIPELINE( DefaultCompute2, ComputePipeline,
{
	shader.Load( __FILE__ );
})
#endif


#ifdef SHADER
#if SHADER & SH_COMPUTE

// from https://www.shadertoy.com/view/4dSfDK

#define M_PI 3.1415926535897932384626433832795
#define M_TWO_PI (2.0 * M_PI)

float iTime;
vec2 iResolution;

float rand(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898,12.1414))) * 83758.5453);
}

float noise(vec2 n) {
    const vec2 d = vec2(0.0, 1.0);
    vec2 b = floor(n);
    vec2 f = smoothstep(vec2(0.0), vec2(1.0), fract(n));
    return mix(mix(rand(b), rand(b + d.yx), f.x), mix(rand(b + d.xy), rand(b + d.yy), f.x), f.y);
}

vec3 ramp(float t) {
	return t <= .5 ? vec3( 1. - t * 1.4, .2, 1.05 ) / t : vec3( .3 * (1. - t) * 2., .2, 1.05 ) / t;
}
vec2 polarMap(vec2 uv, float shift, float inner) {

    uv = vec2(0.5) - uv;
    
    
    float px = 1.0 - fract(atan(uv.y, uv.x) / 6.28 + 0.25) + shift;
    float py = (sqrt(uv.x * uv.x + uv.y * uv.y) * (1.0 + inner * 2.0) - inner) * 2.0;
    
    return vec2(px, py);
}
float fire(vec2 n) {
    return noise(n) + noise(n * 2.1) * .6 + noise(n * 5.4) * .42;
}

float shade(vec2 uv, float t) {
    uv.x += uv.y < .5 ? 23.0 + t * .035 : -11.0 + t * .03;    
    uv.y = abs(uv.y - .5);
    uv.x *= 35.0;
    
    float q = fire(uv - t * .013) / 2.0;
    vec2 r = vec2(fire(uv + q / 2.0 + t - uv.x - uv.y), fire(uv + q - t));
    
    return pow((r.y + r.y) * max(.0, uv.y) + .1, 4.0);
}

vec3 color(float grad) {
    
    float m2 = 0.15; //iMouse.z < 0.0001 ? 0.15 : iMouse.y * 3.0 / iResolution.y;
    grad =sqrt( grad);
    vec3 color = vec3(1.0 / (pow(vec3(0.5, 0.0, .1) + 2.61, vec3(2.0))));
    vec3 color2 = color;
    color = ramp(grad);
    color /= (m2 + max(vec3(0), color));
    
    return color;

}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	
    float m1 = 1.6; //iMouse.z < 0.0001 ? 1.6 : iMouse.x * 5.0 / iResolution.x;
    
    float t = iTime;
    vec2 uv = fragCoord / iResolution.yy;
    float ff = 1.0 - uv.y;
    uv.x -= (iResolution.x / iResolution.y - 1.0) / 2.0;
    vec2 uv2 = uv;
    uv2.y = 1.0 - uv2.y;
   	uv = polarMap(uv, 1.3, m1);
   	uv2 = polarMap(uv2, 1.9, m1);

    vec3 c1 = color(shade(uv, t)) * ff;
    vec3 c2 = color(shade(uv2, t)) * (1.0 - ff);
    
    fragColor = vec4(c1 + c2, 1.0);
}


layout(binding=0, rgba8) writeonly uniform image2D  un_OutImage;

void main ()
{
	iTime = 1.0;
	iResolution = vec2((gl_WorkGroupSize * gl_NumWorkGroups).xy);

	vec2 fragCoord = vec2(gl_GlobalInvocationID.xy);
	vec4 fragColor;

	mainImage( fragColor, fragCoord );

	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), fragColor );
}

#endif
#endif	// SHADER
