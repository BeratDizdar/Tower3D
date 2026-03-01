#include <stdio.h>
#include "tw3d.h"

int main() {
    const char* arb_shader =
        "!!ARBfp1.0\n"
        "TEMP wall_c, detail_c, new_wall;\n"
        "TEX wall_c, fragment.texcoord[0], texture[0], 2D;\n"
        "TEX detail_c, fragment.texcoord[0], texture[1], 2D;\n"
        "MUL new_wall, detail_c, {1.0, 0.2, 0.6, 1.0};\n"
        "MUL result.color, new_wall, wall_c;\n"
        "END";

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
    twSetFog(inst, (TwFloat[]){0, 0, 0}, 0.2);
    twSetWorldLight(inst, (TwFloat[]) { 0, 0, 0, 1 });
    twSetLight(inst, TW_LIGHT0, (TwFloat[]) { 0, 0, 1, 1 }, (TwFloat[]) { 1, 0.6, 0.2, 1 }, (TwFloat[]) { 0, 0, 0, 1 });

    /* ASSET PIPELINE */
    int tex1 = twLoadTexture(inst, TW_NEAREST, TW_REPEAT, "snow.png");
    int tex2 = twLoadTexture(inst, TW_NEAREST, TW_REPEAT, "tex.png");

    /* SHADER */
    TwID shader_id = twLoadShaderARB(inst, TW_FRAGMENT_PROGRAM_ARB, arb_shader);

    /* BUFFER */
    TwFloat data[] = {
        0.0, 1.0, 0.0f, 0.0f, 1.0f, -0.5f,  0.5f, 0.0f,
        0.0, 0.0, 0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f,
        1.0, 0.0, 0.0f, 0.0f, 1.0f,  0.5f, -0.5f, 0.0f,
        1.0, 1.0, 0.0f, 0.0f, 1.0f,  0.5f,  0.5f, 0.0f,
    };
    TwMat4 view_mat, model_mat;
    float angle = 0.0f;
    twClearColor(inst, 0.2f, 0.3f, 0.3f, 1.0f);
    twInitCamera(inst, 60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    while (twIsWindowActive(inst)) {
        twResetGpuCallCount(inst);
        angle += 45.0f * twGetDeltaTime(inst);
        twClear(inst, TW_COLOR_BUFFER_BIT | TW_DEPTH_BUFFER_BIT);

        /* MATH & DISPATCH */
        twCameraLookAt(inst, (TwFloat[]) { 0, 0, 2 }, (TwFloat[]) { 0, 0, 0 }, (TwFloat[]) { 0, 1, 0 });
        twCameraView(inst, &view_mat);
        twMathTransform(&model_mat, &(TwVec3){0, 0, 0}, angle, &(TwVec3){0.0f, 1.0f, 0.0f}, &(TwVec3){1, 1, 1});
        twLoadModelViewMatrix(inst, &view_mat, &model_mat);

        /* DRAW */
        twBindShaderARB(inst, TW_FRAGMENT_PROGRAM_ARB, shader_id);
        twActiveTextureARB(inst, TW_TEXTURE1_ARB);
        twBindTextureIndex(inst, tex2);
        twActiveTextureARB(inst, TW_TEXTURE0_ARB);

        twDrawMesh(inst, tex1, TW_T2F_N3F_V3F, TW_QUADS, 4, &data[0]);
        twUnbindShaderARB(inst, TW_FRAGMENT_PROGRAM_ARB);

        twSwapBuffer(inst);
    }

    printf("GPU Call Count: %d\n", twGetGpuCallCount(inst));

    twDeleteShaderARB(inst, 1, &shader_id);
    twCloseWindow(inst);
    twDeleteInstance(inst);

    return 0;
}