/*
 Cheese - Blender OBJ Viewer
 Computer Graphics - C++ / OpenGL

 This program reads my Cheese.obj model from Blender and displays it in 3D.
 I am practicing OBJ parsing, triangles, perspective, camera movement,
 rasterization, and depth testing from class.

 Keep Cheese.cpp, Cheese.obj, and Cheese.mtl in the same folder.
 The color below comes from the Kd value in Cheese.mtl.
*/

#include <GLUT/glut.h>  // GLUT creates the window and handles keyboard input.
#include <vector>       // vector stores a list that can grow as we read vertices.
#include <fstream>      // ifstream opens and reads my OBJ file.
#include <sstream>      // stringstream separates the words on each OBJ line.
#include <string>       // string stores text such as "v" and "f".
#include <iostream>     // cout and cerr print information in the terminal.
#include <cmath>        // sin and cos help rotate the camera.
#include <algorithm>    // min and max help calculate the model's size.
#include <cstdlib>      // exit closes the program.

struct Vec3 { float x, y, z; };  // One vertex has three coordinates in 3D space.
struct Triangle { Vec3 a, b, c; };  // Three vertices make one triangle.

std::vector<Vec3> vertices;       // Stores every v line from the OBJ file.
std::vector<Triangle> triangles;   // Stores triangles made from the f lines.
float centerX = 0, centerY = 0, centerZ = 0; // Center of my cheese model.
float modelSize = 1;               // Largest dimension, used for camera distance.
float angle = 0.5f;                // Camera rotation in radians.
float distanceFromModel = 6;       // How far the camera is from the cheese.
bool wireframe = false;            // False means solid triangles, true means outlines.

// OBJ face entries can look like 3, 3/2, or 3/2/1.
// We only need the first number because it identifies the vertex position.
int getIndex(const std::string& text) {
    int n = std::stoi(text.substr(0, text.find('/')));
    // Blender's OBJ numbering starts at 1, but C++ vectors start at 0.
    // Negative OBJ indices count backward from the end of the vertex list.
    return n > 0 ? n - 1 : static_cast<int>(vertices.size()) + n;
}

// Read my Blender model one line at a time.
bool loadOBJ() {
    std::ifstream file("Cheese.obj");
    if (!file) return false;  // Return false if the file cannot be opened.
    std::string line;
    while (std::getline(file, line)) { // Keep reading until the file ends.
        std::stringstream ss(line);
        std::string type;
        ss >> type; // The first word tells us what information follows.
        if (type == "v") { // v means vertex position (x, y, z).
            Vec3 v;
            if (ss >> v.x >> v.y >> v.z) vertices.push_back(v);
        } else if (type == "f") { // f means face, listing vertex indices.
            std::vector<int> face;
            std::string item;
            while (ss >> item) {
                try {
                    int index = getIndex(item);
                    if (index < 0 || index >= static_cast<int>(vertices.size())) {
                        face.clear(); break;
                    }
                    face.push_back(index);
                } catch (...) { face.clear(); break; }
            }
            // Triangle fan: keep the first vertex and connect the next two.
            // For example, a four-sided face becomes two triangles.
            for (size_t i = 1; i + 1 < face.size(); ++i)
                triangles.push_back({vertices[face[0]], vertices[face[i]],
                                     vertices[face[i + 1]]});
        }
    }
    return !vertices.empty() && !triangles.empty();
}

// Find the center and size so the model fits in the viewing window.
void setupModel() {
    Vec3 low = vertices[0], high = vertices[0];
    for (const Vec3& v : vertices) {
        low.x = std::min(low.x, v.x); high.x = std::max(high.x, v.x);
        low.y = std::min(low.y, v.y); high.y = std::max(high.y, v.y);
        low.z = std::min(low.z, v.z); high.z = std::max(high.z, v.z);
    }
    centerX = (low.x + high.x) / 2;
    centerY = (low.y + high.y) / 2;
    centerZ = (low.z + high.z) / 2;
    modelSize = std::max({high.x-low.x, high.y-low.y, high.z-low.z, 0.1f});
    distanceFromModel = modelSize * 2.2f;
}

// Draw the cheese using OpenGL triangles.
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear color and depth.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); // Reset the previous camera transformation.
    // sin and cos place the camera in a circle around the model.
    gluLookAt(centerX + sin(angle)*distanceFromModel,
              centerY + modelSize*0.35f,
              centerZ + cos(angle)*distanceFromModel,
              centerX, centerY, centerZ, 0, 1, 0);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    glColor3f(1.0f, 0.75f, 0.1f); // Cheese.mtl diffuse color.
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_TRIANGLES); // OpenGL rasterizes these triangles into pixels.
    for (const Triangle& t : triangles) {
        glVertex3f(t.a.x, t.a.y, t.a.z);
        glVertex3f(t.b.x, t.b.y, t.b.z);
        glVertex3f(t.c.x, t.c.y, t.c.z);
    }
    glEnd();
    glutSwapBuffers(); // Double buffering displays the finished frame.
}

// Adjust the perspective when the window changes size.
void reshape(int width, int height) {
    if (height == 0) height = 1; // Prevent division by zero.
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Field of view = 60 degrees; near and far planes clip the visible scene.
    gluPerspective(60.0, (double)width / height,
                   modelSize * 0.01, modelSize * 100);
    glMatrixMode(GL_MODELVIEW);
}

// Keyboard controls allow me to examine the model from different views.
void keyboard(unsigned char key, int, int) {
    if (key == 'a') angle -= 0.15f; // Rotate left.
    if (key == 'd') angle += 0.15f; // Rotate right.
    if (key == 'w') distanceFromModel = std::max(modelSize*0.2f, distanceFromModel-modelSize*0.2f);
    if (key == 's') distanceFromModel += modelSize*0.2f;
    if (key == 't') wireframe = !wireframe; // Toggle triangle outlines.
    if (key == 27) std::exit(0); // Escape closes the program.
    glutPostRedisplay(); // Request a new frame after a key is pressed.
}

int main(int argc, char** argv) {
    if (!loadOBJ()) {
        std::cerr << "Could not read triangles from Cheese.obj\n";
        return 1;
    }
    setupModel();
    std::cout << "Vertices: " << vertices.size()
              << " | Triangles: " << triangles.size() << "\n"
              << "A/D rotate, W/S zoom, T wireframe, Esc quit\n";
    glutInit(&argc, argv); // Start GLUT.
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(850, 650);
    glutCreateWindow("Blender Cheese - C++ Viewer");
    glEnable(GL_DEPTH_TEST); // Keep the closest surface at each pixel (Z-buffer).
    glDisable(GL_CULL_FACE); // Show both sides of triangles while testing.
    glClearColor(0.12f, 0.10f, 0.18f, 1.0f); // Dark purple background.
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop(); // Wait for events and keep the window open.
    return 0;
}
