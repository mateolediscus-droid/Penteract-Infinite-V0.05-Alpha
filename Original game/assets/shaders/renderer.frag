#version 450 core

#extension GL_ARB_bindless_texture : require
//#extension GL_ARB_gpu_shader_int64 : require

const float EPS = 1e-6;
const float INF = 1e30;

struct vec5
{
	vec4 abcd;
	float e;
};
vec5 add(in vec5 a, in vec5 b) { return vec5(a.abcd + b.abcd, a.e + b.e); }
vec5 sub(in vec5 a, in vec5 b) { return vec5(a.abcd - b.abcd, a.e - b.e); }
vec5 mul(in vec5 a, in vec5 b) { return vec5(a.abcd * b.abcd, a.e * b.e); }
vec5 div(in vec5 a, in vec5 b) { return vec5(a.abcd / b.abcd, a.e / b.e); }
vec5 add(in vec5 a, in float b) { return vec5(a.abcd + vec4(b), a.e + b); }
vec5 sub(in vec5 a, in float b) { return vec5(a.abcd - vec4(b), a.e - b); }
vec5 mul(in vec5 a, in float b) { return vec5(a.abcd * vec4(b), a.e * b); }
vec5 div(in vec5 a, in float b) { return vec5(a.abcd / vec4(b), a.e / b); }
float dot5(in vec5 a, in vec5 b) { return dot(a.abcd, b.abcd) + dot(a.e, b.e); }
float length5(in vec5 v) { return sqrt(dot5(v, v)); }
vec5 normalize5(in vec5 v) { return div(v, sqrt(dot5(v, v))); }
vec4 abcd(in vec5 v) { return v.abcd; }
vec4 bcde(in vec5 v) { return vec4(v.abcd.yzw, v.e); }
struct ivec5
{
	ivec4 abcd;
	int e;
};
ivec5 add(in ivec5 a, in ivec5 b) { return ivec5(a.abcd + b.abcd, a.e + b.e); }
ivec5 sub(in ivec5 a, in ivec5 b) { return ivec5(a.abcd - b.abcd, a.e - b.e); }
ivec5 mul(in ivec5 a, in ivec5 b) { return ivec5(a.abcd * b.abcd, a.e * b.e); }
ivec5 div(in ivec5 a, in ivec5 b) { return ivec5(a.abcd / b.abcd, a.e / b.e); }
ivec5 add(in ivec5 a, in int b) { return ivec5(a.abcd + ivec4(b), a.e + b); }
ivec5 sub(in ivec5 a, in int b) { return ivec5(a.abcd - ivec4(b), a.e - b); }
ivec5 mul(in ivec5 a, in int b) { return ivec5(a.abcd * ivec4(b), a.e * b); }
ivec5 div(in ivec5 a, in int b) { return ivec5(a.abcd / ivec4(b), a.e / b); }
ivec4 abcd(in ivec5 v) { return v.abcd; }
ivec4 bcde(in ivec5 v) { return ivec4(v.abcd.yzw, v.e); }

vec5 min5(in vec5 a, in vec5 b) { return vec5(min(a.abcd, b.abcd), min(a.e, b.e)); }
vec5 max5(in vec5 a, in vec5 b) { return vec5(max(a.abcd, b.abcd), max(a.e, b.e)); }
vec5 abs5(in vec5 v) { return vec5(abs(v.abcd), abs(v.e)); }
vec5 sign5(in vec5 v) { return vec5(sign(v.abcd), sign(v.e)); }
vec5 floor5(in vec5 v) { return vec5(floor(v.abcd), floor(v.e)); }
vec5 round5(in vec5 v) { return vec5(round(v.abcd), round(v.e)); }
vec5 mix5(in vec5 a, in vec5 b, float t) { return vec5(mix(a.abcd, b.abcd, t), mix(a.e, b.e, t)); }
vec5 fract5(in vec5 v) { return vec5(fract(v.abcd), fract(v.e)); }
vec5 clamp5(in vec5 v, in vec5 a, in vec5 b) { return vec5(clamp(v.abcd, a.abcd, b.abcd), clamp(v.e, a.e, b.e)); }

struct bvec5
{
	bvec4 abcd;
	bool e;
};
bvec5 lessThan5(in vec5 a, in vec5 b) { return bvec5(lessThan(a.abcd, b.abcd), a.e < b.e); }
bvec5 greaterThan5(in vec5 a, in vec5 b) { return bvec5(greaterThan(a.abcd, b.abcd), a.e > b.e); }
bvec5 greaterThanEqual5(in vec5 a, in vec5 b) { return bvec5(greaterThanEqual(a.abcd, b.abcd), a.e >= b.e); }
bvec5 equal5(in vec5 a, in vec5 b) { return bvec5(equal(a.abcd, b.abcd), a.e == b.e); }
bvec5 lessThan5(in ivec5 a, in ivec5 b) { return bvec5(lessThan(a.abcd, b.abcd), a.e < b.e); }
bvec5 greaterThan5(in ivec5 a, in ivec5 b) { return bvec5(greaterThan(a.abcd, b.abcd), a.e > b.e); }
bvec5 greaterThanEqual5(in ivec5 a, in ivec5 b) { return bvec5(greaterThanEqual(a.abcd, b.abcd), a.e >= b.e); }
bvec5 equal5(in ivec5 a, in ivec5 b) { return bvec5(equal(a.abcd, b.abcd), a.e == b.e); }
bool any5(in bvec5 v) { return any(v.abcd) || v.e; }
bool all5(in bvec5 v) { return all(v.abcd) && v.e; }

vec5 mix5(in vec5 a, in vec5 b, in bvec5 t) { return vec5(mix(a.abcd, b.abcd, t.abcd), mix(a.e, b.e, t.e)); }

vec5 vec5_ivec5(in ivec5 v) { return vec5(vec4(v.abcd), float(v.e)); }
ivec5 ivec5_vec5(in vec5 v) { return ivec5(ivec4(v.abcd), int(v.e)); }

float ind_get(in vec5 v, int i)
{
	switch (i)
	{
	case 0: return v.abcd.x;
	case 1: return v.abcd.y;
	case 2: return v.abcd.z;
	case 3: return v.abcd.w;
	case 4: return v.e;
	}
	return v.abcd.x;
}
int ind_get(in ivec5 v, int i)
{
	switch (i)
	{
	case 0: return v.abcd.x;
	case 1: return v.abcd.y;
	case 2: return v.abcd.z;
	case 3: return v.abcd.w;
	case 4: return v.e;
	}
	return v.abcd.x;
}
void ind_set(inout vec5 v, int i, float value)
{
	switch (i)
	{
	case 0: v.abcd.x = value; break;
	case 1: v.abcd.y = value; break;
	case 2: v.abcd.z = value; break;
	case 3: v.abcd.w = value; break;
	case 4: v.e = value; break;
	}
}
void ind_set(inout ivec5 v, int i, int value)
{
	switch (i)
	{
	case 0: v.abcd.x = value; break;
	case 1: v.abcd.y = value; break;
	case 2: v.abcd.z = value; break;
	case 3: v.abcd.w = value; break;
	case 4: v.e = value; break;
	}
}

out vec4 color;

in vec2 uv;
uniform vec4 screenSize; // w, h, 1/w, 1/h
uniform float time;
struct Camera
{
	vec5 pos;
	vec5 up;
	vec5 forward;
	vec5 left;
	vec5 over;
	vec5 yonder;
	float vFov;
};
layout(std430, binding = 0) readonly buffer CameraBuf
{
	Camera cam;
};
uniform vec5 lightDir;

layout(std430, binding = 1) readonly buffer BlockDataHandles
{
	uvec2 blockData[];
};
uniform usampler3D chunks;
uniform ivec4 chunksMinBound;
uniform ivec4 chunksSize; // = maxBound - minBound

layout(std430, binding = 2) readonly buffer BlockTexturesBuffer
{
	int textures[];
};

layout(std430, binding = 3) readonly buffer TileTexturesBuffer
{
	uvec2 tiles[];
};

struct Target
{
	bool targeting;
	ivec5 blockPos;
	int side;
};
uniform Target target;

uint getBlock(in uvec2 handle, ivec5 v)
{
	int texX = v.abcd.y;
	int texY = v.abcd.z;
	int texZ = v.abcd.w + v.e * 8;
	
	uvec4 col = texelFetch(usampler3D(handle), ivec3(texX, texY, texZ), 0);
	
	uint c = uint(v.abcd.x) >> 3u;
	uint shift = (uint(v.abcd.x) & 7u) << 2u;
	
	return (col[int(c)] >> shift) & 15u;
}

struct RayHit
{
	bool hit;
	vec5 pos;
	vec5 normal;
	vec4 texCoord;
	float dist;
	uint id;
	ivec5 blockPos;
	int side;
};

vec4 computeTexCoord(in vec5 p, in vec5 n)
{
	vec4 texCoord;

	if (abs(n.abcd.y) > 0.5)
	{
		texCoord = vec4(p.abcd.z, 1.0 - p.abcd.x, p.abcd.w, p.e);
	}
	else if (abs(n.abcd.x) > 0.5)
	{
		texCoord = vec4(p.abcd.y, 1.0 - p.abcd.z, p.abcd.w, p.e);
	}
	else if (abs(n.abcd.z) > 0.5)
	{
		texCoord = vec4(p.abcd.y, 1.0 - p.abcd.x, p.abcd.w, p.e);
	}
	else if (abs(n.abcd.w) > 0.5)
	{
		texCoord = vec4(p.abcd.z, 1.0 - p.abcd.x, p.abcd.y, p.e);
	}
	else
	{
		texCoord = vec4(p.abcd.z, 1.0 - p.abcd.x, p.abcd.w, p.abcd.y);
	}

	return fract(texCoord);
}
int getNormalAxis(vec5 n)
{
	for (int i = 0; i < 5; ++i)
	{
		if (abs(ind_get(n, i)) > 0.5)
		{
			return i;
		}
	}
	return 0;
}
ivec4 getTangentAxes(vec5 n)
{
	if (abs(n.abcd.y) > 0.5)
	{
		return ivec4(2, 0, 3, 4);
	}
	else if (abs(n.abcd.x) > 0.5)
	{
		return ivec4(1, 2, 3, 4);
	}
	else if (abs(n.abcd.z) > 0.5)
	{
		return ivec4(1, 0, 3, 4);
	}
	else if (abs(n.abcd.w) > 0.5)
	{
		return ivec4(2, 0, 2, 4);
	}
	else
	{
		return ivec4(2, 0, 3, 1);
	}

	return ivec4(0);
}

int minAxis5(vec5 v)
{
	float m = v.abcd.x;
	int axis = 0;

	if (v.abcd.y < m) { m = v.abcd.y; axis = 1; }
	if (v.abcd.z < m) { m = v.abcd.z; axis = 2; }
	if (v.abcd.w < m) { m = v.abcd.w; axis = 3; }
	if (v.e < m) { axis = 4; }

	return axis;
}

RayHit trace5D(in vec5 ro, in vec5 rd, float maxDist, out vec4 tileColor)
{
	RayHit result;
	result.hit = false;
	result.pos = ro;
	result.blockPos = ivec5_vec5(ro);
	result.normal = vec5(vec4(0.0), 0.0);
	result.texCoord = vec4(0.0);
	result.dist = maxDist;
	result.id = 0u;
	
	const ivec5 CHUNK_SIZE_I = ivec5(ivec4(32, 8, 8, 8), 8);
	const vec5 CHUNK_SIZE = vec5(vec4(32.0, 8.0, 8.0, 8.0), 8.0);
	const vec5 ONE = vec5(vec4(1.0), 1.0);
	const ivec5 ONE_I = ivec5(ivec4(1), 1);
	const vec5 ZERO = vec5(vec4(0.0), 0.0);
	const ivec5 ZERO_I = ivec5(ivec4(0), 0);
	
	ivec5 step = ivec5_vec5(sign5(rd));
	vec5 stepF = vec5_ivec5(step);
	vec5 invRd = div(ONE, max5(abs5(rd), vec5(vec4(EPS, EPS, EPS, EPS), EPS)));
	vec5 tDelta = invRd;
	
	// DDA into chunks
	vec5 tDeltaC = mul(invRd, CHUNK_SIZE);
	vec5 roChunk = div(ro, CHUNK_SIZE);
	ivec5 vC = ivec5_vec5(floor5(roChunk));
	vec5 tMaxC = mul(mix5(fract5(roChunk), sub(ONE, fract5(roChunk)), greaterThan5(stepF, ZERO)), tDeltaC);
	
	for (int i = 0; i < 5; ++i)
	{
		if (ind_get(step, i) == 0)
		{
			ind_set(tDelta, i, INF);
			ind_set(tDeltaC, i, INF);
			ind_set(tMaxC, i, INF);
		}
	}

	float t = 0.0;
	int axisC = -1;
	bool checkBackSide = false;

	int maxChunkSteps = 4 + chunksSize.x + chunksSize.y + chunksSize.z + chunksSize.w + 8;

	for (int chunkI = 0; chunkI < maxChunkSteps && t < maxDist; ++chunkI)
	{
		// early bounds check
		ivec4 rel4 = bcde(vC) - chunksMinBound;
		if (any(lessThan(rel4, ivec4(0))) || any(greaterThanEqual(rel4, chunksSize)) ||
			vC.abcd.x < 0 || vC.abcd.x >= 4)
		{
			axisC = minAxis5(tMaxC);
			t = ind_get(tMaxC, axisC);
			ind_set(tMaxC, axisC, ind_get(tMaxC, axisC) + ind_get(tDeltaC, axisC));
			ind_set(vC, axisC, ind_get(vC, axisC) + ind_get(step, axisC));
			continue;
		}
		
		uint chunkDataIdx = texelFetch(chunks, ivec3(rel4.x, rel4.y, rel4.z + rel4.w * chunksSize.z), 0)[vC.abcd.x];
		if (chunkDataIdx == 0u) // empty chunk
		{
			axisC = minAxis5(tMaxC);
			t = ind_get(tMaxC, axisC);
			ind_set(tMaxC, axisC, ind_get(tMaxC, axisC) + ind_get(tDeltaC, axisC));
			ind_set(vC, axisC, ind_get(vC, axisC) + ind_get(step, axisC));
			continue;
		}
		
		--chunkDataIdx;
		
		// DDA into blocks
		vec5 l_ro = sub(add(ro, mul(rd, t)), mul(vec5_ivec5(vC), CHUNK_SIZE));
		ivec5 v = ivec5_vec5(clamp5(floor5(l_ro), ZERO, sub(CHUNK_SIZE, 1.0)));
		vec5 vF = vec5_ivec5(v);
		vec5 tMax = mul(mix5(sub(l_ro, vF), sub(ONE, sub(l_ro, vF)), greaterThan5(stepF, ZERO)), tDelta);
		
		for (int i = 0; i < 5; ++i)
		{
			if (ind_get(step, i) == 0)
			{
				ind_set(tMax, i, INF);
			}
		}

		int axis5 = (axisC >= 0) ? axisC : minAxis5(tMax);
		float t5 = 0.0;
		checkBackSide = false;
		
		for (int blockI = 0; blockI < 128 && t + t5 < maxDist; ++blockI)
		{
			if (any5(lessThan5(v, ZERO_I)) || any5(greaterThanEqual5(v, CHUNK_SIZE_I))) break;
			
			uint blockId = getBlock(blockData[int(chunkDataIdx)], v);

			if (blockId != 0u)
			{
				result.dist = t + t5;
				result.pos = add(ro, mul(rd, result.dist));
				result.blockPos = add(mul(vC, CHUNK_SIZE_I), v);
				result.normal = vec5(vec4(0.0), 0.0);
				ind_set(result.normal, axis5, -ind_get(stepF, axis5));
				result.texCoord = computeTexCoord(result.pos, result.normal);
				
				result.id = blockId;
				
				int side = axis5 * 2 + (ind_get(result.normal, axis5) < 0.0 ? 0 : 1);
				result.side = side;

				float slice = floor(result.texCoord.w * 16.0);
				vec3 texUVW = vec3(
					result.texCoord.x,
					result.texCoord.y,
					(slice + result.texCoord.z) / 16.0
				);

				tileColor = texture(sampler3D(tiles[textures[int(blockId) * 10 + side]]), texUVW);
				
				if (tileColor.a > 0.001)
				{
					result.hit = true;
					return result;
				}

				if (!checkBackSide)
				{
					checkBackSide = true;
					axis5 = minAxis5(tMax);
					t5 = ind_get(tMax, axis5);
					continue;
				}
				checkBackSide = false;
			}
			
			axis5 = minAxis5(tMax);
			t5 = ind_get(tMax, axis5);
			ind_set(tMax, axis5, ind_get(tMax, axis5) + ind_get(tDelta, axis5));
			ind_set(v, axis5, ind_get(v, axis5) + ind_get(step, axis5));
		}
		
		axisC = minAxis5(tMaxC);
		t = ind_get(tMaxC, axisC);
		ind_set(tMaxC, axisC, ind_get(tMaxC, axisC) + ind_get(tDeltaC, axisC));
		ind_set(vC, axisC, ind_get(vC, axisC) + ind_get(step, axisC));
	}
	
	result.dist = t;
	result.pos = add(ro, mul(rd, result.dist));
	return result;
}

uint getBlockGlobal(ivec5 p)
{
	const vec5 CHUNK_SIZE = vec5(vec4(32.0, 8.0, 8.0, 8.0), 8.0);
	const ivec5 CHUNK_SIZE_I = ivec5(ivec4(32, 8, 8, 8), 8);

	ivec5 vC = ivec5_vec5(floor5(div(vec5_ivec5(p), CHUNK_SIZE)));

	ivec5 local = sub(p, mul(vC, CHUNK_SIZE_I));

	ivec4 rel4 = bcde(vC) - chunksMinBound;
	if (any(lessThan(rel4, ivec4(0))) || any(greaterThanEqual(rel4, chunksSize)))
	{
		return 0u;
	}

	if (vC.abcd.x < 0 || vC.abcd.x >= 4)
	{
		return 0u;
	}

	uint chunkDataIdx = texelFetch(
		chunks,
		ivec3(rel4.x, rel4.y, rel4.z + rel4.w * chunksSize.z),
		0
	)[vC.abcd.x];

	if (chunkDataIdx == 0u)
	{
		return 0u;
	}

	return getBlock(blockData[int(chunkDataIdx - 1u)], local);
}

bool isSolid(ivec5 p)
{
	return getBlockGlobal(p) != 0u;
}

float computeAO(ivec5 blockPos, in vec5 normal, in vec4 texCoord)
{
	int normalAxis = getNormalAxis(normal);
	ivec4 axes = getTangentAxes(normal);

	ivec5 air = blockPos;
	ind_set(air, normalAxis, ind_get(air, normalAxis) + (ind_get(normal, normalAxis) > 0.0 ? 1 : -1));

	float occ = 0.0;

	for (int i = 0; i < 4; ++i)
	{
		int axis = axes[i];

		ivec5 n0 = air;
		ivec5 n1 = air;
		
		int iOff = (i == 1 ? -1 : 1);

		ind_set(n0, axis, ind_get(n0, axis) - iOff);
		ind_set(n1, axis, ind_get(n1, axis) + iOff);

		float o0 = isSolid(n0) ? 1.0 : 0.0;
		float o1 = isSolid(n1) ? 1.0 : 0.0;

		occ += mix(o0, o1, texCoord[i]);
	}

	float diagOcc = 0.0;
	for (int i = 0; i < 4; ++i)
	for (int j = i + 1; j < 4; ++j)
	{
		int ai = axes[i];
		int aj = axes[j];

		float ui = texCoord[i];
		float uj = texCoord[j];

		ivec5 p00 = air;
		ivec5 p10 = air;
		ivec5 p01 = air;
		ivec5 p11 = air;

		int iOff = (i == 1 ? -1 : 1);
		int jOff = (j == 1 ? -1 : 1);

		ind_set(p00, ai, ind_get(p00, ai) - iOff);
		ind_set(p00, aj, ind_get(p00, aj) - jOff);

		ind_set(p10, ai, ind_get(p10, ai) + iOff);
		ind_set(p10, aj, ind_get(p10, aj) - jOff);

		ind_set(p01, ai, ind_get(p01, ai) - iOff);
		ind_set(p01, aj, ind_get(p01, aj) + jOff);

		ind_set(p11, ai, ind_get(p11, ai) + iOff);
		ind_set(p11, aj, ind_get(p11, aj) + jOff);

		float o00 = isSolid(p00) ? 1.0 : 0.0;
		float o10 = isSolid(p10) ? 1.0 : 0.0;
		float o01 = isSolid(p01) ? 1.0 : 0.0;
		float o11 = isSolid(p11) ? 1.0 : 0.0;

		diagOcc +=
			o00 * (1.0 - ui) * (1.0 - uj) +
			o10 * ui         * (1.0 - uj) +
			o01 * (1.0 - ui) * uj +
			o11 * ui         * uj;
	}

	float ao = 1.0 - occ * 0.5 - diagOcc * 0.5;
	return clamp(ao, 0.0, 1.0);
}

uniform bool shadows;
uniform bool ambientOcclusion;
uniform bool water;

float waterWave(in vec5 p, float timeOffset, float waveScaleH, float waveHeight)
{
	float aOffset =
		sin((p.abcd.y + timeOffset) * waveScaleH) *
		cos((p.abcd.z + timeOffset) * waveScaleH) *
		sin((p.abcd.w + timeOffset) * waveScaleH) *
		cos((p.e + timeOffset) * waveScaleH);
	aOffset = (aOffset * 0.5) - 0.5;
	aOffset *= waveHeight;

	return aOffset;
}

const float slopeUB = 1.0;
const float g = sin(atan(1.0, slopeUB));
void main()
{
	float aspect = screenSize.x * screenSize.w;
	vec5 ro = cam.pos;
	vec2 ndc = uv * 2.0 - 1.0;
	float tanHalfFov = tan(cam.vFov * 0.5);
	float px = ndc.x * tanHalfFov * aspect;
	float py = -ndc.y * tanHalfFov;
	vec5 rd = normalize5(
		add(
			add(cam.forward, mul(cam.left, -px)),
			mul(cam.up, -py)
		)
	);
	
	color = vec4(vec3(0.0), 1.0);
	
	const float fogStart = 48.0;
	const float fogEnd = 64.0;
	const vec3 fogColor = vec3(0.0);

	vec4 tileColor = vec4(0.0);
	RayHit hit = trace5D(ro, rd, fogEnd, tileColor);
	if (hit.hit)
	{
		float lighting = max(dot5(hit.normal, lightDir), 0.0);
		if (shadows)
		{
			vec4 shadowTile = vec4(0.0);
			RayHit hitShadow = trace5D(add(hit.pos, mul(hit.normal, 0.001)), lightDir, 32.0, shadowTile);
			if (hitShadow.hit)
			{
				lighting *= 0.4;
			}
		}
		if (ambientOcclusion)
		{
			lighting *= computeAO(hit.blockPos, hit.normal, hit.texCoord) * 0.5 + 0.5;
		}
		color = vec4(tileColor.rgb * (lighting * 0.8 + 0.2), 1.0);
		if (target.targeting && all5(equal5(target.blockPos, hit.blockPos)))
		{
			color.rgb += vec3(0.1 + sin(time * 2.4) * 0.05);
			if (target.side == hit.side)
			{
				color.rgb += vec3(0.12);
			}
		}
	}
	
	{
		float fog = ((clamp(hit.dist, fogStart, fogEnd) - fogStart) / (fogEnd - fogStart));
		color.rgb = mix(color.rgb, color.rgb * fogColor, clamp(fog, 0.0, 1.0));
	}

	if (!water)
	{
		return;
	}

	// water at maximum wave height
	const float waterHeight = 30.75;
	const float waveHeight = 0.2;
	const float waveScaleH = 7.0;
	const float timeOffset = time * 0.25;

	float fA = waterHeight + waterWave(ro, timeOffset, waveScaleH, waveHeight);
	
	if (fA - ro.abcd.x > 0.0)
	{
		float waterFog = hit.pos.abcd.x < fA ? ((clamp(hit.dist, 0.0, 16.0) - 0.0) / (16.0 - 0.0)) : 0.0;
		vec3 waterFogColor = normalize(vec3(0.07, 0.2, 0.39));
		color.rgb *= waterFogColor;
		color.rgb = mix(color.rgb, waterFogColor * 0.3, clamp(waterFog + 0.2, 0.0, 1.0));
		return;
	}

	vec5 intersection = add(ro, mul(rd, 0.001));
	const int stepCount = 50;
	bool hitWater = false;
	float waterDist = 0.001;
	for (int i = 0; i < stepCount && waterDist < fogEnd; ++i)
	{
		float f = (intersection.abcd.x - (waterHeight + waterWave(intersection, timeOffset, waveScaleH, waveHeight))) * g;

		if (f < 0.001)
		{
			hitWater = true;
			break;
		}

		float step = f / -rd.abcd.x;
		waterDist += step;
		intersection = add(intersection, mul(rd, step));
	}

	if (!hitWater)
	{
		return;
	}

	if (intersection.abcd.x < hit.pos.abcd.x)
	{
		return;
	}

	// make the water texture move
	intersection.abcd.y += timeOffset;

	vec4 waterTexCoord = computeTexCoord(intersection, vec5(vec4(1, 0, 0, 0), 0));

	float slice = floor(waterTexCoord.w * 16.0);
	vec3 texUVW = vec3(
		waterTexCoord.x,
		waterTexCoord.y,
		(slice + waterTexCoord.z) / 16.0
	);

	vec3 waterColor = texture(sampler3D(tiles[8]), texUVW).rgb * 0.7;
	float waterAlpha = 0.3;
	{
		float fog = ((clamp(waterDist, fogStart, fogEnd) - fogStart) / (fogEnd - fogStart));
		waterColor = mix(waterColor, fogColor, clamp(fog, 0.0, 1.0));
	}
	if (hit.hit)
	{
		float waterFog = ((clamp(hit.dist - waterDist, 0.0, 12.0) - 0.0) / (12.0 - 0.0));
		vec3 waterFogColor = normalize(vec3(0.07, 0.2, 0.39));
		color.rgb = mix(color.rgb, color.rgb * waterFogColor, clamp(waterFog + 0.6, 0.0, 1.0));
	}
	color.rgb = mix(color.rgb, waterColor, waterAlpha);
}