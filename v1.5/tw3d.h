/*
	Tower 3D v1.5

	Dizayn Kaynakları:
	- https://hexdocs.pm/graphmath/api-reference.html
	- https://docs.gl
	- https://docs.vulkan.org/spec/latest/index.html
	- https://www.cs.cmu.edu/afs/cs/academic/class/15462-f10/www/lec_slides/glslref.pdf

*/

#ifndef __tw3d_h__
#define __tw3d_h__

typedef struct TwInstance TwInstance;
typedef struct Mat4 { float m[16]; } Mat4;
typedef struct Vec3 { float x,y,z; } Vec3;
typedef unsigned int TwID;
typedef int TwBool;
#define TW_TRUE (TwBool)1
#define TW_FALSE (TwBool)0

/* MATH */
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

/* INSTANCE */
TwInstance* twCreateInstance();
void twClearTempPools(TwInstance* inst);
void twDeleteInstance(TwInstance* inst);

/* SURFACE */
TwBool twCreateSurfaceInMain(TwInstance* inst, const char* name);
void twSetSwapIntervals(TwInstance* inst, TwBool bVsync);
TwBool twUpdateSurface(TwInstance* inst);
float twGetDeltaTime(TwInstance* inst);
void twSwapBuffers(TwInstance* inst);

/* INPUT */
TwBool twKeyPressed(TwInstance* inst, int key);
TwBool twKeyReleased(TwInstance* inst, int key);
TwBool twKeyDown(TwInstance* inst, int key);
int twGetMousePos(TwInstance* inst);
TwBool twMouseLeft(TwInstance* inst);
TwBool twMouseRight(TwInstance* inst);

/* RENDER */
void twBindContextToSurface(TwInstance* inst);
void twSetContextViewport(TwInstance* inst, int x, int y, int w, int h);
void twClearColorAndDepth(TwInstance* inst, float r, float g, float b);
void twDrawCommands(TwInstance* inst);
	void twBindRule(TwInstance* inst, TwID rule);
		TwID twCreateRuleInMainPool(TwInstance* inst, TwBool depth, TwBool bfcull, TwBool alpha);
		TwID twCreateRuleInTempPool(TwInstance* inst, TwBool depth, TwBool bfcull, TwBool alpha);
	void twBindTexture(TwInstance* inst, TwID texture);
		TwID twLoadTextureToMainPool(TwInstance* inst, const char* filepath);
		TwID twLoadTextureToTempPool(TwInstance* inst, const char* filepath);
	void twBindBuffer(TwInstance* inst, TwID buffer);
		TwID twLoadBufferToMainPool(TwInstance* inst, float* verticies, int vcount, unsigned int* indicies, int icount);
		TwID twLoadBufferToTempPool(TwInstance* inst, float* verticies, int vcount, unsigned int* indicies, int icount);
	void twBindShader(TwInstance* inst, TwID shader);
		TwID twLoadShaderInMainPool(TwInstance* inst, const char* vshader, const char* fshader);
		int twGetUniformLocation(TwInstance* inst, TwID shader, const char* var);
		void twSendUniformMat4(TwInstance* inst, TwID shader, int location, Mat4 val);
		void twSendUniformVec3(TwInstance* inst, TwID shader, int location, Vec3 val);
		void twSendUniformFloat(TwInstance* inst, TwID shader, int location, float val);

#endif // __tw3d_h__