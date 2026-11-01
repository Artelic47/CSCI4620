#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

// A 3D vector used for positions and normals
struct Vector3 {
    float x, y, z;
};

// A 2D vector used for UV texture coordinates
struct Vector2 {
    float u, v;
};

// One corner of an OBJ face
struct FaceCorner {
    int positionIndex;
    int textureIndex;
    int normalIndex;
};

// A triangle contains three 3D positions
struct Triangle {
    Vector3 a;
    Vector3 b;
    Vector3 c;
};

// Stores the geometry parsed from an OBJ file
struct Mesh {
    std::vector<Vector3> positions;
    std::vector<Vector2> texcoords;
    std::vector<Vector3> normals;
    std::vector<Triangle> triangles;
};

// Convert a positive OBJ index to a C++ vector index
// OBJ indices begin at 1; C++ vector indices begin at 0
int objIndexToCpp(int objIndex, std::size_t size) {
    if (objIndex <= 0) {
        throw std::runtime_error(
            "This basic parser expects positive OBJ indices."
        );
    }

    int index = objIndex - 1;

    if (static_cast<std::size_t>(index) >= size) {
        throw std::runtime_error(
            "OBJ index is outside the corresponding array."
        );
    }

    return index;
}

// Parse one face corner, such as:
// 1/2/3
// 1//3
// 1/2
// 1

FaceCorner parseFaceCorner(const std::string& token) {
    FaceCorner corner = {-1, -1, -1};

    std::stringstream stream(token);
    std::string part;

    // First field: position index
    if (!std::getline(stream, part, '/') || part.empty()) {
        throw std::runtime_error(
            "Face corner is missing a position index."
        );
    }

    corner.positionIndex = std::stoi(part);

    // Second field: optional texture index
    if (std::getline(stream, part, '/')) {
        if (!part.empty()) {
            corner.textureIndex = std::stoi(part);
        }

        // Third field: optional normal index 
        if (std::getline(stream, part, '/')) {
            if (!part.empty()) {
                corner.normalIndex = std::stoi(part);
            }
        }
    }

    return corner;
}

// Read an OBJ file and construct a triangle mesh 
Mesh loadOBJ(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Could not open OBJ file: " + filename
        );
    }

    Mesh mesh;
    std::string line;

    // Read the file dynamically, one line at a time
    while (std::getline(file, line)) {
        std::stringstream stream(line);
        std::string type;

        // Skip empty lines
        if (!(stream >> type)) {
            continue;
        }

        // Skip comments
        if (type[0] == '#') {
            continue;
        }

        if (type == "v") {
            Vector3 position;
            if (stream >> position.x
                       >> position.y
                       >> position.z) {
                mesh.positions.push_back(position);
            }
        }
        else if (type == "vt") {
            Vector2 uv;
            if (stream >> uv.u >> uv.v) {
                mesh.texcoords.push_back(uv);
            }
        }
        else if (type == "vn") {
            Vector3 normal;
            if (stream >> normal.x
                       >> normal.y
                       >> normal.z) {
                mesh.normals.push_back(normal);
            }
        }
        else if (type == "f") {
            std::vector<FaceCorner> corners;
            std::string token;

            // Read every corner listed on this face.
            while (stream >> token) {
                corners.push_back(parseFaceCorner(token));
            }

            if (corners.size() < 3) {
                throw std::runtime_error(
                    "A face must have at least 3 corners."
                );
            }

            // Triangulate a polygon using a triangle fan.
            // For a triangle, this loop runs exactly once.
            for (std::size_t i = 1;
                 i + 1 < corners.size();
                 ++i) {

                int ia = objIndexToCpp(
                    corners[0].positionIndex,
                    mesh.positions.size()
                );

                int ib = objIndexToCpp(
                    corners[i].positionIndex,
                    mesh.positions.size()
                );

                int ic = objIndexToCpp(
                    corners[i + 1].positionIndex,
                    mesh.positions.size()
                );

                Triangle triangle = {
                    mesh.positions[ia],
                    mesh.positions[ib],
                    mesh.positions[ic]
                };

                mesh.triangles.push_back(triangle);
            }
        }

        // Other entries, including o, s, mtllib, and usemtl,
        // are ignored in this minimal geometry parser.
    }

    return mesh;
}

// Print one 3D position.
void printVector(const Vector3& v) {
    std::cout << "("
              << v.x << ", "
              << v.y << ", "
              << v.z << ")";
}

int main(int argc, char* argv[]) {
    // Use triangle.obj unless another filename is supplied.
    std::string filename =
        (argc > 1) ? argv[1] : "triangle.obj";

    try {
        Mesh mesh = loadOBJ(filename);

        std::cout << "OBJ file loaded successfully.\n";
        std::cout << "Positions: "
                  << mesh.positions.size() << "\n";
        std::cout << "Texture coordinates: "
                  << mesh.texcoords.size() << "\n";
        std::cout << "Normals: "
                  << mesh.normals.size() << "\n";
        std::cout << "Triangles: "
                  << mesh.triangles.size() << "\n";

        if (!mesh.triangles.empty()) {
            const Triangle& t = mesh.triangles[0];

            std::cout << "\nFirst triangle:\n";

            std::cout << "Corner 1: ";
            printVector(t.a);
            std::cout << "\n";

            std::cout << "Corner 2: ";
            printVector(t.b);
            std::cout << "\n";

            std::cout << "Corner 3: ";
            printVector(t.c);
            std::cout << "\n";
        }
        else {
            std::cout
                << "No triangles were found in the OBJ file.\n";
        }
    }
    catch (const std::exception& error) {
        std::cerr << "Error: "
                  << error.what() << "\n";
        return 1;
    }

    return 0;
}

