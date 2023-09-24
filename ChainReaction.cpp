#include <glad.h>
#include <glfw3.h>
#include "Draw.h"
#include "GLXtras.h"
#include "Sprite.h"
#include <iostream>
#include <time.h>
#include <vector>

const int nCells = 10; // grid is nCells X nCells
int winWidth = 600, winHeight = 600, margin = 40;
Sprite play, background, title, replay, winner1, winner2;
string dir = "C:/Users/Documents/Team6/Images/";
bool gameStarted = false;
bool gameStatus = true;
int playerWon = 0;

class Cell {
public:
	int i = 0, j = 0;
	int nDisks = 0;
	int owner = 0; // 0 for unowned, 1 for player1, 2 for player2
	vec2 p;
	vec2 getPoint(int i);	//out of n-disks get the ith disk
	void Draw(float cellSize);
};

class Grid {
public:
	Cell cells[nCells][nCells];
	float xMargin, yMargin, width, height;
	float cellWidth;	// # width of each cell in the grid
	vec3 color;	// # color of grid�
	int currentPlayer = 1;
	void Draw();
	bool GameStatus();
	bool UpdateOnce();
	void Update();
	bool ReadyToExplode();
	void Reset();
	bool Hit(float xMouse, float yMouse, int& iIndex, int& jIndex);
	vec2 gridLoc(int iIndex, int jIndex);
	Grid(float x, float y, float width, float height, vec3 color);
};

Grid grid((float)margin, (float)margin, (float)winWidth - 2 * margin, (float)winHeight - 2 * margin, vec3(1, 0, 0));
Grid* g = &grid;

int hitI = 0, hitJ = 0;

vec3 backColor(0, 0, 0), yel(1, 1, 0), cyn(0, 1, 1), green(0, 1, 0), blue(0, 0, 1);

vec2 Cell::getPoint(int i) {	//out of n-disks get the ith disk
	if (nDisks == 1)
		return p;
	if (nDisks == 2) {
		if (i == 0) return p + vec2(g->cellWidth) / 10;
		if (i == 1) return p - vec2(g->cellWidth) / 10;
	}
	if (nDisks == 3) {
		if (i == 0) return p + vec2(0.f, g->cellWidth) / 10;
		if (i == 3) return p + g->cellWidth * vec2(-1.f, (float)-sqrt(2)) / 10;
		if (i == 2) return p + g->cellWidth * vec2(1.f, (float)-sqrt(2)) / 10;
	}
	return p;
}

void Cell::Draw(float cellSize) {
	if (nDisks == 1)
		Disk(p, 18, owner == 1 ? green : blue); // use green color for player 1 and blue color for player 2
	if (nDisks == 2) {
		Disk(p + vec2(cellSize) / 10, 18, owner == 1 ? green : blue);
		Disk(p - vec2(cellSize) / 10, 18, owner == 1 ? green : blue);
	}
	if (nDisks == 3) {
		Disk(p + vec2(0.f, cellSize) / 10, 18, owner == 1 ? green : blue);
		Disk(p + cellSize * vec2(-1.f, (float)-sqrt(2)) / 10, 18, owner == 1 ? green : blue);
		Disk(p + cellSize * vec2(1.f, (float)-sqrt(2)) / 10, 18, owner == 1 ? green : blue);
	}
	if (nDisks > 3)
		Disk(p, 24, yel);
}

void Grid::Draw() {
	float xSpacing = width / nCells, ySpacing = height / nCells;	//width and height of grid
	UseDrawShader(ScreenMode());
	for (int i = 0; i < nCells + 1; i++) {
		// vertical
		float xLine = xMargin + i * xSpacing;
		Line(vec2(xLine, yMargin), vec2(xLine, yMargin + height), 2, color);
		// horizontal
		float yLine = yMargin + i * ySpacing;
		Line(vec2(xMargin, yLine), vec2(xMargin + width, yLine), 2, color);
	}
	// draw disks
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++) {
			Cell c = cells[i][j];
			if (c.owner > 0)
				c.Draw(cellWidth);
		}
}

// Animation

struct MovingDisk {
	int iStart, jStart;
	int iStop, jStop;
	MovingDisk(int iStart, int jStart, int iStop, int jStop) :
		iStart(iStart), jStart(jStart), iStop(iStop), jStop(jStop) { }
	MovingDisk() { }
};

std::vector<MovingDisk> movingDisks;
bool animating = false;
float animationDuration = 1; // in seconds
time_t animationStart = 0;

void AddMovingDisk(int iStart, int jStart, int iStop, int jStop) {
	MovingDisk md(iStart, jStart, iStop, jStop);
	movingDisks.push_back(md);
	animating = true;
	animationStart = clock();
}

void FinishUpdate() {	//Finish the animations update.
	for (size_t i = 0; i < movingDisks.size(); i++) {
		MovingDisk md = movingDisks[i];
		Cell& cStart = g->cells[md.iStart][md.jStart];
		Cell& cStop = g->cells[md.iStop][md.jStop];
		cStop.nDisks++;
		cStop.owner = cStart.owner;
	}
	for (size_t i = 0; i < movingDisks.size(); i++) {
		MovingDisk md = movingDisks[i];
		Cell& cStart = g->cells[md.iStart][md.jStart];
		cStart.nDisks = 0;
		cStart.owner = 0;
	}
	movingDisks.resize(0);
}

bool Grid::GameStatus() {
	bool isPlayer1Alive = false;
	bool isPlayer2Alive = false;
	playerWon = 0;
	for (int i = 0; i < nCells; i++) {
		for (int j = 0; j < nCells; j++) {
			if (cells[i][j].owner == 1) {
				isPlayer1Alive = true;
				playerWon = 1;
			}
			else if (cells[i][j].owner == 2) {
				isPlayer2Alive = true;
				playerWon = 2;
			}
			if ((isPlayer1Alive && isPlayer2Alive) == true)	return true;	//game continues if both players alive
		}
	}
	return false;
}

bool Grid::ReadyToExplode() {
	for (int i = 0; i < nCells; i++) {
		for (int j = 0; j < nCells; j++) {
			Cell c = cells[i][j];
			if (((i == j && (i == 0 || i == nCells - 1)) || (i == 0 && j == nCells - 1) ||
				 (i == nCells - 1 && j == 0)) && (c.nDisks > 1))	//corners
					return true;
			if (i == 0 && c.nDisks > 2)	//left edge
					return true;
			if (i == nCells - 1 && c.nDisks > 2)	//right edge
					return true;
			if (j == 0 && c.nDisks > 2)	//bottom edge
					return true;
			if (j == nCells - 1 && c.nDisks > 2)	//top edge
					return true;
			if (i > 0 && i < nCells - 1 && j > 0 && j < nCells - 1 && c.nDisks > 3)	//middle cells
					return true;
		}
	}
	return false;
}

bool Grid::UpdateOnce() {
	for (int i = 0; i < nCells; i++) {
		for (int j = 0; j < nCells; j++) {
			Cell &c = cells[i][j];
			if (((i == j && (i == 0 || i == nCells - 1)) || (i == 0 && j == nCells - 1) || (i == nCells - 1 && j == 0)) && (c.nDisks > 1)) {	//corners
				if (i == 0 && j == 0) {
					AddMovingDisk(i, j, i + 1, j);
					AddMovingDisk(i, j, i, j + 1);
					c.nDisks = 0;
				}
				else if (i == nCells - 1 && j == nCells - 1) {
					AddMovingDisk(i, j, i - 1, j);
					AddMovingDisk(i, j, i, j - 1);
					c.nDisks = 0;
				}
				else if (i == 0 && j == nCells - 1) {
					AddMovingDisk(i, j, i + 1, j);
					AddMovingDisk(i, j, i, j - 1);
					c.nDisks = 0;
				}
				else if (i == nCells - 1 && j == 0) {
					AddMovingDisk(i, j, i - 1, j);
					AddMovingDisk(i, j, i, j + 1);
					c.nDisks = 0;
				}
			}
			else if (i == 0 && c.nDisks > 2) {	//left edge
				AddMovingDisk(i, j, i + 1, j);
				AddMovingDisk(i, j, i, j + 1);
				AddMovingDisk(i, j, i, j - 1);
				c.nDisks = 0;
			}
			else if (i == nCells - 1 && c.nDisks > 2) {	//right edge
				AddMovingDisk(i, j, i - 1, j);
				AddMovingDisk(i, j, i, j + 1);
				AddMovingDisk(i, j, i, j - 1);
				c.nDisks = 0;
			}
			else if (j == 0 && c.nDisks > 2) {	//bottom edge
				AddMovingDisk(i, j, i + 1, j);
				AddMovingDisk(i, j, i, j + 1);
				AddMovingDisk(i, j, i - 1, j);
				c.nDisks = 0;
			}
			else if (j == nCells - 1 && c.nDisks > 2) {	//top edge
				AddMovingDisk(i, j, i + 1, j);
				AddMovingDisk(i, j, i, j - 1);
				AddMovingDisk(i, j, i - 1, j);
				c.nDisks = 0;
			}
			else if (i > 0 && i < nCells - 1 && j > 0 && j < nCells - 1 && c.nDisks > 3) {	//middle cells
				AddMovingDisk(i, j, i + 1, j);
				AddMovingDisk(i, j, i - 1, j);
				AddMovingDisk(i, j, i, j + 1);
				AddMovingDisk(i, j, i, j - 1);
				c.nDisks = 0;
			}
		}
	}
	return false;
}

void Grid::Update() {
	for (; UpdateOnce(); );
}

bool Grid::Hit(float xMouse, float yMouse, int& iIndex, int& jIndex) {
	iIndex = (int)((xMouse - xMargin) / (width / nCells));
	jIndex = (int)((yMouse - yMargin) / (height / nCells));
	bool hit = iIndex >= 0 && iIndex < nCells && jIndex >= 0 && jIndex < nCells && (xMouse - xMargin) > 0 && (yMouse - yMargin) > 0;
	if (hit) {
		Cell& c = cells[iIndex][jIndex];
		if (c.owner == 0) {
			c.owner = currentPlayer;
		}
		if (c.owner == currentPlayer) {
			c.nDisks++;
			// switch to next player
			currentPlayer = currentPlayer == 1 ? 2 : 1;
		}
	}
	return hit;
}

vec2 Grid::gridLoc(int iIndex, int jIndex) {
	float Vx = iIndex * (width / nCells) + xMargin + cellWidth / 2;
	float Vy = jIndex * (width / nCells) + yMargin + cellWidth / 2;
	return vec2(Vx, Vy);
}

Grid::Grid(float x, float y, float w, float h, vec3 c) {
	xMargin = x;
	yMargin = y;
	width = w;
	height = h;
	color = c;
	cellWidth = width / nCells;
	for (int i = 0; i < nCells; i++) {
		for (int j = 0; j < nCells; j++) {
			Cell& c = cells[i][j];
			c.i = i;
			c.j = j;
			c.p = g->gridLoc(i, j);
		}
	}
}

void Grid::Reset() {
	for (int i = 0; i < nCells; i++) {
		for (int j = 0; j < nCells; j++) {
			Cell& c = cells[i][j];
			c.nDisks = c.owner = 0;
		}
	}
}

void Display() {
	glClearColor(backColor.x, backColor.y, backColor.z, 1);	// set background color
	glClear(GL_COLOR_BUFFER_BIT);	// clear background and z-buffer
	if (gameStarted && gameStatus) {
		grid.Draw();
		if (animating) {
			float elapsed = (float)(clock() - animationStart) / CLOCKS_PER_SEC;
			float t = elapsed / animationDuration;
			if (t > 1) {
				animating = false;
				FinishUpdate();
				if (!grid.ReadyToExplode()) {
					gameStatus = grid.GameStatus();
				}
			}
			else {
				for (size_t i = 0; i < movingDisks.size(); i++) {
					MovingDisk md = movingDisks[i];
					vec2 pStart = g->gridLoc(md.iStart, md.jStart);
					vec2 pStop = g->gridLoc(md.iStop, md.jStop);
					vec2 p = pStart + t * (pStop - pStart);
					Cell c = g->cells[md.iStart][md.jStart];
					Disk(p, 18, c.owner == 1 ? green : blue); // use green color for player 1 and blue color for player 2
				}
			}
		}
	}
	else if (gameStarted == true && gameStatus == false) {
		if (playerWon == 1) {
			winner1.SetScale(vec2(.5f, .25f));
			winner1.SetPosition(vec2(0.f, 0.5f));
			winner1.Display();
		} 
		else if (playerWon == 2) {
			winner2.SetScale(vec2(.5f, .25f));
			winner2.SetPosition(vec2(0.f, 0.5f));
			winner2.Display();
		}		
		replay.Display();
	}
	else {
		background.Display();
		title.Display();
		play.Display();
	}
	glFlush();
}

void MouseButton(float x, float y, bool left, bool down) {
	if (gameStarted && gameStatus == true) {
		if (down) {
			if (grid.Hit(x, y, hitI, hitJ))	
				printf("HIT cell(%i, %i)!!\n", hitI, hitJ);
			else printf("no hit!\n");
		}
	}
	if (gameStarted && gameStatus == false && replay.Hit(x, y)) {
		gameStatus = true;
		grid.Reset();
	}
	if (!gameStarted && play.Hit(x, y))
		gameStarted = true;
}

const char* usage = R"(
	mouse-click on block to pick
)";

int main() {
	GLFWwindow* w = InitGLFW(800, 400, winWidth, winHeight, "Chain Reaction");
	printf("Usage:%s", usage);
	background.Initialize(dir + "welcome_bg.jpg");	//check for image extensions
	title.Initialize(dir + "game_title.png");
	play.Initialize(dir+"PlayButton32.png");
	title.SetScale(vec2(.5f, .25f));
	play.SetScale(vec2(.25f, .15f));
	title.SetPosition(vec2(0.f, 0.5f));
	play.SetPosition(vec2(0.f, -0.6f));
	winner1.Initialize(dir + "winner-1.png"); 
	winner2.Initialize(dir + "winner-2.png");
	replay.Initialize(dir + "replay.png");
	replay.SetScale(vec2(.25f, .15f));
	replay.SetPosition(vec2(0.f, -0.6f));
	RegisterMouseButton(MouseButton);
	while (!glfwWindowShouldClose(w)) {
		if (!animating)
			grid.Update();
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
	return 0;
}

