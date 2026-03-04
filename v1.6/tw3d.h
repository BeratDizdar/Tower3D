/*
	Tower 3D v1.6 - Berat Dizdar
	- Approaching Zero Driver Overhead

	Akılda Bulunmalı:
	- https://www.slideshare.net/slideshow/approaching-zero-driver-overhead/32554457#3
	- https://www.slideshare.net/slideshow/opengl-45-update-for-nvidia-gpus/37922750
	- https://www.slideshare.net/slideshow/s4379-opengl44scenerenderingtechniques/43492203#2
	- https://www.slideshare.net/slideshow/optimizing-the-graphics-pipeline-with-compute-gdc-2016/59747720
	- https://www.slideshare.net/slideshow/nvidia-opengl-in-2016/66696151
	- https://www.gamingonlinux.com/2014/04/the-gdc-video-on-approaching-zero-driver-overhead-in-opengl-is-now-up/
	- https://www.khronos.org/news/permalink/glnext-the-future-of-high-performance-graphics-presented-by-valve-at-gdc
	- https://www.lunarg.com/wp-content/uploads/2023/05/SPIRV-Osaka-MAY2023.pdf
	- https://community.khronos.org/t/doom-3/37313/3
	- https://doomwiki.org/wiki/Tiago_Sousa
	- https://wikis.khronos.org/opengl/Post_Transform_Cache
	- https://wikis.khronos.org/opengl/Bindless_Texture
	- https://wikis.khronos.org/opengl/Direct_State_Access
	- https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions
	- https://caseymuratori.com/blog_0015
	- https://zackoverflow.dev/writing/premature-abstraction/

	Kaynaklar:
	- https://hexdocs.pm/graphmath/api-reference.html
	- https://docs.gl
	- https://registry.khronos.org/OpenGL/api/GL/glcorearb.h
	- https://wikis.khronos.org/opengl/Rendering_Pipeline_Overview
	- https://vkguide.dev/
	- https://docs.vulkan.org/spec/latest/index.html
	- https://docs.vulkan.org/tutorial/latest/00_Introduction.html
	- https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/introduction.html
	- https://docs.vulkan.org/guide/latest/index.html
	- https://docs.vulkan.org/refpages/latest/refpages/index.html
	- https://github.com/KhronosGroup/SPIRV-Headers/blob/main/include/spirv/1.0/spirv.h
	- https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html
	- https://shader-slang.org/docs/
	- https://iquilezles.org
	- https://steamcdn-a.akamaihd.net/apps/valve/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf
	- https://www.redblobgames.com/articles/sdf-fonts/
	- https://wikis.khronos.org/opengl/Shader_Compilation#Program_pipelines
	- https://wikis.khronos.org/opengl/Vertex_Specification
	- https://ktstephano.github.io

	Altın Kurallar:
	- Tek başına yetecek bir API olacak.
	- Kendi ihtiyacıma göre yazıyorum.
	- Gereksiz şeyleri almam, gerekirse budarım.
	- Mantıklı bir mimari kurmalıyım.
	- Modern özellikler eklemeliyim ama gerekliyse.

	Yardımcı Araçlar:
	- Çekirdek API: OpenGL 4.6
	- Shader Dili: Slang / (GLSL şimdilik)
	- TODO: Texture Yükleme: stb_image.h (https://github.com/nothings/stb/blob/master/stb_image.h)
	- TODO: Model Yükleme: cgltf.h (https://github.com/jkuhlmann/cgltf)
	- TODO: Ses Motoru: miniaudio (https://github.com/mackron/miniaudio)
	- Matematik: Kendim yaparım.
	- Derleyici (x86_64): TCC / [GCC & MSVC]

*/
#ifndef __tw3d_h_v1_6__
#define __tw3d_h_v1_6__

#pragma region PLATFORM-DEFS
typedef signed char         i8_t;
typedef signed short        i16_t;
typedef signed int          i32_t;
typedef signed long long    i64_t;
typedef unsigned char       u8_t;
typedef unsigned short      u16_t;
typedef unsigned int        u32_t;
typedef unsigned long long  u64_t;
typedef float               f32_t;
typedef double              f64_t;
typedef u64_t               size_t;
typedef i64_t               intptr_t;
typedef u64_t               uintptr_t;
typedef i32_t               bool_t;
#define TW_TRUE             (bool_t)1
#define TW_FALSE            (bool_t)0
#pragma endregion

typedef struct Mat4 { float m[16]; } Mat4;
typedef struct Vec3 { float x, y, z; } Vec3;
typedef unsigned int TwID;

typedef struct TwInstance TwInstance;

TwInstance* TwCreateInstance();
void TwDeleteInstance(TwInstance* inst);

bool_t TwCreateSurface(TwInstance* inst, const char* title);
bool_t TwUpdateSurface(TwInstance* inst);
f32_t TwGetDeltaTime(TwInstance* inst);
void TwSwapBuffers(TwInstance* inst);
void TwBindContextToSurface(TwInstance* inst, bool_t vsync);

/*
bool_t (*KeyPressed)(TwInstance* inst, i32_t key);
bool_t (*KeyReleased)(TwInstance* inst, i32_t key);
bool_t (*KeyDown)(TwInstance* inst, i32_t key);
i32_t  (*GetMouseX)(TwInstance* inst);
i32_t  (*GetMouseY)(TwInstance* inst);
bool_t (*MouseLeft)(TwInstance* inst);
bool_t (*MouseRight)(TwInstance* inst);
*/

Mat4 twMatIdentity();
Mat4 twMatZero();
Mat4 twMatTranslate(Vec3 v);
Mat4 twMatRotateX(float angle);
Mat4 twMatRotateY(float angle);
Mat4 twMatRotateZ(float angle);
Mat4 twMatScale(float k);
Mat4 twMatTransform(Mat4 a, Vec3 pos, float angle, Vec3 axis, Vec3 scale);
Mat4 twMatAdd(Mat4 a, Mat4 b);
Mat4 twMatSub(Mat4 a, Mat4 b);
Mat4 twMatMultiply(Mat4 a, Mat4 b);
Mat4 twMatPerspective(float fov, float aspect, float znear, float zfar);
Mat4 twMatLookAt(Vec3 eye, Vec3 center, Vec3 up);
Mat4 twMatOrtho(float left, float right, float bottom, float top, float znear, float zfar);
Mat4 twMatRound(Mat4 a);
Vec3 twVec3Add(Vec3 a, Vec3 b);
Vec3 twVec3Sub(Vec3 a, Vec3 b);
Vec3 twVec3Normalize(Vec3 a);
Vec3 twVec3Cross(Vec3 a, Vec3 b);
float twMatAt(Mat4 a, unsigned int r, unsigned int c);
float twVec3Dot(Vec3 a, Vec3 b);
float twVec3DistanceCheby(Vec3 a, Vec3 b);
float twVec3DistanceMinkow(Vec3 a, Vec3 b, float p);
float twLerp(float v0, float v1, float t);
float twSquareRoot(float n);
float twSin(float a);
float twCos(float a);
float twRandom(float min, float max);

#endif // __tw3d_h_v1_6__