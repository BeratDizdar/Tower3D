#include "tw3d.h"
#include <stdio.h>
#include <math.h>

int main() {
    TwInstanceDesc desc = { 0 };
    desc.title = "Tower3D Test Window";
    desc.flag = TW_DESC_FLAG_FULLSCREEN;
    desc.exit_key = 27; // ESC
    TwInstance* inst = twCreateInstance(&desc);
    twInstanceAllocator(inst, TW_ALLOC_POOL_TEX, 16);
    twInstanceAllocator(inst, TW_ALLOC_POOL_SHADER, 16);
    twInstanceAllocator(inst, TW_ALLOC_POOL_MODEL, 16);
    twOpenWindow(inst);
    twSwapInterval(inst, 1);

    /* ASSET PIPELINE */
    TwID tex1 = twLoadTexture(inst, TW_NEAREST, TW_REPEAT, "snow.png");
    TwID tex2 = twLoadTexture(inst, TW_NEAREST, TW_REPEAT, "tex.png");
    TwID model = twLoadT3D(inst, "model.t3d");
    TwID shader_id = twLoadShaderFileARB(inst, "shader.arb");

    TwMat4 view_mat, model_mat;
    float angle = 0.0f;
    TwFloat cam_x = 0.0f, cam_y = 0.0f, cam_z = 2.0f;
    TwFloat cam_yaw = 0.0f;
    TwFloat move_speed = 1.0f;
    TwFloat turn_speed = 1.0f;
    twClearColor(inst, 0.2f, 0.3f, 0.3f, 1.0f);
    twInitCamera(inst, 60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    while (twIsWindowActive(inst)) {
        twResetGpuCallCount(inst);
        TwFloat dt = twGetDeltaTime(inst);
        angle += 45.0f * dt;

        TwFloat dir_x = sin(cam_yaw);
        TwFloat dir_z = -cos(cam_yaw);
        TwFloat right_x = cos(cam_yaw);
        TwFloat right_z = sin(cam_yaw);
        if (twKeyDown(inst, 'D')) cam_yaw += turn_speed * dt;
        if (twKeyDown(inst, 'A')) cam_yaw -= turn_speed * dt;
        if (twKeyDown(inst, 'W')) { cam_x += dir_x * move_speed * dt; cam_z += dir_z * move_speed * dt; }
        if (twKeyDown(inst, 'S')) { cam_x -= dir_x * move_speed * dt; cam_z -= dir_z * move_speed * dt; }
        if (twKeyDown(inst, 'Q')) { cam_x -= right_x * move_speed * dt; cam_z -= right_z * move_speed * dt; }
        if (twKeyDown(inst, 'E')) { cam_x += right_x * move_speed * dt; cam_z += right_z * move_speed * dt; }

        TwFloat center_x = cam_x + dir_x;
        TwFloat center_y = cam_y;
        TwFloat center_z = cam_z + dir_z;

        /* MATH & DISPATCH */
        twCameraLookAt(inst, (TwFloat[]) { cam_x, cam_y, cam_z }, (TwFloat[]) { center_x, center_y, center_z }, (TwFloat[]) { 0, 1, 0 });
        twCameraView(inst, &view_mat);
        twMathTransform(&model_mat, (TwFloat[]){0, 0, 0}, angle, (TwFloat[]){0.0f, 1.0f, 0.0f}, (TwFloat[]){1, 1, 1});
        twLoadModelViewMatrix(inst, &view_mat, &model_mat);
        /* DRAW */
        twClear(inst, TW_COLOR_BUFFER_BIT | TW_DEPTH_BUFFER_BIT);
        twBindShaderARB(inst, shader_id);
        twBindTextureIndex(inst, tex2);
        twDrawModel(inst, model);

        twSwapBuffer(inst);
    }
    printf("GPU Call Count: %d\n", twGetGpuCallCount(inst));
    twCloseWindow(inst);
    twDeleteInstance(inst);
    return 0;
}