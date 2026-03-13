#ifndef PLATONIC_SOLIDS_H
#define PLATONIC_SOLIDS_H

// Platonic solid vertex/edge data -- all vertices normalized to unit sphere

struct ShapeDescriptor {
  int vertexCount;
  int edgeCount;
  const float (*vertices)[3];
  const int (*edges)[2];
};

// --- Tetrahedron (4V, 6E) ---
static const float TETRA_VERTICES[][3] = {
    {0.0f, 0.0f, 1.0f},
    {0.942809f, 0.0f, -0.333333f},        // sqrt(8/9) = 0.942809
    {-0.471405f, 0.816497f, -0.333333f},  // -sqrt(2/9), sqrt(2/3)
    {-0.471405f, -0.816497f, -0.333333f}, // -sqrt(2/9), -sqrt(2/3)
};
static const int TETRA_EDGES[][2] = {
    {0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3},
};

// --- Cube (8V, 12E) ---
// a = 1/sqrt(3) = 0.57735
// 0=(-,-,-) 1=(+,-,-) 2=(-,+,-) 3=(+,+,-) 4=(-,-,+) 5=(+,-,+) 6=(-,+,+)
// 7=(+,+,+)
static const float CUBE_VERTICES[][3] = {
    {-0.57735f, -0.57735f, -0.57735f}, {0.57735f, -0.57735f, -0.57735f},
    {-0.57735f, 0.57735f, -0.57735f},  {0.57735f, 0.57735f, -0.57735f},
    {-0.57735f, -0.57735f, 0.57735f},  {0.57735f, -0.57735f, 0.57735f},
    {-0.57735f, 0.57735f, 0.57735f},   {0.57735f, 0.57735f, 0.57735f},
};
static const int CUBE_EDGES[][2] = {
    {0, 1}, {1, 3}, {3, 2}, {2, 0}, // front face
    {4, 5}, {5, 7}, {7, 6}, {6, 4}, // back face
    {0, 4}, {1, 5}, {2, 6}, {3, 7}, // connecting
};

// --- Octahedron (6V, 12E) ---
static const float OCTA_VERTICES[][3] = {
    {1.0f, 0.0f, 0.0f},  {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},  {0.0f, 0.0f, -1.0f},
};
// Each vertex connects to 4 neighbors (all except its opposite)
static const int OCTA_EDGES[][2] = {
    {0, 2}, {0, 3}, {0, 4}, {0, 5}, {1, 2}, {1, 3},
    {1, 4}, {1, 5}, {2, 4}, {2, 5}, {3, 4}, {3, 5},
};

// --- Dodecahedron (20V, 30E) ---
// Golden ratio phi = (1+sqrt(5))/2 = 1.618034
// Raw vertices: 8 at (+-1,+-1,+-1), 4 at (0,+-1/phi,+-phi),
// 4 at (+-1/phi,+-phi,0), 4 at (+-phi,0,+-1/phi)
// All normalized to unit sphere
static const float DODECA_VERTICES[][3] = {
    // 8 cube vertices (+-1,+-1,+-1) normalized
    {-0.57735f, -0.57735f, -0.57735f}, // 0
    {-0.57735f, -0.57735f, 0.57735f},  // 1
    {-0.57735f, 0.57735f, -0.57735f},  // 2
    {-0.57735f, 0.57735f, 0.57735f},   // 3
    {0.57735f, -0.57735f, -0.57735f},  // 4
    {0.57735f, -0.57735f, 0.57735f},   // 5
    {0.57735f, 0.57735f, -0.57735f},   // 6
    {0.57735f, 0.57735f, 0.57735f},    // 7
    // 4 at (0, +-1/phi, +-phi) normalized: 1/phi=0.618034, phi=1.618034
    // norm = sqrt(0 + 0.381966 + 2.618034) = sqrt(3) = 1.732051
    {0.0f, -0.356822f, -0.934172f}, // 8:  (0, -1/phi, -phi)/norm
    {0.0f, -0.356822f, 0.934172f},  // 9:  (0, -1/phi, +phi)/norm
    {0.0f, 0.356822f, -0.934172f},  // 10: (0, +1/phi, -phi)/norm
    {0.0f, 0.356822f, 0.934172f},   // 11: (0, +1/phi, +phi)/norm
    // 4 at (+-1/phi, +-phi, 0) normalized
    {-0.356822f, -0.934172f, 0.0f}, // 12
    {-0.356822f, 0.934172f, 0.0f},  // 13
    {0.356822f, -0.934172f, 0.0f},  // 14
    {0.356822f, 0.934172f, 0.0f},   // 15
    // 4 at (+-phi, 0, +-1/phi) normalized
    {-0.934172f, 0.0f, -0.356822f}, // 16
    {-0.934172f, 0.0f, 0.356822f},  // 17
    {0.934172f, 0.0f, -0.356822f},  // 18
    {0.934172f, 0.0f, 0.356822f},   // 19
};
// Dodecahedron edges -- each vertex has degree 3
static const int DODECA_EDGES[][2] = {
    {0, 8},  {0, 12}, {0, 16},  {1, 9},   {1, 12},  {1, 17},  {2, 10}, {2, 13},
    {2, 16}, {3, 11}, {3, 13},  {3, 17},  {4, 8},   {4, 14},  {4, 18}, {5, 9},
    {5, 14}, {5, 19}, {6, 10},  {6, 15},  {6, 18},  {7, 11},  {7, 15}, {7, 19},
    {8, 10}, {9, 11}, {12, 14}, {13, 15}, {16, 17}, {18, 19},
};

// --- Icosahedron (12V, 30E) ---
// Cyclic permutations of (0, +-1, +-phi) normalized to unit sphere
// norm = sqrt(0 + 1 + phi^2) = sqrt(1 + 2.618034) = sqrt(3.618034) = 1.902113
// 1/norm = 0.525731, phi/norm = 0.850651
static const float ICOSA_VERTICES[][3] = {
    {0.0f, -0.525731f, -0.850651f}, // 0
    {0.0f, -0.525731f, 0.850651f},  // 1
    {0.0f, 0.525731f, -0.850651f},  // 2
    {0.0f, 0.525731f, 0.850651f},   // 3
    {-0.525731f, -0.850651f, 0.0f}, // 4
    {-0.525731f, 0.850651f, 0.0f},  // 5
    {0.525731f, -0.850651f, 0.0f},  // 6
    {0.525731f, 0.850651f, 0.0f},   // 7
    {-0.850651f, 0.0f, -0.525731f}, // 8
    {-0.850651f, 0.0f, 0.525731f},  // 9
    {0.850651f, 0.0f, -0.525731f},  // 10
    {0.850651f, 0.0f, 0.525731f},   // 11
};
// Each vertex degree 5
static const int ICOSA_EDGES[][2] = {
    {0, 2},  {0, 4},  {0, 6}, {0, 8}, {0, 10}, {1, 3},   {1, 4},  {1, 6},
    {1, 9},  {1, 11}, {2, 5}, {2, 7}, {2, 8},  {2, 10},  {3, 5},  {3, 7},
    {3, 9},  {3, 11}, {4, 8}, {4, 9}, {5, 8},  {5, 9},   {6, 10}, {6, 11},
    {7, 10}, {7, 11}, {4, 6}, {5, 7}, {8, 9},  {10, 11},
};

static const ShapeDescriptor SHAPES[] = {
    {4, 6, TETRA_VERTICES, TETRA_EDGES},
    {8, 12, CUBE_VERTICES, CUBE_EDGES},
    {6, 12, OCTA_VERTICES, OCTA_EDGES},
    {20, 30, DODECA_VERTICES, DODECA_EDGES},
    {12, 30, ICOSA_VERTICES, ICOSA_EDGES},
};

static const int SHAPE_COUNT = 5;
static const int MAX_VERTICES = 20;
static const int MAX_EDGES = 30; // Dodecahedron and Icosahedron

#endif // PLATONIC_SOLIDS_H
