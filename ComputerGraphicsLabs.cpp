#include <GL/glew.h>
#include <GL/freeglut.h>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <Windows.h>

namespace fs = std::filesystem;

struct Vertex
{
    float x, y, z;
};

struct Model
{
    std::vector<Vertex> vertices;
    std::vector<int> indices;

    float posX = 0;
    float posY = 0;
    float posZ = 0;

    float centerX = 0;
    float centerY = 0;
    float centerZ = 0;
};

std::vector<Model> models;

float degToRad(float deg)
{
    return deg * 0.01745329251f;
}

float camX = 0;
float camY = 1;
float camZ = 5;

float yaw = -90;
float pitch = 0;

float frontX = 0;
float frontY = 0;
float frontZ = -1;

bool keys[256] = { false };

bool mouseCaptured = false;

int lastMouseX = 0;
int lastMouseY = 0;


void updateCameraFront()
{
    frontX = cosf(degToRad(yaw)) * cosf(degToRad(pitch));
    frontY = sinf(degToRad(pitch));
    frontZ = sinf(degToRad(yaw)) * cosf(degToRad(pitch));
}

bool loadOBJ(const std::string& path)
{
    Model model;

    std::ifstream file(path);

    if (!file.is_open())
        return false;

    std::string line;

    while (std::getline(file, line))
    {
        std::stringstream ss(line);

        std::string type;
        ss >> type;

        if (type == "v")
        {
            Vertex v;

            ss >> v.x;
            ss >> v.y;
            ss >> v.z;

            model.vertices.push_back(v);
        }

        if (type == "f")
        {
            std::vector<int> faceIndices;

            std::string token;

            while (ss >> token)
            {
                std::stringstream parser(token);

                std::string vertexIndex;

                std::getline(parser, vertexIndex, '/');

                int index = std::stoi(vertexIndex) - 1;

                faceIndices.push_back(index);
            }

            for (int i = 1;
                i < faceIndices.size() - 1;
                i++)
            {
                model.indices.push_back(faceIndices[0]);

                model.indices.push_back(faceIndices[i]);

                model.indices.push_back(faceIndices[i + 1]);
            }
        }
    }

    float minX = 999999;
    float minY = 999999;
    float minZ = 999999;

    float maxX = -999999;
    float maxY = -999999;
    float maxZ = -999999;

    for (auto& v : model.vertices)
    {
        if (v.x < minX) minX = v.x;
        if (v.y < minY) minY = v.y;
        if (v.z < minZ) minZ = v.z;

        if (v.x > maxX) maxX = v.x;
        if (v.y > maxY) maxY = v.y;
        if (v.z > maxZ) maxZ = v.z;
    }

    model.centerX = (minX + maxX) * 0.5f;
    model.centerY = (minY + maxY) * 0.5f;
    model.centerZ = (minZ + maxZ) * 0.5f;

    models.push_back(model);

    return true;
}


void loadAllModels()
{
    std::cout << "\n[ MODEL LOADER ]\n";

    int loadedCount = 0;

    if (!fs::exists("resources/models"))
    {
        std::cout << "[ ERROR ] Folder not found: resources/models\n";

        return;
    }


    for (auto& file : fs::directory_iterator("resources/models"))
    {
        if (file.path().extension() != ".obj") continue;


        std::string path = file.path().string();

        std::string name = file.path().filename().string();


        size_t before = models.size();


        bool loaded = loadOBJ(path);


        if (!loaded)
        {
            std::cout
                << "[ FAILED ] "
                << name
                << "\n";

            continue;
        }


        Model& model = models.back();


        std::cout
            << "[ OK ] "
            << name
            << "\n";

        std::cout
            << "       center: "
            << model.centerX << " "
            << model.centerY << " "
            << model.centerZ
            << "\n";

        std::cout
            << "       vertices: "
            << model.vertices.size()
            << "\n";

        std::cout
            << "       triangles: "
            << model.indices.size() / 3
            << "\n";

        std::cout
            << "\n";

        loadedCount++;
    }

    std::cout
        << "TOTAL LOADED: "
        << loadedCount
        << "\n\n\n";
}


void drawModel(const Model& model)
{
    glBegin(GL_TRIANGLES);

    for (int i = 0; i < model.indices.size(); i += 3)
    {
        auto v1 = model.vertices[model.indices[i]];

        auto v2 = model.vertices[model.indices[i + 1]];

        auto v3 = model.vertices[model.indices[i + 2]];

        float ax = v2.x - v1.x;
        float ay = v2.y - v1.y;
        float az = v2.z - v1.z;

        float bx = v3.x - v1.x;
        float by = v3.y - v1.y;
        float bz = v3.z - v1.z;

        float nx = ay * bz - az * by;

        float ny = az * bx - ax * bz;

        float nz = ax * by - ay * bx;

        float len = sqrtf(nx * nx + ny * ny + nz * nz);

        if (len > 0)
        {
            nx /= len;
            ny /= len;
            nz /= len;
        }

        glNormal3f(
            nx,
            ny,
            nz
        );

        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glVertex3f(v3.x, v3.y, v3.z);
    }

    glEnd();
}


void drawOutlinedModel(const Model& model)
{
    glPushMatrix();

    glTranslatef(
        -model.centerX,
        -model.centerY,
        -model.centerZ
    );

    glDisable(GL_LIGHTING);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glDepthMask(GL_FALSE);

    glPushMatrix();

    glScalef(
        1.21f,
        1.21f,
        1.21f
    );

    glColor3f(
        0.0f,
        0.0f,
        0.0f
    );

    drawModel(model);

    glPopMatrix();

    glDepthMask(GL_TRUE);

    glEnable(GL_LIGHTING);

    glCullFace(GL_BACK);


    GLfloat material[] =
    {
        1.0f,
        1.0f,
        1.0f,
        1.0f
    };

    glMaterialfv(
        GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE,
        material
    );

    drawModel(model);

    glDisable(GL_CULL_FACE);

    glPopMatrix();
}

void processMovement()
{
    float speed = 0.08f;

    float rightX = frontZ;
    float rightZ = -frontX;

    float rightLength = sqrtf(rightX * rightX + rightZ * rightZ);

    if (rightLength > 0.0f)
    {
        rightX /= rightLength;
        rightZ /= rightLength;
    }

    if (GetAsyncKeyState('W') & 0x8000)
    {
        camX += frontX * speed;
        camY += frontY * speed;
        camZ += frontZ * speed;
    }

    if (GetAsyncKeyState('S') & 0x8000)
    {
        camX -= frontX * speed;
        camY -= frontY * speed;
        camZ -= frontZ * speed;
    }

    if (GetAsyncKeyState('A') & 0x8000)
    {
        camX += rightX * speed;
        camZ += rightZ * speed;
    }

    if (GetAsyncKeyState('D') & 0x8000)
    {
        camX -= rightX * speed;
        camZ -= rightZ * speed;
    }
}

void display()
{
    processMovement();

    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
        camX,
        camY,
        camZ,

        camX + frontX,
        camY + frontY,
        camZ + frontZ,

        0,
        1,
        0
    );

    GLfloat lightPosition[] =
    {
        20.0f,
        30.0f,
        20.0f,
        0.0f
    };

    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        lightPosition
    );

    for (auto& model : models)
    {
        drawOutlinedModel(model);
    }

    glutSwapBuffers();
}


void idle()
{
    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        mouseCaptured = state == GLUT_DOWN;

        lastMouseX = x;
        lastMouseY = y;
    }
}


void mouseMove(int x, int y)
{
    if (!mouseCaptured) return;

    float sensitivity = 0.2f;

    yaw += (x - lastMouseX) * sensitivity;

    pitch -= (y - lastMouseY) * sensitivity;

    if (pitch > 89) pitch = 89;

    if (pitch < -89) pitch = -89;

    lastMouseX = x;
    lastMouseY = y;

    updateCameraFront();
}

void reshape(int width, int height)
{
    if (height == 0) height = 1;

    glViewport(
        0,
        0,
        width,
        height
    );

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    gluPerspective(
        70.0,
        (double)width / height,
        0.1,
        1000.0
    );

    glMatrixMode(GL_MODELVIEW);
}

void init()
{
    glewInit();

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_NORMALIZE);

    glShadeModel(GL_SMOOTH);

    glClearColor(
        1.0f,
        1.0f,
        1.0f,
        1
    );

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPosition[] =
    {
        -0.6f,
        1.0f,
        0.35f,
        0.0f
    };

    GLfloat diffuse[] =
    {
        0.8f,
        0.8f,
        0.8f,
        1.0f
    };


    GLfloat ambient[] =
    {
        0.15f,
        0.15f,
        0.15f,
        1.0f
    };


    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        lightPosition
    );


    glLightfv(
        GL_LIGHT0,
        GL_DIFFUSE,
        diffuse
    );


    glLightfv(
        GL_LIGHT0,
        GL_AMBIENT,
        ambient
    );

    updateCameraFront();

    glutIgnoreKeyRepeat(1);

    loadAllModels();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGBA |
        GLUT_DEPTH
    );

    glutInitWindowSize(
        1280,
        720
    );

    glutCreateWindow(
        "OpenGL Demo"
    );

    init();

    glutDisplayFunc(display);
    glutIdleFunc(idle);

    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMove);

    glutReshapeFunc(reshape);

    glutMainLoop();

    return 0;
}