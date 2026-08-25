#include <iostream>
#include <vector>
#include <string>
#include "gl_backend.hpp"

int main() {
    if (!GLBackend::Instance().Initialize()) {
        std::cerr << "Failed to init GLBackend" << std::endl;
        return 1;
    }

    GLVKContextScope scope;

    GLint num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    std::cout << "Supported OpenGL Extensions (" << num_extensions << "):" << std::endl;

    typedef const GLubyte* (APIENTRYP PFNGLGETSTRINGIPROC) (GLenum name, GLuint index);
    PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)eglGetProcAddress("glGetStringi");

    if (glGetStringi) {
        for (int i = 0; i < num_extensions; i++) {
            const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
            if (ext) {
                std::string s(ext);
                if (s.find("16") != std::string::npos || s.find("8") != std::string::npos || s.find("half") != std::string::npos || s.find("float") != std::string::npos || s.find("shader") != std::string::npos) {
                    std::cout << "  " << s << std::endl;
                }
            }
        }
    }

    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return 0;
}
