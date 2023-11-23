#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define ADJACENT_UP 0
#define ADJACENT_DOWN 1
#define ADJACENT_LEFT 2
#define ADJACENT_RIGHT 3

/*
static uint32_t board[4*4];
static uint32_t available[4*4]; // 32 bits means it can track pipes 32 long
static uint32_t starts[4*4]; // do we need this?
static bool test[10][5] = {false};
*/

typedef enum
{
	COLOR_EMPTY = 0,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_BLUE,
	COLOR_GOLD,
	COLOR_ORANGE,
	COLOR_YELLOW,
	COLOR_PURPLE,
	COLOR_CYAN,
	COLOR_AQUA,
	COLOR_MAGENTA,
	COLOR_PINK,
	NUM_COLORS
} PipeColor;

// an enum for temporary board values, used in algorithms
typedef enum
{
	TEMP_PIPE = NUM_COLORS,
	TEMP_EMPTY_VISITED
} TempCellState;

typedef struct
{
	uint32_t *cells;
	size_t rows;
	size_t cols;
} GameBoard;

typedef struct
{
	size_t size;
	PipeColor color;
} PipePlan;

typedef struct
{
	PipePlan *plans;
} PipePlans;

// ---------------- SDL funcs -----------------
bool startSDL();
void quitSDL();

bool startSDL()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return false;
	}
	if (IMG_Init(IMG_INIT_PNG) < 0)
	{
		fprintf(stderr, "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}
	return true;
}

void quitSDL()
{
	IMG_Quit();
	SDL_Quit();
}

// -------------- GameBoard funcs ------------------
void freeGameBoard(GameBoard *gb);
void initGameBoard(GameBoard *gb, size_t rows, size_t cols);
void zeroGameBoard(GameBoard *gb);
uint32_t* gameBoardCell(GameBoard *gb, size_t row, size_t col);
size_t gameBoardNumAdjacentWithColor(GameBoard *gb, size_t row, size_t col, PipeColor color);
bool hasEmptyCellGameBoard(GameBoard *gb);
size_t randEmptyGameBoard(GameBoard *gb);
void dfsFillGameBoard(GameBoard *gb, size_t row, size_t col, uint32_t from, uint32_t to);
bool isEmptyConnectedGameBoard(GameBoard *gb, size_t row, size_t col);
bool placePipeGameBoard(GameBoard *gb, PipePlan plan, size_t *startRow, size_t *startCol, size_t *endRow, size_t *endCol);
bool appendPipeGameBoard(GameBoard *gb, PipePlan plan, size_t row, size_t col, size_t size, 
						 size_t *endRow, size_t *endCol);
void clearPipeGameBoard(GameBoard *gb, PipeColor color);
void printGameBoard(GameBoard *gb);

void playGame(GameBoard *gb);

void freeGameBoard(GameBoard *gb)
{
	// free heap alloced mem
	if (gb->cells)
	{
		free(gb->cells);
		gb->cells = NULL;
	}
	gb->rows = 0;
	gb->cols = 0;
}

void initGameBoard(GameBoard *gb, size_t rows, size_t cols)
{
	gb->cells = malloc(rows * cols * sizeof(uint32_t));
	gb->rows = rows;
	gb->cols = cols;
	zeroGameBoard(gb);
}

void zeroGameBoard(GameBoard *gb)
{

	for (size_t r = 0; r < gb->rows; r++)
	{
		for (size_t c = 0; c < gb->cols; c++)
		{
			*gameBoardCell(gb, r, c) = 0;
		}
	}
}

uint32_t* gameBoardCell(GameBoard *gb, size_t row, size_t col)
{
	if (row >= gb->rows || col >= gb->cols)
	{
		return NULL;
	}
	return &gb->cells[row * gb->cols + col];
}

size_t gameBoardNumAdjacentWithColor(GameBoard *gb, size_t row, size_t col, PipeColor color)
{
	size_t count = 0;
	if (row > 0 && *gameBoardCell(gb, row - 1, col) == color)
		count++;

	if (row < gb->rows - 1 && *gameBoardCell(gb, row + 1, col) == color)
		count++;

	if (col > 0 && *gameBoardCell(gb, row, col - 1) == color)
		count++;

	if (col < gb->cols - 1 && *gameBoardCell(gb, row, col + 1) == color)
		count++;

	return count;
}

bool hasEmptyCellGameBoard(GameBoard *gb)
{
	for (size_t r = 0; r < gb->rows; r++)
	{
		for (size_t c = 0; c < gb->cols; c++)
		{
			if (*gameBoardCell(gb, r, c) == COLOR_EMPTY)
			{
				return true;
			}
		}
	}
	return false;
}

size_t randEmptyGameBoard(GameBoard *gb)
{
	if (!hasEmptyCellGameBoard(gb))
	{
		return -1;
	}

	size_t r = rand() % gb->rows;
	size_t c = rand() % gb->cols;
	while (*gameBoardCell(gb, r, c) != 0)
	{
		r = rand() % gb->rows;
		c = rand() % gb->cols;
	}
	return r * gb->cols + c;
}

void dfsFillGameBoard(GameBoard *gb, size_t row, size_t col, uint32_t from, uint32_t to)
{
	// if out of bounds, return
	if (row < 0 || row >= gb->rows || col < 0 ||  col >= gb->cols)
	{
		return;
	}

	// if not the from value, return
	if (*gameBoardCell(gb, row, col) == from)
	{
		*gameBoardCell(gb, row, col) = to;
		dfsFillGameBoard(gb, row-1, col,   from, to);
		dfsFillGameBoard(gb, row+1, col,   from, to);
		dfsFillGameBoard(gb, row,   col-1, from, to);
		dfsFillGameBoard(gb, row,   col+1, from, to);
	}
}

// are all the empty cells connected to each other, given that a pipe is placed at row, col?
bool isEmptyConnectedGameBoard(GameBoard *gb, size_t row, size_t col)
{
	// place a temporary pipe at row, col
	*gameBoardCell(gb, row, col) = TEMP_PIPE;

	// find the first empty cell
	size_t emptyRow = -1;
	size_t emptyCol = -1;
	for (size_t r = 0; r < gb->rows; r++)
	{
		for (size_t c = 0; c < gb->cols; c++)
		{
			if (*gameBoardCell(gb, r, c) == COLOR_EMPTY)
			{
				emptyRow = r;
				emptyCol = c;
				break;
			}
		}
	}

	// trivially true if no empty cells
	if (emptyRow == -1)
	{
		return true;
	}

	// recursively fill the empty cells with TEMP_EMPTY_VISITED
	dfsFillGameBoard(gb, emptyRow, emptyCol, COLOR_EMPTY, TEMP_EMPTY_VISITED);

	// check if all empty cells are filled with TEMP_EMPTY_VISITED, and also reset them back
	// to COLOR_EMPTY
	bool allVisited = true;
	for (size_t r = 0; r < gb->rows; r++)
	{
		for (size_t c = 0; c < gb->cols; c++)
		{
			if (*gameBoardCell(gb, r, c) == COLOR_EMPTY)
			{
				allVisited = false;
			}
			else if (*gameBoardCell(gb, r, c) == TEMP_EMPTY_VISITED)
			{
				*gameBoardCell(gb, r, c) = COLOR_EMPTY;
			}
		}
	}

	// reset the temporary pipe back to empty
	*gameBoardCell(gb, row, col) = COLOR_EMPTY;

	return allVisited;
}

bool placePipeGameBoard(GameBoard *gb, PipePlan plan, size_t *startRow, size_t *startCol,
						size_t *endRow, size_t *endCol)
{
	// 0 means available, 1 means unavailable
	GameBoard rejectBoard;
	initGameBoard(&rejectBoard, gb->rows, gb->cols);

	size_t startIndex = -1;
	while (true)
	{
		// count the number of non-rejected empty cells
		size_t nonRejectedEmpty = 0;
		for (size_t r = 0; r < gb->rows; r++)
		{
			for (size_t c = 0; c < gb->cols; c++)
			{
				if (*gameBoardCell(gb, r, c) == COLOR_EMPTY 
					&& *gameBoardCell(&rejectBoard, r, c) == 0)
				{
					nonRejectedEmpty += 1;
				}
			}
		}
		if (nonRejectedEmpty == 0)
		{
			freeGameBoard(&rejectBoard);
			return false;
		}

		// choose a random empty position to start at. nonRejectedEmpty is > 0, so this
		// will not return -1
		size_t index = randEmptyGameBoard(gb);
		*startRow = index / gb->cols;
		*startCol = index % gb->cols;

		while (true)
		{
			// the random empty position is not rejected, yet
			if (*gameBoardCell(&rejectBoard, *startRow, *startCol) == 0)
			{
				// check to see if placing a pipe here will keep all empty cells connected
				if (isEmptyConnectedGameBoard(gb, *startRow, *startCol))
				{
					// found a good starting position
					break;
				}
				// if not, reject it, and we have one less non-rejected empty cell
				else
				{
					*gameBoardCell(&rejectBoard, *startRow, *startCol) = 1;
					nonRejectedEmpty -= 1;

					// if there are no more non-rejected empty cells, then we cannot place
					// the pipe
					if (nonRejectedEmpty == 0)
					{
						freeGameBoard(&rejectBoard);
						return false;
					}
				}
			}

			// choose a new random empty position
			index = randEmptyGameBoard(gb);
			*startRow = index / gb->cols;
			*startCol = index % gb->cols;
		}

		size_t currentSize = 1;
		*gameBoardCell(gb, *startRow, *startCol) = plan.color;

		bool placed = appendPipeGameBoard(gb, plan, *startRow, *startCol, currentSize, endRow, endCol);
		if (placed)
		{
			freeGameBoard(&rejectBoard);
			return true;
		}
		else
		{
			// mark this cell as rejected, clear all cells with the current pipe color, try again
			*gameBoardCell(&rejectBoard, *startRow, *startCol) = 1;
			clearPipeGameBoard(gb, plan.color);
		}
	}
	freeGameBoard(&rejectBoard);
	return false;
}


bool appendPipeGameBoard(GameBoard *gb, PipePlan plan, size_t row, size_t col, size_t size, 
						 size_t *endRow, size_t *endCol)
{
	bool adjacentAvailable[4] = {false, false, false, false};
	bool anyAdjacentAvailable = false;

	if (row > 0
		&& *gameBoardCell(gb, row-1, col) == COLOR_EMPTY
		&& gameBoardNumAdjacentWithColor(gb, row-1, col, plan.color) < 2
		&& isEmptyConnectedGameBoard(gb, row-1, col))
	{
		adjacentAvailable[ADJACENT_UP] = true;
		anyAdjacentAvailable = true;
	}
	if (row < gb->rows - 1
		&& *gameBoardCell(gb, row+1, col) == COLOR_EMPTY
		&& gameBoardNumAdjacentWithColor(gb, row+1, col, plan.color) < 2
		&& isEmptyConnectedGameBoard(gb, row+1, col))
	{
		adjacentAvailable[ADJACENT_DOWN] = true;
		anyAdjacentAvailable = true;
	}
	if (col > 0
		&& *gameBoardCell(gb, row, col-1) == COLOR_EMPTY
		&& gameBoardNumAdjacentWithColor(gb, row, col-1, plan.color) < 2
		&& isEmptyConnectedGameBoard(gb, row, col-1))
	{
		adjacentAvailable[ADJACENT_LEFT] = true;
		anyAdjacentAvailable = true;
	}
	if (col < gb->cols - 1
		&& *gameBoardCell(gb, row, col+1) == COLOR_EMPTY
		&& gameBoardNumAdjacentWithColor(gb, row, col+1, plan.color) < 2
		&& isEmptyConnectedGameBoard(gb, row, col+1))
	{
		adjacentAvailable[ADJACENT_RIGHT] = true;
		anyAdjacentAvailable = true;
	}

	if (!anyAdjacentAvailable)
	{
		return false;
	}

	size += 1;
	while (anyAdjacentAvailable)
	{
		// pick random adjacent cell
		size_t adjacentIndex = rand() % 4;
		while (!adjacentAvailable[adjacentIndex])
		{
			adjacentIndex = rand() % 4;
		}
		size_t adjRow;
		size_t adjCol;
		switch (adjacentIndex)
		{
			default:
			case ADJACENT_UP:
				adjRow = row - 1;
				adjCol = col;
				break;
			case ADJACENT_DOWN:
				adjRow = row + 1;
				adjCol = col;
				break;
			case ADJACENT_LEFT:
				adjRow = row;
				adjCol = col - 1;
				break;
			case ADJACENT_RIGHT:
				adjRow = row;
				adjCol = col + 1;
				break;
		}

		// attempt to place the pipe
		bool pipePlaced = false;
		*gameBoardCell(gb, adjRow, adjCol) = plan.color;
		if (size == plan.size)
		{
			*endRow = adjRow;
			*endCol = adjCol;
			return true;
		}
		else
		{
			if (appendPipeGameBoard(gb, plan, adjRow, adjCol, size, endRow, endCol))
			{
				return true;
			}
			else
			{
				adjacentAvailable[adjacentIndex] = false;
				anyAdjacentAvailable = false;
				for (size_t i = 0; i < 4; i++)
				{
					if (adjacentAvailable[i])
					{
						anyAdjacentAvailable = true;
					}
				}
			}
		}
	}
	return false;
}

void clearPipeGameBoard(GameBoard *gb, PipeColor color)
{
	for (size_t r = 0; r < gb->rows; r++)
	{
		for (size_t c = 0; c < gb->cols; c++)
		{
			if (*gameBoardCell(gb, r, c) == color)
			{
				*gameBoardCell(gb, r, c) = 0;
			}
		}
	}
}


void printGameBoard(GameBoard *gb)
{
	printf("---- Board ----\n");
	for (size_t r = 0; r < gb->rows; r++)
	{
		printf("    ");
		for (size_t c = 0; c < gb->cols; c++)
		{
			if (*gameBoardCell(gb, r, c) == COLOR_EMPTY)
			{
				printf("_ ");
			}
			else
			{
				printf("%i ", *gameBoardCell(gb, r, c));
			}
		}
		printf("\t\n");
	}
	printf("---------------\n");
}

void playGame(GameBoard *gb)
{
	if (!startSDL())
	{
		quitSDL();
		return;
	}
	SDL_Window *window = SDL_CreateWindow("pipes", 0, 0, 800, 600, SDL_WINDOW_SHOWN);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	
	int boardHeight = windowHeight - 100;
	int boardWidth = boardHeight;
	int cellSize = boardHeight / gb->rows;
	int boardX = (windowWidth - boardWidth) / 2;
	int boardY = (windowHeight - boardHeight) / 2;

	PipeColor selectedColor = COLOR_EMPTY;
	bool piping = false;
	int lastPipeRow, lastPipeCol;

	bool running = true;
	while (running)
	{
		int mouseX, mouseY;
		bool isCellHovered = false;
		int hoveredCellRow, hoveredCellCol;
		SDL_Event event;

		SDL_GetMouseState(&mouseX, &mouseY);
		if (mouseX >= boardX && mouseX <= boardX + boardWidth
			&& mouseY >= boardY && mouseY <= boardY + boardHeight)
		{
			isCellHovered = true;
			hoveredCellRow = (mouseY - boardY) / cellSize;
			hoveredCellCol = (mouseX - boardX) / cellSize;
			if (piping && (hoveredCellRow != lastPipeRow || hoveredCellCol != lastPipeCol))
			{
				if (gameBoardNumAdjacentWithColor(gb, hoveredCellRow, hoveredCellCol, selectedColor) > 0)
				{
					*gameBoardCell(gb, hoveredCellRow, hoveredCellCol) = selectedColor;
					lastPipeRow = hoveredCellRow;
					lastPipeCol = hoveredCellCol;
				}
			}
		}

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				running = false;
			}
			if (event.type == SDL_MOUSEBUTTONDOWN)
			{
				if (!piping && isCellHovered)
				{
					selectedColor = *gameBoardCell(gb, hoveredCellRow, hoveredCellCol);
					if (selectedColor != COLOR_EMPTY)
					{
						piping = true;
					}
				}
			}
			if (event.type == SDL_MOUSEBUTTONUP)
			{
				piping = false;			
			}
		}

		SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
		SDL_Rect pipeBackgroundRect = {boardX, boardY, boardWidth, boardHeight};
		SDL_RenderFillRect(renderer, &pipeBackgroundRect);

		for (int r = 0; r < gb->rows; r++)
		{
			for (int c = 0; c < gb->cols; c++)
			{
				int red, green, blue;

				switch (*gameBoardCell(gb, r, c))
				{
					case COLOR_EMPTY:
						red = 0; green = 0; blue = 0;
						break;
					case COLOR_RED:
						red = 255; green = 0; blue = 0;
						break;
					case COLOR_GREEN:
						red = 0; green = 255; blue = 0;
						break;
					case COLOR_BLUE:
						red = 0; green = 0; blue = 255;
						break;
					case COLOR_GOLD:
						red = 125; green = 125; blue = 0;
						break;
					case COLOR_ORANGE:
						red = 255; green = 128; blue = 0;
						break;
					case COLOR_YELLOW:
						red = 255; green = 255; blue = 0;
						break;
					case COLOR_PURPLE:
						red = 128; green = 0; blue = 255;
						break;
					case COLOR_CYAN:
						red = 0; green = 255; blue = 255;
						break;
					case COLOR_AQUA:
						red = 0; green = 128; blue = 255;
						break;
					case COLOR_MAGENTA:
						red = 255; green = 0; blue = 255;
						break;
					case COLOR_PINK:
						red = 255; green = 0; blue = 128;
						break;
					default:
						red = 0; green = 0; blue = 0;
						break;
				}

				if (isCellHovered && r == hoveredCellRow && c == hoveredCellCol)
				{
					// make cell brighter by averaging it with 255
					red = (red + 255) / 2;
					green = (green + 255) / 2;
					blue = (blue + 255) / 2;
				}

				SDL_SetRenderDrawColor(renderer, red, green, blue, 255);

				SDL_Rect destination = {
					c * cellSize + boardX + 7, 
					r * cellSize + boardY + 7, 
					cellSize-10, 
					cellSize-10
				};
				SDL_RenderFillRect(renderer, &destination);
			}
		}
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	quitSDL();
}

int main(int argc, char *argv[])
{
	#define PIPECOUNT 8
	srand(time(NULL));

	if (startSDL())
	{

		GameBoard board;
		initGameBoard(&board, 8, 8);

		PipePlan plans[PIPECOUNT];
		size_t startRows[PIPECOUNT];
		size_t startCols[PIPECOUNT];
		size_t endRows[PIPECOUNT];
		size_t endCols[PIPECOUNT];
		for (int p = 0; p < PIPECOUNT; p++)
		{
			plans[p].size = 8;
			plans[p].color = p + 1;
		}

		for (int p = 0; p < PIPECOUNT; p++)
		{
			//printf("Placing pipe %i\n", p);
			size_t sr, sc, er, ec = -1;
			bool success = placePipeGameBoard(&board, plans[p], &startRows[p], &startCols[p], &endRows[p], &endCols[p]);
			if (!success)
			{
				printf("Failed to place pipe %i\n", p);
				printf("Clearing board... and trying again\n");
				zeroGameBoard(&board);
				p = -1;
			}
		}
		printf("Board is Completed! Here is the completed board:\n");
		printGameBoard(&board);
		
		zeroGameBoard(&board);
		for (int p = 0; p < PIPECOUNT; p++)
		{
			*gameBoardCell(&board, startRows[p], startCols[p]) = plans[p].color;
			*gameBoardCell(&board, endRows[p], endCols[p]) = plans[p].color;
		}
		printf("And here is the board with just the endpoints:\n");
		printGameBoard(&board);

		playGame(&board);

		freeGameBoard(&board);
	}
	quitSDL();
	return 0;
}
