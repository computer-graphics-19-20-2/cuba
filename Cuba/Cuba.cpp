#include "stdafx.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include <GL/glut.h>

#include "bevgrafmath.h"


static constexpr int WINDOW_WIDTH = 1024;
static constexpr int WINDOW_HEIGHT = 768;
static constexpr int BEZIER_SIZE = 300;
static constexpr int POINT_SIZE = 10;
static constexpr int CLICK_THRESHOLD = 15;

int updateQueuedFor = 0;

std::vector<vec2> movableControlPoints;
std::vector<vec2> fixedControlPoints;
std::vector<vec2> bezierCurvePoints;
bool shouldRecalculateBezier;

vec2 *draggedControlPoint = nullptr;

vec2 square[4]{
	{ -25, 25 },
	{ 25, 25 },
	{ 25, -25 },
	{ -25, -25 }
};

mat4 translationMatrix, scalingMatrix, rotationMatrix, transformation;
vec3 fillColor;


/* Function forward declarations */
/* Initialization Functions */
void initGL();
void initWindow();
void initEventHandlers();
void initBezier();

void setDefaultTransformations();

/* Game Loop and its friends */
void gameLoop();
void update(const int delta);
void draw();
void drawSquare();
void drawBezier();

/* Event Handlers */
void onMouseClick(const int button, const int action, const int xMouse, const int yMouse);
void onMouseMove(const int xMouse, const int yMouse);
void onKeyDown(const unsigned char key, const int x, const int y);

/* Square Property Animators */
void squarePosition(const vec2 &translationVector);
void squareSize(const double lambda);
void squareAngle(const double angle);
void squareColor(const vec3 cl);

/* Bezier-curve related stuff */
bool isInBezierBox(float x, float y);
void recalculateBezier();

/* The main man, this whole program is made for. */
void makeAnimation();

vec2 *getClickedPoint(const vec2& cursorPosition, std::vector<vec2>& controlPoints);


int main(int argc, char** argv) {
	glutInit(&argc, argv);

	initWindow();

	initGL();

	initEventHandlers();

	initBezier();

	setDefaultTransformations();

	glutMainLoop();

	return 0;
}

void makeAnimation() {
	// TODO: Create the animation!
}

void initGL(void) {
	glClearColor(1.0, 1.0, 1.0, 1.0);

	glPointSize((float) POINT_SIZE);

	glMatrixMode(GL_PROJECTION);
	gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
}

void initWindow() {
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("CUBA: Cubic Bezier Animation");
}

void initEventHandlers() {
	glutIdleFunc(gameLoop);
	glutDisplayFunc(gameLoop);

	glutMouseFunc(onMouseClick);
	glutMotionFunc(onMouseMove);
	glutKeyboardFunc(onKeyDown);
}

void initBezier() {
	const float quarterBezierSize = (float)BEZIER_SIZE * 0.25f;

	fixedControlPoints.push_back(vec2{ 0.0, WINDOW_HEIGHT - BEZIER_SIZE });
	fixedControlPoints.push_back(vec2{ (float)BEZIER_SIZE, (float)WINDOW_HEIGHT });

	movableControlPoints.push_back(vec2{ quarterBezierSize, WINDOW_HEIGHT - quarterBezierSize });
	movableControlPoints.push_back(vec2{ BEZIER_SIZE - quarterBezierSize, WINDOW_HEIGHT - (3 * quarterBezierSize) });

	shouldRecalculateBezier = true;
}

void setDefaultTransformations() {
	squarePosition(vec2{ 50, WINDOW_HEIGHT / 2.0 });
	squareAngle(0.0);
	squareSize(1.0);
	squareColor(vec3{ 0.0, 0.0, 0.0 });
}

void gameLoop() {
	const int startTime = glutGet(GLUT_ELAPSED_TIME);

	while (updateQueuedFor > 10) {
		update(10);
		updateQueuedFor -= 10;
	}

	draw();

	const int lastGameLoopTime = glutGet(GLUT_ELAPSED_TIME);
	const int diff = lastGameLoopTime - startTime;
	updateQueuedFor += diff;
}

void update(const int delta) {
	// TODO: Update the animation!

	if (shouldRecalculateBezier) {
		recalculateBezier();

		shouldRecalculateBezier = false;
	}

	transformation = translationMatrix * rotationMatrix * scalingMatrix;
}

void draw() {
	glClear(GL_COLOR_BUFFER_BIT);

	drawSquare();

	drawBezier();

	glFlush();

	glutSwapBuffers();
}

void drawSquare() {
	glColor3f(fillColor.x, fillColor.y, fillColor.z);

	glBegin(GL_POLYGON);

	for (const vec2 &v : square) {
		const vec4 h = transformation * vec4{ v.x, v.y, 0, 1 };

		const vec3 ih = hToIh(h);

		glVertex2f(ih.x, ih.y);
	}

	glEnd();
}

void drawBezier() {
	glColor3f(0.f, 0.f, 0.f);

	glBegin(GL_LINE_STRIP);

	for (const vec2 &v : bezierCurvePoints) {
		glVertex2f(v.x, v.y);
	}

	glEnd();

	glBegin(GL_POINTS);

	glVertex2f(fixedControlPoints[0].x, fixedControlPoints[0].y);
	glVertex2f(fixedControlPoints[1].x, fixedControlPoints[1].y);

	glColor3f(1.0, 0.0, 0.0);
	glVertex2f(movableControlPoints[0].x, movableControlPoints[0].y);
	glVertex2f(movableControlPoints[1].x, movableControlPoints[1].y);

	glEnd();
}

void onMouseClick(const int button, const int action, const int xMouse, const int yMouse) {
	if (button == GLUT_LEFT_BUTTON && action == GLUT_DOWN) {
		const vec2 cursorPosition = { (float)xMouse, (float)WINDOW_HEIGHT - yMouse };

		vec2 *pointUnderCursor = getClickedPoint(cursorPosition, movableControlPoints);

		if (pointUnderCursor) {
			draggedControlPoint = pointUnderCursor;
		}
	}

	if (button == GLUT_LEFT_BUTTON && action == GLUT_UP) {
		draggedControlPoint = nullptr;
	}
}

void onMouseMove(const int xMouse, const int yMouse) {
	if (draggedControlPoint && isInBezierBox((float)xMouse, (float) WINDOW_HEIGHT - yMouse)) {
		draggedControlPoint->x = (float)xMouse;
		draggedControlPoint->y = (float)WINDOW_HEIGHT - yMouse;

		shouldRecalculateBezier = true;
	}
}

void onKeyDown(const unsigned char key, const int x, const int y) {
	// Is enter
	if (key == 13) {
		// TODO: Start the animation!
	}
}


void squarePosition(const vec2 &translationVector) {
	translationMatrix = translate(vec3{ translationVector.x, translationVector.y, 0 });
}

void squareSize(const double lambda) {
	scalingMatrix = scale(vec3{ (float)lambda, (float)lambda, (float)lambda });
}

void squareAngle(const double angle) {
	rotationMatrix = rotateZ((float)angle);
}

void squareColor(const vec3 cl) {
	fillColor = cl;
}

bool isInBezierBox(float x, float y) {
	if (x < fixedControlPoints[0].x || x > fixedControlPoints[1].x) {
		return false;
	}

	if (y < fixedControlPoints[0].y || y > fixedControlPoints[1].y) {
		return false;
	}

	return true;
}

void recalculateBezier() {
	bezierCurvePoints.clear();

	const float delta = 0.05f;

	for (float t = 0; t < 1 + delta; t += delta) {
		const float inv = 1 - t;
		const float q = 3 * inv * t;

		const float c0 = inv * inv * inv;
		const float c1 = q * inv;
		const float c2 = q * t;
		const float c3 = t * t * t;

		bezierCurvePoints.push_back(
			c0 * fixedControlPoints[0]
			+ c1 * movableControlPoints[0]
			+ c2 * movableControlPoints[1]
			+ c3  * fixedControlPoints[1]);
	}
}

vec2 *getClickedPoint(const vec2& cursorPosition, std::vector<vec2>& controlPoints) {
	const auto clickedIterator = std::find_if(controlPoints.begin(), controlPoints.end(), [&cursorPosition](const vec2& point) {
		return dist2(cursorPosition, point) <= CLICK_THRESHOLD;
	});

	if (clickedIterator == controlPoints.end()) {
		return nullptr;
	} else {
		return &(*clickedIterator);
	}
}
