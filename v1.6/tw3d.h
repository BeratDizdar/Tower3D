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

/* PLATFORM */

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

typedef struct Mat4 { float m[16]; } Mat4;
typedef struct Vec3 { float x, y, z; } Vec3;
typedef unsigned int TwID;

typedef struct TwInstance TwInstance;
typedef struct TwAPI {
	struct {
		bool_t (*Delete)(TwInstance* inst);
	} Instance;
	struct {
		bool_t (*Create)(TwInstance* inst, const char* title);
		bool_t (*Update)(TwInstance* inst);
		void   (*SetSwapIntervals)(TwInstance* inst, bool_t vsync);
		f32_t  (*GetDeltaTime)(TwInstance* inst);
		void   (*SwapBuffers)(TwInstance* inst);
	} Surface;
	struct {
		bool_t (*KeyPressed)(TwInstance* inst, i32_t key);
		bool_t (*KeyReleased)(TwInstance* inst, i32_t key);
		bool_t (*KeyDown)(TwInstance* inst, i32_t key);
		i32_t  (*GetMouseX)(TwInstance* inst);
		i32_t  (*GetMouseY)(TwInstance* inst);
		bool_t (*MouseLeft)(TwInstance* inst);
		bool_t (*MouseRight)(TwInstance* inst);
	} Input;
	struct {
		void (*BindToSurface)(TwInstance* inst, struct TwAPI* api);
		struct {
			void (* CreateTextures)(unsigned int target, int n, unsigned int* textures);
			void (* DeleteTextures)(int n, const unsigned int* textures);
			void (* TextureStorage2D)(unsigned int texture, int levels, unsigned int internalformat, int width, int height);
			void (* TextureSubImage2D)(unsigned int texture, int level, int xoffset, int yoffset, int width, int height, unsigned int format, unsigned int type, const void* pixels);
			void (* TextureParameteri)(unsigned int texture, unsigned int pname, int param);
			void (* BindTextureUnit)(unsigned int unit, unsigned int texture);
		} Texture;
		struct {
			void (* Clear)(unsigned int mask);
			void (* ClearColor)(float red, float green, float blue, float alpha);
			void (* ClearDepth)(double depth);
			void (* ClearStencil)(int s);
			void (* DrawBuffer)(unsigned int buf);
			void (* ReadBuffer)(unsigned int src);
			void (* ReadPixels)(int x, int y, int width, int height, unsigned int format, unsigned int type, void* pixels);
		} Render;
		struct {
			void (* CreateFramebuffers)(int n, unsigned int* framebuffers);
			void (* CreateRenderbuffers)(int n, unsigned int* renderbuffers);
			void (* DeleteFramebuffers)(int n, const unsigned int* framebuffers);
			void (* DeleteRenderbuffers)(int n, const unsigned int* renderbuffers);
			void (* BindFramebuffer)(unsigned int target, unsigned int framebuffer);
			void (* NamedRenderbufferStorage)(unsigned int renderbuffer, unsigned int internalformat, int width, int height);
			void (* NamedFramebufferTexture)(unsigned int framebuffer, unsigned int attachment, unsigned int texture, int level);
			void (* NamedFramebufferTextureLayer)(unsigned int framebuffer, unsigned int attachment, unsigned int texture, int level, int layer);
			void (* NamedFramebufferDrawBuffers)(unsigned int framebuffer, int n, const unsigned int* bufs);
			void (* BlitNamedFramebuffer)(unsigned int readFramebuffer, unsigned int drawFramebuffer, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, unsigned int mask, unsigned int filter);
			unsigned int (* CheckNamedFramebufferStatus)(unsigned int framebuffer, unsigned int target);
			void (* InvalidateNamedFramebufferData)(unsigned int framebuffer, int numAttachments, const unsigned int* attachments);
			void (* NamedFramebufferParameteri)(unsigned int framebuffer, unsigned int pname, int param);
			void (* NamedFramebufferRenderbuffer)(unsigned int framebuffer, unsigned int attachment, unsigned int renderbuffertarget, unsigned int renderbuffer);
			void (* GenerateTextureMipmap)(unsigned int texture);
		} FrameBuffer;
		struct {
			unsigned int (* CreateProgram)(void);
			unsigned int (* CreateShader)(unsigned int type);
			void (* ShaderBinary)(int count, const unsigned int* shaders, unsigned int binaryformat, const void* binary, int length);
			void (* ShaderSource)(unsigned int shader, int count, const char* const* string, const int* length);
			void (* CompileShader)(unsigned int shader);
			void (* AttachShader)(unsigned int program, unsigned int shader);
			void (* LinkProgram)(unsigned int program);
			void (* DetachShader)(unsigned int program, unsigned int shader);
			void (* UseProgram)(unsigned int program);
			void (* DeleteProgram)(unsigned int program);
			void (* DeleteShader)(unsigned int shader);
			void (* ProgramUniform1i)(unsigned int program, int location, int v0);
			void (* ProgramUniform1f)(unsigned int program, int location, float v0);
			void (* ProgramUniform3f)(unsigned int program, int location, float v0, float v1, float v2);
			void (* ProgramUniform4f)(unsigned int program, int location, float v0, float v1, float v2, float v3);
			void (* ProgramUniformMatrix4fv)(unsigned int program, int location, int count, unsigned char transpose, const float* value);
			void (* ProgramBinary)(unsigned int program, unsigned int binaryFormat, const void* binary, int length);
			void (* GetProgramBinary)(unsigned int program, int bufSize, int* length, unsigned int* binaryFormat, void* binary);
			void (* GetShaderiv)(unsigned int shader, unsigned int pname, int* params);
			void (* GetProgramiv)(unsigned int program, unsigned int pname, int* params);
			void (* GetShaderInfoLog)(unsigned int shader, int bufSize, int* length, char* infoLog);
			void (* GetProgramInfoLog)(unsigned int program, int bufSize, int* length, char* infoLog);
			void (* UseProgramStages)(unsigned int pipeline, unsigned int stages, unsigned int program);
			unsigned int (* CreateShaderProgramv)(unsigned int type, int count, const char* const* strings);
			void (* ProgramParameter)(unsigned int program, unsigned int pname, int value);
			void (* MemoryBarrier)(unsigned int barriers);
			void (* DispatchCompute)(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z);
			void (* DispatchComputeIndirect)(intptr_t indirect);
		} Shader;
		struct {
			void (* CreateBuffers)(int n, unsigned int* buffers);
			void (* CreateVertexArrays)(int n, unsigned int* arrays);
			void (* BindVertexArray)(unsigned int array);
			void (* DeleteBuffers)(int n, const unsigned int* buffers);
			void (* DeleteVertexArrays)(int n, const unsigned int* arrays);
			void (* BindBufferBase)(unsigned int target, unsigned int index, unsigned int buffer);
			void (* BindBufferRange)(unsigned int target, unsigned int index, unsigned int buffer, intptr_t offset, size_t size);
			void (* BindBuffersBase)(unsigned int target, unsigned int first, int count, const unsigned int* buffers);
			void (* BindBuffersRange)(unsigned int target, unsigned int first, int count, const unsigned int* buffers, const intptr_t* offsets, const size_t* sizes);
			void (* DrawElements)(unsigned int mode, int count, unsigned int type, const void* indices);
			void (* DrawElementsBaseVertex)(unsigned int mode, int count, unsigned int type, const void* indices, int basevertex);
			void (* MultiDrawElementsIndirect)(unsigned int mode, unsigned int type, const void* indirect, int drawcount, int stride);
			void (* NamedBufferStorage)(unsigned int buffer, size_t size, const void* data, unsigned int flags);
			void (* NamedBufferSubData)(unsigned int buffer, intptr_t offset, size_t size, const void* data);
			void (* CopyNamedBufferSubData)(unsigned int readBuffer, unsigned int writeBuffer, intptr_t readOffset, intptr_t writeOffset, size_t size);
			void (* ClearNamedBufferData)(unsigned int buffer, unsigned int internalformat, unsigned int format, unsigned int type, const void* data);
			void (* EnableVertexArrayAttrib)(unsigned int vaobj, unsigned int index);
			void (* DisableVertexArrayAttrib)(unsigned int vaobj, unsigned int index);
			void (* VertexArrayAttribFormat)(unsigned int vaobj, unsigned int attribindex, int size, unsigned int type, unsigned char normalized, unsigned int relativeoffset);
			void (* VertexArrayAttribBinding)(unsigned int vaobj, unsigned int attribindex, unsigned int bindingindex);
			void (* VertexArrayBindingDivisor)(unsigned int vaobj, unsigned int bindingindex, unsigned int divisor);
			void (* VertexArrayVertexBuffer)(unsigned int vaobj, unsigned int bindingindex, unsigned int buffer, intptr_t offset, int stride);
			void (* VertexArrayElementBuffer)(unsigned int vaobj, unsigned int buffer);
		} Buffer;
		struct {
			void (* Disable)(unsigned int cap);
			void (* Enable)(unsigned int cap);
			void (* Viewport)(int x, int y, int width, int height);
			void (* ColorMask)(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
			void (* FrontFace)(unsigned int mode);
			void (* CullFace)(unsigned int mode);
			void (* DepthFunc)(unsigned int func);
			void (* DepthMask)(unsigned char flag);
			void (* BlendFunc)(unsigned int sfactor, unsigned int dfactor);
			void (* BlendFuncSeparate)(unsigned int sfactorRGB, unsigned int dfactorRGB, unsigned int sfactorAlpha, unsigned int dfactorAlpha);
			void (* BlendColor)(float red, float green, float blue, float alpha);
			void (* BlendEquation)(unsigned int mode);
			void (* BlendEquationSeparate)(unsigned int modeRGB, unsigned int modeAlpha);
			void (* StencilFunc)(unsigned int func, int ref, unsigned int mask);
			void (* StencilFuncSeparate)(unsigned int face, unsigned int func, int ref, unsigned int mask);
			void (* StencilMask)(unsigned int mask);
			void (* StencilMaskSeparate)(unsigned int face, unsigned int mask);
			void (* StencilOp)(unsigned int fail, unsigned int zfail, unsigned int zpass);
			void (* StencilOpSeparate)(unsigned int face, unsigned int sfail, unsigned int dpfail, unsigned int dppass);
			void (* ClipControl)(unsigned int origin, unsigned int depth);
			void (* PixelStorei)(unsigned int pname, int param);
			void (* Scissor)(int x, int y, int width, int height);
			void (* PolygonOffset)(float factor, float units);
			void (* PolygonMode)(unsigned int face, unsigned int mode);
			void (* ViewportIndexedf)(unsigned int index, float x, float y, float w, float h);
			void (* ScissorIndexed)(unsigned int index, int left, int bottom, int width, int height);
			void (* DepthRangeIndexed)(unsigned int index, double n, double f);
		} State;
		struct {
			void (* CreateQueries)(unsigned int target, int n, unsigned int* ids);
			void (* QueryCounter)(unsigned int id, unsigned int target);
			void (* GetQueryObjectui64v)(unsigned int id, unsigned int pname, u64_t* params);
		} Quary;
		struct {
			void* (*FenceSync)(unsigned int condition, unsigned int flags);
			unsigned int (*ClientWaitSync)(void* sync, unsigned int flags, u64_t timeout);
			void (*DeleteSync)(void* sync);
		} Sync;
		struct {
			void (* CreateSamplers)(int n, unsigned int* samplers);
			void (* BindSampler)(unsigned int unit, unsigned int sampler);
			void (* BindSamplers)(unsigned int first, int count, const unsigned int* samplers);
			void (* SamplerParameteri)(unsigned int sampler, unsigned int pname, int param);
			void (* DeleteSamplers)(int count, const unsigned int* samplers);
		} Sampler;
		struct {
			void (* ActiveShaderProgram)(unsigned int pipeline, unsigned int program);
			void (* BindProgramPipeline)(unsigned int pipeline);
			void (* CreateProgramPipelines)(int n, unsigned int* pipelines);
			void (* DeleteProgramPipelines)(int n, const unsigned int* pipelines);
			void (* GetProgramPipelineInfoLog)(unsigned int pipeline, int bufSize, int* length, char* infoLog);
			void (* ValidateProgramPipeline)(unsigned int pipeline);
		} Pipeline;
		struct {
			void (* DebugMessageCallback)(void (* callback)(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* message, const void* userParam), const void* userParam);
			void (* ObjectPtrLabel)(const void* ptr, int length, const char* label);
			void (* ObjectLabel)(unsigned int identifier, unsigned int name, int length, const char* label);
			void (* PushDebugGroup)(unsigned int source, unsigned int id, int length, const char* message);
			void (* PopDebugGroup)(void);
		} Debug;
	} Context;
	struct {
		// TODO
	} Audio;
} TwAPI;

TwInstance* TwCreateInstance();
TwAPI* TwLoadVirtualTable();

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