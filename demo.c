#include <stdio.h>
#include "tw3d.h"

int main() {
	TwInstance* inst = twCreateInstance();
	twCreateSurface(inst, "Tower3D");
	twGLBindContext(inst);

	while (twSurfaceActive(inst)) {
        twUpdateSurface(inst);

        twGLSwapBuffers(inst);
	}

	twDeleteInstance(inst);
	return 0;
}