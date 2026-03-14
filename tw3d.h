/*
	Tower3D - Berat Dizdar
	Amaç:
	- Geliştiricelere platform abstraction layer vermek.
	- Sıfır render soyutlaması yaratmak.
	- Niyetim geliştiriciyi sadece kullanacağı grafik arayüzü ile başbaşa bırakmak

	Neden:
	- bkz. https://www.joelonsoftware.com/2002/11/11/the-law-of-leaky-abstractions/

	Surface Meselesi:
	- OpenGL: bindcontext
	- D3D: gethandler
	- Vulkan: getsurface
*/

#ifndef __tw3d_h__
#define __tw3d_h__

typedef signed char         i8;
typedef signed short        i16;
typedef signed int          i32;
typedef signed long long    i64;
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef float               f32;
typedef double              f64;
typedef int                 b32;
#define TW_TRUE (b32)1
#define TW_FALSE (b32)0
typedef struct Mat4 { f32 m[16]; } Mat4;
typedef struct Vec3 { f32 x, y, z; } Vec3;

typedef struct TwInstance TwInstance;

TwInstance* twCreateInstance();
void twDeleteInstance(TwInstance* inst);

i32 twCreateSurface(TwInstance* inst, const char* title);
i32 twSurfaceActive(TwInstance* inst);
void twGetSurfaceSize(TwInstance* inst, i32* width, i32* height);
void twUpdateSurface(TwInstance* inst);
void* twGetSurfaceHandler(TwInstance* inst); // d3d
void twGLBindContext(TwInstance* inst); // gl
void twGLSwapBuffers(TwInstance* inst); // gl
f32 twGetDeltaTime(TwInstance* inst);

i32 twKeyPressed(TwInstance* inst, i32 key);
i32 twKeyReleased(TwInstance* inst, i32 key);
i32 twKeyDown(TwInstance* inst, i32 key);
void twSetCursorMode(TwInstance* inst, b32 locked);
i32 twGetMouseDX(TwInstance* inst);
i32 twGetMouseDY(TwInstance* inst);
i32 twGetMouseX(TwInstance* inst);
i32 twGetMouseY(TwInstance* inst);
i32 twMouseLeft(TwInstance* inst);
i32 twMouseRight(TwInstance* inst);

Mat4 twIdMatrix();
Mat4 twPerspective(f64 fovy, f64 aspect, f64 znear, f64 zfar);
Mat4 twLookAt(Vec3 eye, Vec3 center, Vec3 up);
Mat4 twTranslate(Vec3 pos);
Mat4 twRotate(f32 angle, Vec3 axis);
Mat4 twScale(Vec3 scale);
Mat4 twMultMatrix(Mat4 a, Mat4 b);
Mat4 twOrtho(f32 left, f32 right, f32 bottom, f32 top, f32 nval, f32 fval);

#endif