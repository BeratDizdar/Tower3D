#include <stdio.h>
#include "tw3d.h"
#include <math.h>

int main() {
    TwInstanceDesc desc = { 0 };
    desc.title = "Tower3D Test Window";
    desc.flag = TW_DESC_FLAG_FULLSCREEN;
    desc.exit_key = 27; // ESC
    TwInstance* inst = twCreateInstance(&desc);
    twInstanceAllocator(inst, TW_ALLOC_TEXPOOL, 16);

    twOpenWindow(inst);
    twSwapInterval(inst, 1);
    twSetFeatures(inst, TW_FEATURE_TEXTURE_2D | TW_FEATURE_FOG_AND_LIGHTING 
        | TW_FEATURE_LIGHTS | TW_FEATURE_NORMALIZE
        | TW_FEATURE_ALPHA_TEST | TW_FEATURE_COLOR_MATERIAL | TW_FEATURE_DEPTH_TEST);
    twFogi(inst, TW_FOG_MODE, TW_EXP2);
    twHint(inst, TW_FOG_HINT, TW_FASTEST);
    twAlphaFunc(inst, TW_GREATER, 0.1f);
    twColorMaterial(inst, TW_FRONT, TW_AMBIENT_AND_DIFFUSE);
    //twCullFace(inst, TW_BACK);
    twFogfv(inst, TW_FOG_COLOR, (TwFloat[4]){ 0, 0, 0, 1 });
    twFogf(inst, TW_FOG_DENSITY, 0.4);
    twLightModelfv(inst, TW_LIGHT_MODEL_AMBIENT, (TwFloat[4]) { 0.1f, 0.1f, 0.1f, 1.0f });
    twLightfv(inst, TW_LIGHT0, TW_DIFFUSE, (TwFloat[4]) { 1, 0.6, 0.2, 1.0f });
    twLightfv(inst, TW_LIGHT0, TW_AMBIENT, (TwFloat[4]) { 0.05, 0.01, 0.01, 1.0f });
    twLightfv(inst, TW_LIGHT0, TW_POSITION, (TwFloat[4]) { 0, 1, 1, 1.0f });
    twLightf(inst, TW_LIGHT0, TW_CONSTANT_ATTENUATION, 1.0f);
    twLightf(inst, TW_LIGHT0, TW_LINEAR_ATTENUATION, 0.02f);
    twLightf(inst, TW_LIGHT0, TW_QUADRATIC_ATTENUATION, 0.08f);

    int tex = twLoadTexture(inst, TW_NEAREST, TW_REPEAT, "tex.png");

    TwFloat plane_complete[] = { // vert | uv | normal
        -0.5f,  0.5f, 0.0f, 0.0, 1.0, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0, 0.0, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 1.0, 0.0, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f, 1.0, 1.0, 0.0f, 0.0f, 1.0f,
    };

    twClearColor(inst, 0.2f, 0.3f, 0.3f, 1.0f);

    TwMat4 proj_mat, view_mat, model_mat, modelview_mat;
    float camz = 2.0f;
    float angle = 0.0f;
    twMathPerspective(&proj_mat, 60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    while (twIsWindowActive(inst)) {
        twResetGpuCallCount(inst);
        float dt = twGetDeltaTime(inst);
        if (twKeyDown(inst, 'S')) camz += 1 * dt;
        if (twKeyDown(inst, 'W')) camz -= 1 * dt;
        angle += 45.0f * dt;

        twClear(inst, TW_COLOR_BUFFER_BIT | TW_DEPTH_BUFFER_BIT);

        twLoadProjectionMatrix(inst, &proj_mat);
        twMathLookAt(&view_mat, &(TwVec3){ 0, 0, camz }, & (TwVec3) { 0, 0, 0 }, & (TwVec3) { 0, 1, 0 });
        twMathRotate(&model_mat, angle, &(TwVec3){ 0.5f, 1.0f, 0.5f });
        twMathMultiply(&modelview_mat, &view_mat, &model_mat);
        twLoadModelViewMatrix(inst, &modelview_mat);

        TwSizei stride = 8 * sizeof(TwFloat);
        twVertexPointer(inst, 3, TW_FLOAT, stride, &plane_complete[0]);
        twTexCoordPointer(inst, 2, TW_FLOAT, stride, &plane_complete[3]);
        twNormalPointer(inst, TW_FLOAT, stride, &plane_complete[5]);

        twEnableClientState(inst, TW_VERTEX_ARRAY);
        twEnableClientState(inst, TW_TEXTURE_COORD_ARRAY);
        twEnableClientState(inst, TW_NORMAL_ARRAY);

        twBindTextureIndex(inst, tex);
        twDrawArrays(inst, TW_QUADS, 0, 4);
        
        twDisableClientState(inst, TW_NORMAL_ARRAY);
        twDisableClientState(inst, TW_TEXTURE_COORD_ARRAY);
        twDisableClientState(inst, TW_VERTEX_ARRAY);

        twSwapBuffer(inst);
    }

    printf("GPU Call Count: %d\n", twGetGpuCallCount(inst));

    twCloseWindow(inst);
    twDeleteInstance(inst);

    return 0;
}