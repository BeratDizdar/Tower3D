#pragma region INCLUDE BOKLARI
#include "tw3d.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <gl/GL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#pragma endregion

#define TW3D_API __declspec(dllexport)

typedef struct TwInstance {
    struct {
        HGLRC context;
        HDC device;
        HWND handler;

        LARGE_INTEGER timer_freq;
        LARGE_INTEGER last_time;

        int exit_key;
        bool_t is_active;
    } win32;
} TwInstance;

#pragma region INSTANCE BOKU

TW3D_API TwInstance* TwCreateInstance() {
    TwInstance* inst = (TwInstance*)calloc(1, sizeof(TwInstance));
    return inst;
}
TW3D_API void TwDeleteInstance(TwInstance* inst) {
    if (!inst) return;
    if (inst->win32.context) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(inst->win32.context);
    }
    if (inst->win32.handler && inst->win32.device) {
        ReleaseDC(inst->win32.handler, inst->win32.device);
        DestroyWindow(inst->win32.handler);
    }
    free(inst);
    return;
}

#pragma endregion

#pragma region SURFACE BOKU

static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
    switch (umessage) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    default: return DefWindowProc(hwnd, umessage, wparam, lparam);
    }
}
TW3D_API bool_t TwCreateSurface(TwInstance* inst, const char* title) {
    HINSTANCE module = GetModuleHandleA(NULL);
    WNDCLASSA wnd = { 0 };
    wnd.hInstance = module;
    wnd.lpfnWndProc = WndProc;
    wnd.lpszClassName = "wndclass1";
    wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
    wnd.style = CS_OWNDC;
    RegisterClassA(&wnd);

    // tcc kullandığım için böyle yaptım
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32 != NULL) {
        FARPROC dpi_func = GetProcAddress(user32, "SetProcessDPIAware");
        if (dpi_func != NULL) {
            ((BOOL(WINAPI*)(void))dpi_func)();
        }
        FreeLibrary(user32);
    }
    inst->win32.handler = CreateWindowExA(
        WS_EX_APPWINDOW, wnd.lpszClassName, title, WS_POPUP | WS_VISIBLE, // WS_POPUP - WS_POPUPWINDOW
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, module, NULL);

    QueryPerformanceFrequency(&inst->win32.timer_freq);
    QueryPerformanceCounter(&inst->win32.last_time);

    inst->win32.is_active = TW_TRUE;
    inst->win32.exit_key = 27;
    return TW_TRUE;
}
TW3D_API bool_t TwUpdateSurface(TwInstance* inst) {
    return inst->win32.is_active;
}
TW3D_API f32_t TwGetDeltaTime(TwInstance* inst) {
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    float delta = (float)(current_time.QuadPart - inst->win32.last_time.QuadPart) / (float)inst->win32.timer_freq.QuadPart;
    inst->win32.last_time = current_time;
    if (delta > 0.1f) delta = 0.1f;
    return delta;
}
TW3D_API void TwSwapBuffers(TwInstance* inst) {
    SwapBuffers(inst->win32.device);
    MSG msg = { 0 };
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) inst->win32.is_active = TW_FALSE;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    if (GetAsyncKeyState(inst->win32.exit_key)) inst->win32.is_active = TW_FALSE;
}
TW3D_API void TwBindContextToSurface(TwInstance* inst, bool_t vsync) {
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

	((int(*)(int))wglGetProcAddress("wglSwapIntervalEXT"))(vsync);
}

#pragma endregion

#pragma region CONTEXT BOKU



#pragma endregion

#pragma region MATEMATIK BOKU

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
		a.y* b.z - a.z * b.y,
			a.z* b.x - a.x * b.z,
			a.x* b.y - a.y * b.x,
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

#pragma endregion

