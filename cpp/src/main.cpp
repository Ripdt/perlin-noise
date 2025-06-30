#include <iostream>
#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <numbers>
#include <string>
#include <math.h>
#include <random>
#include <algorithm>

//--------------------------------------------------
// Utils
//--------------------------------------------------

class Vector3D {
public:
  float x, y, z;
  Vector3D(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
  Vector3D(const Vector3D& other) : x(other.x), y(other.y), z(other.z) {}

  Vector3D operator+(const Vector3D& other) const {
    return Vector3D(x + other.x, y + other.y, z + other.z);
  }
  Vector3D operator-(const Vector3D& other) const {
    return Vector3D(x - other.x, y - other.y, z - other.z);
  }
  Vector3D operator/(const float scalar) const {
    return Vector3D(x / scalar, y / scalar, z / scalar);
  }
  void operator=(const Vector3D& other) {
    x = other.x;
    y = other.y;
    z = other.z;
  }
};

struct Mouse {
  std::pair<int, int> posicao = { -1, -1 };
  int botao = -1;
  float sense = .4f;
}; 

float g_translateX = 0.0f, g_translateY = 0.0f, g_translateZ = 0.0f;
float g_rotateX = 0.0f, g_rotateY = 0.0f, g_rotateZ = 0.0f;
float g_scaleX = 1.0f, g_scaleY = 1.0f, g_scaleZ = 1.0f;

//--------------------------------------------------
// Globals
//--------------------------------------------------

#define LARGURA 512
#define ALTURA 512

struct Terreno {
  std::vector<Vector3D> vertices;
  int largura, profundidade;
};

Terreno terreno;
Mouse mouse;
static int delay = 10;


//--------------------------------------------------
// Perlin Noise 2D Simples
//--------------------------------------------------

// Tabela de gradientes unitários para 8 direções (2D)
struct Grad2 {
  float x, y;
};

#define GRAD2_SIZE 8
#define GRAD2_MASK (GRAD2_SIZE - 1)

constexpr Grad2 grad2_table[GRAD2_SIZE] = {
  { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
  { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 }
};

#define PERMUTATION_SIZE 256
#define PERMUTATION_MASK PERMUTATION_SIZE - 1

int p[PERMUTATION_SIZE * 2];

void init_perlin() {
  std::vector<int> perm(PERMUTATION_SIZE);
  for (int i = 0; i < PERMUTATION_SIZE; ++i) perm[i] = i;

  // Embaralhamento aleatório
  std::random_device rd;
  std::mt19937 g(rd()); // Mersenne Twister 19937 -- usei para algoritmos genéticos
  std::shuffle(perm.begin(), perm.end(), g);

  for (int i = 0; i < PERMUTATION_SIZE; ++i) {
    p[i] = perm[i];
    p[PERMUTATION_SIZE + i] = perm[i];
  }
}

float fade(float t) {
  return t * t * t * (t * (t * 6 - 15) + 10); // função smoothstep (6x^5 - 15x^4 + 10x^3)
}

float interpolar(float t, float a, float b) {
  return a + t * (b - a); // interpolação linear
}

float gradiente(int hash, float x, float y) {
  const Grad2& g = grad2_table[hash & GRAD2_MASK];
  return g.x * x + g.y * y;
}

float perlin(float x, float z) {
  int X = (int)floor(x) & PERMUTATION_MASK; // garante que X e Z está no intervalo [0, 255]
  int Z = (int)floor(z) & PERMUTATION_MASK;

  x -= floor(x);
  z -= floor(z);

  float u = fade(x);
  float v = fade(z);

  const int A = p[X] + Z;
  const int AA = p[A];
  const int AB = p[A + 1];

  const int B = p[X + 1] + Z;
  const int BA = p[B];
  const int BB = p[B + 1];

  return interpolar(
                    v, 
                    interpolar(u, gradiente(p[AA], x, z),     gradiente(p[BA], x - 1, z)), 
                    interpolar(u, gradiente(p[AB], x, z - 1), gradiente(p[BB], x - 1, z - 1))
         );
}

void gerar_terreno_perlin(Terreno& terreno, int largura, int profundidade, float escala, float altura) {
  terreno.vertices.clear();
  terreno.largura = largura;
  terreno.profundidade = profundidade;

  for (int z = 0; z < profundidade; ++z) {
    for (int x = 0; x < largura; ++x) {
      float nx = x * escala;
      float nz = z * escala;
      float y = perlin(nx, nz) * altura;
      terreno.vertices.emplace_back(Vector3D(x - largura / 2, y, z - profundidade / 2));
    }
  }
}

//--------------------------------------------------

int main(int argc, char** argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(LARGURA, ALTURA);
  glutCreateWindow("Perlin noise");
  glClearColor(0.f, 0.f, 0.f, 0.f);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-100, 100, -100, 100, -200, 200);
  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_DEPTH_TEST);

  init_perlin();

  const float largura_terreno = 100.0f;
  const float profundidade_terreno = 100.0f;
  const float escala_perlin = 0.1f;
  const float altura_perlin = 50.0f;

  gerar_terreno_perlin(terreno,
                       largura_terreno, 
                       profundidade_terreno, 
                       escala_perlin, 
                       altura_perlin);

  glutDisplayFunc([]() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(g_translateX, g_translateY, g_translateZ);
    glRotatef(g_rotateX, 1, 0, 0);
    glRotatef(g_rotateY, 0, 1, 0);
    glRotatef(g_rotateZ, 0, 0, 1);
    glScalef(g_scaleX, g_scaleY, g_scaleZ);

    // Desenha o terreno como uma malha de quadrados coloridos
    for (int z = 0; z < terreno.profundidade - 1; ++z) {
      for (int x = 0; x < terreno.largura - 1; ++x) {
        int idx0 = z * terreno.largura + x;
        int idx1 = z * terreno.largura + (x + 1);
        int idx2 = (z + 1) * terreno.largura + (x + 1);
        int idx3 = (z + 1) * terreno.largura + x;

        glBegin(GL_QUADS);
        for (int i : { idx0, idx1, idx2, idx3 }) {
          const Vector3D& vertice = terreno.vertices[i];

          float minY = -25.0f, maxY = 25.0f;
          float t = (vertice.y - minY) / (maxY - minY);
          t = std::clamp(t, 0.0f, 1.0f);

          float r, g, b;
          if (t < 0.5f) {
            r = interpolar(t * 2, 0.1f, 0.6f);
            g = interpolar(t * 2, 0.5f, 0.3f);
            b = interpolar(t * 2, 0.1f, 0.1f);
          }
          else {
            r = interpolar((t - 0.5f) * 2, 0.6f, 1.0f);
            g = interpolar((t - 0.5f) * 2, 0.3f, 1.0f);
            b = interpolar((t - 0.5f) * 2, 0.1f, 1.0f);
          }
          glColor3f(r, g, b);

          glVertex3f(vertice.x, vertice.y, vertice.z);
        }
        glEnd();
      }
    }

    glutSwapBuffers();
    });

  glutKeyboardFunc([](unsigned char key, int x, int y) {
    switch (key) {
    case '+': g_scaleX *= 1.1f; g_scaleY *= 1.1f; g_scaleZ *= 1.1f; break;
    case '-': g_scaleX *= 0.9f; g_scaleY *= 0.9f; g_scaleZ *= 0.9f; break;
    case 'x': g_scaleX *= 1.1f; break;
    case 'y': g_scaleY *= 1.1f; break;
    case 'z': g_scaleZ *= 1.1f; break;
    case 'X': g_scaleX *= 0.9f; break;
    case 'Y': g_scaleY *= 0.9f; break;
    case 'Z': g_scaleZ *= 0.9f; break;
    case ' ': exit(0); break;
    }
    glutPostRedisplay();
    });

  glutMotionFunc([](int x, int y) {
    if (mouse.posicao.first < 0 || mouse.posicao.second < 0)
      mouse.posicao = { x, y };

    if (mouse.botao == GLUT_LEFT_BUTTON) {
      const float offsetX = (x - mouse.posicao.first) * mouse.sense;
      const float offsetY = (mouse.posicao.second - y) * mouse.sense;
      g_translateX += offsetX;
      g_translateY += offsetY;
    }
    else if (mouse.botao == GLUT_RIGHT_BUTTON) {
      const int offsetX = x - mouse.posicao.first;
      const int offsetY = y - mouse.posicao.second;
      const float grauX = offsetX / 180.f * -1;
      const float grauY = offsetY / 180.f * -1;
      g_rotateY += grauX * 180.f / (float)std::numbers::pi;
      g_rotateX += grauY * 180.f / (float)std::numbers::pi;
    }

    mouse.posicao = { x, y };
    glutPostRedisplay();
    });

  glutMouseFunc([](int button, int state, int x, int y) {
    mouse.botao = button;
    mouse.posicao = { x, y };
    });

  glutTimerFunc(delay, [](int value) {
    glutPostRedisplay();
    glutTimerFunc(delay, [](int v) { glutPostRedisplay(); glutTimerFunc(delay, [](int) {}, 0); }, 0);
    }, 0);

  glutMainLoop();
  return 0;
}