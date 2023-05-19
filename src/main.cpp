/**
 * @file main.cpp
 * @author Niklas Wicklund, Robin Nordmark, Tim Olsén
 * @brief Includes the main function and the Cloth class with its auxiliary class Vertex to simulate a cloth
 * @date 2023-05-18
 * 
 */
#include <iostream>
#include <vector>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm.hpp>

/*
Class representing a vertex in the cloth and its properties
*/
class Vertex {
public:
    glm::fvec3 pos;
    glm::fvec3 prevPos;
    glm::fvec3 acceleration;
    float mass;
    bool fixed;
    bool destroyed;

    Vertex(float x, float y, float z, glm::fvec3 acceleration, bool fixed, float mass = 1.0f) 
        : pos(glm::fvec3(x, y, z)), prevPos(glm::fvec3(x, y, z)), acceleration(acceleration), fixed(fixed), destroyed(false), mass(mass) {
    }
};

/*
Class representing a cloth and implements Verlet integration and the Jakobsen method
*/
class Cloth {
    using ClothMat = std::vector<std::vector<Vertex>>;
    
    ClothMat vertices;
    float segmentLength;
    int rows;
    int cols;
    Vertex * grabbedVertex = nullptr;
    glm::fvec3 mousePosition;
    bool rightMousePressed = false;
    float timeSinceLastMouse = 0.0f;

public:
    Cloth(glm::fvec3 start, float segmentLength, int rows, int cols)
        : segmentLength(segmentLength), rows(rows), cols(cols) {
        
        // Initialize vertices based on number of rows and columns
        for (int r = 0; r < rows; ++r) {
            bool fixed = r == 0;
            std::vector<Vertex> tmp;
            for (int c = 0; c < cols; ++c) {
                glm::fvec3 pos = start + glm::fvec3(segmentLength * c, segmentLength * r, 0);
                glm::fvec3 acc = glm::fvec3(0, 981.0f, 0);
                float mass = 2.0f;
                tmp.push_back(Vertex(pos.x, pos.y, pos.z, acc, fixed, mass));
            }
            vertices.push_back(tmp);
        }
    }

    void releaseLeftMouseButton() {
        rightMousePressed = false;
    }

    void pressRightMouseButton(double x, double y) {
        rightMousePressed = true;
    }

    void releasePoint() {
        grabbedVertex = nullptr;
    }
    
    bool isGrabbingPoint() {
        return grabbedVertex != nullptr;
    }

    /**
     * @brief Sets the mouse position and destroys vertices close to it if the right mouse button is pressed
     * 
     * 
     * @param x 
     * @param y 
     */
    void setMousePosition(double x, double y) {
        mousePosition = glm::fvec3(x, y, 0);
        if (grabbedVertex != nullptr) mousePosition.z = grabbedVertex->pos.z;

        // Only destroy vertices if right mouse button is pressed and only 60 times per second
        if (!rightMousePressed || glfwGetTime() - timeSinceLastMouse < 1 / 60.0f) return;
        timeSinceLastMouse = glfwGetTime();

        double threshold = 10;
        // If a vertex is close enough to the mouse position, destroy it
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                double distance = glm::length(vertices[r][c].pos - glm::fvec3(x, y, vertices[r][c].pos.z));
                if (distance < threshold) {
                    vertices[r][c].destroyed = true;
                }
            }
        }
    }
    
    /**
     * @brief Grabs a point if the mouse is close enough to it
     * 
     * @param x the x-coordinate of the mouse in screen space
     * @param y the y-coordinate of the mouse in screen space
     */
    void grabPoint(double x, double y) {
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
    
    /**
     * @brief Updates the cloth by applying Verlet integration and the Jakobsen method to all vertices
     * 
     * @param dt the time since the last update
     */
    void update(float dt) {
        float drag = 0.02;
        // Apply verlet integration to all vertices except the fixed ones
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (vertices[r][c].fixed) continue;
                
                // Implementation of Verlet integration
                Vertex v = vertices[r][c];
                glm::fvec3 copy = glm::fvec3(v.pos);
                vertices[r][c].pos = v.pos + (1.0f - drag) * (v.pos - v.prevPos) + dt * dt* v.mass*v.acceleration;
                vertices[r][c].prevPos = copy;
            }
        }

        // If a vertex is currently grabbed, set its position to the mouse position
        if (grabbedVertex != nullptr) {
            grabbedVertex->pos = mousePosition;
        }
        
        // The numer of times the Jakobsen method is run
        int iterations = 2;
        for (int i = 0; i < iterations; ++i) {
            satisfyConstraints();
        }
    }

    /**
     * @brief Applies the Jakobsen method to all vertices by checking the distance between vertices and moving them accordingly
     * 
     */
    void satisfyConstraints() {

        int breakingLimit = 20;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (vertices[r][c].fixed || vertices[r][c].destroyed) continue;
                
                // Check constraint against vertex to the left
                if (c > 0 && !vertices[r][c-1].destroyed) {
                    glm::vec3 delta = (vertices[r][c].pos - vertices[r][c-1].pos);
                    float distance = glm::length(delta);
                    float tmp = distance / segmentLength;

                    // If the distance between two vertices is too large while no vertex is grabbed, destroy the concerned vertices
                    if (tmp > breakingLimit && grabbedVertex == nullptr) {
                        vertices[r][c].destroyed = true; vertices[r][c - 1].destroyed = true;
                        return;
                    }

                    float difference = (distance - segmentLength) / distance;
                    // If the vertex to the left is fixed, the current vertex is moved the whole distance
                    // Otherwise, the distance is split between the two vertices
                    if (vertices[r][c - 1].fixed) {
                        vertices[r][c].pos -= delta * difference * 1.0f; 
                    }
                    else {
                        vertices[r][c].pos -= delta * difference * 0.5f;
                        vertices[r][c - 1].pos += delta * difference * 0.5f;
                    }
                }

                // Check constraint against vertex above
                if (r > 0 && !vertices[r - 1][c].destroyed) {
                    glm::vec3 delta = (vertices[r][c].pos - vertices[r - 1][c].pos);
                    float distance = glm::length(delta);
                    float tmp = distance / segmentLength;

                    if (tmp > breakingLimit && grabbedVertex == nullptr) {
                        vertices[r][c].destroyed = true; vertices[r - 1][c].destroyed = true;
                        return;
                    }
                    
                    float difference = (distance - segmentLength) / distance;
                    
                    if (vertices[r - 1][c].fixed) {
                        vertices[r][c].pos -= delta * difference * 1.0f;
                    }
                    else {
                        vertices[r][c].pos -= delta * difference * 0.5f;
                        vertices[r-1][c].pos += delta * difference * 0.5f;
                    }
                }
            }
        }
    }

    // Draw all lines between the vertices
    void draw(GLFWwindow * window) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int r = 0; r < rows; ++r) {
            glBegin(GL_LINE_STRIP);
            for (int c = 0; c < cols; ++c) {
                // If a vertex is destroyed, start a new line strip and skip the current vertex
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
        glfwSwapBuffers(window);
    }
};

void cursor_pos_callback(GLFWwindow * window, double xpos, double ypos) {
    Cloth * cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));
    // If the mouse is moved, set the mouse position in the cloth
    cloth->setMousePosition(xpos, ypos);
}

void mouse_button_callback(GLFWwindow * window, int button, int action, int mods) {
    Cloth * cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // If left mouse button is pressed, try to grab a point
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            cloth->grabPoint(xpos, ypos);
        }
        else if (action == GLFW_RELEASE) {
            cloth->releasePoint();
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            // If right mouse button is pressed, inform the cloth that it is pressed
            cloth->pressRightMouseButton(xpos,ypos);
        }
        else if (action == GLFW_RELEASE) {
            cloth->releaseLeftMouseButton();
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW" << std::endl;
        return -1;
    }

    int width = 2000;
    int height = 1500;

    GLFWwindow * window = glfwCreateWindow(width, height, "Cloth Simulation", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height,0, -10, 10);

    // Set callbacks for mouse events
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    
    int rows = 60;
    int cols = 100;
    float segmentLength = 10;
    
    Cloth cloth(glm::fvec3(500, 0, 0), segmentLength,rows, cols);

    glfwSetWindowUserPointer(window, &cloth);

    double lastUpdateTime = glfwGetTime();
    float dt = 0;
    
    while (!glfwWindowShouldClose(window)) {
        // Calculate the time since the last update (deltaTime dt)
        dt = glfwGetTime() - lastUpdateTime;
        lastUpdateTime = glfwGetTime();
        cloth.draw(window);
        cloth.update(dt);
        glfwPollEvents();
    }
    
    glfwTerminate();    
    return 0;
}
