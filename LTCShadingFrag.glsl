#version 460 core

out vec4 FragColor;
in vec2 TexCoords; // Maybe this is not used 
in vec3 WorldPos;
in vec3 Normal;

uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

uniform sampler2D LTC_texture1; // LTC_texture1 : LTC Inv_M
uniform sampler2D LTC_texture2; // LTC_texture2 : LTC Frensel Approximation

uniform vec3 camPos;

const float PI = 3.14159265359;

const float LUT_SIZE = 64;
const float LUT_SCALE = (LUT_SIZE - 1) / LUT_SIZE;
const float LUT_BIAS = 0.5 / LUT_SIZE;

struct RectLight{
	vec3 center;
	vec3 dir_width;
	vec3 dir_height;
	float half_width;
	float half_height;
	vec3 color;
};

uniform RectLight rect;

void getRectVertices(RectLight Rect, out vec3 v1, out vec3 v2, out vec3 v3, out vec3 v4)
{
	// Anticlockwise
	v1 = Rect.center + normalize(Rect.dir_width) * Rect.half_width + normalize(Rect.dir_height) * Rect.half_height;
	v2 = Rect.center - normalize(Rect.dir_width) * Rect.half_width + normalize(Rect.dir_height) * Rect.half_height;
	v3 = Rect.center - normalize(Rect.dir_width) * Rect.half_width - normalize(Rect.dir_height) * Rect.half_height;
	v4 = Rect.center + normalize(Rect.dir_width) * Rect.half_width - normalize(Rect.dir_height) * Rect.half_height;
}

float EdgeIntegral(vec3 v1, vec3 v2)
{
	float x = dot(v1, v2);
	float y = abs(x);

	float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
	float b = 3.4175940 + (4.1616724 + y) * y;
	float v = a / b;
	
	float theta_sintheta = (x > 0.0) ? v : 0.5 * inversesqrt(max(1.0 - x * x, 1e-7)) - v;

	return (cross(v1, v2) * theta_sintheta * 0.5 / PI).z;
}

// Clipping
void LTC_clipping(inout vec3 L[5], out int n)
{
	int config = 0;
	if (L[0].z > 0.0) {
		config += 1;
	}

	if (L[1].z > 0.0) {
		config += 2;
	}

	if (L[2].z > 0.0) {
		config += 4;
	}

	if (L[3].z > 0.0) {
		config += 8;
	}

	n = 0;

	if (config == 0) {
		return;
	}
	else if (config == 1) {
		n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
	}
	else if (config == 2) {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
	}
	else if (config == 3) {
		n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
	}
	else if (config == 4) {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
	}
	else if (config == 5) {
		return;  // This case may be impossible
	}
	else if (config == 6) {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
	}
	else if (config == 7) {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
	}
	else if (config == 8) {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] =  L[3];
	}
	else if (config == 9) {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
	}
	else if (config == 10) {
		return; // This case may be also impossible
	}
	else if (config == 11) {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
	}
	else if (config == 12) {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
	}
	else if (config == 13) {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
	}
	else if (config == 14) {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
	}
	else if (config == 15) {
        n = 4;
	}

    if (n == 3) {
        L[3] = L[0];
	}
    if (n == 4) {
        L[4] = L[0];
	}
}

float LTC_Evaluate(vec3 N, vec3 V, vec3 pos, mat3 inv_M, RectLight rect)
{
	float sum = 0.0;

	vec3 p0, p1, p2, p3;
	getRectVertices(rect, p0, p1, p2, p3);

	// Change of bases
	vec3 T1, T2;
	T1 = normalize(V - N * dot(V, N));
	T2 = cross(N, T1);
	inv_M = inv_M * transpose(mat3(T1, T2, N));

	vec3 L[5];
	L[0] = inv_M * (p0 - pos);
	L[1] = inv_M * (p1 - pos);
	L[2] = inv_M * (p2 - pos);
	L[3] = inv_M * (p3 - pos);
	L[4] = L[0];

	int n = 4;

	LTC_clipping(L, n);

	if (n == 0) {
		return 0.0;
	}

	L[0] = normalize(L[0]);
	L[1] = normalize(L[1]);
	L[2] = normalize(L[2]);
	L[3] = normalize(L[3]);
	L[4] = normalize(L[4]);

	sum += EdgeIntegral(L[0], L[1]);
	sum += EdgeIntegral(L[1], L[2]);
	sum += EdgeIntegral(L[2], L[3]);
	if (n >= 4) {
		sum += EdgeIntegral(L[3], L[4]);
	}
	if (n == 5) {
		sum += EdgeIntegral(L[4], L[0]);
	}

	sum = abs(sum);

	return sum;
}

void main()
{
	vec3 N = normalize(Normal);
	vec3 V = normalize(camPos - WorldPos);

	float ndotv = clamp(dot(N, V), 0.0, 1.0);
	vec2 uv = vec2(roughness, acos(ndotv) / (0.5 * PI));
	uv = uv * LUT_SCALE + LUT_BIAS;

	vec4 t1 = texture(LTC_texture1, uv);
	vec4 t2 = texture(LTC_texture2, uv);

	mat3 inv_M = mat3(
		vec3(t1.x, 0, t1.y),
		vec3(   0, 1,    0),
		vec3(t1.z, 0, t1.w)
	);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	float spec = LTC_Evaluate(N, V, WorldPos, inv_M, rect);
	vec3 F = F0 * t2.x + (1 - F0) * t2.y; // LTC Fresnel
	vec3 specular = spec * F;

	vec3 diffuse = (1 - F) * (1 - metallic) * albedo / PI * LTC_Evaluate(N, V, WorldPos, mat3(1.0), rect);

	vec3 ambient = vec3(0.03) * albedo * ao;

	vec3 color = rect.color * (specular + diffuse) + ambient;

	// Tone mapping
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));
	FragColor = vec4(color, 1.0);
}

