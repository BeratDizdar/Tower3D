#include "tw3d.h"
#include <Windows.h>
#include <gl/GL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#include "stb_image.h"

#define TW3D_API __declspec(dllexport)

typedef struct {
	unsigned int vao;
	unsigned int vbo;
	unsigned int ibo;
	int icount;
} TwBackendBuffer;

typedef struct TwPool {
	unsigned int data[64];
	int cap;
	int count;
} TwPool;

typedef struct TwInstance {
	struct {
		HGLRC context;
		HDC device;
		HWND handler;

		LARGE_INTEGER timer_freq;
		LARGE_INTEGER last_time;

		int exit_key;
		TwBool is_active;
	} win32;
	struct {
		int (*wglSwapIntervalEXT)(int);

		void (*glGenVertexArrays)(int n, unsigned int* arrays);
		void (*glBindVertexArray)(unsigned int array);
		void (*glDeleteVertexArrays)(int n, const unsigned int* arrays);

		void (*glGenBuffers)(int n, unsigned int* buffers);
		void (*glBindBuffer)(unsigned int target, unsigned int buffer);
		void (*glBufferData)(unsigned int target, intptr_t size, const void* data, unsigned int usage);
		void (*glDeleteBuffers)(int n, const unsigned int* buffers);
		void (*glEnableVertexAttribArray)(unsigned int index);
		void (*glVertexAttribPointer)(unsigned int index, int size, unsigned int type, unsigned char normalized, int stride, const void* pointer);

		unsigned int (*glCreateShader)(unsigned int type);
		void (*glShaderSource)(unsigned int shader, int count, const char** string, const int* length);
		void (*glCompileShader)(unsigned int shader);
		void (*glGetShaderiv)(unsigned int shader, unsigned int pname, int* params);
		void (*glGetShaderInfoLog)(unsigned int shader, int bufSize, int* length, char* infoLog);
		unsigned int (*glCreateProgram)(void);
		void (*glAttachShader)(unsigned int program, unsigned int shader);
		void (*glLinkProgram)(unsigned int program);
		void (*glGetProgramiv)(unsigned int program, unsigned int pname, int* params);
		void (*glGetProgramInfoLog)(unsigned int program, int bufSize, int* length, char* infoLog);
		void (*glUseProgram)(unsigned int program);
		void (*glDeleteShader)(unsigned int shader);
		void (*glDeleteProgram)(unsigned int program);

		int  (*glGetUniformLocation)(unsigned int program, const char* name);
		void (*glUniform1i)(int location, int v0);
		void (*glUniform1f)(int location, float v0);
		void (*glUniform3f)(int location, float v0, float v1, float v2);
		void (*glUniformMatrix4fv)(int location, int count, unsigned char transpose, const float* value);

		void (*glActiveTexture)(unsigned int texture);
	} vtbl;
	struct {
		TwPool rule;
		TwPool texture;
		TwPool buffer;
		TwPool shader;
	} MainPool;
	struct {
		TwPool texture;
		TwPool buffer;
	} TempPool;
	TwBackendBuffer g_buffers[128];
	int g_bufferCount;
	int g_active_icount;
} TwInstance;

static char* tw_readFile(const char* filepath) {
	FILE* f = fopen(filepath, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* buffer = (char*)malloc(len + 1);
	fread(buffer, 1, len, f);
	buffer[len] = '\0';
	fclose(f);
	return buffer;
}

/* ################################################## */

TW3D_API Mat4 twMatIdentity() {
	return (Mat4) { .m[0] = 1, .m[5] = 1, .m[10] = 1, .m[15] = 1 };
}
TW3D_API Mat4 twMatZero() {
	return (Mat4) { 0 };
}
TW3D_API Mat4 twMatTranslate(Vec3 v) {
	Mat4 res = twMatIdentity();
	res.m[12] = v.x; res.m[13] = v.y; res.m[14] = v.z;
	return res;
}
TW3D_API Mat4 twMatRotateX(float angle) {
	Mat4 res = twMatIdentity();
	res.m[5] = cos(angle); res.m[6] = -sin(angle);
	res.m[9] = sin(angle); res.m[10] = cos(angle);
	return res;
}
TW3D_API Mat4 twMatRotateY(float angle) {
	Mat4 res = twMatIdentity();
	res.m[0] = cos(angle); res.m[2] = sin(angle);
	res.m[8] = -sin(angle); res.m[10] = cos(angle);
	return res;
}
TW3D_API Mat4 twMatRotateZ(float angle) {
	Mat4 res = twMatIdentity();
	res.m[0] = cos(angle); res.m[1] = sin(angle);
	res.m[4] = -sin(angle); res.m[5] = cos(angle);
	return res;
}
TW3D_API Mat4 twMatScale(float k) {
	Mat4 res = twMatIdentity();
	res.m[0] = k; res.m[5] = k; res.m[10] = k;
	return res;
}
TW3D_API Mat4 twMatTransform(Mat4 a, Vec3 pos, float angle, Vec3 axis, Vec3 scale) {
	float c = (float)cos(angle);
	float s = (float)sin(angle);
	float t = 1.0f - c;
	axis = twVec3Normalize(axis);
	float x = axis.x, y = axis.y, z = axis.z;
	Mat4 trs = twMatIdentity();
	trs.m[0] = (t * x * x + c) * scale.x;
	trs.m[1] = (t * x * y + s * z) * scale.x;
	trs.m[2] = (t * x * z - s * y) * scale.x;
	trs.m[4] = (t * x * y - s * z) * scale.y;
	trs.m[5] = (t * y * y + c) * scale.y;
	trs.m[6] = (t * y * z + s * x) * scale.y;
	trs.m[8] = (t * x * z + s * y) * scale.z;
	trs.m[9] = (t * y * z - s * x) * scale.z;
	trs.m[10] = (t * z * z + c) * scale.z;
	trs.m[12] = pos.x;
	trs.m[13] = pos.y;
	trs.m[14] = pos.z;
	return twMatMultiply(a, trs);
}
TW3D_API Mat4 twMatAdd(Mat4 a, Mat4 b) {
	return (Mat4) {
		a.m[0] + b.m[0], a.m[1] + b.m[1], a.m[2] + b.m[2], a.m[3] + b.m[3],
		a.m[4] + b.m[4], a.m[5] + b.m[5], a.m[6] + b.m[6], a.m[7] + b.m[7],
		a.m[8] + b.m[8], a.m[9] + b.m[9], a.m[10] + b.m[10], a.m[11] + b.m[11],
		a.m[12] + b.m[12], a.m[13] + b.m[13], a.m[14] + b.m[14], a.m[15] + b.m[15],
	};
}
TW3D_API Mat4 twMatSub(Mat4 a, Mat4 b) {
	return (Mat4) {
		a.m[0] - b.m[0], a.m[1] - b.m[1], a.m[2] - b.m[2], a.m[3] - b.m[3],
		a.m[4] - b.m[4], a.m[5] - b.m[5], a.m[6] - b.m[6], a.m[7] - b.m[7],
		a.m[8] - b.m[8], a.m[9] - b.m[9], a.m[10] - b.m[10], a.m[11] - b.m[11],
		a.m[12] - b.m[12], a.m[13] - b.m[13], a.m[14] - b.m[14], a.m[15] - b.m[15],
	};
}
TW3D_API Mat4 twMatMultiply(Mat4 a, Mat4 b) {
	return (Mat4) {
		a.m[0] * b.m[0] + a.m[4] * b.m[1] + a.m[8] * b.m[2] + a.m[12] * b.m[3],
		a.m[1] * b.m[0] + a.m[5] * b.m[1] + a.m[9] * b.m[2] + a.m[13] * b.m[3],
		a.m[2] * b.m[0] + a.m[6] * b.m[1] + a.m[10] * b.m[2] + a.m[14] * b.m[3],
		a.m[3] * b.m[0] + a.m[7] * b.m[1] + a.m[11] * b.m[2] + a.m[15] * b.m[3],

		a.m[0] * b.m[4] + a.m[4] * b.m[5] + a.m[8] * b.m[6] + a.m[12] * b.m[7],
		a.m[1] * b.m[4] + a.m[5] * b.m[5] + a.m[9] * b.m[6] + a.m[13] * b.m[7],
		a.m[2] * b.m[4] + a.m[6] * b.m[5] + a.m[10] * b.m[6] + a.m[14] * b.m[7],
		a.m[3] * b.m[4] + a.m[7] * b.m[5] + a.m[11] * b.m[6] + a.m[15] * b.m[7],

		a.m[0] * b.m[8] + a.m[4] * b.m[9] + a.m[8] * b.m[10] + a.m[12] * b.m[11],
		a.m[1] * b.m[8] + a.m[5] * b.m[9] + a.m[9] * b.m[10] + a.m[13] * b.m[11],
		a.m[2] * b.m[8] + a.m[6] * b.m[9] + a.m[10] * b.m[10] + a.m[14] * b.m[11],
		a.m[3] * b.m[8] + a.m[7] * b.m[9] + a.m[11] * b.m[10] + a.m[15] * b.m[11],

		a.m[0] * b.m[12] + a.m[4] * b.m[13] + a.m[8] * b.m[14] + a.m[12] * b.m[15],
		a.m[1] * b.m[12] + a.m[5] * b.m[13] + a.m[9] * b.m[14] + a.m[13] * b.m[15],
		a.m[2] * b.m[12] + a.m[6] * b.m[13] + a.m[10] * b.m[14] + a.m[14] * b.m[15],
		a.m[3] * b.m[12] + a.m[7] * b.m[13] + a.m[11] * b.m[14] + a.m[15] * b.m[15]
	};
}
TW3D_API Mat4 twMatPerspective(float fov, float aspect, float znear, float zfar) {
	return (Mat4) {
		.m[0] = tan(2 / (fov * (3.14159f / 180.0f))) / aspect,
		.m[5] = tan(2 / (fov * (3.14159f / 180.0f))),
		.m[10] = (zfar + znear) / (znear - zfar),
		.m[11] = -1,
		.m[14] = (2.0f * zfar * znear) / (znear - zfar),
	};
}
TW3D_API Mat4 twMatLookAt(Vec3 eye, Vec3 center, Vec3 up) {
	Vec3 f = twVec3Normalize(twVec3Sub(center, eye));
	Vec3 s = twVec3Normalize(twVec3Cross(f, up));
	Vec3 u = twVec3Cross(s, f);
	return (Mat4) {
		s.x, u.x, -f.x, 0.0f,
		s.y, u.y, -f.y, 0.0f,
		s.z, u.z, -f.z, 0.0f,
		-twVec3Dot(s, eye), -twVec3Dot(u, eye), twVec3Dot(f, eye), 1.0f,
	};
}
TW3D_API Mat4 twMatOrtho(float left, float right, float bottom, float top, float znear, float zfar) {
	return (Mat4) {
		.m[0] = 2 / (right - left),
		.m[5] = 2 / (top - bottom),
		.m[10] = -2 / (zfar - znear),
		.m[12] = -(right + left) / (right - left),
		.m[13] = -(top + bottom) / (top - bottom),
		.m[14] = -(zfar + znear) / (zfar - znear),
		.m[15] = 1,
	};
}
TW3D_API Mat4 twMatRound(Mat4 a) {
	return (Mat4) {
		(float)round(a.m[0]), (float)round(a.m[1]), (float)round(a.m[2]), (float)round(a.m[3]),
		(float)round(a.m[4]), (float)round(a.m[5]), (float)round(a.m[6]), (float)round(a.m[7]),
		(float)round(a.m[8]), (float)round(a.m[9]), (float)round(a.m[10]), (float)round(a.m[11]),
		(float)round(a.m[12]), (float)round(a.m[13]), (float)round(a.m[14]), (float)round(a.m[15]),
	};
}
TW3D_API Vec3 twVec3Add(Vec3 a, Vec3 b) {
	return (Vec3) {
		a.x + b.x, a.y + b.y, a.z + b.z,
	};
}
TW3D_API Vec3 twVec3Sub(Vec3 a, Vec3 b) {
	return (Vec3) {
		a.x - b.x, a.y - b.y, a.z - b.z,
	};
}
TW3D_API Vec3 twVec3Normalize(Vec3 a) {
	float mag = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
	if (mag == 0.0f) return (Vec3) { 0.0f, 0.0f, 0.0f };
	return (Vec3) { a.x / mag, a.y / mag, a.z / mag };
}
TW3D_API Vec3 twVec3Cross(Vec3 a, Vec3 b) {
	return (Vec3) {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}
TW3D_API float twVec3Dot(Vec3 a, Vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
TW3D_API float twVec3DistanceCheby(Vec3 a, Vec3 b) {
	float dx = (float)fabs(a.x - b.x);
	float dy = (float)fabs(a.y - b.y);
	float dz = (float)fabs(a.z - b.z);

	float max_xy = dx > dy ? dx : dy;
	return max_xy > dz ? max_xy : dz;
}
TW3D_API float twVec3DistanceMinkow(Vec3 a, Vec3 b, float p) {
	float dx = (float)fabs(a.x - b.x);
	float dy = (float)fabs(a.y - b.y);
	float dz = (float)fabs(a.z - b.z);
	return (float)pow(pow(dx, p) + pow(dy, p) + pow(dz, p), 1.0f / p);
}
TW3D_API float twMatAt(Mat4 a, unsigned int r, unsigned int c) {
	return a.m[c * 4 + r];
}
TW3D_API float twLerp(float v0, float v1, float t) {
	return v0 + t * (v1 - v0);
}
TW3D_API float twSquareRoot(float n) { return (float)sqrt(n); }
TW3D_API float twSin(float a) { return (float)sin(a); }
TW3D_API float twCos(float a) { return (float)cos(a); }
TW3D_API float twRandom(float min, float max) { return min + ((float)rand() / (float)RAND_MAX) * (max - min); }

/* ################################################## */

TW3D_API TwInstance* twCreateInstance() {
	TwInstance* inst = (TwInstance*)calloc(1, sizeof(TwInstance));
	inst->MainPool.rule.cap = 64;
	inst->MainPool.texture.cap = 64;
	inst->MainPool.buffer.cap = 64;
	inst->MainPool.shader.cap = 64;

	inst->TempPool.texture.cap = 64;
	inst->TempPool.buffer.cap = 64;
	return inst;
}
TW3D_API void twClearTempPools(TwInstance* inst) {
	if (inst->TempPool.texture.count > 0) {
		glDeleteTextures(inst->TempPool.texture.count, inst->TempPool.texture.data);
		inst->TempPool.texture.count = 0;
	}

	if (inst->TempPool.buffer.count > 0 && inst->vtbl.glDeleteBuffers) {
		for (int i = 0; i < inst->TempPool.buffer.count; i++) {
			int buf_id = inst->TempPool.buffer.data[i];
			TwBackendBuffer* b = &inst->g_buffers[buf_id];

			inst->vtbl.glDeleteVertexArrays(1, &b->vao);
			inst->vtbl.glDeleteBuffers(1, &b->vbo);
			inst->vtbl.glDeleteBuffers(1, &b->ibo);
		}
		inst->TempPool.buffer.count = 0;
	}
}
TW3D_API void twDeleteInstance(TwInstance* inst) {
	if (!inst) return;
	twClearTempPools(inst);

	if (inst->MainPool.texture.count > 0)
		glDeleteTextures(inst->MainPool.texture.count, inst->MainPool.texture.data);
	if (inst->MainPool.buffer.count > 0 && inst->vtbl.glDeleteBuffers) {
		for (int i = 0; i < inst->MainPool.buffer.count; i++) {
			int buf_id = inst->MainPool.buffer.data[i];
			TwBackendBuffer* b = &inst->g_buffers[buf_id];

			inst->vtbl.glDeleteVertexArrays(1, &b->vao);
			inst->vtbl.glDeleteBuffers(1, &b->vbo);
			inst->vtbl.glDeleteBuffers(1, &b->ibo);
		}
	}

	if (inst->MainPool.shader.count > 0 && inst->vtbl.glDeleteProgram) {
		for (int i = 0; i < inst->MainPool.shader.count; i++) {
			inst->vtbl.glDeleteProgram(inst->MainPool.shader.data[i]);
		}
	}

	if (inst->win32.context) {
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(inst->win32.context);
	}
	if (inst->win32.handler && inst->win32.device) {
		ReleaseDC(inst->win32.handler, inst->win32.device);
		DestroyWindow(inst->win32.handler);
	}
	free(inst);
}

/* ################################################## */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
	switch (umessage) {
	case WM_DESTROY: PostQuitMessage(0); return 0;
	case WM_CLOSE: DestroyWindow(hwnd); return 0;
	default: return DefWindowProc(hwnd, umessage, wparam, lparam);
	}
}
TW3D_API TwBool twCreateSurfaceInMain(TwInstance* inst, const char* name) {
	HINSTANCE module = GetModuleHandleA(NULL);
	WNDCLASSA wnd = { 0 };
	wnd.hInstance = module;
	wnd.lpfnWndProc = WndProc;
	wnd.lpszClassName = "wndclass1";
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.style = CS_OWNDC;
	RegisterClassA(&wnd);
	
	HMODULE user32 = LoadLibraryA("user32.dll");
	if (user32 != NULL) {
		FARPROC dpi_func = GetProcAddress(user32, "SetProcessDPIAware");
		if (dpi_func != NULL) {
			((BOOL(WINAPI*)(void))dpi_func)();
		}
		FreeLibrary(user32);
	}
	inst->win32.handler = CreateWindowExA(
		WS_EX_APPWINDOW, wnd.lpszClassName, name, WS_POPUP | WS_VISIBLE, // WS_POPUP - WS_POPUPWINDOW
		0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		NULL, NULL, module, NULL);

	QueryPerformanceFrequency(&inst->win32.timer_freq);
	QueryPerformanceCounter(&inst->win32.last_time);

	inst->win32.is_active = TW_TRUE;
	inst->win32.exit_key = 27;
	return TW_TRUE;
}
TW3D_API void twSetSwapIntervals(TwInstance* inst, TwBool bVsync) {
	// Sürücü bu özelliği destekliyor mu diye Mimar kontrolü!
	if (inst->vtbl.wglSwapIntervalEXT != NULL) {
		inst->vtbl.wglSwapIntervalEXT(bVsync);
	}
	else {
		printf("UYARI: Ekran kartin V-Sync (wglSwapIntervalEXT) desteklemiyor.\n");
	}
}
TW3D_API TwBool twUpdateSurface(TwInstance* inst) {
	return inst->win32.is_active;
}
TW3D_API float twGetDeltaTime(TwInstance* inst) {
	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	float delta = (float)(current_time.QuadPart - inst->win32.last_time.QuadPart) / (float)inst->win32.timer_freq.QuadPart;
	inst->win32.last_time = current_time;
	if (delta > 0.1f) delta = 0.1f;
	return delta;
}
TW3D_API void twSwapBuffers(TwInstance* inst) {
	SwapBuffers(inst->win32.device);
	MSG msg = { 0 };
	while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) inst->win32.is_active = TW_FALSE;
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	if (GetAsyncKeyState(inst->win32.exit_key)) inst->win32.is_active = TW_FALSE;
}

/* ################################################## */

TW3D_API TwBool twKeyPressed(TwInstance* inst, int key);
TW3D_API TwBool twKeyReleased(TwInstance* inst, int key);
TW3D_API TwBool twKeyDown(TwInstance* inst, int key);
TW3D_API int twGetMousePos(TwInstance* inst);
TW3D_API TwBool twMouseLeft(TwInstance* inst);
TW3D_API TwBool twMouseRight(TwInstance* inst);

/* ################################################## */

TW3D_API void twBindContextToSurface(TwInstance* inst) {
	inst->win32.device = GetDC(inst->win32.handler);
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	
	int pf = ChoosePixelFormat(inst->win32.device, &pfd);
	SetPixelFormat(inst->win32.device, pf, &pfd);
	inst->win32.context = wglCreateContext(inst->win32.device);
	wglMakeCurrent(inst->win32.device, inst->win32.context);
	
	inst->vtbl.wglSwapIntervalEXT = (int(*)(int))wglGetProcAddress("wglSwapIntervalEXT");

	inst->vtbl.glGenVertexArrays = (void(*)(int, unsigned int*))wglGetProcAddress("glGenVertexArrays");
	inst->vtbl.glBindVertexArray = (void(*)(unsigned int))wglGetProcAddress("glBindVertexArray");
	inst->vtbl.glDeleteVertexArrays = (void(*)(int, const unsigned int*))wglGetProcAddress("glDeleteVertexArrays");

	inst->vtbl.glGenBuffers = (void(*)(int, unsigned int*))wglGetProcAddress("glGenBuffers");
	inst->vtbl.glBindBuffer = (void(*)(unsigned int, unsigned int))wglGetProcAddress("glBindBuffer");
	inst->vtbl.glBufferData = (void(*)(unsigned int, intptr_t, const void*, unsigned int))wglGetProcAddress("glBufferData");
	inst->vtbl.glDeleteBuffers = (void(*)(int, const unsigned int*))wglGetProcAddress("glDeleteBuffers");
	inst->vtbl.glEnableVertexAttribArray = (void(*)(unsigned int))wglGetProcAddress("glEnableVertexAttribArray");
	inst->vtbl.glVertexAttribPointer = (void(*)(unsigned int, int, unsigned int, unsigned char, int, const void*))wglGetProcAddress("glVertexAttribPointer");

	inst->vtbl.glCreateShader = (unsigned int(*)(unsigned int))wglGetProcAddress("glCreateShader");
	inst->vtbl.glShaderSource = (void(*)(unsigned int, int, const char**, const int*))wglGetProcAddress("glShaderSource");
	inst->vtbl.glCompileShader = (void(*)(unsigned int))wglGetProcAddress("glCompileShader");
	inst->vtbl.glGetShaderiv = (void(*)(unsigned int, unsigned int, int*))wglGetProcAddress("glGetShaderiv");
	inst->vtbl.glGetShaderInfoLog = (void(*)(unsigned int, int, int*, char*))wglGetProcAddress("glGetShaderInfoLog");

	inst->vtbl.glCreateProgram = (unsigned int(*)(void))wglGetProcAddress("glCreateProgram");
	inst->vtbl.glAttachShader = (void(*)(unsigned int, unsigned int))wglGetProcAddress("glAttachShader");
	inst->vtbl.glLinkProgram = (void(*)(unsigned int))wglGetProcAddress("glLinkProgram");
	inst->vtbl.glGetProgramiv = (void(*)(unsigned int, unsigned int, int*))wglGetProcAddress("glGetProgramiv");
	inst->vtbl.glGetProgramInfoLog = (void(*)(unsigned int, int, int*, char*))wglGetProcAddress("glGetProgramInfoLog");
	inst->vtbl.glUseProgram = (void(*)(unsigned int))wglGetProcAddress("glUseProgram");
	inst->vtbl.glDeleteShader = (void(*)(unsigned int))wglGetProcAddress("glDeleteShader");
	inst->vtbl.glDeleteProgram = (void(*)(unsigned int))wglGetProcAddress("glDeleteProgram");

	inst->vtbl.glGetUniformLocation = (int(*)(unsigned int, const char*))wglGetProcAddress("glGetUniformLocation");
	inst->vtbl.glUniform1i = (void(*)(int, int))wglGetProcAddress("glUniform1i");
	inst->vtbl.glUniform1f = (void(*)(int, float))wglGetProcAddress("glUniform1f");
	inst->vtbl.glUniform3f = (void(*)(int, float, float, float))wglGetProcAddress("glUniform3f");
	inst->vtbl.glUniformMatrix4fv = (void(*)(int, int, unsigned char, const float*))wglGetProcAddress("glUniformMatrix4fv");

	inst->vtbl.glActiveTexture = (void(*)(unsigned int))wglGetProcAddress("glActiveTexture");
}
TW3D_API void twSetContextViewport(TwInstance* inst, int x, int y, int w, int h) {
	glViewport(x, y, w, h);
}
TW3D_API void twClearColorAndDepth(TwInstance* inst, float r, float g, float b) {
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
TW3D_API void twDrawCommands(TwInstance* inst) {
	glDrawElements(0x0004, inst->g_active_icount, 0x1405, 0);
}
static unsigned int tw_packRule(TwBool depth, TwBool bfcull, TwBool alpha) {
	unsigned int rule = 0;
	if (alpha) rule |= 1;
	if (bfcull) rule |= 2;
	if (depth) rule |= 4;
	return rule;
}
TW3D_API void twBindRule(TwInstance* inst, TwID rule) {
	unsigned int rData = inst->MainPool.rule.data[rule];
	if (rData & 1) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
	}
	else {
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
	if (rData & 2) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
	else {
		glDisable(GL_CULL_FACE);
	}
	if (rData & 4) {
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}
}
TW3D_API TwID twCreateRuleInMainPool(TwInstance* inst, TwBool depth, TwBool bfcull, TwBool alpha) {
	TwPool* p = &inst->MainPool.rule;
	p->data[p->count] = tw_packRule(depth, bfcull, alpha);
	return p->count++;
}
static unsigned int tw_loadTexCore(const char* filepath) {
	int w, h, c;
	stbi_set_flip_vertically_on_load(1);
	unsigned char* px = stbi_load(filepath, &w, &h, &c, 4);
	unsigned int texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
	stbi_image_free(px);
	return texID;
}
TW3D_API TwID twLoadTextureToMainPool(TwInstance* inst, const char* filepath) {
	TwPool* p = &inst->MainPool.texture;
	p->data[p->count] = tw_loadTexCore(filepath);
	return p->data[p->count++];
}
TW3D_API TwID twLoadTextureToTempPool(TwInstance* inst, const char* filepath) {
	TwPool* p = &inst->TempPool.texture;
	p->data[p->count] = tw_loadTexCore(filepath);
	return p->data[p->count++];
}
TW3D_API void twBindTexture(TwInstance* inst, TwID texture) {
	if (inst->vtbl.glActiveTexture) inst->vtbl.glActiveTexture(0x84C0);
	glBindTexture(GL_TEXTURE_2D, texture);
}
static unsigned int tw_loadBufCore(TwInstance* inst, float* verts, int vcount, unsigned int* inds, int icount) {
	TwBackendBuffer b = { 0 };
	b.icount = icount;
	inst->vtbl.glGenVertexArrays(1, &b.vao);
	inst->vtbl.glBindVertexArray(b.vao);
	inst->vtbl.glGenBuffers(1, &b.vbo);
	inst->vtbl.glBindBuffer(0x8892, b.vbo);
	inst->vtbl.glBufferData(0x8892, vcount * 8 * sizeof(float), verts, 0x88E4);
	inst->vtbl.glGenBuffers(1, &b.ibo);
	inst->vtbl.glBindBuffer(0x8893, b.ibo);
	inst->vtbl.glBufferData(0x8893, icount * sizeof(unsigned int), inds, 0x88E4);
	int stride = 8 * sizeof(float);
	inst->vtbl.glEnableVertexAttribArray(0);
	inst->vtbl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	inst->vtbl.glEnableVertexAttribArray(1);
	inst->vtbl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	inst->vtbl.glEnableVertexAttribArray(2);
	inst->vtbl.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	inst->vtbl.glBindVertexArray(0);
	int current_index = inst->g_bufferCount;
	inst->g_buffers[current_index] = b;
	inst->g_bufferCount++;
	return current_index;
}
TW3D_API TwID twLoadBufferToMainPool(TwInstance* inst, float* verts, int vc, unsigned int* inds, int ic) {
	TwPool* p = &inst->MainPool.buffer;
	p->data[p->count] = tw_loadBufCore(inst, verts, vc, inds, ic);
	return p->data[p->count++];
}
TW3D_API TwID twLoadBufferToTempPool(TwInstance* inst, float* verts, int vc, unsigned int* inds, int ic) {
	TwPool* p = &inst->TempPool.buffer;
	p->data[p->count] = tw_loadBufCore(inst, verts, vc, inds, ic);
	return p->data[p->count++];
}
TW3D_API void twBindBuffer(TwInstance* inst, TwID buffer) {
	TwBackendBuffer* b = &inst->g_buffers[buffer];
	inst->g_active_icount = b->icount;
	inst->vtbl.glBindVertexArray(b->vao);
}
static unsigned int tw_compileShader(TwInstance* inst, unsigned int type, const char* source) {
	unsigned int id = inst->vtbl.glCreateShader(type);
	inst->vtbl.glShaderSource(id, 1, &source, NULL);
	inst->vtbl.glCompileShader(id);
	return id;
}
TW3D_API TwID twLoadShaderInMainPool(TwInstance* inst, const char* vshader, const char* fshader) {
	char* vSrc = tw_readFile(vshader);
	char* fSrc = tw_readFile(fshader);
	unsigned int vs = tw_compileShader(inst, 0x8B31, vSrc);
	unsigned int fs = tw_compileShader(inst, 0x8B30, fSrc);
	unsigned int prog = inst->vtbl.glCreateProgram();
	inst->vtbl.glAttachShader(prog, vs);
	inst->vtbl.glAttachShader(prog, fs);
	inst->vtbl.glLinkProgram(prog);
	inst->vtbl.glDeleteShader(vs);
	inst->vtbl.glDeleteShader(fs);
	free(vSrc); free(fSrc);
	TwPool* p = &inst->MainPool.shader;
	p->data[p->count] = prog;
	return p->data[p->count++];
}
TW3D_API void twBindShader(TwInstance* inst, TwID shader) {
	inst->vtbl.glUseProgram(shader);
}
TW3D_API int twGetUniformLocation(TwInstance* inst, TwID shader, const char* var) {
	return inst->vtbl.glGetUniformLocation(shader, var);
}
TW3D_API void twSendUniformMat4(TwInstance* inst, TwID shader, int loc, Mat4 val) {
	if (loc != -1) inst->vtbl.glUniformMatrix4fv(loc, 1, GL_FALSE, val.m);
}
TW3D_API void twSendUniformVec3(TwInstance* inst, TwID shader, int loc, Vec3 val) {
	if (loc != -1) inst->vtbl.glUniform3f(loc, val.x, val.y, val.z);
}
TW3D_API void twSendUniformFloat(TwInstance* inst, TwID shader, int loc, float val) {
	if (loc != -1) inst->vtbl.glUniform1f(loc, val);
}