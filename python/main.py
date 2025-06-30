import glfw
from OpenGL.GL import *
from OpenGL.GLU import *  # Adicione isso
import numpy as np
from noise import pnoise2
import math

WIDTH, HEIGHT = 800, 600

GRID_SIZE = 100
SCALE = 0.1
AMPLITUDE = 4.0

def generate_terrain(size, scale, amplitude):
    vertices = []
    indices = []
    normals = []

    # Gera vértices
    heights = np.zeros((size, size))
    for z in range(size):
        for x in range(size):
            world_x = x * scale
            world_z = z * scale
            height = pnoise2(world_x, world_z, octaves=4, persistence=0.5, lacunarity=2.0) * amplitude
            heights[z, x] = height
            vertices.extend([world_x, height, world_z])

    # Gera normais (diferença central)
    for z in range(size):
        for x in range(size):
            sx = heights[z, min(x+1, size-1)] - heights[z, max(x-1, 0)]
            sz = heights[min(z+1, size-1), x] - heights[max(z-1, 0), x]
            n = np.array([-sx, 2.0, -sz])
            n = n / np.linalg.norm(n)
            normals.extend(n.tolist())

    # Gera índices
    for z in range(size - 1):
        for x in range(size - 1):
            i = z * size + x
            indices.extend([i, i + size, i + 1])
            indices.extend([i + 1, i + size, i + size + 1])

    return (
        np.array(vertices, dtype=np.float32),
        np.array(indices, dtype=np.uint32),
        np.array(normals, dtype=np.float32)
    )

def main():
    if not glfw.init():
        return

    window = glfw.create_window(WIDTH, HEIGHT, "Terreno Procedural - Perlin Noise", None, None)
    glfw.make_context_current(window)

    # Adicione estas linhas:
    glViewport(0, 0, WIDTH, HEIGHT)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluPerspective(45, WIDTH/HEIGHT, 0.1, 100.0)
    glMatrixMode(GL_MODELVIEW)

    vertices, indices, normals = generate_terrain(GRID_SIZE, SCALE, AMPLITUDE)

    vao = glGenVertexArrays(1)
    vbo = glGenBuffers(1)
    ebo = glGenBuffers(1)

    glBindVertexArray(vao)
    glBindBuffer(GL_ARRAY_BUFFER, vbo)
    glBufferData(GL_ARRAY_BUFFER, vertices.nbytes, vertices, GL_STATIC_DRAW)

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.nbytes, indices, GL_STATIC_DRAW)

    glEnableVertexAttribArray(0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * 4, ctypes.c_void_p(0))

    glEnable(GL_DEPTH_TEST)

    # Remova ou comente as linhas de iluminação e cor
    # glEnable(GL_LIGHTING)
    # glEnable(GL_LIGHT0)
    # glLightfv(GL_LIGHT0, GL_POSITION,  (50, 100, 50, 1))
    # glLightfv(GL_LIGHT0, GL_DIFFUSE,   (1, 1, 1, 1))
    # glLightfv(GL_LIGHT0, GL_SPECULAR,  (1, 1, 1, 1))
    # glLightfv(GL_LIGHT0, GL_AMBIENT,   (0.2, 0.2, 0.2, 1))

    # glEnable(GL_COLOR_MATERIAL)
    # glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)

    angle = 0.0

    while not glfw.window_should_close(window):
        glfw.poll_events()
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()
        glTranslatef(-5, -5, -30)
        glRotatef(30, 1, 0, 0)
        glRotatef(angle, 0, 1, 0)
        angle += 0.2

        glBegin(GL_TRIANGLES)
        for i in range(0, len(indices), 3):
            for j in range(3):
                idx = indices[i + j]
                x, y, z = vertices[idx*3:idx*3+3]
                glVertex3f(x, y, z)
        glEnd()

        glfw.swap_buffers(window)

    glfw.terminate()

if __name__ == "__main__":
    main()
