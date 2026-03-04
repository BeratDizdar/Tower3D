#include "include/glad/glad.h"
#include "tw3d.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#include "../v1.5/stb_image.h"

int main() {
    TwInstance* inst = TwCreateInstance();
    TwCreateSurface(inst, "Tower3D - v1.6 (Pure AZDO)");
    TwBindContextToSurface(inst, 1);
    
    gladLoadGL();

    glClearColor(0.2, 0.3, 0.3, 1.0);
    while (TwUpdateSurface(inst)) {
        float dt = TwGetDeltaTime(inst);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        TwSwapBuffers(inst);
    }

    TwDeleteInstance(inst);

    return 0;
}