#include "tw3d.h"
#include <stdio.h>

float plane_verts[] = {
    -0.5f,  0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
    -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
     0.5f,  0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
};

unsigned int plane_inds[] = {
    0, 1, 2,
    2, 3, 0
};

int main() {
    TwInstance* inst = twCreateInstance();
    twCreateSurfaceInMain(inst, "Tower 3D v1.5 - Ilk Kan");
    twBindContextToSurface(inst);
    twSetSwapIntervals(inst, TW_TRUE);
    TwID ruleSolid = twCreateRuleInMainPool(inst, TW_TRUE, TW_FALSE, TW_FALSE);
    TwID bufPlane = twLoadBufferToMainPool(inst, plane_verts, 4, plane_inds, 6);
    TwID texStone = twLoadTextureToMainPool(inst, "tex.png");
    TwID shdBasic = twLoadShaderInMainPool(inst, "test.vert", "test.frag");
    int loc_mvp = twGetUniformLocation(inst, shdBasic, "u_mvp");
    float rotation = 0.0f;
    while (twUpdateSurface(inst)) {

        float dt = twGetDeltaTime(inst);
        rotation += 1.0f * dt;
        Mat4 proj = twMatPerspective(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
        Mat4 view = twMatLookAt((Vec3) { 0, 0, -2 }, (Vec3) { 0, 0, 0 }, (Vec3) { 0, 1, 0 });
        Mat4 model = twMatRotateY(rotation);
        Mat4 mvp = twMatMultiply(proj, twMatMultiply(view, model));
        twClearColorAndDepth(inst, 0.1f, 0.1f, 0.12f);

        twBindRule(inst, ruleSolid);
        twBindTexture(inst, texStone);
        twBindBuffer(inst, bufPlane);
        twBindShader(inst, shdBasic);
        twSendUniformMat4(inst, shdBasic, loc_mvp, mvp);
        twDrawCommands(inst);
        twSwapBuffers(inst);
    }
    twDeleteInstance(inst);
    return 0;
}