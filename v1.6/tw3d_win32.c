#include "tw3d.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef MemoryBarrier
#undef Clear
#undef Enable
#undef Disable
#include <gl/GL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    int (*wglSwapIntervalEXT)(int);
} TwInstance;

// #############################################################################

static void* TwGetGLProcAddress(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1)) {
        static HMODULE module = NULL;
        if (module == NULL) {
            module = GetModuleHandleA("opengl32.dll");
        }
        if (module != NULL) {
            p = (void*)GetProcAddress(module, name);
        }
    }
    return p;
}

static void __IMPL_LoadGLFunctions(TwAPI* api) {
    // --- TEXTURE ---
    api->Context.Texture.CreateTextures = (void*)TwGetGLProcAddress("glCreateTextures");
    api->Context.Texture.DeleteTextures = (void*)TwGetGLProcAddress("glDeleteTextures");
    api->Context.Texture.TextureStorage2D = (void*)TwGetGLProcAddress("glTextureStorage2D");
    api->Context.Texture.TextureSubImage2D = (void*)TwGetGLProcAddress("glTextureSubImage2D");
    api->Context.Texture.TextureParameteri = (void*)TwGetGLProcAddress("glTextureParameteri");
    api->Context.Texture.BindTextureUnit = (void*)TwGetGLProcAddress("glBindTextureUnit");

    // --- RENDER ---
    api->Context.Render.Clear = (void*)TwGetGLProcAddress("glClear");
    api->Context.Render.ClearColor = (void*)TwGetGLProcAddress("glClearColor");
    api->Context.Render.ClearDepth = (void*)TwGetGLProcAddress("glClearDepth");
    api->Context.Render.ClearStencil = (void*)TwGetGLProcAddress("glClearStencil");
    api->Context.Render.DrawBuffer = (void*)TwGetGLProcAddress("glDrawBuffer");
    api->Context.Render.ReadBuffer = (void*)TwGetGLProcAddress("glReadBuffer");
    api->Context.Render.ReadPixels = (void*)TwGetGLProcAddress("glReadPixels");

    // --- FRAMEBUFFER ---
    api->Context.FrameBuffer.CreateFramebuffers = (void*)TwGetGLProcAddress("glCreateFramebuffers");
    api->Context.FrameBuffer.CreateRenderbuffers = (void*)TwGetGLProcAddress("glCreateRenderbuffers");
    api->Context.FrameBuffer.DeleteFramebuffers = (void*)TwGetGLProcAddress("glDeleteFramebuffers");
    api->Context.FrameBuffer.DeleteRenderbuffers = (void*)TwGetGLProcAddress("glDeleteRenderbuffers");
    api->Context.FrameBuffer.BindFramebuffer = (void*)TwGetGLProcAddress("glBindFramebuffer");
    api->Context.FrameBuffer.NamedRenderbufferStorage = (void*)TwGetGLProcAddress("glNamedRenderbufferStorage");
    api->Context.FrameBuffer.NamedFramebufferTexture = (void*)TwGetGLProcAddress("glNamedFramebufferTexture");
    api->Context.FrameBuffer.NamedFramebufferTextureLayer = (void*)TwGetGLProcAddress("glNamedFramebufferTextureLayer");
    api->Context.FrameBuffer.NamedFramebufferDrawBuffers = (void*)TwGetGLProcAddress("glNamedFramebufferDrawBuffers");
    api->Context.FrameBuffer.BlitNamedFramebuffer = (void*)TwGetGLProcAddress("glBlitNamedFramebuffer");
    api->Context.FrameBuffer.CheckNamedFramebufferStatus = (void*)TwGetGLProcAddress("glCheckNamedFramebufferStatus");
    api->Context.FrameBuffer.InvalidateNamedFramebufferData = (void*)TwGetGLProcAddress("glInvalidateNamedFramebufferData");
    api->Context.FrameBuffer.NamedFramebufferParameteri = (void*)TwGetGLProcAddress("glNamedFramebufferParameteri");
    api->Context.FrameBuffer.NamedFramebufferRenderbuffer = (void*)TwGetGLProcAddress("glNamedFramebufferRenderbuffer");
    api->Context.FrameBuffer.GenerateTextureMipmap = (void*)TwGetGLProcAddress("glGenerateTextureMipmap");

    // --- SHADER ---
    api->Context.Shader.CreateProgram = (void*)TwGetGLProcAddress("glCreateProgram");
    api->Context.Shader.CreateShader = (void*)TwGetGLProcAddress("glCreateShader");
    api->Context.Shader.ShaderBinary = (void*)TwGetGLProcAddress("glShaderBinary");
    api->Context.Shader.ShaderSource = (void*)TwGetGLProcAddress("glShaderSource");
    api->Context.Shader.CompileShader = (void*)TwGetGLProcAddress("glCompileShader");
    api->Context.Shader.AttachShader = (void*)TwGetGLProcAddress("glAttachShader");
    api->Context.Shader.LinkProgram = (void*)TwGetGLProcAddress("glLinkProgram");
    api->Context.Shader.DetachShader = (void*)TwGetGLProcAddress("glDetachShader");
    api->Context.Shader.UseProgram = (void*)TwGetGLProcAddress("glUseProgram");
    api->Context.Shader.DeleteProgram = (void*)TwGetGLProcAddress("glDeleteProgram");
    api->Context.Shader.DeleteShader = (void*)TwGetGLProcAddress("glDeleteShader");
    api->Context.Shader.ProgramUniform1i = (void*)TwGetGLProcAddress("glProgramUniform1i");
    api->Context.Shader.ProgramUniform1f = (void*)TwGetGLProcAddress("glProgramUniform1f");
    api->Context.Shader.ProgramUniform3f = (void*)TwGetGLProcAddress("glProgramUniform3f");
    api->Context.Shader.ProgramUniform4f = (void*)TwGetGLProcAddress("glProgramUniform4f");
    api->Context.Shader.ProgramUniformMatrix4fv = (void*)TwGetGLProcAddress("glProgramUniformMatrix4fv");
    api->Context.Shader.ProgramBinary = (void*)TwGetGLProcAddress("glProgramBinary");
    api->Context.Shader.GetProgramBinary = (void*)TwGetGLProcAddress("glGetProgramBinary");
    api->Context.Shader.GetShaderiv = (void*)TwGetGLProcAddress("glGetShaderiv");
    api->Context.Shader.GetProgramiv = (void*)TwGetGLProcAddress("glGetProgramiv");
    api->Context.Shader.GetShaderInfoLog = (void*)TwGetGLProcAddress("glGetShaderInfoLog");
    api->Context.Shader.GetProgramInfoLog = (void*)TwGetGLProcAddress("glGetProgramInfoLog");
    api->Context.Shader.UseProgramStages = (void*)TwGetGLProcAddress("glUseProgramStages");
    api->Context.Shader.CreateShaderProgramv = (void*)TwGetGLProcAddress("glCreateShaderProgramv");
    api->Context.Shader.ProgramParameter = (void*)TwGetGLProcAddress("glProgramParameter");
    api->Context.Shader.MemoryBarrier = (void*)TwGetGLProcAddress("glMemoryBarrier");
    api->Context.Shader.DispatchCompute = (void*)TwGetGLProcAddress("glDispatchCompute");
    api->Context.Shader.DispatchComputeIndirect = (void*)TwGetGLProcAddress("glDispatchComputeIndirect");

    // --- BUFFER ---
    api->Context.Buffer.CreateBuffers = (void*)TwGetGLProcAddress("glCreateBuffers");
    api->Context.Buffer.CreateVertexArrays = (void*)TwGetGLProcAddress("glCreateVertexArrays");
    api->Context.Buffer.BindVertexArray = (void*)TwGetGLProcAddress("glBindVertexArray");
    api->Context.Buffer.DeleteBuffers = (void*)TwGetGLProcAddress("glDeleteBuffers");
    api->Context.Buffer.DeleteVertexArrays = (void*)TwGetGLProcAddress("glDeleteVertexArrays");
    api->Context.Buffer.BindBufferBase = (void*)TwGetGLProcAddress("glBindBufferBase");
    api->Context.Buffer.BindBufferRange = (void*)TwGetGLProcAddress("glBindBufferRange");
    api->Context.Buffer.BindBuffersBase = (void*)TwGetGLProcAddress("glBindBuffersBase");
    api->Context.Buffer.BindBuffersRange = (void*)TwGetGLProcAddress("glBindBuffersRange");
    api->Context.Buffer.DrawElements = (void*)TwGetGLProcAddress("glDrawElements");
    api->Context.Buffer.DrawElementsBaseVertex = (void*)TwGetGLProcAddress("glDrawElementsBaseVertex");
    api->Context.Buffer.MultiDrawElementsIndirect = (void*)TwGetGLProcAddress("glMultiDrawElementsIndirect");
    api->Context.Buffer.NamedBufferStorage = (void*)TwGetGLProcAddress("glNamedBufferStorage");
    api->Context.Buffer.NamedBufferSubData = (void*)TwGetGLProcAddress("glNamedBufferSubData");
    api->Context.Buffer.CopyNamedBufferSubData = (void*)TwGetGLProcAddress("glCopyNamedBufferSubData");
    api->Context.Buffer.ClearNamedBufferData = (void*)TwGetGLProcAddress("glClearNamedBufferData");
    api->Context.Buffer.EnableVertexArrayAttrib = (void*)TwGetGLProcAddress("glEnableVertexArrayAttrib");
    api->Context.Buffer.DisableVertexArrayAttrib = (void*)TwGetGLProcAddress("glDisableVertexArrayAttrib");
    api->Context.Buffer.VertexArrayAttribFormat = (void*)TwGetGLProcAddress("glVertexArrayAttribFormat");
    api->Context.Buffer.VertexArrayAttribBinding = (void*)TwGetGLProcAddress("glVertexArrayAttribBinding");
    api->Context.Buffer.VertexArrayBindingDivisor = (void*)TwGetGLProcAddress("glVertexArrayBindingDivisor");
    api->Context.Buffer.VertexArrayVertexBuffer = (void*)TwGetGLProcAddress("glVertexArrayVertexBuffer");
    api->Context.Buffer.VertexArrayElementBuffer = (void*)TwGetGLProcAddress("glVertexArrayElementBuffer");

    // --- STATE ---
    api->Context.State.Disable = (void*)TwGetGLProcAddress("glDisable");
    api->Context.State.Enable = (void*)TwGetGLProcAddress("glEnable");
    api->Context.State.Viewport = (void*)TwGetGLProcAddress("glViewport");
    api->Context.State.ColorMask = (void*)TwGetGLProcAddress("glColorMask");
    api->Context.State.FrontFace = (void*)TwGetGLProcAddress("glFrontFace");
    api->Context.State.CullFace = (void*)TwGetGLProcAddress("glCullFace");
    api->Context.State.DepthFunc = (void*)TwGetGLProcAddress("glDepthFunc");
    api->Context.State.DepthMask = (void*)TwGetGLProcAddress("glDepthMask");
    api->Context.State.BlendFunc = (void*)TwGetGLProcAddress("glBlendFunc");
    api->Context.State.BlendFuncSeparate = (void*)TwGetGLProcAddress("glBlendFuncSeparate");
    api->Context.State.BlendColor = (void*)TwGetGLProcAddress("glBlendColor");
    api->Context.State.BlendEquation = (void*)TwGetGLProcAddress("glBlendEquation");
    api->Context.State.BlendEquationSeparate = (void*)TwGetGLProcAddress("glBlendEquationSeparate");
    api->Context.State.StencilFunc = (void*)TwGetGLProcAddress("glStencilFunc");
    api->Context.State.StencilFuncSeparate = (void*)TwGetGLProcAddress("glStencilFuncSeparate");
    api->Context.State.StencilMask = (void*)TwGetGLProcAddress("glStencilMask");
    api->Context.State.StencilMaskSeparate = (void*)TwGetGLProcAddress("glStencilMaskSeparate");
    api->Context.State.StencilOp = (void*)TwGetGLProcAddress("glStencilOp");
    api->Context.State.StencilOpSeparate = (void*)TwGetGLProcAddress("glStencilOpSeparate");
    api->Context.State.ClipControl = (void*)TwGetGLProcAddress("glClipControl");
    api->Context.State.PixelStorei = (void*)TwGetGLProcAddress("glPixelStorei");
    api->Context.State.Scissor = (void*)TwGetGLProcAddress("glScissor");
    api->Context.State.PolygonOffset = (void*)TwGetGLProcAddress("glPolygonOffset");
    api->Context.State.PolygonMode = (void*)TwGetGLProcAddress("glPolygonMode");
    api->Context.State.ViewportIndexedf = (void*)TwGetGLProcAddress("glViewportIndexedf");
    api->Context.State.ScissorIndexed = (void*)TwGetGLProcAddress("glScissorIndexed");
    api->Context.State.DepthRangeIndexed = (void*)TwGetGLProcAddress("glDepthRangeIndexed");

    // --- QUERY (Quary) ---
    api->Context.Quary.CreateQueries = (void*)TwGetGLProcAddress("glCreateQueries");
    api->Context.Quary.QueryCounter = (void*)TwGetGLProcAddress("glQueryCounter");
    api->Context.Quary.GetQueryObjectui64v = (void*)TwGetGLProcAddress("glGetQueryObjectui64v");

    // --- SYNC ---
    api->Context.Sync.FenceSync = (void*)TwGetGLProcAddress("glFenceSync");
    api->Context.Sync.ClientWaitSync = (void*)TwGetGLProcAddress("glClientWaitSync");
    api->Context.Sync.DeleteSync = (void*)TwGetGLProcAddress("glDeleteSync");

    // --- SAMPLER ---
    api->Context.Sampler.CreateSamplers = (void*)TwGetGLProcAddress("glCreateSamplers");
    api->Context.Sampler.BindSampler = (void*)TwGetGLProcAddress("glBindSampler");
    api->Context.Sampler.BindSamplers = (void*)TwGetGLProcAddress("glBindSamplers");
    api->Context.Sampler.SamplerParameteri = (void*)TwGetGLProcAddress("glSamplerParameteri");
    api->Context.Sampler.DeleteSamplers = (void*)TwGetGLProcAddress("glDeleteSamplers");

    // --- PIPELINE ---
    api->Context.Pipeline.ActiveShaderProgram = (void*)TwGetGLProcAddress("glActiveShaderProgram");
    api->Context.Pipeline.BindProgramPipeline = (void*)TwGetGLProcAddress("glBindProgramPipeline");
    api->Context.Pipeline.CreateProgramPipelines = (void*)TwGetGLProcAddress("glCreateProgramPipelines");
    api->Context.Pipeline.DeleteProgramPipelines = (void*)TwGetGLProcAddress("glDeleteProgramPipelines");
    api->Context.Pipeline.GetProgramPipelineInfoLog = (void*)TwGetGLProcAddress("glGetProgramPipelineInfoLog");
    api->Context.Pipeline.ValidateProgramPipeline = (void*)TwGetGLProcAddress("glValidateProgramPipeline");

    // --- DEBUG ---
    api->Context.Debug.DebugMessageCallback = (void*)TwGetGLProcAddress("glDebugMessageCallback");
    api->Context.Debug.ObjectPtrLabel = (void*)TwGetGLProcAddress("glObjectPtrLabel");
    api->Context.Debug.ObjectLabel = (void*)TwGetGLProcAddress("glObjectLabel");
    api->Context.Debug.PushDebugGroup = (void*)TwGetGLProcAddress("glPushDebugGroup");
    api->Context.Debug.PopDebugGroup = (void*)TwGetGLProcAddress("glPopDebugGroup");
}

// #############################################################################

static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
    switch (umessage) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    default: return DefWindowProc(hwnd, umessage, wparam, lparam);
    }
}
static bool_t __IMPL_Delete(TwInstance* inst) {
    if (!inst) return TW_FALSE;
    if (inst->win32.context) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(inst->win32.context);
    }
    if (inst->win32.handler && inst->win32.device) {
        ReleaseDC(inst->win32.handler, inst->win32.device);
        DestroyWindow(inst->win32.handler);
    }
    free(inst);
    return TW_TRUE;
}
static bool_t __IMPL_Create(TwInstance* inst, const char* title) {
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
static bool_t __IMPL_Update(TwInstance* inst) {
    return inst->win32.is_active;
}
static void   __IMPL_SetSwapIntervals(TwInstance* inst, bool_t vsync) {
    if (inst->wglSwapIntervalEXT != NULL) {
        inst->wglSwapIntervalEXT(vsync);
    }
    else {
        printf("UYARI: Ekran kartin V-Sync (wglSwapIntervalEXT) desteklemiyor.\n");
    }
}
static f32_t  __IMPL_GetDeltaTime(TwInstance* inst) {
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    float delta = (float)(current_time.QuadPart - inst->win32.last_time.QuadPart) / (float)inst->win32.timer_freq.QuadPart;
    inst->win32.last_time = current_time;
    if (delta > 0.1f) delta = 0.1f;
    return delta;
}
static void   __IMPL_SwapBuffers(TwInstance* inst) {
    SwapBuffers(inst->win32.device);
    MSG msg = { 0 };
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) inst->win32.is_active = TW_FALSE;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    if (GetAsyncKeyState(inst->win32.exit_key)) inst->win32.is_active = TW_FALSE;
}
//static bool_t __IMPL_KeyPressed (TwInstance* inst, i32_t key);
//static bool_t __IMPL_KeyReleased (TwInstance* inst, i32_t key);
//static bool_t __IMPL_KeyDown (TwInstance* inst, i32_t key);
//static i32_t  __IMPL_GetMouseX (TwInstance* inst);
//static i32_t  __IMPL_GetMouseY (TwInstance* inst);
//static bool_t __IMPL_MouseLeft (TwInstance* inst);
//static bool_t __IMPL_MouseRight (TwInstance* inst);
static void   __IMPL_BindToSurface(TwInstance* inst, TwAPI* api) {
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

    inst->wglSwapIntervalEXT = (int(*)(int))TwGetGLProcAddress("wglSwapIntervalEXT");

    static bool_t is_gl_loaded = TW_FALSE;
    if (!is_gl_loaded) {
        __IMPL_LoadGLFunctions(api);
        is_gl_loaded = TW_TRUE;
    }
}

// #############################################################################

static void __LoadImplFunctions(TwAPI* api) {
    api->Instance.Delete = __IMPL_Delete;
    api->Surface.Create = __IMPL_Create;
    api->Surface.Update = __IMPL_Update;
    api->Surface.SetSwapIntervals = __IMPL_SetSwapIntervals;
    api->Surface.GetDeltaTime = __IMPL_GetDeltaTime;
    api->Surface.SwapBuffers = __IMPL_SwapBuffers;
    //api->Input.KeyPressed = __IMPL_KeyPressed;
    //api->Input.KeyReleased = __IMPL_KeyReleased;
    //api->Input.KeyDown = __IMPL_KeyDown;
    //api->Input.GetMouseX = __IMPL_GetMouseX;
    //api->Input.GetMouseY = __IMPL_GetMouseY;
    //api->Input.MouseLeft = __IMPL_MouseLeft;
    //api->Input.MouseRight = __IMPL_MouseRight;
    api->Context.BindToSurface = __IMPL_BindToSurface;
}

// #############################################################################

TW3D_API TwInstance* TwCreateInstance() {
    TwInstance* inst = (TwInstance*)calloc(1, sizeof(TwInstance));
    return inst;
}
TW3D_API TwAPI* TwLoadVirtualTable() {
    static TwAPI api = { 0 };
    static bool_t is_impl_loaded = TW_FALSE;
    if (!is_impl_loaded) {
        __LoadImplFunctions(&api);
    }
    return &api;
}

// #############################################################################

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
