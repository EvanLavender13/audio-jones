// Based on "The Butterfly Effect" by Martijn Steinrucken aka BigWings
// https://www.shadertoy.com/view/XsVGRV
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, gradient LUT coloring, per-butterfly FFT
// brightness, removed camera dive and song-timed fades.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float scrollPhase;
uniform float driftPhase;
uniform float flapPhase;
uniform float shiftPhase;
uniform float flapSpeed;
uniform float spread;
uniform float patternDetail;
uniform float wingShape;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define PI 3.141592653589793238
#define TWOPI 6.283185307179586
#define S01(x, offset, frequency) (sin((x+offset)*frequency*TWOPI)*.5+.5)
#define S(x, offset, frequency) sin((x+offset)*frequency*TWOPI)
#define saturate(x) clamp(x,0.,1.)

float hash(vec2 seed) {
    seed *= 213234598766.2345768238784;
    return fract(sin(seed.x)*1234765.876 + cos(seed.y)*8764238764.98787687);
}

vec2 hash2( vec2 p ) { p=vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))); return fract(sin(p)*18.5453); }

vec4 hash4(float seed) {
    vec2 n1 = hash2(vec2(seed, sin(seed*123432.2345)));
    vec2 n2 = hash2(n1);

    return vec4(n1, n2);
}

// return distance, and cell id
vec2 voronoi( in vec2 x )
{
    vec2 n = floor( x );
    vec2 f = fract( x );

    vec3 m = vec3( 8.0 );
    for( float j=-1.; j<=1.; j++ )        // iterate cell neighbors
    for( float i=-1.; i<=1.; i++ )
    {
        vec2  g = vec2( i, j );            // vector holding offset to current cell
        vec2  o = hash2( n + g );        // unique random offset per cell
          o.y*=.1;
        vec2  r = g - f + o;            // current pixel pos in local coords

        float d = dot( r, r );            // squared dist from center of local coord system

        if( d<m.x )                        // if dist is smallest...
            m = vec3( d, o );            // .. save new smallest dist and offset
    }

    return vec2( sqrt(m.x), m.y+m.z );
}

float skewcirc(vec2 uv, vec2 p, float size, float skew, float blur) {
    uv -= p;

    uv.x *= 1.+uv.y*skew;

    float c = length(uv);
    c = smoothstep(size+blur, size-blur, c);
    return c;
}

float fourierCurve(float x, vec4 offs, vec4 amp, vec4 pulse) {
    // returns a fourier-synthesied signal followed by a band-filter
    x *= 3. * pulse.w;

    vec4 c = vec4(    S(x, offs.x, 1.),
                      S(x, offs.y, 2.),
                     S(x, offs.z, 4.),
                     S(x, offs.w, 8.));

    float v = dot(c, amp*vec4(1., .5, .25, .125));

    pulse.y/=2.;

    v *= smoothstep(pulse.x-pulse.y-pulse.z, pulse.x-pulse.y, x);
    v *= smoothstep(pulse.x+pulse.y+pulse.z, pulse.x+pulse.y, x);
    return v;
}

vec4 Wing(vec2 st, vec2 id, float t_base, float radius, vec2 center, vec4 misc, vec4 offs, vec4 amp, vec4 pattern1, vec4 global, vec4 detail) {
    // returns a wings shape in the lower right quadrant (.5<st.x<1)
    // we do this by drawing a circle... (white if st.y<radius, black otherwise)
    // ...and scaling the radius based on st.x
    // when st.x<.5 or st.x>1 radius will be 0, inside of the interval it will be
    // an upside down parabola with a maximum of 1

    vec2 o=vec2(0.);

    // use upsidedown parabola 1-((x - center)*4)^2
    float b = mix(center.x, center.y, st.x);    // change the center based on the angle to further control the wings shape
    float a = (st.x-b)*4.;            // *4 so curve crosses 0 at .5 and 1.
    a *= a;
    a = 1.-a;                        // flip curve upside down
    float f = max(0., a);            // make curve 0 outside of interval

    f = pow(f, wingShape);

    o.x = st.x;

    float r = 0.;
    float x = st.x*2.;

    vec2 vor = voronoi(vec2(st.x, st.y*.1)*40.*detail.z*patternDetail);

    r = fourierCurve(x-b, offs, amp,vec4(global.x, global.y, max(.1, global.z), .333));

    r = (radius + r*.1)*f;

    float edge = 0.01;//max(.001, fwidth(r))*4.;

    o.x = smoothstep(r, r-edge, st.y);
    o.y=r;

    vec3 mainCol = texture(gradientLUT, vec2(t_base, 0.5)).rgb;
    vec3 edgeCol = texture(gradientLUT, vec2(fract(t_base + 0.3), 0.5)).rgb;
    vec3 detailCol = cross(edgeCol, mainCol);

    vec3 col = mainCol;

    misc = pow(misc, vec4(10.));

    r -= misc.y*fourierCurve(x-b, amp, offs, vec4(offs.xw, amp.wz));

    float edgeBand =  smoothstep(r-edge*3.*misc.w, r, st.y);
    col = mix(col, edgeCol, edgeBand);
    r = st.y-r;

    float clockValue = fourierCurve(r*.5+.5, pattern1, offs, amp)*global.x;

    float distValue = fourierCurve(length(st-offs.yx), pattern1.wzyx, amp, global);

    col += (clockValue+pow(distValue,3.))*detail.z;


    float d= distance(st, fract(st*20.*detail.x*detail.x));
    col += st.y*st.y*smoothstep(.1, .0, d)*detail.w*5.*fourierCurve(st.x,pattern1, offs, amp);

    col *= mix(1., st.y*st.y*(1.-vor.x*vor.x)*15., detail.x*detail.w);

    return vec4(col, o.x);
}

vec4 body(vec2 uv, vec4 n) {

    float eyes = skewcirc(uv, vec2(.005, .06), .01, 0., 0.001);

    uv.x+=.01;
    uv.x *= 3.;

    vec2 p = vec2(-.0, 0.);
    float size = .08;
    float skew = 2.1;
    float blur = .005;

    float v = skewcirc(uv, p, size, skew, blur);

    p.y -= .1;
    uv.x *= mix(.5, 1.5, n.x);
    v += skewcirc(uv, p, size, skew, blur);

    vec4 col = n.w*.1+ vec4(.1)* saturate(1.-uv.x*10.)*mix(.1, .6, S01(uv.y, 0., mix(20., 40., n.y)));
    col +=.1;
    col.a = saturate(v);


    col = mix(col, n*n, eyes);

    return col;
}

void main() {
    vec2 p = gl_FragCoord.xy / resolution.xy;
    p -= 0.5;
    p.x *= resolution.x / resolution.y;
    p *= spread;

    p.x += driftPhase;
    p.y += floor(p.x + 0.5) * 0.5 + 0.05;
    p.y -= scrollPhase;

    vec2 id = floor(p + 0.5);
    p = fract(p + 0.5) - 0.5;
    p.x = abs(p.x);

    float shifter = floor(shiftPhase);
    float t_base = hash2(id + shifter).x;
    float it = t_base * 10.0 + 0.25;

    vec4 pattern1 = hash4(it + 0.345);
    vec4 n1 = hash4(it);
    vec4 n2 = hash4(it + 0.3);
    vec4 n3 = hash4(n1.x);
    vec4 global = hash4(it * 12.);
    vec4 detail = hash4(it * -12.);
    vec4 nBody = hash4(it * 0.1425);

    p.x -= 0.01 * n1.x;

    vec4 col = vec4(1.);
    vec4 bodyCol = body(p, nBody);

    float wingFlap = pow(sin((flapPhase + (hash2(id.xy).x * 20.0 + 10.0) * flapSpeed) * TWOPI) * 0.5 + 0.5, 60.0);
    p.x *= mix(1., 20., wingFlap);

    vec2 st = vec2(atan(p.x, p.y), length(p));
    st.x /= PI;

    vec4 top = vec4(0.);
    if (st.x < 0.6) top = Wing(st, id, t_base, 0.5, vec2(0.25, 0.4), n1, n2, n3, pattern1, global, detail);
    vec4 bottom = vec4(0.);
    if (st.x > 0.4) bottom = Wing(st, id, t_base, 0.4, vec2(0.5, 0.75), n2, n3, n1, pattern1, global, detail);

    wingFlap = (1. - wingFlap * 0.9);
    vec4 wings = mix(bottom, top, top.a);
    wings.rgb *= wingFlap;

    col = mix(bodyCol * bodyCol.a, wings, wings.a);

    float freq = baseFreq * pow(maxFreq / baseFreq, t_base);
    float bin = freq / (sampleRate * 0.5);
    float energy = 0.0;
    if (bin <= 1.0) energy = texture(fftTexture, vec2(bin, 0.5)).r;
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;
    col.rgb *= brightness;

    finalColor = vec4(col.rgb, 1.0);
}
