#include "tw3d.h"

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_TRIANGLES        0x0004
#define GL_UNSIGNED_INT     0x1405
#define GL_FLOAT            0x1406
#define GL_VERTEX_SHADER    0x8B31
#define GL_FRAGMENT_SHADER  0x8B30

const char* vertexShaderSource = "#version 460 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main() {\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 460 core\n"
"out vec4 FragColor;\n"
"void main() {\n"
"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
"}\n\0";

int main() {
    TwInstance* inst = TwCreateInstance();
    TwAPI* api = TwLoadVirtualTable();
    api->Surface.Create(inst, "Tower3D - v1.6");
    api->Context.BindToSurface(inst, api);
    api->Surface.SetSwapIntervals(inst, TW_TRUE);

    u32_t vs = api->Context.Shader.CreateShader(GL_VERTEX_SHADER);
    api->Context.Shader.ShaderSource(vs, 1, &vertexShaderSource, 0);
    api->Context.Shader.CompileShader(vs);

    u32_t fs = api->Context.Shader.CreateShader(GL_FRAGMENT_SHADER);
    api->Context.Shader.ShaderSource(fs, 1, &fragmentShaderSource, 0);
    api->Context.Shader.CompileShader(fs);

    u32_t shader = api->Context.Shader.CreateProgram();
    api->Context.Shader.AttachShader(shader, vs);
    api->Context.Shader.AttachShader(shader, fs);
    api->Context.Shader.LinkProgram(shader);
    api->Context.Shader.DeleteShader(vs);
    api->Context.Shader.DeleteShader(fs);

    f32_t vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
    u32_t indices[] = { 0, 1, 2 };

    u32_t vao, vbo, ebo;
    api->Context.Buffer.CreateVertexArrays(1, &vao);
    api->Context.Buffer.CreateBuffers(1, &vbo);
    api->Context.Buffer.CreateBuffers(1, &ebo);

    api->Context.Buffer.NamedBufferStorage(vbo, sizeof(vertices), vertices, 0);
    api->Context.Buffer.NamedBufferStorage(ebo, sizeof(indices), indices, 0);

    api->Context.Buffer.EnableVertexArrayAttrib(vao, 0);
    api->Context.Buffer.VertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, TW_FALSE, 0);
    api->Context.Buffer.VertexArrayAttribBinding(vao, 0, 0);

    api->Context.Buffer.VertexArrayVertexBuffer(vao, 0, vbo, 0, 3 * sizeof(float));
    api->Context.Buffer.VertexArrayElementBuffer(vao, ebo);

    api->Context.Render.ClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    while (api->Surface.Update(inst)) {
        api->Context.Render.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        api->Context.Shader.UseProgram(shader);
        api->Context.Buffer.BindVertexArray(vao);
        api->Context.Buffer.DrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

        api->Surface.SwapBuffers(inst);
    }

    api->Context.Buffer.DeleteVertexArrays(1, &vao);
    api->Context.Buffer.DeleteBuffers(1, &vbo);
    api->Context.Buffer.DeleteBuffers(1, &ebo);
    api->Context.Shader.DeleteProgram(shader);
    api->Instance.Delete(inst);

    return 0;
}