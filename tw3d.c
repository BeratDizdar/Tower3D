#include "tw3d.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>
#include <math.h>

#define sqrtf sqrt
#define cosf cos
#define sinf sin
#define tanf tan

#define TW3D_API __declspec(dllexport)

typedef struct TwInstance {
    struct {
        HGLRC context;
        HDC device;
        HWND handler;

        LARGE_INTEGER timer_freq;
        LARGE_INTEGER last_time;

        u8 keys[256];
        u8 prev_keys[256];
        i32 mouse_x, mouse_y;
        i32 prev_mouse_x, prev_mouse_y;
        i32 mouse_dx, mouse_dy;

        i32 exit_key;
        b32 is_active;
        b32 cursor_locked;

        u8 mouse_left, prev_mouse_left;
        u8 mouse_right, prev_mouse_right;
    } win32;
    struct {
        // buraya taşıyacayağım unutmadan
    } input;
} TwInstance;

#pragma region INSTANCE
TW3D_API TwInstance* twCreateInstance() {
    TwInstance* inst = (TwInstance*)calloc(1, sizeof(TwInstance));
    return inst;
}

TW3D_API void twDeleteInstance(TwInstance* inst) {
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

#pragma region SURFACE
static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
    switch (umessage) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    default: return DefWindowProc(hwnd, umessage, wparam, lparam);
    }
}

TW3D_API i32 twCreateSurface(TwInstance* inst, const char* title) {
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
        if (dpi_func != NULL) ((BOOL(WINAPI*)(void))dpi_func)();
        FreeLibrary(user32);
    }
    inst->win32.handler = CreateWindowExA(
        WS_EX_APPWINDOW, wnd.lpszClassName, title, WS_POPUP | WS_VISIBLE, // WS_POPUP - WS_POPUPWINDOW
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, module, NULL);

    QueryPerformanceFrequency(&inst->win32.timer_freq);
    QueryPerformanceCounter(&inst->win32.last_time);

    inst->win32.is_active = 1;
    inst->win32.exit_key = 27;
    return 1;
}

TW3D_API i32 twSurfaceActive(TwInstance* inst) { return inst->win32.is_active; }

TW3D_API void twUpdateSurface(TwInstance* inst) {
    for (int i = 0; i < 256; i++) inst->win32.prev_keys[i] = inst->win32.keys[i];
    inst->win32.prev_mouse_left = inst->win32.mouse_left;
    inst->win32.prev_mouse_right = inst->win32.mouse_right;
    inst->win32.prev_mouse_x = inst->win32.mouse_x;
    inst->win32.prev_mouse_y = inst->win32.mouse_y;

    for (int i = 0; i < 256; i++) inst->win32.keys[i] = (GetAsyncKeyState(i) & 0x8000) ? 1 : 0;
    inst->win32.mouse_left = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
    inst->win32.mouse_right = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 1 : 0;

    POINT p;
    GetCursorPos(&p);
    ScreenToClient(inst->win32.handler, &p);

    if (inst->win32.cursor_locked) {
        RECT rect;
        GetClientRect(inst->win32.handler, &rect);
        int cx = (rect.right - rect.left) / 2;
        int cy = (rect.bottom - rect.top) / 2;

        inst->win32.mouse_x = p.x;
        inst->win32.mouse_y = p.y;
        inst->win32.mouse_dx = p.x - cx;
        inst->win32.mouse_dy = p.y - cy;

        POINT center_pt = { cx, cy };
        ClientToScreen(inst->win32.handler, &center_pt);
        SetCursorPos(center_pt.x, center_pt.y);
    }
    else {
        inst->win32.mouse_x = p.x;
        inst->win32.mouse_y = p.y;
        inst->win32.mouse_dx = inst->win32.mouse_x - inst->win32.prev_mouse_x;
        inst->win32.mouse_dy = inst->win32.mouse_y - inst->win32.prev_mouse_y;
    }

    MSG msg = { 0 };
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) inst->win32.is_active = TW_FALSE;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    if (GetAsyncKeyState(inst->win32.exit_key)) inst->win32.is_active = TW_FALSE;
}

TW3D_API void* twGetSurfaceHandler(TwInstance* inst) { return (void*)inst->win32.handler; }

TW3D_API void twGLBindContext(TwInstance* inst, b32 vsync) {
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

TW3D_API void twGLSwapBuffers(TwInstance* inst) {
    SwapBuffers(inst->win32.device);
}

TW3D_API f32 twGetDeltaTime(TwInstance* inst) {
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    f32 delta = (f32)(current_time.QuadPart - inst->win32.last_time.QuadPart) / (f32)inst->win32.timer_freq.QuadPart;
    inst->win32.last_time = current_time;
    if (delta > 0.1f) delta = 0.1f;
    return delta;
}
#pragma endregion

#pragma region INPUT

TW3D_API i32 twKeyDown(TwInstance* inst, i32 key) { return inst->win32.keys[key]; }

TW3D_API i32 twKeyPressed(TwInstance* inst, i32 key) {
    return (inst->win32.keys[key] && !inst->win32.prev_keys[key]);
}
TW3D_API i32 twKeyReleased(TwInstance* inst, i32 key) {
    return (!inst->win32.keys[key] && inst->win32.prev_keys[key]);
}

TW3D_API void twSetCursorMode(TwInstance* inst, b32 locked) {
    if (inst->win32.cursor_locked == locked) return;

    inst->win32.cursor_locked = locked;
    if (locked) {
        while (ShowCursor(FALSE) >= 0);
    }
    else {
        while (ShowCursor(TRUE) < 0);
    }
}

TW3D_API i32 twGetMouseDX(TwInstance* inst) { return inst->win32.mouse_dx; }

TW3D_API i32 twGetMouseDY(TwInstance* inst) { return inst->win32.mouse_dy; }

TW3D_API i32 twGetMouseX(TwInstance* inst) { return inst->win32.mouse_x; }

TW3D_API i32 twGetMouseY(TwInstance* inst) { return inst->win32.mouse_y; }

TW3D_API i32 twMouseLeft(TwInstance* inst) { return inst->win32.mouse_left; }

TW3D_API i32 twMouseRight(TwInstance* inst) { return inst->win32.mouse_right; }

#pragma endregion

#pragma region MATH
TW3D_API Mat4 twIdMatrix() { return (Mat4) {.m[0] = 1,.m[5] = 1,.m[10] = 1,.m[15] = 1,}; }

TW3D_API Mat4 twPerspective(f64 fovy, f64 aspect, f64 znear, f64 zfar) {
    f32 f = 1.0 / tan(fovy * 0.5 * (3.14159f / 180.0));
    return (Mat4) {
        .m[0] = f / aspect,
        .m[5] = f,
        .m[10] = (zfar + znear) / (znear - zfar),
        .m[11] = -1,
        .m[14] = (2 * zfar * znear) / (znear - zfar),
        .m[15] = 0,
    };
}

static Vec3 Normalize(Vec3 a) {
    f32 mag = (f32)sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    if (mag == 0.0f) return (Vec3) { 0, 0, 0 };
    return (Vec3) { a.x / mag, a.y / mag, a.z / mag };
}

static Vec3 Cross(Vec3 a, Vec3 b) {
    return (Vec3) {
        a.y* b.z - a.z * b.y,
        a.z* b.x - a.x * b.z,
        a.x* b.y - a.y * b.x,
    };
}

static float Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

TW3D_API Mat4 twLookAt(Vec3 e, Vec3 c, Vec3 up) {
    Vec3 f = Normalize((Vec3) { c.x - e.x, c.y - e.y, c.z - e.z });
    Vec3 s = Normalize(Cross(f, up));
    Vec3 u = Cross(s, f);
    return (Mat4) {
        s.x, u.x, -f.x, 0,
        s.y, u.y, -f.y, 0,
        s.z, u.z, -f.z, 0,
        -Dot(s, e), -Dot(u, e), Dot(f, e), 1,
    };
}

TW3D_API Mat4 twTranslate(Vec3 pos) {
    return (Mat4) {
        .m[0] = 1,
        .m[5] = 1,
        .m[10] = 1,
        .m[12] = pos.x,
        .m[13] = pos.y,
        .m[14] = pos.z,
        .m[15] = 1,
    };
}

TW3D_API Mat4 twRotate(f32 angle, Vec3 axis) {
    float c = cosf(angle * 3.14159 / 180.0);
    f32 s = sinf(angle * 3.14159 / 180.0);
    f32 len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (len > 0.0f) { axis.x /= len; axis.y /= len; axis.z /= len; }
    f32 x = axis.x;
    f32 y = axis.y;
    f32 z = axis.z;
    return (Mat4) {
        .m[0] = x * x * (1 - c) + c,
        .m[1] = x * y * (1 - c) + z * s,
        .m[2] = x * z * (1 - c) - y * s,
        .m[4] = y * x * (1 - c) - z * s,
        .m[5] = y * y * (1 - c) + c,
        .m[6] = y * z * (1 - c) + x * s,
        .m[8] = x * z * (1 - c) + y * s,
        .m[9] = y * z * (1 - c) - x * s,
        .m[10] = z * z * (1 - c) + c,
        .m[15] = 1,
    };
}

TW3D_API Mat4 twScale(Vec3 scale) {
    return (Mat4) {
        .m[0] = scale.x,
        .m[5] = scale.y,
        .m[10] = scale.z,
        .m[15] = 1,
    };
}

TW3D_API Mat4 twMultMatrix(Mat4 a, Mat4 b) {
    Mat4 res = { 0 };
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            res.m[i * 4 + j] =
                a.m[i * 4 + 0] * b.m[0 * 4 + j] +
                a.m[i * 4 + 1] * b.m[1 * 4 + j] +
                a.m[i * 4 + 2] * b.m[2 * 4 + j] +
                a.m[i * 4 + 3] * b.m[3 * 4 + j];
        }
    }
    return res;
}

TW3D_API Mat4 twOrtho(f32 left, f32 right, f32 bottom, f32 top, f32 nval, f32 fval) {
    f32 A = 2 / (right - left);
    f32 B = 2 / (top - bottom);
    f32 C = -2 / (fval - nval);
    f32 tx = - (right + left) / (right - left);
    f32 ty = - (top + bottom) / (top - bottom);
    f32 tz = - (fval + nval) / (fval - nval);
    return (Mat4) {
        .m[0] = A,
        .m[5] = B,
        .m[10] = C,
        .m[12] = tx,
        .m[13] = ty,
        .m[14] = tz,
        .m[15] = 1,
    };
}
#pragma endregion
