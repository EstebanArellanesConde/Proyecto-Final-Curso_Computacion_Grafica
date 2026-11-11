#version 330 core
in vec3 color;
in vec3 world;
in vec2 screen;
flat in float material;
in float stone;
in vec2 uv;
flat in float layer;
flat in float houseSeed;
uniform sampler2DArray modelTextures;
uniform int windowLayer;
uniform mat4 inverseTransform;
uniform vec3 eye;
uniform vec3 altar;
uniform vec3 candleLights[13];
uniform float lakeTime;
uniform vec3 statue;
uniform vec4 reflectionSources[18];
uniform int reflectionCount;
uniform sampler2D lightMap;
uniform vec2 lightOrigin;
uniform vec2 lightExtent;
uniform bool night;
uniform bool sky;
out vec4 fragmentColor;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p)
{
    vec2 cell = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(cell), hash(cell + vec2(1,0)), f.x),
               mix(hash(cell + vec2(0,1)), hash(cell + 1.0), f.x), f.y);
}

vec3 atmosphere(vec3 ray)
{
    float altitude = clamp(ray.y, 0.0, 1.0);
    if (night) {
        vec3 tone = mix(vec3(0.21,0.29,0.39), vec3(0.029,0.061,0.14), pow(altitude, 0.40));
        vec2 grid = vec2(atan(ray.z,ray.x), ray.y) * 310.0;
        float star = step(0.994, hash(floor(grid))) * (1.0-smoothstep(0.0,0.26,length(fract(grid)-0.5)));
        vec3 moonDir = normalize(vec3(-0.30,0.35,-1.0));
        float moon = distance(ray,moonDir);
        vec2 lunar = (ray-moonDir).xy*67.0;
        float terrain = 0.54+0.29*noise(lunar*2.8)+0.12*noise(lunar*7.3);
        tone += vec3(0.77,0.83,0.94) * (star*smoothstep(0.03,0.16,altitude)
            + (1.0-smoothstep(0.025,0.028,moon))*terrain + exp(-moon*12.0)*0.10);
        return tone;
    }
    vec3 tone = mix(vec3(0.73,0.83,0.86), vec3(0.075,0.43,0.65), pow(altitude,0.46));
    vec2 cloudUV = ray.xz / max(0.08, ray.y + 0.12) * 3.6;
    float cloud = noise(cloudUV)*0.58 + noise(cloudUV*2.2)*0.27 + noise(cloudUV*4.8)*0.15;
    float cover = smoothstep(0.53,0.73,cloud) * smoothstep(0.015,0.075,altitude);
    return mix(tone,mix(vec3(0.84,0.88,0.88),vec3(0.98,0.96,0.90),smoothstep(0.62,0.81,cloud)),cover);
}

vec3 water(vec3 ray, float range)
{
    vec2 p = world.xz;
    float detail = 1.0 - smoothstep(45.0,340.0,range);
    float waves = (noise(p*vec2(0.22,1.9)+vec2(0.12,-0.82)*lakeTime)
        +0.33*noise(p*vec2(0.80,5.9)+vec2(-0.14,1.20)*lakeTime)-0.65)*detail;
    vec3 reflected = reflect(ray,normalize(vec3(waves*0.09,1.0,waves*0.13)));
    float fresnel = 0.18 + 0.73*pow(1.0-clamp(-ray.y,0.0,1.0),3.4);
    float shore = 1.0-smoothstep(41.0,100.0,length(p));
    vec3 base = mix(vec3(0.24,0.42,0.41),vec3(0.44,0.42,0.23),shore);
    if (night) base = mix(vec3(0.055,0.10,0.16),vec3(0.10,0.15,0.18),shore);
    vec3 glimmer = night ? vec3(0.12,0.17,0.22) : vec3(0.23,0.28,0.25);
    vec3 tone = mix(base,atmosphere(reflected),fresnel)+glimmer*waves*0.32;
    if (night && abs(p.x)<78.0 && abs(p.y)<111.0) {
        float breaks = smoothstep(0.24,0.80,noise(p*vec2(1.9,10.0)+vec2(0.04,1.14)*lakeTime));
        for (int i=0; i<reflectionCount; ++i) {
            vec3 lamp = reflectionSources[i].xyz;
            vec2 lampOffset=p-lamp.xz;
            float distanceSquared=dot(lampOffset,lampOffset);
            if (distanceSquared>380.0) continue;
            vec2 towardsEye = eye.xz-lamp.xz;
            vec2 axis = towardsEye/max(0.01,length(towardsEye));
            float height = max(0.1,eye.y);
            vec2 center = mix(eye.xz,lamp.xz,height/(height+lamp.y));
            vec2 offset = p-center;
            float across = dot(offset,vec2(-axis.y,axis.x));
            float along = dot(offset,axis);
            float band = exp(-abs(across+0.21*waves)*6.0-abs(along)*0.57);
            tone += vec3(1.0,0.51,0.13)*band*breaks*reflectionSources[i].w
                *(1.0-smoothstep(39.0,380.0,distanceSquared));
        }
    }
    return tone;
}

bool cutPaper(vec2 flag, bool rosette)
{
    if (flag.y>0.955+0.042*cos(flag.x*37.7)) return true;
    vec2 p=(flag-0.5)*vec2(1.15,1.0);
    float r=length(p);
    if (rosette) {
        float angle=atan(p.y,p.x);
        if (r<0.145+0.065*cos(6.0*angle) || (r>0.275 && r<0.292)) return true;
    } else {
        float diamond=abs(p.x)+abs(p.y)*0.79;
        if (diamond<0.094 || (diamond>0.17 && diamond<0.213)) return true;
    }
    vec2 dotUV=fract(flag*5.0)-0.5;
    bool edge=(abs(p.x)>0.38 || abs(p.y)>0.30) && abs(p.x)<0.52 && abs(p.y)<0.43;
    return edge && abs(dotUV.x)+abs(dotUV.y)<0.24;
}

vec3 adoquin(vec2 p)
{
    vec2 tile=p*vec2(3.45,3.9);
    vec2 cell=floor(tile), local=fract(tile);
    float first=10.0, second=10.0;
    vec2 stoneId=cell;
    for (int z=-1; z<=1; ++z) {
        for (int x=-1; x<=1; ++x) {
            vec2 offset=vec2(x,z), id=cell+offset;
            vec2 jitter=vec2(hash(id+17.4),hash(id+42.8));
            vec2 delta=offset+mix(vec2(0.18),vec2(0.82),jitter)-local;
            float distanceSquared=dot(delta,delta);
            if (distanceSquared<first) {
                second=first;
                first=distanceSquared;
                stoneId=id;
            } else {
                second=min(second,distanceSquared);
            }
        }
    }
    float edge=sqrt(second)-sqrt(first);
    float face=smoothstep(0.034,0.103,edge);
    float detail=0.82+0.28*hash(stoneId)+0.15*noise(p*vec2(21.0,28.0));
    vec3 tint=mix(vec3(0.95,0.98,1.03),vec3(1.07,1.02,0.93),hash(stoneId+51.1));
    return tint*mix(0.50,detail,face)+vec3(0.065)*smoothstep(0.048,0.079,edge)*(1.0-smoothstep(0.08,0.14,edge));
}

void main()
{
    if (sky) {
        vec4 point = inverseTransform * vec4(screen,1.0,1.0);
        fragmentColor = vec4(atmosphere(normalize(point.xyz / point.w - eye)),1.0);
        return;
    }
    vec3 difference = world - eye;
    float range = length(difference);
    vec3 ray = difference / max(0.001, range);
    if (material == 1.0) {
        fragmentColor = vec4(water(ray,range),1.0);
        return;
    }
    if (material == 15.0) {
        float espuma=color.r*smoothstep(0.31,0.80,noise(world.xz*vec2(13.0,24.0)+vec2(0.3,1.45)*lakeTime));
        vec3 brillo=night ? vec3(0.38,0.54,0.62) : vec3(0.72,0.86,0.84);
        fragmentColor=vec4(mix(water(ray,range),brillo,espuma),1.0);
        return;
    }
    if (material == 3.0 && !night) discard;
    if (material == 4.0 && fract((world.x+world.y)*24.0) < 0.29 && fract(world.y*31.0) < 0.33) discard;
    bool paper = material == 6.0 || material == 7.0;
    if (paper && range<37.0 && cutPaper(uv,material == 6.0)) discard;

    vec3 albedo = color;
    if (material == 5.0) {
        vec4 surface = texture(modelTextures,vec3(uv,layer));
        if (surface.a < 0.35) discard;
        albedo *= surface.rgb;
    }
    if (stone>0.01 && range<67.0) {
        albedo *= mix(vec3(1),adoquin(world.xz),stone*(1.0-smoothstep(18.0,67.0,range)));
    } else if (material==11.0) {
        vec2 board = vec2(world.x*2.1,world.z*26.0+world.y*9.8);
        float fiber=noise(board)+0.38*noise(board*vec2(1.1,3.2));
        float bands=noise(vec2(world.x*0.48,world.z*10.3+world.y*4.0));
        albedo *= 0.72+0.32*fiber+0.21*bands;
    } else if (material==13.0) {
        float stitch=abs(fract(uv.x*19.0)-0.5);
        float band=abs(uv.y-0.50);
        float diamond=stitch+band*0.62;
        float embroidered=float((diamond>0.135 && diamond<0.19) || (diamond>0.235 && diamond<0.274));
        float piping=float((band>0.35 && band<0.391) || band>0.465);
        vec3 thread=mix(vec3(0.88,0.56,0.26),vec3(0.97,0.81,0.56),step(0.68,fract(uv.x*19.0)));
        albedo=mix(albedo,thread,max(embroidered*0.77,piping*0.90));
        albedo*=0.91+0.13*noise(world.xz*12.4+world.y*17.3);
    }
    vec3 tone = albedo * (0.94 + 0.10*noise(world.xz*5.3 + world.y*vec2(2.7,4.2)));
    if (night) {
        tone *= vec3(0.39,0.50,0.70);
        vec3 local = world - altar;
        float warm = 1.0 / (1.0 + dot(local,local)*0.29);
        tone += albedo * vec3(0.82,0.38,0.10) * warm;
        vec2 light = texture(lightMap,(world.xz-lightOrigin)/lightExtent).rg;
        float height = max(0.0,world.y-light.y-2.3);
        float pathLight = min(light.x,1.72)*exp(-height*0.41);
        tone += albedo * vec3(0.76,0.38,0.14) * pathLight;
        float veladoras=0.0;
        for (int i=0; i<candleLights.length(); ++i) {
            vec3 offset=world-candleLights[i];
            float glow=max(0.0,1.0-dot(offset,offset)*0.43);
            veladoras+=glow*glow;
        }
        tone+=albedo*vec3(0.44,0.23,0.081)*veladoras;
        vec3 monumentLight = (world-statue)*vec3(1.0,0.39,1.0);
        tone += albedo * vec3(0.46,0.36,0.24)*exp(-0.15*dot(monumentLight,monumentLight));
        if (material == 5.0 && layer == float(windowLayer) && houseSeed>0.39) {
            float glow = clamp(0.53+dot(albedo,vec3(0.33)),0.53,0.86);
            tone = vec3(1.0,0.65,0.29)*glow;
        }
        if (material == 2.0 || material == 3.0) tone = color * 1.18;
        if (material == 9.0) tone = color * vec3(1.23,1.14,0.90);
        if (material == 10.0) tone = color*vec3(1.36,1.27,1.02);
        if (material == 12.0) tone = max(tone,color*vec3(1.16,1.13,0.95));
        if (material == 14.0) tone += albedo*vec3(0.025,0.060,0.030);
        if (paper || material == 8.0) tone += albedo*vec3(0.13,0.095,0.072);
    } else if (material == 2.0) {
        tone = vec3(0.26,0.30,0.28);
    } else if (material == 9.0) {
        tone = color * 0.71;
    } else if (material == 10.0) {
        tone = mix(vec3(0.62,0.55,0.39),color,0.36);
    } else if (material == 12.0) {
        tone = mix(vec3(0.71,0.52,0.27),color,0.49);
    }
    float haze = 1.0 - exp(-max(0.0,range-50.0)*(night ? 0.0018 : 0.00145));
    vec3 horizon = night ? vec3(0.16,0.23,0.33) : vec3(0.64,0.76,0.78);
    fragmentColor = vec4(mix(tone,horizon,haze),1.0);
}
