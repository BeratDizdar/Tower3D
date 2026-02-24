#include <stdio.h>
#include "tw3d.h"

int main() {
    TwInstanceDesc desc = { 0 };
    desc.title = "Tower3D Test Window";
    desc.flag = TW_DESC_FLAG_FULLSCREEN;
    desc.exit_key = 27; // ESC
    TwInstance* inst = twCreateInstance(&desc);
    twOpenWindow(inst);
    twSwapInterval(inst, 1);

    TwFloat vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    twClearColor(inst, 0.2f, 0.3f, 0.3f, 1.0f);

    while (twIsWindowActive(inst)) {
        if (twKeyDown(inst, 'H')) {
            printf("Hello, Tower3D\n");
        }
        
        twClear(inst, TW_COLOR_BUFFER_BIT | TW_DEPTH_BUFFER_BIT);
        twVertexPointer(inst, 3, TW_FLOAT, 0, vertices);
        twEnableClientState(inst, TW_VERTEX_ARRAY);
        twDrawArrays(inst, TW_TRIANGLES, 0, 3);

        twDisableClientState(inst, TW_VERTEX_ARRAY);

        twSwapBuffer(inst);
    }

    twCloseWindow(inst);
    twDeleteInstance(inst);

    return 0;
}