#include <stdio.h>
#include "tw3d.h"

int main() {
    TwInstanceDesc desc = { 0 };
    desc.title = "Tower3D Test Window";
    desc.flag = TW_DESC_FLAG_FULLSCREEN;
    desc.exit_key = 27; // ESC
    TwInstance* inst = twCreateInstance(&desc);
    twInstanceAllocator(inst, TW_ALLOC_TEXPOOL, 16);
    twOpenWindow(inst);
    twSwapInterval(inst, 1);

    /* FEATURE & ENVIRONMENT */
    twSetFeatures(inst, TW_FEATURE_TEXTURE_2D | TW_FEATURE_FOG_AND_LIGHTING 
        | TW_FEATURE_LIGHTS | TW_FEATURE_NORMALIZE
        | TW_FEATURE_ALPHA_TEST | TW_FEATURE_COLOR_MATERIAL | TW_FEATURE_DEPTH_TEST);
    twSetFog(inst, &(TwVec3){0, 0, 0}, 0.2);
    twSetWorldLight(inst, &(TwVec3) { 0, 0, 0 });
    twSetLight(inst, TW_LIGHT0, &(TwVec3) { 0, 0, 1 }, &(TwVec3) { 1, 0.6, 0.2 }, &(TwVec3) { 0, 0, 0 });

    /* ASSET PIPELINE */
    int tex = twLoadTexture(inst, TW_NEAREST, TW_REPEAT, "tex.png");

    /* BUFFER */
    TwFloat data[] = {
        0.0, 1.0, 0.0f, 0.0f, 1.0f, -0.5f,  0.5f, 0.0f,
        0.0, 0.0, 0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f,
        1.0, 0.0, 0.0f, 0.0f, 1.0f,  0.5f, -0.5f, 0.0f,
        1.0, 1.0, 0.0f, 0.0f, 1.0f,  0.5f,  0.5f, 0.0f,
    };
    
    TwMat4 proj_mat, view_mat, model_mat, modelview_mat;
    float angle = 0.0f;

    twClearColor(inst, 0.2f, 0.3f, 0.3f, 1.0f);
    twMathPerspective(&proj_mat, 60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    twLoadProjectionMatrix(inst, &proj_mat);
    while (twIsWindowActive(inst)) {
        twResetGpuCallCount(inst);
        angle += 45.0f * twGetDeltaTime(inst);
        twClear(inst, TW_COLOR_BUFFER_BIT | TW_DEPTH_BUFFER_BIT);

        /* MATH & DISPATCH */
        twMathLookAt(&view_mat, &(TwVec3){ 0, 0, 2 }, &(TwVec3){ 0, 0, 0 }, &(TwVec3) { 0, 1, 0 });
        twMathTransform(&model_mat, &(TwVec3){0, 0, 0}, angle, &(TwVec3){0.0f, 1.0f, 0.0f}, &(TwVec3){1, 1, 1});
        twMathMultiply(&modelview_mat, &view_mat, &model_mat);
        twLoadModelViewMatrix(inst, &modelview_mat);

        /* DRAW */
        twBindTextureIndex(inst, tex);
        twInterleavedArrays(inst, TW_T2F_N3F_V3F, &data[0]);
        twDrawArrays(inst, TW_QUADS, 0, 4);

        twSwapBuffer(inst);
    }

    printf("GPU Call Count: %d\n", twGetGpuCallCount(inst));

    twCloseWindow(inst);
    twDeleteInstance(inst);

    return 0;
}