#include <iostream>
#include <windows.h>
#include <GL/glut.h>
#include <math.h>
#include <cstdlib>
using namespace std;


#define NUM_STARS 100
float starPositions[NUM_STARS][2];
bool  starsInitialized = false;


float vinePhase = 0.0f;


#define MAX_VINES         26
#define MAX_VINE_PTS      14
#define MAX_BRANCHES_PER  5
#define MAX_BRANCH_PTS    7

struct VineBranch {
    int     numPoints;
    float   points[MAX_BRANCH_PTS][2];
    float   baseThickness;
};

struct Vine {
    int          numPoints;
    float        points[MAX_VINE_PTS][2];
    float        topThickness;
    float        bottomThickness;
    GLubyte      colorR, colorG, colorB;
    int          numBranches;
    VineBranch   branches[MAX_BRANCHES_PER];
    float        phaseOffset;          // gives each vine its own sway timing
};

Vine vines[MAX_VINES];
int  totalVines = 0;
bool vinesInitialized = false;


void circle(GLfloat rx, GLfloat ry, GLfloat cx, GLfloat cy) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 360; i++) {
        float angle = 3.1416f * i / 180.0f;
        glVertex2f(rx * cosf(angle) + cx, ry * sinf(angle) + cy);
    }
    glEnd();
}


void myInit() {
    glClearColor(0.30f, 0.08f, 0.08f, 1.0f);    // Deep maroon sky
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 700, 0, 800, -10.0, 10.0);
}





void drawMidpointCircle(int xc, int yc, int r) {
    int x = 0;
    int y = r;
    int d = 1 - r;

    while (x <= y) {

        glBegin(GL_LINES);
            glVertex2i(xc - x, yc + y);  glVertex2i(xc + x, yc + y);
            glVertex2i(xc - x, yc - y);  glVertex2i(xc + x, yc - y);
            glVertex2i(xc - y, yc + x);  glVertex2i(xc + y, yc + x);
            glVertex2i(xc - y, yc - x);  glVertex2i(xc + y, yc - x);
        glEnd();


        if (d < 0) {
            d += 2 * x + 3;             // E was chosen
        } else {
            d += 2 * (x - y) + 5;       // SE was chosen
            y--;
        }
        x++;
    }
}


void drawMoon() {
    int moonX = 350;
    int moonY = 705;
    int moonR = 28;

    glColor3ub(60, 30, 30);
    drawMidpointCircle(moonX, moonY, moonR + 3);   // dim outer halo

    glColor3ub(245, 240, 225);
    drawMidpointCircle(moonX, moonY, moonR);       // bright moon disc
}



void drawStars() {
    if (!starsInitialized) {
        for (int i = 0; i < NUM_STARS; i++) {
            starPositions[i][0] = (float)(rand() % 700);
            starPositions[i][1] = 380.0f + (float)(rand() % 420);
        }
        starsInitialized = true;
    }
    glColor3ub(150, 50, 50);
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_STARS; i++)
        glVertex2f(starPositions[i][0], starPositions[i][1]);
    glEnd();
}


void drawBackground() {
    glBegin(GL_QUADS);
        glColor3ub(60, 20, 20);
        glVertex2f(0,   0);    glVertex2f(700, 0);
        glVertex2f(700, 345);  glVertex2f(0,   345);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(90, 40, 40);
        glVertex2f(0,   340);  glVertex2f(700, 340);
        glVertex2f(700, 120);  glVertex2f(0,   120);
    glEnd();
}



static void drawTaperedSegment(float x1, float y1, float x2, float y2,
                               float w1, float w2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    // Perpendicular unit vector to the segment
    float px = -dy / len;
    float py =  dx / len;

    glBegin(GL_QUADS);
        glVertex2f(x1 + px * w1, y1 + py * w1);
        glVertex2f(x1 - px * w1, y1 - py * w1);
        glVertex2f(x2 - px * w2, y2 - py * w2);
        glVertex2f(x2 + px * w2, y2 + py * w2);
    glEnd();
}



static void initVines() {
    srand(42);   // deterministic layout

    // Cluster pattern: mix of dense groups and sparse gaps.
    float anchorTable[] = {
        // Dense cluster on the left edge
         8,  22,  38,  58,  76,
        // Sparse middle-left
        125, 168,
        // Cluster: center-left
        212, 230, 252, 278,
        // Sparse middle
        335, 378,
        // Cluster: above the building
        425, 448, 472, 498,
        // Sparse mid-right
        545, 578,
        // Dense cluster on the right edge
        618, 638, 658, 678, 692
    };
    int nAnchors = (int)(sizeof(anchorTable) / sizeof(float));
    if (nAnchors > MAX_VINES) nAnchors = MAX_VINES;
    totalVines = nAnchors;

    for (int v = 0; v < totalVines; v++) {
        Vine& vine = vines[v];

        // ---- Spine: 7–12 connected points hanging downward ----
        int numSeg = 7 + rand() % 6;
        if (numSeg > MAX_VINE_PTS) numSeg = MAX_VINE_PTS;
        vine.numPoints = numSeg;

        float length    = 220.0f + (rand() % 240);   // total drop: 220–460
        float curX      = anchorTable[v] + ((rand() % 20) - 10) * 0.5f;
        float curY      = 800.0f;
        float driftBias = ((rand() % 80) - 40) * 0.01f;   // gentle leaning tendency

        for (int s = 0; s < numSeg; s++) {
            vine.points[s][0] = curX;
            vine.points[s][1] = curY;

            float stepDown = length / (numSeg - 1);
            float jitter   = ((rand() % 60) - 30) * 0.18f; // -5.4 .. +5.4
            curX += jitter + driftBias * stepDown * 0.05f;
            curY -= stepDown;
        }

        // ---- Tapering thickness ----
        vine.topThickness    = 1.4f + (rand() % 30) * 0.05f;   // 1.4 .. 2.9
        vine.bottomThickness = 0.5f + (rand() %  8) * 0.05f;   // 0.5 .. 0.85

        // ---- Color variant (all dark) ----
        switch (rand() % 4) {
            case 0: vine.colorR = 12; vine.colorG = 3; vine.colorB = 3; break; // dark red
            case 1: vine.colorR =  6; vine.colorG = 2; vine.colorB = 2; break; // near black
            case 2: vine.colorR = 18; vine.colorG = 4; vine.colorB = 4; break; // muted maroon
            default: vine.colorR = 4; vine.colorG = 1; vine.colorB = 1; break; // very black
        }

        // ---- Branches: 2–5 per vine ----
        int nBr = 2 + rand() % 4;
        if (nBr > MAX_BRANCHES_PER) nBr = MAX_BRANCHES_PER;
        vine.numBranches = nBr;

        for (int b = 0; b < nBr; b++) {
            VineBranch& br = vine.branches[b];

            // Attach somewhere along the middle of the spine
            int   startIdx = 1 + rand() % (numSeg - 2);
            float bx = vine.points[startIdx][0];
            float by = vine.points[startIdx][1];

            int branchSegs = 3 + rand() % 4;          // 3–6 segments
            if (branchSegs > MAX_BRANCH_PTS - 1) branchSegs = MAX_BRANCH_PTS - 1;
            br.numPoints = branchSegs + 1;

            // Initial branch direction: angled out from vertical, then curls back
            float side       = (rand() % 2 == 0) ? 1.0f : -1.0f;
            float angle      = side * (0.6f + (rand() % 50) * 0.008f);
            float branchLen  = 25.0f + (rand() % 60);
            float stepSize   = branchLen / branchSegs;

            br.points[0][0] = bx;
            br.points[0][1] = by;

            for (int s = 1; s <= branchSegs; s++) {
                float dx = sinf(angle) * stepSize;
                float dy = -cosf(angle) * stepSize;
                bx += dx + ((rand() % 40) - 20) * 0.06f;
                by += dy + ((rand() % 20) - 10) * 0.06f;

                br.points[s][0] = bx;
                br.points[s][1] = by;

                // Gravity pulls branch back toward vertical, plus tiny kink
                angle *= 0.85f;
                angle += ((rand() % 30) - 15) * 0.008f;
            }

            br.baseThickness = 0.7f + (rand() % 12) * 0.04f;
        }

        // Phase offset so vines sway out of sync
        vine.phaseOffset = (float)(rand() % 628) * 0.01f;
    }

    vinesInitialized = true;
}


void drawHangingVines() {
    if (!vinesInitialized) initVines();

    for (int v = 0; v < totalVines; v++) {
        const Vine& vine = vines[v];

        // -------- [TRANSLATION] per-vine sway offset -------------
        float tx = 2.5f * sinf(vinePhase + vine.phaseOffset);
        float ty = 0.0f;

        glPushMatrix();
        glTranslatef(tx, ty, 0.0f);    // <<< 2D TRANSLATION applied here

        glColor3ub(vine.colorR, vine.colorG, vine.colorB);

        // ---- Draw the main spine as a chain of tapered quads ----
        for (int s = 0; s < vine.numPoints - 1; s++) {
            float t1 = (float)s / (vine.numPoints - 1);
            float t2 = (float)(s + 1) / (vine.numPoints - 1);
            float w1 = vine.topThickness * (1.0f - t1) + vine.bottomThickness * t1;
            float w2 = vine.topThickness * (1.0f - t2) + vine.bottomThickness * t2;

            drawTaperedSegment(
                vine.points[s][0],     vine.points[s][1],
                vine.points[s + 1][0], vine.points[s + 1][1],
                w1, w2);
        }


        for (int b = 0; b < vine.numBranches; b++) {
            const VineBranch& br = vine.branches[b];
            for (int s = 0; s < br.numPoints - 1; s++) {
                float t1 = (float)s / (br.numPoints - 1);
                float t2 = (float)(s + 1) / (br.numPoints - 1);
                float w1 = br.baseThickness * (1.0f - t1) + 0.25f * t1;
                float w2 = br.baseThickness * (1.0f - t2) + 0.25f * t2;

                drawTaperedSegment(
                    br.points[s][0],     br.points[s][1],
                    br.points[s + 1][0], br.points[s + 1][1],
                    w1, w2);
            }
        }

        glPopMatrix();
    }
}


//   TREES

void drawCircleTree(float wx, float wy) {
    glPushMatrix();
    glTranslatef(wx - 13.5f, wy - 81.0f, 0);
    glScalef(0.9f, 0.9f, 1.0f);

    glColor3ub(18,22,8);  circle(8,22,0,150);
    glColor3ub(18,22,8);  circle(8,22,10,170);
    glColor3ub(18,22,8);  circle(8,22,13,140);
    glColor3ub(18,22,8);  circle(7,25,22,150);
    glColor3ub(18,22,8);  circle(8,22,30,150);
    glColor3ub(18,22,8);  circle(10,40,0,250);
    glColor3ub(12,15,5);  circle(9,22,30,295);
    glColor3ub(12,15,5);  circle(8,15,30,293);
    glColor3ub(12,15,5);  circle(9,22,45,285);
    glColor3ub(12,15,5);  circle(9,22,45,280);
    glColor3ub(12,15,5);  circle(8,15,45,275);
    glColor3ub(12,15,5);  circle(9,22,55,235);
    glColor3ub(12,15,5);  circle(9,32,50,255);
    glColor3ub(12,15,5);  circle(9,22,59,225);
    glColor3ub(12,15,5);  circle(9,22,56,255);
    glColor3ub(12,15,5);  circle(9,22,63,195);
    glColor3ub(12,15,5);  circle(9,22,50,180);
    glColor3ub(12,15,5);  circle(9,22,58,165);
    glColor3ub(12,15,5);  circle(9,22,49,150);
    glColor3ub(12,15,5);  circle(9,22,38,140);
    glColor3ub(8,12,3);   circle(10,20,55,190);
    glColor3ub(12,15,5);  circle(9.5f,19,55,190);
    glColor3ub(8,12,3);   circle(10,20,55,205);
    glColor3ub(12,15,5);  circle(9.5f,20,55,205);
    glColor3ub(8,12,3);   circle(10,20,50,218);
    glColor3ub(12,15,5);  circle(9.5f,20,50,218);
    glColor3ub(12,15,5);  circle(10,20,27,130);
    glColor3ub(12,15,5);  circle(35,70,20,200);
    glColor3ub(12,15,5);  circle(15,30,30,255);
    glColor3ub(8,12,3);   circle(10,20,42,225);
    glColor3ub(12,15,5);  circle(9.5f,20,42,224);
    glColor3ub(8,12,3);   circle(10,20,30,225);
    glColor3ub(12,15,5);  circle(10,20,30,224);
    glColor3ub(8,12,3);   circle(10,18,42,263);
    glColor3ub(12,15,5);  circle(10,18,42,262);
    glColor3ub(8,12,3);   circle(10,20,30,180);
    glColor3ub(12,15,5);  circle(10,20,30,179);
    glColor3ub(8,12,3);   circle(10,20,20,180);
    glColor3ub(12,15,5);  circle(10,20,20,179);
    glColor3ub(8,12,3);   circle(10,20,40,155);
    glColor3ub(12,15,5);  circle(10,20,40,156);
    glColor3ub(12,15,5);  circle(9,22,20,280);
    glColor3ub(8,12,3);   circle(9,21,20,270);
    glColor3ub(12,15,5);  circle(9,21,20,269);
    glColor3ub(12,15,5);  circle(9,22,10,255);
    glColor3ub(8,12,3);   circle(9,20,10,245);
    glColor3ub(12,15,5);  circle(9,20,10.5f,244);

    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(75,35,5);
    glVertex2f(15,90);  glVertex2f(22,90);  glVertex2f(21,100); glVertex2f(20,110);
    glVertex2f(18,120); glVertex2f(16,130); glVertex2f(17,140); glVertex2f(18,145);
    glVertex2f(20,150); glVertex2f(22,150); glVertex2f(21,147); glVertex2f(20,145);
    glVertex2f(18,140); glVertex2f(16,130); glVertex2f(13,120); glVertex2f(16,130);
    glVertex2f(18,140); glVertex2f(20,145); glVertex2f(22,150); glVertex2f(22,160);
    glVertex2f(18,150);
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(75,35,5);
    glVertex2f(15,125); glVertex2f(19,130); glVertex2f(14,140); glVertex2f(14,150);
    glVertex2f(13,160); glVertex2f(10,170); glVertex2f(12,170); glVertex2f(12,160);
    glVertex2f(11,150); glVertex2f(11,140);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(75,35,5);
    glVertex2f(31,140); glVertex2f(29,140); glVertex2f(28,120); glVertex2f(25,120);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(75,35,5);
    glVertex2f(16,100); glVertex2f(21,100); glVertex2f(24.5f,120); glVertex2f(28,120);
    glEnd();

    glPopMatrix();
}

// ---- Tree 2
void drawPineTree(float wx, float wy) {
    glPushMatrix();
    glTranslatef(wx - 261.0f, wy - 81.0f, 0);
    glScalef(0.9f, 0.9f, 1.0f);

    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(75,35,5);
    glVertex2f(290,90);  glVertex2f(295,90);
    glVertex2f(295,120); glVertex2f(290,120);
    glEnd();

    glBegin(GL_TRIANGLE_FAN); glColor3ub(18,22,8);
    glVertex2f(280,120); glVertex2f(305,120); glVertex2f(292.5f,180); glVertex2f(292.5f,180); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(15,18,5);
    glVertex2f(281,135); glVertex2f(304,135); glVertex2f(292.5f,190); glVertex2f(292.5f,190); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(18,22,8);
    glVertex2f(282,150); glVertex2f(303,150); glVertex2f(292.5f,180); glVertex2f(292.5f,180); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(15,18,5);
    glVertex2f(283,160); glVertex2f(302,160); glVertex2f(292.5f,190); glVertex2f(292.5f,190); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(18,22,8);
    glVertex2f(284,170); glVertex2f(301,170); glVertex2f(292.5f,200); glVertex2f(292.5f,200); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(15,18,5);
    glVertex2f(285,180); glVertex2f(300,180); glVertex2f(292.5f,210); glVertex2f(292.5f,210); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(18,22,8);
    glVertex2f(286,190); glVertex2f(299,190); glVertex2f(292.5f,260); glVertex2f(292.5f,260); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(15,18,5);
    glVertex2f(286,200); glVertex2f(299,200); glVertex2f(292.5f,270); glVertex2f(292.5f,270); glEnd();

    glPopMatrix();
}

// ---- Tree 3
void drawHospitalPineTree(float wx, float wy) {
    glPushMatrix();
    glTranslatef(wx - 585.0f, wy - 81.0f, 0);
    glScalef(0.9f, 0.9f, 1.0f);

    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(139,99,47);
    glVertex2f(650,90);  glVertex2f(655,90);
    glVertex2f(655,110); glVertex2f(650,110);
    glEnd();

    glBegin(GL_TRIANGLE_FAN); glColor3ub(10,22,8);
    glVertex2f(640,110); glVertex2f(665,110); glVertex2f(652.5f,140); glVertex2f(652.5f,140); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(8,18,5);
    glVertex2f(641,120); glVertex2f(664,120); glVertex2f(652.5f,160); glVertex2f(652.5f,160); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(10,22,8);
    glVertex2f(642,130); glVertex2f(663,130); glVertex2f(652.5f,180); glVertex2f(652.5f,180); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(8,18,5);
    glVertex2f(642.5f,140); glVertex2f(662.5f,140); glVertex2f(652.5f,185); glVertex2f(652.5f,185); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(10,22,8);
    glVertex2f(643,150);  glVertex2f(662,150);  glVertex2f(652.5f,195); glVertex2f(652.5f,195); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(8,18,5);
    glVertex2f(643.5f,160); glVertex2f(661.5f,160); glVertex2f(652.5f,210); glVertex2f(652.5f,210); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(10,22,8);
    glVertex2f(644,170);  glVertex2f(661,170);  glVertex2f(652.5f,230); glVertex2f(652.5f,230); glEnd();
    glBegin(GL_TRIANGLE_FAN); glColor3ub(8,18,5);
    glVertex2f(644.5f,180); glVertex2f(660.5f,180); glVertex2f(652.5f,250); glVertex2f(652.5f,250); glEnd();

    glPopMatrix();
}

// ---- Tree 4
void drawSimpleTree(float wx, float wy) {
    glPushMatrix();
    glTranslatef(wx - 140.0f, wy - 150.0f, 0);

    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(75, 35, 5);
    glVertex3f(140, 150, 0); glVertex3f(150, 150, 0);
    glVertex3f(150, 190, 0); glVertex3f(140, 190, 0);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(15, 20, 5);
    glVertex3f(130, 180, 0); glVertex3f(160, 180, 0);
    glVertex3f(145, 230, 0); glVertex3f(145, 230, 0);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(20, 26, 8);
    glVertex3f(133, 190, 0); glVertex3f(157, 190, 0);
    glVertex3f(145, 240, 0); glVertex3f(145, 240, 0);
    glEnd();

    glPopMatrix();
}



//   BUILDING – HAWKINS LAB

void drawBuilding() {
    glPushMatrix();
    glTranslatef(103.0f, 150.0f, 0.0f);
    glScalef(0.9f, 0.9f, 1.0f);

    // -------- LEFT LOW WING --------
    glBegin(GL_QUADS);
        glColor3ub(48, 42, 44);
        glVertex2f(348, 220); glVertex2f(390, 220);
        glVertex2f(390, 275); glVertex2f(348, 275);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(60, 53, 55);
        glVertex2f(348, 268); glVertex2f(390, 268);
        glVertex2f(390, 275); glVertex2f(348, 275);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(28, 24, 26);
        glVertex2f(344, 275); glVertex2f(392, 275);
        glVertex2f(392, 282); glVertex2f(344, 282);
    glEnd();

    // -------- RIGHT LOW WING --------
    glBegin(GL_QUADS);
        glColor3ub(48, 42, 44);
        glVertex2f(458, 220); glVertex2f(495, 220);
        glVertex2f(495, 275); glVertex2f(458, 275);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(60, 53, 55);
        glVertex2f(458, 268); glVertex2f(495, 268);
        glVertex2f(495, 275); glVertex2f(458, 275);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(28, 24, 26);
        glVertex2f(456, 275); glVertex2f(499, 275);
        glVertex2f(499, 282); glVertex2f(456, 282);
    glEnd();

    // -------- CENTRAL TALLER BLOCK --------
    glBegin(GL_QUADS);
        glColor3ub(55, 48, 50);
        glVertex2f(390, 220); glVertex2f(458, 220);
        glVertex2f(458, 305); glVertex2f(390, 305);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(68, 60, 62);
        glVertex2f(390, 296); glVertex2f(458, 296);
        glVertex2f(458, 305); glVertex2f(390, 305);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(35, 30, 32);
        glVertex2f(390, 220); glVertex2f(458, 220);
        glVertex2f(458, 228); glVertex2f(390, 228);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(28, 24, 26);
        glVertex2f(386, 305); glVertex2f(462, 305);
        glVertex2f(462, 313); glVertex2f(386, 313);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(80, 70, 72);
        glVertex2f(386, 311); glVertex2f(462, 311);
        glVertex2f(462, 313); glVertex2f(386, 313);
    glEnd();


    glBegin(GL_QUADS);
        glColor3ub(60, 52, 54);
        glVertex2f(479, 282); glVertex2f(483, 282);
        glVertex2f(483, 335); glVertex2f(479, 335);
    glEnd();
    glBegin(GL_LINES);
        glColor3ub(72, 62, 64);
        glVertex2f(474, 322); glVertex2f(488, 322);
    glEnd();


    glColor3ub(90, 8, 8);
    circle(5, 6, 481, 340);
    glColor3ub(225, 30, 30);
    circle(2.6f, 3.2f, 481, 340);


    glBegin(GL_QUADS);
        glColor3ub(15, 12, 14);
        glVertex2f(416, 220); glVertex2f(432, 220);
        glVertex2f(432, 253); glVertex2f(416, 253);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(35, 30, 32);
        glVertex2f(414, 253); glVertex2f(434, 253);
        glVertex2f(434, 257); glVertex2f(414, 257);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(35, 30, 32);
        glVertex2f(412, 218); glVertex2f(436, 218);
        glVertex2f(436, 222); glVertex2f(412, 222);
    glEnd();


    glBegin(GL_QUADS); glColor3ub(22, 18, 20);
        glVertex2f(354, 240); glVertex2f(370, 240);
        glVertex2f(370, 258); glVertex2f(354, 258);
    glEnd();
    glBegin(GL_QUADS); glColor3ub(22, 18, 20);
        glVertex2f(374, 240); glVertex2f(388, 240);
        glVertex2f(388, 258); glVertex2f(374, 258);
    glEnd();
    glBegin(GL_QUADS); glColor3ub(22, 18, 20);
        glVertex2f(461, 240); glVertex2f(475, 240);
        glVertex2f(475, 258); glVertex2f(461, 258);
    glEnd();
    glBegin(GL_QUADS); glColor3ub(22, 18, 20);
        glVertex2f(478, 240); glVertex2f(493, 240);
        glVertex2f(493, 258); glVertex2f(478, 258);
    glEnd();


    glBegin(GL_QUADS);
        glColor3ub(40, 35, 37);
        glVertex2f(391, 263); glVertex2f(457, 263);
        glVertex2f(457, 286); glVertex2f(391, 286);
    glEnd();
    glBegin(GL_QUADS);
        glColor3ub(50, 44, 46);
        glVertex2f(393, 265); glVertex2f(455, 265);
        glVertex2f(455, 284); glVertex2f(393, 284);
    glEnd();
    glColor3ub(195, 190, 180);
    glRasterPos2i(396, 271);
    const char* signText = "HAWKINS LAB";
    for (int i = 0; signText[i] != '\0'; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, signText[i]);


    glColor3ub(28, 24, 26);
    glBegin(GL_LINES);
        glVertex2f(348, 235); glVertex2f(495, 235);
    glEnd();

    glPopMatrix();
}



void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // ---- SKY LAYER ----
    drawStars();
    drawMoon();

    // ---- GROUND LAYER ----
    drawBackground();

    // ---- HAWKINS LAB ----
    drawBuilding();

    // ---- TREES ----
    drawCircleTree(30,  220);
    drawCircleTree(655, 220);

    drawPineTree(105, 200);
    drawPineTree(225, 200);
    drawPineTree(530, 200);

    drawHospitalPineTree(570, 220);
    drawHospitalPineTree(688, 220);

    drawSimpleTree(170, 230);
    drawSimpleTree(290, 230);
    drawSimpleTree(460, 230);
    drawSimpleTree(617, 230);

    drawHangingVines();

    glutSwapBuffers();
}



void updateAnimation(int value) {
    vinePhase += 0.05f;
    if (vinePhase > 6.2832f)
        vinePhase -= 6.2832f;

    glutPostRedisplay();
    glutTimerFunc(30, updateAnimation, 0);
}


//   main()

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    int windowWidth  = 1450;
    int windowHeight = 750;
    glutInitWindowSize(windowWidth, windowHeight);

    int screenWidth  = glutGet(GLUT_SCREEN_WIDTH);
    int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
    glutInitWindowPosition((screenWidth  - windowWidth)  / 2,
                           (screenHeight - windowHeight) / 2);

    glutCreateWindow("Upside Down – Hawkins");

    myInit();
    glutDisplayFunc(display);
    glutTimerFunc(30, updateAnimation, 0);

    glutMainLoop();
    return 0;
}
