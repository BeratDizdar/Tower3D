#include "include/glad/glad.h"
#include "tw3d.h"


const char* vertex_shader_src =
"#version 460 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec2 aTex;\n"
"out vec2 TexCoord;\n"
"void main() {\n"
"   gl_Position = vec4(aPos, 1.0);\n"
"   TexCoord = aTex;\n"
"}\n";

const char* fragment_shader_src =
"#version 460 core\n"
"out vec4 FragColor;\n"
"in vec2 TexCoord;\n"
"layout (binding = 0) uniform sampler2D tex;\n"
"void main() {\n"
"   FragColor = texture(tex, TexCoord);\n"
"}\n";

int main() {
    TwInstance* inst = twCreateInstance();
    twCreateSurface(inst, "Tower3D - v1.6");
    twBindContextToSurface(inst, 1);

    gladLoadGL();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2, 0.3, 0.3, 1.0);

    u32_t tex = twLoadTexture("asset/mermer.png");
    
    f32_t vertices[] = { // X Y Z | U V
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 0.0f,
    };
    u32_t indices[] = { 0, 1, 2, 2, 3, 0 };

    u32_t vao, vbo, ebo;
    twCreateVertexArrayAndBuffers(&vao, &vbo, sizeof(vertices), &vertices, &ebo, sizeof(indices), &indices);
    twVertexArrayAttribute(vao, 0, 3, 0);
    twVertexArrayAttribute(vao, 1, 2, 3);
    twVertexArrayBindBuffers(vao, vbo, 5, ebo);

    u32_t vs = twCompileVertexShader(vertex_shader_src);
    u32_t fs = twCompileFragmentShader(fragment_shader_src);
    u32_t prog = twCreateProgramVFBeforeDelete(vs, fs);

    float time = 0.0f;
    while (twUpdateSurface(inst)) {
        float dt = twGetDeltaTime(inst);
        time += dt;
        twClearColorAndDepth();

        glUseProgram(prog);
        glBindTextureUnit(0, tex);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        twSwapBuffers(inst);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &tex);
    twDeleteProgram(prog);
    twDeleteInstance(inst);

    return 0;
}