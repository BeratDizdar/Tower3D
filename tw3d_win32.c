#include "tw3d.h"
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <stdlib.h>
#include <math.h>

#define TW3D_API __declspec(dllexport)

typedef struct TwInstance {
	HWND hwnd;
	HDC hdc;
	HGLRC hrc;
	TwBoolean is_active;
	TwBoolean is_window_active;

	LARGE_INTEGER performance_frequency;
	LARGE_INTEGER last_counter;

	TwUint width, height, x, y;
	TwByte exit_key;
	const char* title;
} TwInstance;

void twMathIdentity(TwMat4* out_mat) {
	for (int i = 0; i < 16; i++) out_mat->m[i] = 0.0f;
	out_mat->m[0] = 1.0f; out_mat->m[5] = 1.0f;
	out_mat->m[10] = 1.0f; out_mat->m[15] = 1.0f;
}

void twMathOrtho(TwMat4* out_mat, TwFloat left, TwFloat right, TwFloat bottom, TwFloat top, TwFloat near_val, TwFloat far_val) {
	twMathIdentity(out_mat);
	out_mat->m[0] = 2.0f / (right - left);
	out_mat->m[5] = 2.0f / (top - bottom);
	out_mat->m[10] = -2.0f / (far_val - near_val);
	out_mat->m[12] = -(right + left) / (right - left);
	out_mat->m[13] = -(top + bottom) / (top - bottom);
	out_mat->m[14] = -(far_val + near_val) / (far_val - near_val);
}

void twMathPerspective(TwMat4* out_mat, TwFloat fov_y, TwFloat aspect, TwFloat near_val, TwFloat far_val) {
	twMathIdentity(out_mat);
	TwFloat f = 1.0f / tan(fov_y * 0.5f * (3.1415926535f / 180.0f));
	out_mat->m[0] = f / aspect;
	out_mat->m[5] = f;
	out_mat->m[10] = (far_val + near_val) / (near_val - far_val);
	out_mat->m[11] = -1.0f;
	out_mat->m[14] = (2.0f * far_val * near_val) / (near_val - far_val);
	out_mat->m[15] = 0.0f;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
	switch (umessage) {
	case WM_DESTROY: PostQuitMessage(0); return 0;
	case WM_CLOSE: DestroyWindow(hwnd); return 0;
	case WM_SIZE:
		glViewport(0, 0, LOWORD(lparam), HIWORD(lparam));
		return 0;
	default: return DefWindowProc(hwnd, umessage, wparam, lparam);
	}
}

TW3D_API TwInstance* twCreateInstance(const TwInstanceDesc* desc) {
	TwInstance* inst = malloc(sizeof(TwInstance));
	if (inst == NULL) return TW_FALSE;

	// tcc kullandığım için gerekli
	HMODULE user32 = LoadLibraryA("user32.dll");
	if (user32 == NULL) return TW_FALSE;
	((void* (*)())GetProcAddress(user32, "SetProcessDPIAware"))();
	FreeLibrary(user32);

	inst->title = desc->title;
	if (desc->flag == TW_DESC_FLAG_FULLSCREEN) {
		inst->width = GetSystemMetrics(SM_CXSCREEN);
		inst->height = GetSystemMetrics(SM_CYSCREEN);
		inst->x = 0;
		inst->y = 0;
	}
	else {
		inst->width = desc->width;
		inst->height = desc->height;
		inst->x = (TwUint)CW_USEDEFAULT;
		inst->y = (TwUint)CW_USEDEFAULT;
	}
	if (desc->exit_key == TW_NONE) {
		inst->exit_key = VK_DELETE; // başka uygun bulamadım ehehe
	}
	else {
		inst->exit_key = desc->exit_key;
	}

	QueryPerformanceFrequency(&inst->performance_frequency);
	QueryPerformanceCounter(&inst->last_counter);

	inst->is_active = TW_TRUE;
	return inst;
}

TW3D_API TwBoolean twDeleteInstance(TwInstance* inst) {
	if (inst != NULL) {
		inst->is_active = TW_FALSE;
		free(inst);
		return TW_TRUE;
	}
	return TW_FALSE;
}

TW3D_API TwBoolean twOpenWindow(TwInstance* inst) {
	HINSTANCE module = GetModuleHandleA(NULL);
	WNDCLASSA wnd = { 0 };
	wnd.hInstance = module;
	wnd.lpfnWndProc = WndProc;
	wnd.lpszClassName = "wndclass1";
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.style = CS_OWNDC;
	RegisterClassA(&wnd);

	inst->hwnd = CreateWindowExA(
		WS_EX_APPWINDOW, wnd.lpszClassName, inst->title, WS_POPUP | WS_VISIBLE, // WS_POPUP - WS_POPUPWINDOW
		(TwInt)inst->x, (TwInt)inst->y, inst->width, inst->height,
		NULL, NULL, module, NULL);
	
	inst->hdc = GetDC(inst->hwnd);
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;

	int pf = ChoosePixelFormat(inst->hdc, &pfd);
	SetPixelFormat(inst->hdc, pf, &pfd);
	inst->hrc = wglCreateContext(inst->hdc);
	wglMakeCurrent(inst->hdc, inst->hrc);

	inst->is_window_active = TW_TRUE;
}

TW3D_API TwBoolean twIsWindowActive(TwInstance* inst) {
	return inst->is_window_active;
}

TW3D_API TwBoolean twCloseWindow(TwInstance* inst) {
	inst->is_window_active = TW_FALSE;
	ReleaseDC(inst->hwnd, inst->hdc);
	DestroyWindow(inst->hwnd);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(inst->hrc);
}

TW3D_API void twSwapBuffer(TwInstance* inst) {
	SwapBuffers(inst->hdc);
	MSG msg = { 0 };
	while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) inst->is_window_active = TW_FALSE;
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	if (GetAsyncKeyState(inst->exit_key)) inst->is_window_active = TW_FALSE;
}

typedef int (WINAPI* TwWglSwapIntervalEXT)(int interval);
TW3D_API void twSwapInterval(TwInstance* inst, TwInt interval) {
	((TwWglSwapIntervalEXT)wglGetProcAddress("wglSwapIntervalEXT"))(interval);
}
TW3D_API void twWait(TwUint msec) {
	Sleep(msec);
}
TW3D_API TwBoolean twKeyDown(TwInstance* inst, TwByte key) {
	return GetAsyncKeyState((int)key);
}

/* RENDER */
TW3D_API void twEnable(TwInstance* inst, TwUint cap) {
	glEnable(cap);
}
TW3D_API void twDisable(TwInstance* inst, TwUint cap) {
	glDisable(cap);
}
TW3D_API void twViewport(TwInstance* inst, TwInt x, TwInt y, TwSizei w, TwSizei h) {
	glViewport(x, y, w, h);
}
TW3D_API void twScissor(TwInstance* inst, TwInt x, TwInt y, TwSizei w, TwSizei h) {
	glScissor(x, y, w, h);
}
TW3D_API void twClearColor(TwInstance* inst, TwFloat red, TwFloat green, TwFloat blue, TwFloat alpha) {
	glClearColor(red, green, blue, alpha);
}
TW3D_API void twClearDepth(TwInstance* inst, TwDouble depth) {
	glClearDepth(depth);
}
TW3D_API void twClear(TwInstance* inst, TwUint mask) {
	glClear(mask);
}
TW3D_API void twClearStencil(TwInstance* inst, TwInt s) {
	glClearStencil(s);
}
TW3D_API void twLoadProjectionMatrix(TwInstance* inst, const TwMat4* m) {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(m->m);
}
TW3D_API void twLoadModelViewMatrix(TwInstance* inst, const TwMat4* m) {
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(m->m);
}
TW3D_API void twBlendFunc(TwInstance* inst, TwUint sfactor, TwUint dfactor) {
	glBlendFunc(sfactor, dfactor);
}
TW3D_API void twDepthFunc(TwInstance* inst, TwUint func) {
	glDepthFunc(func);
}
TW3D_API void twDepthMask(TwInstance* inst, TwBoolean flag) {
	glDepthMask(flag ? GL_TRUE : GL_FALSE);
}
TW3D_API void twCullFace(TwInstance* inst, TwUint mode) {
	glCullFace(mode);
}
TW3D_API void twFrontFace(TwInstance* inst, TwUint mode) {
	glFrontFace(mode);
}
typedef void (WINAPI* TwBindBufferProc)(TwUint, TwUint);
TW3D_API void twBindBuffer(TwInstance* inst, TwUint target, TwUint buffer) {
	((TwBindBufferProc)wglGetProcAddress("glBindBuffer"))(target, buffer);
}
typedef void (WINAPI* TwGenBuffersProc)(TwSizei, TwUint*);
TW3D_API void twGenBuffers(TwInstance* inst, TwSizei n, TwUint* buffers) {
	((TwGenBuffersProc)wglGetProcAddress("glGenBuffers"))(n, buffers);
}
typedef void (WINAPI* TwBufferDataProc)(TwUint, TwSizeiptr, const void*, TwUint);
TW3D_API void twBufferData(TwInstance* inst, TwUint target, TwSizeiptr size, const void* data, TwUint usage) {
	((TwBufferDataProc)wglGetProcAddress("glBufferData"))(target, size, data, usage);
}
typedef void (WINAPI* TwDeleteBuffersProc)(TwSizei, const TwUint*);
TW3D_API void twDeleteBuffers(TwInstance* inst, TwSizei n, const TwUint* buffers) {
	((TwDeleteBuffersProc)wglGetProcAddress("glDeleteBuffers"))(n, buffers);
}
TW3D_API void twTexCoordPointer(TwInstance* inst, TwInt size, TwUint type, TwSizei stride, const void* pointer) {
	glTexCoordPointer(size, type, stride, pointer);
}
TW3D_API void twVertexPointer(TwInstance* inst, TwInt size, TwUint type, TwSizei stride, const void* pointer) {
	glVertexPointer(size, type, stride, pointer);
}
TW3D_API void twColorPointer(TwInstance* inst, TwInt size, TwUint type, TwSizei stride, const void* pointer) {
	glColorPointer(size, type, stride, pointer);
}
TW3D_API void twNormalPointer(TwInstance* inst, TwUint type, TwSizei stride, const void* pointer) {
	glNormalPointer(type, stride, pointer);
}
TW3D_API void twEnableClientState(TwInstance* inst, TwUint array) {
	glEnableClientState(array);
}
TW3D_API void twDisableClientState(TwInstance* inst, TwUint array) {
	glDisableClientState(array);
}
TW3D_API void twDrawArrays(TwInstance* inst, TwUint mode, TwInt first, TwSizei count) {
	glDrawArrays(mode, first, count);
}
TW3D_API void twDrawElements(TwInstance* inst, TwUint mode, TwSizei count, TwUint type, const void* indices) {
	glDrawElements(mode, count, type, indices);
}
TW3D_API void twBindTexture(TwInstance* inst, TwUint target, TwUint texture) {
	glBindTexture(target, texture);
}
TW3D_API void twGenTextures(TwInstance* inst, TwSizei n, TwUint* textures) {
	glGenTextures(n, textures);
}
TW3D_API void twTexParameteri(TwInstance* inst, TwUint target, TwUint pname, TwInt param) {
	glTexParameteri(target, pname, param);
}
TW3D_API void twTexImage2D(TwInstance* inst, TwUint target, TwInt level, TwInt internalformat, TwSizei width, TwSizei height, TwInt border, TwUint format, TwUint type, const void* pixels) {
	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}
TW3D_API void twDeleteTextures(TwInstance* inst, TwSizei n, const TwUint* textures) {
	glDeleteTextures(n, textures);
}
TW3D_API void twLightModelf(TwInstance* inst, TwUint pname, TwFloat param) {
	glLightModelf(pname, param);
}
TW3D_API void twLightModelfv(TwInstance* inst, TwUint pname, const TwFloat* params) {
	glLightModelfv(pname, params);
}
TW3D_API void twLightf(TwInstance* inst, TwUint light, TwUint pname, TwFloat param) {
	glLightf(light, pname, param);
}
TW3D_API void twLightfv(TwInstance* inst, TwUint light, TwUint pname, const TwFloat* params) {
	glLightfv(light, pname, params);
}
TW3D_API void twMaterialf(TwInstance* inst, TwUint face, TwUint pname, TwFloat param) {
	glMaterialf(face, pname, param);
}
TW3D_API void twMaterialfv(TwInstance* inst, TwUint face, TwUint pname, const TwFloat* params) {
	glMaterialfv(face, pname, params);
}
TW3D_API void twShadeModel(TwInstance* inst, TwUint mode) {
	glShadeModel(mode);
}
TW3D_API void twFogf(TwInstance* inst, TwUint pname, TwFloat param) {
	glFogf(pname, param);
}
TW3D_API void twFogfv(TwInstance* inst, TwUint pname, const TwFloat* params) {
	glFogfv(pname, params);
}
TW3D_API void twTexEnvf(TwInstance* inst, TwUint target, TwUint pname, TwFloat param) {
	glTexEnvf(target, pname, param);
}
TW3D_API void twTexEnvfv(TwInstance* inst, TwUint target, TwUint pname, const TwFloat* params) {
	glTexEnvfv(target, pname, params);
}
TW3D_API void twPolygonOffset(TwInstance* inst, TwFloat factor, TwFloat units) {
	glPolygonOffset(factor, units);
}
TW3D_API void twLineWidth(TwInstance* inst, TwFloat width) {
	glLineWidth(width);
}
TW3D_API void twPointSize(TwInstance* inst, TwFloat size) {
	glPointSize(size);
}
TW3D_API void twReadPixels(TwInstance* inst, TwInt x, TwInt y, TwSizei width, TwSizei height, TwUint format, TwUint type, void* pixels) {
	glReadPixels(x, y, width, height, format, type, pixels);
}