#version 420 core

#define MAX_OBJECTS 20

//// Structs ////

struct Material
{
	float shininess;
	float luminance;
	vec3 color;
	int shading;
	// Texture?
	//float n; index of refraction
};

// Perhaps a version of this with texture support?

struct Object3D
{
	vec3 position;
	vec3 direction;
	//mat4 translation;
	float radius;
	float age;
	vec3 shape;
	int geometry; 
	float mass;
	vec3 color;
//	Material material;
};

struct Ray
{
	vec3 position;
	vec3 direction;
//	float totalDist;
};


////// Operators //////

// Union
float opU(float a, float b)
{
	return min(a, b);
}

// polynomial smooth min (k = 0.1);
float smin(float a, float b, float k)
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

///// Signed Distance Geometries ////

float sdSphere(vec3 p, float s)
{
  return length(p)-s;
}
    
float sdTorus(vec3 p, vec2 t)
{
  vec2 q = vec2(length(p.xy)-t.x,p.z);
  return length(q)-t.y;
}

float sdRoundBox(vec3 p, vec3 b, float r)
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0)) - r;
}

float sdBoxInterior( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return -(min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0)));
}


//// Math Utility ////

vec2 rot2D(vec2 p, float angle) 
{
    float s = sin(angle);
    float c = cos(angle);
    return p * mat2(c,s,-s,c);
}

float norm(float n)
{
	return 0.5*n+0.5;
}

/// Ray Marching and Distance Fields ///

float map(vec3 p, Object3D obj[MAX_OBJECTS], int objCount, inout int closestIndex)
{
	float dist = 1000000.0;
	for (int i = 0; i < MAX_OBJECTS; i++) {
		if (i>=objCount) break;
		if (obj[i].geometry == 1) continue;
		float altDist;
		switch(obj[i].geometry) {
			case 4:
				altDist = sdSphere( p - obj[i].position, obj[i].radius);
				if (altDist < dist) {
					dist = altDist;
					closestIndex = i;
				}
				break;
			case 7:
				altDist = sdBoxInterior( p - obj[i].position, obj[i].shape);
				if (altDist < dist) {
					dist = altDist;
					closestIndex = i;
				}
				//dist = min(dist, sdSphere( p + obj[i].position, 4.0 ));
				break;
			default:
				dist = min(dist, 1000000.0);
				break;
		}
	}
	return dist;
}

void intersect(inout Ray r, Object3D obj[MAX_OBJECTS], int objCount, inout int closestIndex, vec3 warpA, vec3 warpB, int warpCount)
{
    const float maxDist = 280.0;
    const float epsilon = 0.005;
    float totalD = 0.0;
	for (int i=0; i < 96; i++)
    {
		float minDist = map(r.position, obj, objCount, closestIndex);

		// Warping from warpA and warpB
		if (warpCount > 1) {
			vec3 diffA = r.position - warpA;
			vec3 diffB = r.position - warpB;
			float diffALength = length(diffA);
			float diffBLength = length(diffB);
			minDist =  min(minDist, min(diffALength, diffBLength));
			float forceA = 1.4 / (diffALength*diffALength*diffALength);
			float forceB = 1.4 / (diffBLength*diffBLength*diffBLength);
			r.direction = normalize(r.direction - minDist * (forceA * diffA + forceB * diffB));
		}

   		r.position += r.direction * minDist * 0.65;

        totalD += minDist;
        if (minDist < epsilon || totalD > maxDist) break;
    }

}

vec3 calcNormal(vec3 p, Object3D obj[MAX_OBJECTS], int objCount, inout int closestIndex)
{
    vec3 eps = vec3(0.002,0.0,0.0);

	return normalize(vec3(
           map(p+eps.xyy, obj, objCount, closestIndex) - map(p-eps.xyy, obj, objCount, closestIndex),
           map(p+eps.yxy, obj, objCount, closestIndex) - map(p-eps.yxy, obj, objCount, closestIndex),
           map(p+eps.yyx, obj, objCount, closestIndex) - map(p-eps.yyx, obj, objCount, closestIndex)));
}

////////////

// Uniforms 

out vec4 color;

uniform float time;
uniform vec2 resolution;
uniform vec2 cameraRotation;
uniform vec3 cameraPos;

uniform int warpCount;
uniform vec3 warpA;
uniform vec3 warpB;

uniform int objectCount;

uniform Object3D objects[MAX_OBJECTS];

/////////

void main()
{
	vec3 eyePos = vec3(0.0, 0.0, 0.0); // Could be set to something else
	
	vec2 uv = (gl_FragCoord.xy / resolution) * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y; 

	Ray ray = Ray(cameraPos, normalize(vec3(uv, 1.2)));
	
	ray.direction.yz = rot2D( ray.direction.yz, cameraRotation.y); 
	ray.direction.xz = rot2D( ray.direction.xz, cameraRotation.x); 

	//vec3 spherePos = objects[0].position;//vec3(0.0, 0.0, 4.0);

	int closestIndex;
	int dummy;
	vec3 normal;
	Object3D closest;
	
	intersect(ray, objects, objectCount, closestIndex, warpA, warpB, warpCount);

	for (int timesWarped = 0; timesWarped < 6; timesWarped++) {

		closest = objects[closestIndex];

		if (closest.geometry == 4) {
			/*
			normal = calcNormal(ray.position, objects, objectCount, dummy);
			ray.direction = reflect(ray.direction, normal);
			ray.position += ray.direction * 0.2;
			*/
			if (length(closest.position - warpA) - closest.radius < 0.05) {
				ray.position -= warpA;
			//	ray.position.y *= -1.0;
				ray.position += warpB;
			} else if (length(closest.position - warpB) - closest.radius < 0.05) {
				ray.position -= warpB;
			//	ray.position.y *= -1.0;
				ray.position += warpA;
			}
			ray.direction = -ray.direction;
			ray.position += ray.direction * 0.2;
			intersect(ray, objects, objectCount, closestIndex, warpA, warpB, warpCount);
		}

	}

	closest = objects[closestIndex];
	vec3 color1 = closest.color;
	if (closest.geometry == 4) {
		color1.r = sin(closest.age*0.04)*0.5 + 0.5;
	} else if (closest.geometry == 7) {
		vec3 p = 0.35*ray.position;
		if ((int(floor(p.x)+floor(p.y)+floor(p.z)) & 1) == 0) {
			color1 = vec3(p.x/8.0+0.5, p.y/8.0+0.5, p.z/8.0+0.5);
		} else {
			color1 = vec3(0.7, 0.7, 0.7);
		}
		
	}

	//ray.position += d * ray.direction;


	normal = calcNormal(ray.position, objects, objectCount, dummy);

	ray.direction = reflect(ray.direction, normal);
	
	color = vec4(color1*(dot(normal, ray.direction)+0.2), 1.0); 

//	color = vec4(sin(20.0*gl_FragCoord.x/resolution.x)*0.5+0.5, 0.8, 0.0, 1.0);
}