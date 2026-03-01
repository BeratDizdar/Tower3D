#include "tw3d.h"
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <stdlib.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#include "stb_image.h"
#include <stdio.h>

#define TW3D_API __declspec(dllexport)

typedef struct TwInstance {
	HWND hwnd;
	HDC hdc;
	HGLRC hrc;
	TwBoolean is_active;
	TwBoolean is_window_active;
	TwUint gpu_call_count;

	// resource
	TwUint* tex_pool;
	TwUint tex_count;
	TwUint tex_cap;

	LARGE_INTEGER timer_freq;
	LARGE_INTEGER last_time;

	TwUint width, height, x, y;
	TwByte exit_key;
	const char* title;
} TwInstance;

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

TW3D_API void twMathIdentity(TwMat4* out_mat) {
	for (int i = 0; i < 16; i++) out_mat->m[i] = 0.0f;
	out_mat->m[0] = 1.0f; out_mat->m[5] = 1.0f;
	out_mat->m[10] = 1.0f; out_mat->m[15] = 1.0f;
}

TW3D_API void twMathOrtho(TwMat4* out_mat, TwFloat left, TwFloat right, TwFloat bottom, TwFloat top, TwFloat near_val, TwFloat far_val) {
	twMathIdentity(out_mat);
	out_mat->m[0] = 2.0f / (right - left);
	out_mat->m[5] = 2.0f / (top - bottom);
	out_mat->m[10] = -2.0f / (far_val - near_val);
	out_mat->m[12] = -(right + left) / (right - left);
	out_mat->m[13] = -(top + bottom) / (top - bottom);
	out_mat->m[14] = -(far_val + near_val) / (far_val - near_val);
}

TW3D_API void twMathPerspective(TwMat4* out_mat, TwFloat fov_y, TwFloat aspect, TwFloat near_val, TwFloat far_val) {
	twMathIdentity(out_mat);
	TwFloat f = 1.0f / tan(fov_y * 0.5f * (3.1415926535f / 180.0f));
	out_mat->m[0] = f / aspect;
	out_mat->m[5] = f;
	out_mat->m[10] = (far_val + near_val) / (near_val - far_val);
	out_mat->m[11] = -1.0f;
	out_mat->m[14] = (2.0f * far_val * near_val) / (near_val - far_val);
	out_mat->m[15] = 0.0f;
}

TW3D_API void twMathLookAt(TwMat4* out_mat, const TwVec3* eye, const TwVec3* center, const TwVec3* up) {
	TwVec3 f, s, u;
	TwFloat length;
	
	f.x = center->x - eye->x;
	f.y = center->y - eye->y;
	f.z = center->z - eye->z;
	length = (TwFloat)sqrt(f.x * f.x + f.y * f.y + f.z * f.z);
	if (length > 0.0f) {
		f.x /= length; f.y /= length; f.z /= length;
	}

	s.x = f.y * up->z - f.z * up->y;
	s.y = f.z * up->x - f.x * up->z;
	s.z = f.x * up->y - f.y * up->x;

	length = (TwFloat)sqrt(s.x * s.x + s.y * s.y + s.z * s.z);
	if (length > 0.0f) {
		s.x /= length; s.y /= length; s.z /= length;
	}

	u.x = s.y * f.z - s.z * f.y;
	u.y = s.z * f.x - s.x * f.z;
	u.z = s.x * f.y - s.y * f.x;

	out_mat->m[0] = s.x;
	out_mat->m[4] = s.y;
	out_mat->m[8] = s.z;
	out_mat->m[12] = -(s.x * eye->x + s.y * eye->y + s.z * eye->z);

	out_mat->m[1] = u.x;
	out_mat->m[5] = u.y;
	out_mat->m[9] = u.z;
	out_mat->m[13] = -(u.x * eye->x + u.y * eye->y + u.z * eye->z);

	out_mat->m[2] = -f.x;
	out_mat->m[6] = -f.y;
	out_mat->m[10] = -f.z;
	out_mat->m[14] = (f.x * eye->x + f.y * eye->y + f.z * eye->z);

	out_mat->m[3] = 0.0f;
	out_mat->m[7] = 0.0f;
	out_mat->m[11] = 0.0f;
	out_mat->m[15] = 1.0f;
}

TW3D_API void twMathRotate(TwMat4* out_mat, TwFloat angle_deg, const TwVec3* axis) {
	TwFloat rad = angle_deg * (TW_PI / 180.0f);
	TwFloat c = cos(rad);
	TwFloat s = sin(rad);
	TwFloat t = 1.0f - c;

	TwFloat length = sqrt(axis->x * axis->x + axis->y * axis->y + axis->z * axis->z);
	TwFloat x = axis->x, y = axis->y, z = axis->z;
	if (length > 0.0f) {
		x /= length; y /= length; z /= length;
	}

	out_mat->m[0] = t * x * x + c;
	out_mat->m[1] = t * x * y + s * z;
	out_mat->m[2] = t * x * z - s * y;
	out_mat->m[3] = 0.0f;

	out_mat->m[4] = t * x * y - s * z;
	out_mat->m[5] = t * y * y + c;
	out_mat->m[6] = t * y * z + s * x;
	out_mat->m[7] = 0.0f;

	out_mat->m[8] = t * x * z + s * y;
	out_mat->m[9] = t * y * z - s * x;
	out_mat->m[10] = t * z * z + c;
	out_mat->m[11] = 0.0f;

	out_mat->m[12] = 0.0f;
	out_mat->m[13] = 0.0f;
	out_mat->m[14] = 0.0f;
	out_mat->m[15] = 1.0f;
}

TW3D_API void twMathMultiply(TwMat4* out_mat, const TwMat4* mat_a, const TwMat4* mat_b) {
	for (int c = 0; c < 4; c++) {
		for (int r = 0; r < 4; r++) {
			out_mat->m[c * 4 + r] =
				mat_a->m[0 * 4 + r] * mat_b->m[c * 4 + 0] +
				mat_a->m[1 * 4 + r] * mat_b->m[c * 4 + 1] +
				mat_a->m[2 * 4 + r] * mat_b->m[c * 4 + 2] +
				mat_a->m[3 * 4 + r] * mat_b->m[c * 4 + 3];
		}
	}
}

TW3D_API void twMathTranslate(TwMat4* out_mat, const TwVec3* xyz) {
	twMathIdentity(out_mat);
	// opengl'in nasıl okuduğu önemli
	out_mat->m[12] = xyz->x;
	out_mat->m[13] = xyz->y;
	out_mat->m[14] = xyz->z;
}

TW3D_API void twMathScale(TwMat4* out_mat, const TwVec3* xyz) {
	twMathIdentity(out_mat);
	out_mat->m[0] =  xyz->x;
	out_mat->m[5] =  xyz->y;
	out_mat->m[10] = xyz->z;
}

TW3D_API TwInstance* twCreateInstance(const TwInstanceDesc* desc) {
	TwInstance* inst = malloc(sizeof(TwInstance));
	if (inst == NULL) return TW_FALSE;

	// tcc kullandığım için gerekli
	HMODULE user32 = LoadLibraryA("user32.dll");
	if (user32 == NULL) return TW_FALSE;
	((void* (*)())GetProcAddress(user32, "SetProcessDPIAware"))();
	FreeLibrary(user32);
	inst->gpu_call_count = 0;
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

	inst->tex_cap = 0;
	inst->is_active = TW_TRUE;
	return inst;
}

TW3D_API TwBoolean twDeleteInstance(TwInstance* inst) {
	if (inst->tex_cap > 0) {
		glDeleteTextures(inst->tex_count, inst->tex_pool);
	}
	free(inst->tex_pool);
	if (inst != NULL) {
		inst->is_active = TW_FALSE;
		free(inst);
		return TW_TRUE;
	}
	return TW_FALSE;
}

TW3D_API TwBoolean twInstanceAllocator(TwInstance* inst, TwDictionary type, TwUint count) {
	switch (type) {
	case TW_ALLOC_TEXPOOL:
		inst->tex_pool = (TwUint*)malloc(count * sizeof(TwUint));
		if (inst->tex_pool == NULL) return TW_FALSE;
		inst->tex_cap = count;
		inst->tex_count = 0;
		break;
	default: return TW_FALSE;
	}
	return TW_TRUE;
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

	QueryPerformanceFrequency(&inst->timer_freq);
	QueryPerformanceCounter(&inst->last_time);

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
TW3D_API TwFloat twGetDeltaTime(TwInstance* inst) {
	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	TwFloat delta = (TwFloat)(current_time.QuadPart - inst->last_time.QuadPart) / (TwFloat)inst->timer_freq.QuadPart;
	inst->last_time = current_time;
	if (delta > 0.1f) delta = 0.1f;
	return delta;
}
TW3D_API TwBoolean twKeyDown(TwInstance* inst, TwByte key) {
	return GetAsyncKeyState((int)key);
}

/* RENDER */

TW3D_API TwUint twGetGpuCallCount(TwInstance* inst) {
	return inst->gpu_call_count;
}
TW3D_API void twResetGpuCallCount(TwInstance* inst) {
	inst->gpu_call_count = 0;
}

TW3D_API void twSetFeatures(TwInstance* inst, TwFeature feature_mask) {
	if (feature_mask & TW_FEATURE_TEXTURE_2D) glEnable(TW_TEXTURE_2D);
	else glDisable(TW_TEXTURE_2D);
	if (feature_mask & TW_FEATURE_LIGHTING) glEnable(TW_LIGHTING);
	else glDisable(TW_LIGHTING);
	if (feature_mask & TW_FEATURE_BLEND) glEnable(TW_BLEND);
	else glDisable(TW_BLEND);
	if (feature_mask & TW_FEATURE_FOG) glEnable(TW_FOG);
	else glDisable(TW_FOG);
	if (feature_mask & TW_FEATURE_NORMALIZE) glEnable(TW_NORMALIZE);
	else glDisable(TW_NORMALIZE);
	if (feature_mask & TW_FEATURE_DEPTH_TEST) glEnable(TW_DEPTH_TEST);
	else glDisable(TW_DEPTH_TEST);
	if (feature_mask & TW_FEATURE_ALPHA_TEST) glEnable(TW_ALPHA_TEST);
	else glDisable(TW_ALPHA_TEST);
	if (feature_mask & TW_FEATURE_COLOR_MATERIAL) glEnable(TW_COLOR_MATERIAL);
	else glDisable(TW_COLOR_MATERIAL);
	if (feature_mask & TW_FEATURE_CULL_FACE) glEnable(TW_CULL_FACE);
	else glDisable(TW_CULL_FACE);

	if (feature_mask & TW_FEATURE_LIGHT0) glEnable(TW_LIGHT0);
	else glDisable(TW_LIGHT0);
	if (feature_mask & TW_FEATURE_LIGHT1) glEnable(TW_LIGHT1);
	else glDisable(TW_LIGHT1);
	if (feature_mask & TW_FEATURE_LIGHT2) glEnable(TW_LIGHT2);
	else glDisable(TW_LIGHT2);
	if (feature_mask & TW_FEATURE_LIGHT3) glEnable(TW_LIGHT3);
	else glDisable(TW_LIGHT3);
	if (feature_mask & TW_FEATURE_LIGHT4) glEnable(TW_LIGHT4);
	else glDisable(TW_LIGHT4);
	if (feature_mask & TW_FEATURE_LIGHT5) glEnable(TW_LIGHT5);
	else glDisable(TW_LIGHT5);
	if (feature_mask & TW_FEATURE_LIGHT6) glEnable(TW_LIGHT6);
	else glDisable(TW_LIGHT6);
	if (feature_mask & TW_FEATURE_LIGHT7) glEnable(TW_LIGHT7);
	else glDisable(TW_LIGHT7);

	inst->gpu_call_count += 17;
}
TW3D_API void twEnable(TwInstance* inst, TwUint cap) {
	glEnable(cap); inst->gpu_call_count++;
}
TW3D_API void twDisable(TwInstance* inst, TwUint cap) {
	glDisable(cap); inst->gpu_call_count++;
}
TW3D_API void twViewport(TwInstance* inst, TwInt x, TwInt y, TwSizei w, TwSizei h) {
	glViewport(x, y, w, h); inst->gpu_call_count++;
}
TW3D_API void twScissor(TwInstance* inst, TwInt x, TwInt y, TwSizei w, TwSizei h) {
	glScissor(x, y, w, h); inst->gpu_call_count++;
}
TW3D_API void twClearColor(TwInstance* inst, TwFloat red, TwFloat green, TwFloat blue, TwFloat alpha) {
	glClearColor(red, green, blue, alpha); inst->gpu_call_count++;
}
TW3D_API void twClearDepth(TwInstance* inst, TwDouble depth) {
	glClearDepth(depth); inst->gpu_call_count++;
}
TW3D_API void twClear(TwInstance* inst, TwUint mask) {
	glClear(mask); inst->gpu_call_count++;
}
TW3D_API void twClearStencil(TwInstance* inst, TwInt s) {
	glClearStencil(s); inst->gpu_call_count++;
}
TW3D_API void twLoadProjectionMatrix(TwInstance* inst, const TwMat4* m) {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(m->m); inst->gpu_call_count+=2;
}
TW3D_API void twLoadModelViewMatrix(TwInstance* inst, const TwMat4* m) {
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(m->m); inst->gpu_call_count+=2;
}

TW3D_API void twAlphaFunc(TwInstance* inst, TwUint func, TwFloat ref) {
	glAlphaFunc(func, ref);
}
TW3D_API void twBlendFunc(TwInstance* inst, TwUint sfactor, TwUint dfactor) {
	glBlendFunc(sfactor, dfactor); inst->gpu_call_count++;
}
TW3D_API void twDepthFunc(TwInstance* inst, TwUint func) {
	glDepthFunc(func); inst->gpu_call_count++;
}
TW3D_API void twDepthMask(TwInstance* inst, TwBoolean flag) {
	glDepthMask(flag ? GL_TRUE : GL_FALSE); inst->gpu_call_count++;
}
TW3D_API void twCullFace(TwInstance* inst, TwUint mode) {
	glCullFace(mode); inst->gpu_call_count++;
}
TW3D_API void twFrontFace(TwInstance* inst, TwUint mode) {
	glFrontFace(mode); inst->gpu_call_count++;
}
typedef void (WINAPI* TwBindBufferProc)(TwUint, TwUint);
TW3D_API void twBindBuffer(TwInstance* inst, TwUint target, TwUint buffer) {
	((TwBindBufferProc)wglGetProcAddress("glBindBuffer"))(target, buffer); inst->gpu_call_count++;
}
typedef void (WINAPI* TwGenBuffersProc)(TwSizei, TwUint*);
TW3D_API void twGenBuffers(TwInstance* inst, TwSizei n, TwUint* buffers) {
	((TwGenBuffersProc)wglGetProcAddress("glGenBuffers"))(n, buffers); inst->gpu_call_count++;
}
typedef void (WINAPI* TwBufferDataProc)(TwUint, TwSizeiptr, const void*, TwUint);
TW3D_API void twBufferData(TwInstance* inst, TwUint target, TwSizeiptr size, const void* data, TwUint usage) {
	((TwBufferDataProc)wglGetProcAddress("glBufferData"))(target, size, data, usage); inst->gpu_call_count++;
}
typedef void (WINAPI* TwDeleteBuffersProc)(TwSizei, const TwUint*);
TW3D_API void twDeleteBuffers(TwInstance* inst, TwSizei n, const TwUint* buffers) {
	((TwDeleteBuffersProc)wglGetProcAddress("glDeleteBuffers"))(n, buffers); inst->gpu_call_count++;
}
TW3D_API void twTexCoordPointer(TwInstance* inst, TwInt size, TwUint type, TwSizei stride, const void* pointer) {
	glTexCoordPointer(size, type, stride, pointer); inst->gpu_call_count++;
}
TW3D_API void twVertexPointer(TwInstance* inst, TwInt size, TwUint type, TwSizei stride, const void* pointer) {
	glVertexPointer(size, type, stride, pointer); inst->gpu_call_count++;
}
TW3D_API void twColorPointer(TwInstance* inst, TwInt size, TwUint type, TwSizei stride, const void* pointer) {
	glColorPointer(size, type, stride, pointer); inst->gpu_call_count++;
}
TW3D_API void twNormalPointer(TwInstance* inst, TwUint type, TwSizei stride, const void* pointer) {
	glNormalPointer(type, stride, pointer); inst->gpu_call_count++;
}
TW3D_API void twEnableClientState(TwInstance* inst, TwUint array) {
	glEnableClientState(array); inst->gpu_call_count++;
}
TW3D_API void twDisableClientState(TwInstance* inst, TwUint array) {
	glDisableClientState(array); inst->gpu_call_count++;
}
TW3D_API void twDrawArrays(TwInstance* inst, TwUint mode, TwInt first, TwSizei count) {
	glDrawArrays(mode, first, count); inst->gpu_call_count++;
}
TW3D_API void twDrawElements(TwInstance* inst, TwUint mode, TwSizei count, TwUint type, const void* indices) {
	glDrawElements(mode, count, type, indices); inst->gpu_call_count++;
}
TW3D_API void twBindTexture(TwInstance* inst, TwUint target, TwUint texture) {
	glBindTexture(target, texture); inst->gpu_call_count++;
}
TW3D_API void twGenTextures(TwInstance* inst, TwSizei n, TwUint* textures) {
	glGenTextures(n, textures); inst->gpu_call_count++;
}
TW3D_API void twTexParameteri(TwInstance* inst, TwUint target, TwUint pname, TwInt param) {
	glTexParameteri(target, pname, param); inst->gpu_call_count++;
}
TW3D_API void twTexImage2D(TwInstance* inst, TwUint target, TwInt level, TwInt internalformat, TwSizei width, TwSizei height, TwInt border, TwUint format, TwUint type, const void* pixels) {
	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels); inst->gpu_call_count++;
}
TW3D_API void twDeleteTextures(TwInstance* inst, TwSizei n, const TwUint* textures) {
	glDeleteTextures(n, textures); inst->gpu_call_count++;
}

TW3D_API TwID twLoadTexture(TwInstance* inst, TwDictionary filter, TwDictionary wrap, const char* filepath) {
	if (inst->tex_count == inst->tex_cap) return TW_ID_ERROR;
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(1);
	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (!data) return TW_ID_ERROR;

	TwUint tex_id;
	glGenTextures(1, &tex_id);
	glBindTexture(TW_TEXTURE_2D, tex_id);

	glTexParameteri(TW_TEXTURE_2D, TW_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(TW_TEXTURE_2D, TW_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(TW_TEXTURE_2D, TW_TEXTURE_WRAP_S, wrap);
	glTexParameteri(TW_TEXTURE_2D, TW_TEXTURE_WRAP_T, wrap);

	GLenum format = TW_RGB;
	if (nrChannels == 4) format = TW_RGBA;
	glTexImage2D(TW_TEXTURE_2D, 0, format, width, height, 0, format, TW_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	inst->gpu_call_count += 7;

	inst->tex_pool[inst->tex_count] = tex_id;
	TwID index = inst->tex_count;
	inst->tex_count++;
	return index;
}

TW3D_API void twBindTextureIndex(TwInstance* inst, TwID texture_id) {
	glBindTexture(TW_TEXTURE_2D, inst->tex_pool[texture_id]); inst->gpu_call_count++;
}

TW3D_API void twLightModelf(TwInstance* inst, TwUint pname, TwFloat param) {
	glLightModelf(pname, param); inst->gpu_call_count++;
}
TW3D_API void twLightModelfv(TwInstance* inst, TwUint pname, const TwFloat* params) {
	glLightModelfv(pname, params); inst->gpu_call_count++;
}
TW3D_API void twLightf(TwInstance* inst, TwUint light, TwUint pname, TwFloat param) {
	glLightf(light, pname, param); inst->gpu_call_count++;
}
TW3D_API void twLightfv(TwInstance* inst, TwUint light, TwUint pname, const TwFloat* params) {
	glLightfv(light, pname, params); inst->gpu_call_count++;
}
TW3D_API void twMaterialf(TwInstance* inst, TwUint face, TwUint pname, TwFloat param) {
	glMaterialf(face, pname, param); inst->gpu_call_count++;
}
TW3D_API void twMaterialfv(TwInstance* inst, TwUint face, TwUint pname, const TwFloat* params) {
	glMaterialfv(face, pname, params); inst->gpu_call_count++;
}
TW3D_API void twShadeModel(TwInstance* inst, TwUint mode) {
	glShadeModel(mode); inst->gpu_call_count++;
}
TW3D_API void twColorMaterial(TwInstance* inst, TwUint face, TwUint mode) {
	glColorMaterial(face, mode); inst->gpu_call_count++;
}

TW3D_API void twFogf(TwInstance* inst, TwUint pname, TwFloat param) {
	glFogf(pname, param); inst->gpu_call_count++;
}
TW3D_API void twFogfv(TwInstance* inst, TwUint pname, const TwFloat* params) {
	glFogfv(pname, params); inst->gpu_call_count++;
}
TW3D_API void twFogi(TwInstance* inst, TwUint pname, TwUint param) {
	glFogi(pname, param);
}
TW3D_API void twHint(TwInstance* inst, TwUint target, TwUint mode) {
	glHint(target, mode);
}

TW3D_API void twTexEnvf(TwInstance* inst, TwUint target, TwUint pname, TwFloat param) {
	glTexEnvf(target, pname, param); inst->gpu_call_count++;
}
TW3D_API void twTexEnvfv(TwInstance* inst, TwUint target, TwUint pname, const TwFloat* params) {
	glTexEnvfv(target, pname, params); inst->gpu_call_count++;
}
TW3D_API void twPolygonOffset(TwInstance* inst, TwFloat factor, TwFloat units) {
	glPolygonOffset(factor, units); inst->gpu_call_count++;
}
TW3D_API void twLineWidth(TwInstance* inst, TwFloat width) {
	glLineWidth(width); inst->gpu_call_count++;
}
TW3D_API void twPointSize(TwInstance* inst, TwFloat size) {
	glPointSize(size); inst->gpu_call_count++;
}
TW3D_API void twReadPixels(TwInstance* inst, TwInt x, TwInt y, TwSizei width, TwSizei height, TwUint format, TwUint type, void* pixels) {
	glReadPixels(x, y, width, height, format, type, pixels); inst->gpu_call_count++;
}