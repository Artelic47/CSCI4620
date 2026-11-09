/*
* Pac-Man Game
*
* Description: A simple 2D game using C++, OpenGL, and GLUT.
* OpenGL draws the maze, characters, and pellets using
* squares and circles. 
* 
* The player uses arrow keys to collect pellets and earn
* points while avoiding the ghost. Both characters must
* stay within the open maze pathways. 
* 
* Features include scoring, collision detection, a win
* condition, and the ability to restart the game. 
*
* Concepts: 2D graphics, geometric shapes, coordinate
* systems, keyboard input, animation, and collision detection.
*/

#include <GLUT/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

const int ROWS = 10;
const int COLS = 10;
const int SIZE = 40;

const float PACMAN_SPEED = 0.25f;
const float GHOST_SPEED = 0.015f;

const int originalMaze[ROWS][COLS] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,1,1,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,0,1,0,1},
    {1,0,1,0,0,0,0,0,0,1},
    {1,0,0,0,1,1,1,1,0,1},
    {1,1,1,1,1,1,1,1,1,1}
};

int maze[ROWS][COLS];

float px, py;
float gx, gy;

int score = 0;
int pelletsLeft = 0;

bool gameOver = false;
bool won = false;

// Draws a filled circle.
void circle(float x, float y, float radius) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);

    for (int i = 0; i <= 30; i++) {
        float angle = 2.0f * 3.1415926f * i / 30.0f;

        glVertex2f(
            x + cos(angle) * radius,
            y + sin(angle) * radius
        );
    }

    glEnd();
}

// Draws text on the screen.
void drawText(float x, float y, const char* message) {
    glRasterPos2f(x, y);

    for (const char* p = message; *p != '\0'; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
}

// Restores the maze and resets the game.
void resetGame() {
    pelletsLeft = 0;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            maze[r][c] = originalMaze[r][c];

            if (maze[r][c] == 0) {
                pelletsLeft++;
            }
        }
    }

    px = 1.5f;
    py = 1.5f;

    gx = 8.5f;
    gy = 8.5f;

    score = 0;
    gameOver = false;
    won = false;

    glutPostRedisplay();
}

// Sets up the game screen.
void reshape(int w, int h) {
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(0, COLS * SIZE, 0, ROWS * SIZE);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Draws the maze, characters, and score.
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {

            float x = c * SIZE + SIZE / 2.0f;
            float y = (ROWS - 1 - r) * SIZE + SIZE / 2.0f;

            if (maze[r][c] == 1) {
                glColor3f(0.0f, 0.2f, 1.0f);

                glBegin(GL_QUADS);
                glVertex2f(x - 19, y - 19);
                glVertex2f(x + 19, y - 19);
                glVertex2f(x + 19, y + 19);
                glVertex2f(x - 19, y + 19);
                glEnd();
            }
            else if (maze[r][c] == 0) {
                glColor3f(1.0f, 1.0f, 1.0f);
                circle(x, y, 3.0f);
            }
        }
    }

    // Display score.
    char scoreText[64];
    std::sprintf(scoreText, "Score: %d", score);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(8.0f, ROWS * SIZE - 22.0f, scoreText);

    // Draw yellow Pac-Man.
    glColor3f(1.0f, 1.0f, 0.0f);
    circle(px * SIZE, (ROWS - py) * SIZE, 12.0f);

    // Draw red ghost.
    glColor3f(1.0f, 0.0f, 0.0f);
    circle(gx * SIZE, (ROWS - gy) * SIZE, 12.0f);

    // Display end-game messages.
    if (gameOver) {
        glColor3f(1.0f, 1.0f, 0.0f);

        if (won) {
            drawText(105.0f, 205.0f, "YOU WIN!");
        }
        else {
            drawText(105.0f, 205.0f, "GAME OVER");
        }

        drawText(55.0f, 175.0f, "Press R to restart");
    }

    glutSwapBuffers();
}

// Moves a character only into an open tunnel cell.
void move(float &x, float &y, float dx, float dy) {
    float nx = x + dx;
    float ny = y + dy;

    int col = static_cast<int>(floor(nx));
    int row = static_cast<int>(floor(ny));

    if (row >= 0 && row < ROWS &&
        col >= 0 && col < COLS &&
        maze[row][col] != 1) {

        x = nx;
        y = ny;
    }
}

// Collects pellets and updates the score.
void collectPellet() {
    int row = static_cast<int>(floor(py));
    int col = static_cast<int>(floor(px));

    if (row >= 0 && row < ROWS &&
        col >= 0 && col < COLS &&
        maze[row][col] == 0) {

        maze[row][col] = 2;
        score += 10;
        pelletsLeft--;

        if (pelletsLeft == 0) {
            won = true;
            gameOver = true;
        }
    }
}

// Updates ghost movement and checks for collisions.
void update(int value) {
    if (!gameOver) {

        collectPellet();

        // Ghost follows Pac-Man at a slower speed.
        float dx = (px > gx) ? GHOST_SPEED :
                   (px < gx ? -GHOST_SPEED : 0.0f);

        float dy = (py > gy) ? GHOST_SPEED :
                   (py < gy ? -GHOST_SPEED : 0.0f);

        move(gx, gy, dx, 0.0f);
        move(gx, gy, 0.0f, dy);

        // Check whether the ghost catches Pac-Man.
        if (fabs(px - gx) < 0.4f &&
            fabs(py - gy) < 0.4f) {

            gameOver = true;
        }

        glutPostRedisplay();
    }

    glutTimerFunc(16, update, 0);
}

// Arrow keys move Pac-Man faster through the tunnels.
void keyboard(int key, int x, int y) {
    if (gameOver) {
        return;
    }

    if (key == GLUT_KEY_UP) {
        move(px, py, 0.0f, -PACMAN_SPEED);
    }
    else if (key == GLUT_KEY_DOWN) {
        move(px, py, 0.0f, PACMAN_SPEED);
    }
    else if (key == GLUT_KEY_LEFT) {
        move(px, py, -PACMAN_SPEED, 0.0f);
    }
    else if (key == GLUT_KEY_RIGHT) {
        move(px, py, PACMAN_SPEED, 0.0f);
    }

    collectPellet();
    glutPostRedisplay();
}

// R restarts the game; Escape exits.
void normalKeyboard(unsigned char key, int x, int y) {
    if (key == 'r' || key == 'R') {
        resetGame();
    }
    else if (key == 27) {
        exit(0);
    }
}

// Starts the OpenGL game.
int main(int argc, char** argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(COLS * SIZE, ROWS * SIZE);

    glutCreateWindow("My Pac-Man Game");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    resetGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(keyboard);
    glutKeyboardFunc(normalKeyboard);

    glutTimerFunc(16, update, 0);

    glutMainLoop();

    return 0;
}
