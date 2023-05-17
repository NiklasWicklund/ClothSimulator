#include <iostream>
#include <vector>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm.hpp>




class Vertex {

public:
    glm::fvec3 pos;
    glm::fvec3 prevPos;
    glm::fvec3 acceleration;
    float mass;
    bool fixed;
    bool destroyed;

    Vertex(float x,float y, float z,glm::fvec3 acceleration, bool fixed) 
        : pos(glm::fvec3(x,y,z)),prevPos(glm::fvec3(x, y, z)),acceleration(acceleration), fixed(fixed),destroyed(false),mass(1.0f){
    }
};
class Cloth {
    using ClothMat = std::vector<std::vector<Vertex>>;
    
    ClothMat vertices;

    float segmentLength;
    int rows;
    int cols;

    

    Vertex* grabbedVertex = nullptr;
    glm::fvec3 mousePosition;


    glm::fvec3 lastMousePosition;
    bool mousePressed = false;
    float timeSinceLastMouse = 0.0f;

public:
    
    Cloth(glm::fvec3 start, float segmentLength, int rows, int cols)
        : segmentLength(segmentLength), rows(rows), cols(cols){
        
        for (int r = 0; r < rows; ++r) {
            bool fixed = r == 0;
            std::vector<Vertex> tmp;
            for (int c = 0; c < cols; ++c) {
                glm::fvec3 pos = start + glm::fvec3(segmentLength*c, segmentLength * r, 0);
                glm::fvec3 acc = glm::fvec3(0, 981.0f, 0);
                tmp.push_back(Vertex(pos.x,pos.y,pos.z,acc,fixed));
            }
            vertices.push_back(tmp);
        }
    }

    void releasePoint() {
        mousePressed = false;
        if (grabbedVertex != nullptr) std::cout << "Release vertex" << std::endl;  //grabbedVertex->pos = glm::fvec3(0.0f, 0.0f, 0.0f);
        grabbedVertex = nullptr;
        
        
    }
    bool isGrabbingPoint() {
        return grabbedVertex != nullptr;
    }
    void setMousePosition(double x, double y) {

        
        double threshold = 10;

        if (!mousePressed || glfwGetTime() - timeSinceLastMouse < 1 / 60.0f) return; // || 

        mousePosition = glm::fvec3(x, y, 0);
        if (grabbedVertex != nullptr) mousePosition.z = grabbedVertex->pos.z;
        
        timeSinceLastMouse = glfwGetTime();

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                double distance = glm::length(vertices[r][c].pos - glm::fvec3(x, y, vertices[r][c].pos.z));
                if (distance < threshold) {
                    vertices[r][c].destroyed = true;
                }
            }
        }

    }
    void grabPoint(double x, double y) {
        mousePressed = true;
        double threshold = 10;
        if (grabbedVertex == nullptr) {
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    double distance = glm::length(vertices[r][c].pos - glm::fvec3(x, y, vertices[r][c].pos.z));
                    if (distance < threshold) {
                        grabbedVertex = &vertices[r][c];
                        mousePosition = glm::fvec3(x, y, vertices[r][c].pos.z);
                        return;
                    }
                }
            }
        }
        
    }
    void update(float dt) {


        /*glm::fvec3 wind(0, 0, 0);
        if (fmod(glfwGetTime(), 2) < dt) {
            float gust_x = ((rand() + 0.2) / (float)RAND_MAX);
            float gust_y = 0;//(((rand()) / (float)RAND_MAX) + 0.3);
            wind = 150.0f * glm::fvec3(gust_x, gust_y, 0) ;
        }
        else {
            wind = glm::fvec3(0, 0, 0);
        }*/
        //Verlet integration
        float gravity = -9.82;
        float drag = 0.02;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (vertices[r][c].fixed) continue;
                
                Vertex v = vertices[r][c];
                glm::fvec3 copy = glm::fvec3(v.pos);
                
                vertices[r][c].pos = v.pos + (1.0f - drag) * (v.pos - v.prevPos) + dt * dt* v.mass*v.acceleration;// + wind*dt;
                
                vertices[r][c].prevPos = copy;
                //std::cout << vertices[r][c].pos.y << " " << vertices[r][c].prevPos.y << std::endl;
                
            }
        }
        if (grabbedVertex != nullptr) {
            grabbedVertex->pos = mousePosition;
        }

        int iterations = 10;
        for (int i = 0; i < iterations; ++i) {
            satisfyConstraints();
        }
    }

    void satisfyConstraints() {
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (vertices[r][c].fixed || vertices[r][c].destroyed) continue;
                
                //Check left and up

                //Left
                if (c > 0 && !vertices[r][c-1].destroyed){
                    glm::vec3 delta = (vertices[r][c].pos - vertices[r][c-1].pos);
                    float distance = glm::length(delta);


                    float tmp = distance / segmentLength;
                    if (tmp > 20) {
                        vertices[r][c].destroyed = true; vertices[r][c - 1].destroyed = true;
                        return;
                    }
                    float difference = (distance - segmentLength) / distance;

                    if (!vertices[r][c-1].fixed)
                        vertices[r][c-1].pos += delta * difference * 0.5f;
                    vertices[r][c].pos -= delta * difference * 0.5f;
                }

                //Up
                if (r > 0 && !vertices[r - 1][c].destroyed) {
                    glm::vec3 delta = (vertices[r][c].pos - vertices[r - 1][c].pos);
                    float distance = glm::length(delta);

                    
                    float difference = (distance - segmentLength) / distance;

                    float tmp = distance / segmentLength;
                    if (tmp > 20) {
                        vertices[r][c].destroyed = true; vertices[r - 1][c].destroyed = true;
                        return;
                    }
                    if (vertices[r - 1][c].fixed) {
                        vertices[r][c].pos -= delta * difference * 1.0f;
                    }
                    else {
                        vertices[r][c].pos -= delta * difference * 0.5f;
                        vertices[r - 1][c].pos += delta * difference * 0.5f;
                    }
                    
                    
                }


            }
        }
    }

    void draw(GLFWwindow* window) {
        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /*glPointSize(10.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        
        glBegin(GL_POINTS);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                glm::fvec3 pos = vertices[r][c].pos;
                glVertex3f(pos.x, pos.y, pos.z);
            }
        }
        glEnd();
        */
        
        for (int r = 0; r < rows; ++r) {
            glBegin(GL_LINE_STRIP);
            for (int c = 0; c < cols; ++c) {
                if (vertices[r][c].destroyed) {
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                    continue;
                }
                glm::fvec3 pos = vertices[r][c].pos;
                glVertex3f(pos.x, pos.y, pos.z);
            }
            glEnd();
        }

        for (int c = 0; c < cols; ++c) {
            glBegin(GL_LINE_STRIP);
            for (int r = 0; r < rows; ++r) {
                if (vertices[r][c].destroyed) {
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                    continue;
                }
                glm::fvec3 pos = vertices[r][c].pos;
                glVertex3f(pos.x, pos.y, pos.z);
            }
            glEnd();
        }
        

        // Swap buffers
        glfwSwapBuffers(window);
    }
};

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    Cloth* cloth = static_cast<Cloth*>(glfwGetWindowUserPointer(window));
    
    cloth->setMousePosition(xpos, ypos);
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Cloth* cloth = static_cast<Cloth*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            cloth->grabPoint(xpos, ypos);
        }
        else if (action == GLFW_RELEASE) {
            cloth->releasePoint();
        }
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    int width = 2000;
    int height = 1500;
    // Create a window and OpenGL context
    GLFWwindow* window = glfwCreateWindow(width, height, "Rope Simulation", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height,0, -10, 10);
    //glOrtho(-10, 10, -7.5, 7.5, -10, 10);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    int rows = 70;
    int cols = 100;
    float segmentLength = 10;
    Cloth cloth(glm::fvec3(500, 0, 0), segmentLength,rows, cols);

    glfwSetWindowUserPointer(window, &cloth);

    double lastUpdateTime = glfwGetTime();
    float dt = 1.0f/200.0f;
    while (!glfwWindowShouldClose(window)) {
        
        dt = glfwGetTime() - lastUpdateTime;
        //std::cout << dt << std::endl;
        lastUpdateTime = glfwGetTime();
        
        cloth.draw(window);
        cloth.update(dt);
        // Poll events
        glfwPollEvents();
    }

    // Clean up
    glfwTerminate();
    return 0;
}