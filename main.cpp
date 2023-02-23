#include <iostream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#else
#include <glad/glad.h>
#endif
#include <GLFW/glfw3.h>
#include <functional>
#include <vector>
#include <chrono>
#include "Grid2D.h"

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

class FluidSimulation {
public:
    static std::function<void()> loop;
    static constexpr int WIDTH = 450;
    static constexpr int HEIGHT = 810;
    static constexpr uint32_t SIZE = 15;
    constexpr static uint32_t numTilesX = WIDTH/SIZE + 1;
    constexpr static uint32_t numTilesMiddleX = numTilesX - 2;
    constexpr static uint32_t numTilesY = HEIGHT/SIZE + 1;
    constexpr static uint32_t numTilesMiddleY = numTilesY - 2;
    constexpr static uint32_t INSTANCE_COUNT = numTilesX*numTilesY;

    struct FluidData {
        Matrix<float, INSTANCE_COUNT, numTilesX> density{}, velX{}, velY{};
    };

    enum BoundConfig {
        REGULAR, MIRROR_X, MIRROR_Y
    };

    explicit FluidSimulation(GLFWwindow* glfwWindow): window(glfwWindow) {
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

    grid.createGrid(WIDTH, HEIGHT, SIZE);
    }

    ~FluidSimulation() {
        glDeleteProgram(shaderProgram);
        grid.deleteBuffers();
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

    void initializeObjects(){
        for (uint32_t i = 0; i < grid.size(); i++) {
            curState.velX[i] = 0.0f;
            curState.velY[i] = 0.0f;
            curState.density[i] = 0.0f;
            prevState.velX[i] = 0.0f;
            prevState.velY[i] = 0.0f;
            prevState.density[i] = 0.0f;
        }
    }

    void createMainLoop(){
        createShaders();

        initializeObjects();

        loop = [this] {
            static auto currentTime = std::chrono::high_resolution_clock::now();
            auto newTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            deltaT = deltaTime;

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            updateGrid(deltaTime);

            glUseProgram(shaderProgram);
            grid.render();

            glfwSwapBuffers(window);
            glfwPollEvents();
        };

    }

    void updateGrid(float deltaTime) {
        addExternalForces(deltaTime);
        updateVelocities(deltaTime);
        updateDensities(deltaTime);

        for (uint32_t i = 0; i < grid.size(); i++){
            curState.density[i] = std::clamp(curState.density[i] - deltaTime*dissolveFactor, 0.0f, 2.0f);
            grid.updateColor(i,curState.density[i]);
        }

        grid.updateBuffer();
    }

    void addExternalForces(float deltaTime) {
        static double prevPos[2]{0.0f,0.0f};
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        y = HEIGHT - y;

        auto i = (uint32_t) (x/SIZE), j = (uint32_t) (y/SIZE);
        if (x > 0 && x < WIDTH && y > 0 && y < HEIGHT) {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                curState.density(i, j) += 0.6;
            }

            curState.velX(i, j) = initialSpeed*deltaTime*float(x - prevPos[0]);
            curState.velY(i, j) = initialSpeed*deltaTime*float(y - prevPos[1]);

            prevPos[0] = x;
            prevPos[1] = y;
        }


    }

    void updateDensities(float deltaTime) {
        curState.density.swap(prevState.density);
        diffuse(curState.density, prevState.density, diffusionFactor, deltaTime);
        curState.density.swap(prevState.density);
        advect(curState.density, prevState.density, curState.velX, curState.velY, deltaTime);
    }

    void updateVelocities(float deltaTime) {
        curState.velX.swap(prevState.velX);
        curState.velY.swap(prevState.velY);
        diffuse(curState.velX, prevState.velX, viscosity, deltaTime);
        diffuse(curState.velY, prevState.velY, viscosity, deltaTime);

        project(curState.velX, curState.velY, prevState.velX, prevState.velY);

        curState.velX.swap(prevState.velX);
        curState.velY.swap(prevState.velY);
        advect(curState.velX, prevState.velX, prevState.velX, prevState.velY, deltaTime, MIRROR_X);
        advect(curState.velY, prevState.velY, prevState.velX, prevState.velY, deltaTime, MIRROR_Y);

        project(curState.velX, curState.velY, prevState.velX, prevState.velY);
    }

    static void diffuse(Matrix<float, INSTANCE_COUNT, numTilesX>& x, const Matrix<float, INSTANCE_COUNT, numTilesX>& x0, float diff, float dt){
        float a = dt*diff*(float)(numTilesMiddleY*numTilesMiddleX);
        for (uint32_t k = 0; k < 20; ++k) {
            for (uint32_t j = 1; j <= numTilesMiddleY; ++j) {
                for (uint32_t i = 1; i <= numTilesMiddleX; ++i) {
                    x(i,j) = (x0(i,j) + a*(x(i-1,j) + x(i+1,j) + x(i,j-1) + x(i,j+1)))/(1+4*a);
                }
            }
            setBounds(x, REGULAR);
        }
    }

    static void advect(Matrix<float, INSTANCE_COUNT, numTilesX>& d, const Matrix<float, INSTANCE_COUNT, numTilesX>& d0, const Matrix<float, INSTANCE_COUNT, numTilesX>& velX, const Matrix<float, INSTANCE_COUNT, numTilesX>& velY, float dt, BoundConfig b = REGULAR){
        int i0, j0, i1, j1;
        float x, y, s0, t0, s1, t1;
        auto fNX = (float) numTilesMiddleX;
        auto fNY = (float) numTilesMiddleY;
        float dt0 = dt*std::min(fNX, fNY);

        for (uint32_t j = 1; j <= numTilesMiddleY; ++j) {
            for (uint32_t i = 1; i <= numTilesMiddleX; ++i) {
                x = (float) i - dt0*velX(i, j);
                if (x < 0.5f) x = 0.5f;
                if (x > fNX + 0.5f) x = fNX + 0.5f;
                y = (float) j - dt0*velY(i, j);
                if (y < 0.5f) y = 0.5f;
                if (y > fNY + 0.5f) y = fNY + 0.5f;
                i0 = (int) x;
                i1 = i0 + 1;
                j0 = (int) y;
                j1 = j0 + 1;
                s1 = x - (float) i0;
                t1 = y - (float) j0;
                s0 = 1 - s1;
                t0 = 1 - t1;

                d(i, j) = s0*(t0*d0(i0, j0) + t1*d0(i0, j1)) + s1*(t0*d0(i1, j0) + t1*d0(i1, j1));
            }
        }
        setBounds(d, b);
    }

    static void project(Matrix<float, INSTANCE_COUNT, numTilesX> &velX, Matrix<float, INSTANCE_COUNT, numTilesX> &velY, Matrix<float, INSTANCE_COUNT, numTilesX> &div, Matrix<float, INSTANCE_COUNT, numTilesX> &p) {
        float h = 1/(float) std::min(numTilesMiddleX, numTilesMiddleY);

        for (uint32_t j = 1; j <= numTilesMiddleY; ++j) {
            for (uint32_t i = 1; i <= numTilesMiddleX; ++i) {
                div(i, j) = -0.5f*h*(velX(i + 1, j) - velX(i - 1, j) + velY(i, j + 1) - velY(i, j - 1));
                p(i, j) = 0;
            }
        }
        setBounds(div);
        setBounds(p);


        for (uint32_t k = 0; k < 20; ++k) {
            for (uint32_t j = 1; j <= numTilesMiddleY; ++j) {
                for (uint32_t i = 1; i <= numTilesMiddleX; ++i) {
                    p(i,j) = (div(i,j) + p(i + 1,j) + p(i - 1,j) + p(i,j + 1) + p(i,j - 1))/4;
                }
            }
            setBounds(p);
        }

        for (uint32_t j = 1; j <= numTilesMiddleY; ++j) {
            for (uint32_t i = 1; i <= numTilesMiddleX; ++i) {
                velX(i,j) -= 0.5f*(p(i + 1,j) - p(i - 1,j))/h;
                velY(i,j) -= 0.5f*(p(i,j + 1) - p(i,j - 1))/h;
            }
        }
        setBounds(velX, MIRROR_X);
        setBounds(velY, MIRROR_Y);

    }

    static void setBounds(Matrix<float, INSTANCE_COUNT, numTilesX>& x, BoundConfig b = REGULAR) {
        for (uint32_t i = 1; i <= numTilesMiddleX; ++i) {
            x(i, 0) = (b == MIRROR_Y) ? -x(i, 1) : x(i, 1);
            x(i, numTilesMiddleY+1) = (b == MIRROR_Y) ? -x(i, numTilesMiddleY) : x(i, numTilesMiddleY);
        }
        for (uint32_t i = 1; i <= numTilesMiddleY; ++i) {
            x(0, i) = (b == MIRROR_X) ? -x(1, i) : x(1, i);
            x(numTilesMiddleX+1, i) = (b == MIRROR_X) ? -x(numTilesMiddleX, i) : x(numTilesMiddleX, i);
        }


        x(0, 0) = 0.5f*(x(1, 0)+x(0, 1));
        x(0, numTilesMiddleY+1) = 0.5f*(x(1, numTilesMiddleY+1)+x(0, numTilesMiddleY));
        x(numTilesMiddleX+1, 0) = 0.5f*(x(numTilesMiddleX, 0)+x(numTilesMiddleX+1, 1));
        x(numTilesMiddleX+1, numTilesMiddleY+1) = 0.5f*(x(numTilesMiddleX, numTilesMiddleY+1)+x(numTilesMiddleX+1, numTilesMiddleY));
    }


public:
    GLFWwindow* window = nullptr;
    GLuint shaderProgram{};
    Grid2D grid{};

    FluidData curState, prevState;
    float viscosity = 0.005f, diffusionFactor = 0.001f, dissolveFactor = 0.02f, initialSpeed = 60.0f, deltaT;
};


std::function<void()> FluidSimulation::loop;
void mainLoop() { FluidSimulation::loop(); }

#ifdef EMSCRIPTEN
EM_BOOL onTouchMove(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData){
    static double prevPos[2]{0.0f,0.0f};
    auto sim = (FluidSimulation *) userData;
    auto x = (float) touchEvent->touches[0].targetX;
    auto y = (float) touchEvent->touches[0].targetY;
    y = FluidSimulation::HEIGHT - y;

    auto i = (uint32_t) (x/FluidSimulation::SIZE), j = (uint32_t) (y/FluidSimulation::SIZE);
    if (x > 0 && x < FluidSimulation::WIDTH && y > 0 && y < FluidSimulation::HEIGHT) {
        sim->curState.density(i, j) += 2.0f;

        sim->curState.velX(i, j) = 20*sim->deltaT*float(x - prevPos[0]);
        sim->curState.velY(i, j) = 20*sim->deltaT*float(y - prevPos[1]);

        prevPos[0] = x;
        prevPos[1] = y;
    }
    return true;
}
#endif

int main() {
    std::cout << "Starting 3" << std::endl;
    GLFWwindow* window = nullptr;


    FluidSimulation example{window};

    std::cout << "Going into loop" << std::endl;
    example.createMainLoop();

#ifdef EMSCRIPTEN
    emscripten_set_touchmove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, &example, 1, &onTouchMove);
    emscripten_set_touchstart_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, &example, 1, &onTouchMove);
    // Define a mail loop function, that will be called as fast as possible
    emscripten_set_main_loop(&mainLoop, 0, 1);
#else
    example.glfwLoop();
#endif

    std::cout << "Loop ended" << std::endl;


    return EXIT_SUCCESS;
}
