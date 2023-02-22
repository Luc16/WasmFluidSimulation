#include <iostream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#else
#include <glad/glad.h>
#endif
#include <GLFW/glfw3.h>
#include <functional>
#include <vector>

template<typename T, size_t size, uint32_t row>
class Matrix{
public:

    void swap(Matrix& other){
        m_matrix.swap(other.m_matrix);
    }

    constexpr T& operator() (uint32_t i, uint32_t j) {
        return m_matrix[i + row*j];
    }
    constexpr const T& operator()(uint32_t i, uint32_t j) const{
        return m_matrix[i + row*j];
    }
    constexpr T& operator[] (uint32_t i) {
        return m_matrix[i];
    }
    constexpr const T& operator[](uint32_t i) const{
        return m_matrix[i];
    }

private:
    std::array<T, size> m_matrix{};
};

const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 inPos;\n"
                                 "layout (location = 1) in vec3 inColor;\n"
                                 "out vec3 fragColor;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(inPos, 1.0);\n"
                                 "   fragColor = inColor;\n"
                                 "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
                                   "in vec3 fragColor;\n"
                                   "out vec4 outColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   outColor = vec4(fragColor, 1.0f);\n"
                                   "}\n\0";

const char *vertexShaderSourceWeb = "precision mediump float;"
                                    "attribute vec3 coordinates;\n"
                                    "attribute vec3 color;\n"
                                    "varying vec3 fragColor;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(coordinates, 1.0);\n"
                                 "   fragColor = color;\n"
                                 "}\0";
const char *fragmentShaderSourceWeb = "precision mediump float;"
                                      "varying vec3 fragColor;"
                                      "void main()\n"
                                   "{\n"
                                   "   gl_FragColor = vec4(fragColor, 1);\n"
                                   "}\n\0";

class Example {
public:
    static std::function<void()> loop;
    static constexpr int WIDTH = 640;
    static constexpr int HEIGHT = 480;

    struct Vertex {
        GLfloat pos[3];
        GLfloat col[3];
    };

    explicit Example(GLFWwindow* glfwWindow): window(glfwWindow) {
        // Initialize glfw
        if (!glfwInit())
            exit(EXIT_FAILURE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // Create the window
        window = glfwCreateWindow(WIDTH, HEIGHT, "Emscripten webgl example", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        // Make this window the current context
        glfwMakeContextCurrent(window);

#ifndef __EMSCRIPTEN__
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            exit(EXIT_FAILURE);
        }
#endif

    }

    ~Example() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void glfwLoop() {
        while (!glfwWindowShouldClose(window)) {
            loop();
        }
    }

    void createShaders(){
        // build and compile our shader program
        // ------------------------------------
        // vertex shader
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
#ifndef __EMSCRIPTEN__
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
#else
        glShaderSource(vertexShader, 1, &vertexShaderSourceWeb, nullptr);
#endif
        glCompileShader(vertexShader);
        // check for shader compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // fragment shader
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
#ifndef __EMSCRIPTEN__
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
#else
        glShaderSource(fragmentShader, 1, &fragmentShaderSourceWeb, nullptr);
#endif
        glCompileShader(fragmentShader);
        // check for shader compile errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // link shaders
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        // check for linking errors
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void createAndBindVertexBuffer(std::vector<Vertex>& vertices) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &IBO);
        glBindVertexArray(VAO);

        std::vector<unsigned short> indices = {0, 1, 2};

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        //color
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));

        glBindVertexArray(VAO);


        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);
    }

    void createMainLoop(){
        createShaders();

        loop = [this] {

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            double x, y;
            glfwGetCursorPos(window, &x, &y);

            std::vector<Vertex> vertices = {
                    {{-0.5f, -0.5f, 0.0f}, {1, 0, 0}}, // left
                    {{0.5f, -0.5f, 0.0f}, {0, 1, 0}}, // right
                    {{2*float(x)/float(WIDTH) - 1.0f, 2*float(HEIGHT - y)/float(HEIGHT) - 1.0f, 0.0f}, {0, 0, 1}}, // left
            };
            createAndBindVertexBuffer(vertices);


            // draw our first triangle
            glUseProgram(shaderProgram);
            // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);
            // glBindVertexArray(0); // no need to unbind it every time


            glfwSwapBuffers(window);
            glfwPollEvents();
        };

    }


private:
    GLFWwindow* window = nullptr;
    unsigned int VBO{}, VAO{}, IBO{}, shaderProgram{};
    std::vector<Vertex> vertexBuffer{};
};


std::function<void()> Example::loop;
void mainLoop() { Example::loop(); }

int main() {
    std::cout << "Starting" << std::endl;
    GLFWwindow* window = nullptr;


    Example example{window};

    std::cout << "Going into loop" << std::endl;
    example.createMainLoop();

#ifdef EMSCRIPTEN
    // Define a mail loop function, that will be called as fast as possible
    emscripten_set_main_loop(&mainLoop, 0, 1);
#else
    // This is the normal C/C++ main loop
    example.glfwLoop();
#endif

    // Tear down the system
    std::cout << "Loop ended" << std::endl;


    return EXIT_SUCCESS;
}
