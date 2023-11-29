#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "board.h"

const SDL_Color colorMap[CELLCOLOR_COUNT] = {
	{255,   0,   0}, // CELLCOLOR_RED
	{  0, 255,   0}, // CELLCOLOR_GREEN
	{  0,   0, 255}, // CELLCOLOR_BLUE
	{255, 255,   0}, // CELLCOLOR_YELLOW
	{125,   0, 255}, // CELLCOLOR_PURPLE
	{  0, 255, 255}, // CELLCOLOR_CYAN
	{255, 255, 255}, // CELLCOLOR_WHITE
	{211, 211, 211}, // CELLCOLOR_LIGHT_GRAY
	{128, 128, 128}, // CELLCOLOR_MEDIUM_GRAY
	{169, 169, 169}, // CELLCOLOR_DARK_GRAY
	{255,   0, 255}, // CELLCOLOR_MAGENTA
	{255, 192, 203}, // CELLCOLOR_PINK
	{139,   0,   0}, // CELLCOLOR_DARK_RED
	{  0, 100,   0}, // CELLCOLOR_DARK_GREEN
	{  0,   0, 139}, // CELLCOLOR_DARK_BLUE
	{ 50,   0,   0}, // CELLCOLOR_ULTRADARK_RED
	{  0,  50,   0}, // CELLCOLOR_ULTRADARK_GREEN
	{  0,   0,  50}, // CELLCOLOR_ULTRADARK_BLUE
	{ 50,  50,   0}, // CELLCOLOR_ULTRADARK_YELLOW
	{ 50,   0,  50}, // CELLCOLOR_ULTRADARK_PURPLE
	{  0,  50,  50}, // CELLCOLOR_ULTRADARK_CYAN
	{255,   0, 255}  // CELLCOLOR_FUCHSIA
};

typedef struct Game
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	Mix_Chunk *clickSound;
	Mix_Chunk *yuhSound;
	Board *board;
	bool gameRunning;
	SDL_Point mouse;
	bool isHovered;
	SDL_Point hoveredCell;
	bool piping;
	SDL_Point *pipeSeq;
	i32 pipeSeqSize;
	CellState endPoint;
	CellColor selectedColor;
	SDL_Rect boardDim;
	i32 cellWidth;
	i32 cellHeight;
} Game;

bool startSDL();
void quitSDL();
bool inBounds(i32, i32, SDL_Rect);
bool pointsEqual(SDL_Point, SDL_Point);
bool pointsAdjacent(SDL_Point, SDL_Point);
void drawPipeEnd(SDL_Renderer*, SDL_Rect*, SDL_Color);
void drawPipeSection(SDL_Renderer*, SDL_Rect*, CellConnection, SDL_Color);
void drawPipe(SDL_Renderer*, SDL_Rect*, Cell*);
void setCellConnection(Board*, SDL_Point, SDL_Point);
void handleInput(Game *g);
void drawGame(Game *g);
void playFlow(Game *g, i32 boardSize);
void game(i32, char*[]);
void testgen();

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
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		fprintf(stderr, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
		return false;
	}
	return true;
}

void quitSDL()
{
	Mix_Quit();
	IMG_Quit();
	SDL_Quit();
}

bool inBounds(i32 x, i32 y, SDL_Rect rect)
{
	return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
}

bool pointsEqual(SDL_Point a, SDL_Point b)
{
	return (a.x == b.x && a.y == b.y);
}

bool pointsAdjacent(SDL_Point a, SDL_Point b)
{
	i32 xDiff = abs(a.x - b.x);
	i32 yDiff = abs(a.y - b.y);
	return (xDiff + yDiff == 1) && (xDiff == 0 || yDiff == 0);
}

void drawPipeEnd(SDL_Renderer *renderer, SDL_Rect *cellDim, SDL_Color color)
{
	SDL_Rect circle = {
		cellDim->x + (0.2 * cellDim->w),
		cellDim->y + (0.2 * cellDim->h),
		cellDim->w - (0.4 * cellDim->w),
		cellDim->h - (0.4 * cellDim->h)
	};
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
	SDL_RenderFillRect(renderer, &circle);
}

void drawPipeSection(SDL_Renderer *renderer, SDL_Rect *cellDim, CellConnection connection,
                     SDL_Color color)
{
	SDL_Rect dest;
	switch (connection)
	{
		case CELLCONNECTION_UP:
			dest.x = cellDim->x + (cellDim->w / 2);
			dest.y = cellDim->y - (cellDim->h / 2);
			dest.h = cellDim->h;
			dest.w = cellDim->w / 10;
			break;
		case CELLCONNECTION_DOWN:
			dest.x = cellDim->x + (cellDim->w / 2);
			dest.y = cellDim->y + (cellDim->h / 2);
			dest.h = cellDim->h;
			dest.w = cellDim->w / 10;
			break;
		case CELLCONNECTION_LEFT:
			dest.x = cellDim->x - (cellDim->w / 2);
			dest.y = cellDim->y + (cellDim->h / 2);
			dest.h = cellDim->h / 10;
			dest.w = cellDim->w;
			break;
		case CELLCONNECTION_RIGHT:
			dest.x = cellDim->x + (cellDim->w / 2);
			dest.y = cellDim->y + (cellDim->h / 2);
			dest.h = cellDim->h / 10;
			dest.w = cellDim->w;
			break;
		default:
		case CELLCONNECTION_NONE:
			break;
	}
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
	SDL_RenderFillRect(renderer, &dest);
}

void drawPipe(SDL_Renderer *renderer, SDL_Rect *cellDim, Cell *cell)
{
	SDL_Color color = colorMap[cell->color];

	if (cell->state == CELLSTATE_PIPE_START || cell->state == CELLSTATE_PIPE_END)
		drawPipeEnd(renderer, cellDim, color);

	if (cell->connection != CELLCONNECTION_NONE)
		drawPipeSection(renderer, cellDim, cell->connection, color);
}

void setCellConnection(Board *board, SDL_Point cell1, SDL_Point cell2)
{
	if (cell2.x > cell1.x)
		boardGet(board, cell1.y, cell1.x)->connection = CELLCONNECTION_RIGHT;

	else if (cell2.x < cell1.x)
		boardGet(board, cell1.y, cell1.x)->connection = CELLCONNECTION_LEFT;

	else if (cell2.y > cell1.y)
		boardGet(board, cell1.y, cell1.x)->connection = CELLCONNECTION_DOWN;

	else if (cell2.y < cell1.y)
		boardGet(board, cell1.y, cell1.x)->connection = CELLCONNECTION_UP;
}

void handleInput(Game *g)
{
	if (!g->gameRunning)
		return;

	SDL_Point mouse;
	SDL_GetMouseState(&mouse.x, &mouse.y);

	if (inBounds(mouse.x, mouse.y, g->boardDim))
	{
		g->isHovered = true;
		g->hoveredCell.y = (mouse.y - g->boardDim.y) / g->cellHeight;
		g->hoveredCell.x = (mouse.x - g->boardDim.x) / g->cellWidth;

		if (g->piping && boardBoundsCheck(g->board, g->hoveredCell.y, g->hoveredCell.x)
			&& pointsAdjacent(g->hoveredCell, g->pipeSeq[g->pipeSeqSize - 1]))
		{
			Cell* hovered = boardGet(g->board, g->hoveredCell.y, g->hoveredCell.x);

			if (hovered->state == CELLSTATE_EMPTY
				|| (hovered->state == g->endPoint && hovered->color == g->selectedColor))
			{
				g->pipeSeq[g->pipeSeqSize] = g->hoveredCell;
				g->pipeSeqSize += 1;
				if (g->pipeSeqSize > 1)
				{
					SDL_Point *prevPipe = &g->pipeSeq[g->pipeSeqSize - 2];
					SDL_Point *currPipe = &g->pipeSeq[g->pipeSeqSize - 1];
					setCellConnection(g->board, *prevPipe, *currPipe);
				}

				if (hovered->state == g->endPoint)
				{
					g->piping = false;
					g->pipeSeqSize = 0;
					Mix_PlayChannel(-1, g->yuhSound, 0);
				}
				else
				{
					hovered->color = g->selectedColor;
					boardSetState(g->board, g->hoveredCell.y, g->hoveredCell.x, CELLSTATE_PIPE_H);
					Mix_PlayChannel(-1, g->clickSound, 0);
				}
			}
			else if (g->pipeSeqSize > 1)
			{
				SDL_Point *prevPipe = &g->pipeSeq[g->pipeSeqSize - 2];
				SDL_Point *currPipe = &g->pipeSeq[g->pipeSeqSize - 1];
				if (pointsEqual(g->hoveredCell, *prevPipe))
				{
					boardGet(g->board, (*currPipe).y, (*currPipe).x)->state = CELLSTATE_EMPTY;
					(*currPipe) = (SDL_Point){0, 0};

					boardGet(g->board, (*prevPipe).y, (*prevPipe).x)->connection = CELLCONNECTION_NONE;

					g->pipeSeqSize -= 1;
					Mix_PlayChannel(-1, g->clickSound, 0);
				}
			}
		}
	}
	else
	{
		g->isHovered = false;
	}

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				g->gameRunning = false;
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (!g->piping && g->isHovered)
				{
					Cell *hovered = boardGet(g->board, g->hoveredCell.y, g->hoveredCell.x);
					if (hovered->state == CELLSTATE_PIPE_START
						|| hovered->state == CELLSTATE_PIPE_END)
					{
						g->piping = true;
						g->selectedColor = hovered->color;
						g->pipeSeqSize = 1;
						g->pipeSeq[0] = g->hoveredCell;
						if (hovered->state == CELLSTATE_PIPE_START)
						{
							g->endPoint = CELLSTATE_PIPE_END;
						}
						else
						{
							g->endPoint = CELLSTATE_PIPE_START;
						}
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (g->piping)
				{
					for (i32 r = 0; r < g->board->height; r++)
					{
						for (i32 c = 0; c < g->board->width; c++)
						{
							Cell* cell = boardGet(g->board, r, c);
							if (cell->color == g->selectedColor)
							{
								if (cell->state != CELLSTATE_PIPE_START
									&& cell->state != CELLSTATE_PIPE_END)
								{
									cell->state = CELLSTATE_EMPTY;
								}
								cell->connection = CELLCONNECTION_NONE;
							}
						}
					}
					g->piping = false;
				}
				break;
		}
	}
}

void drawGame(Game *g)
{
	SDL_SetRenderDrawColor(g->renderer, 50, 50, 50, 255);
	SDL_RenderClear(g->renderer);
	SDL_SetRenderDrawColor(g->renderer, 25, 25, 25, 255);
	SDL_RenderFillRect(g->renderer, &g->boardDim);

	for (i32 r = 0; r < g->board->height; r++)
	{
		for (i32 c = 0; c < g->board->width; c++)
		{
			i32 cellX = g->boardDim.x + (c * g->cellWidth);
			i32 cellY = g->boardDim.y + (r * g->cellHeight);

			SDL_Rect cellBg = {
				cellX + (0.1 * g->cellWidth),
				cellY + (0.1 * g->cellHeight),
				g->cellWidth - (0.2 * g->cellWidth),
				g->cellHeight - (0.2 * g->cellHeight)
			};
			SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 255);
			SDL_RenderFillRect(g->renderer, &cellBg);
		}
	}

	for (i32 r = 0; r < g->board->height; r++)
	{
		for (i32 c = 0; c < g->board->width; c++)
		{
			i32 cellX = g->boardDim.x + (c * g->cellWidth);
			i32 cellY = g->boardDim.y + (r * g->cellHeight);
			SDL_Rect cell = {cellX, cellY, g->cellWidth, g->cellHeight};
			drawPipe(g->renderer, &cell, boardGet(g->board, r, c));
		}
	}

	SDL_RenderPresent(g->renderer);
}


#define DEFAULT_BOARD_SIZE 6
void playFlow(Game *g, i32 boardSize)
{
	boardSize = boardSize < 2 ? DEFAULT_BOARD_SIZE : boardSize;

	g->board = boardCreate(boardSize, boardSize);
	SDL_CreateThread((SDL_ThreadFunction)boardGenerate, "boardGenThread", g->board);

	i32 windowWidth, windowHeight;
	SDL_GetWindowSize(g->window, &windowWidth, &windowHeight);
	g->boardDim = (SDL_Rect){
		(windowWidth - (windowWidth * 0.8)) / 2,
		(windowHeight - (windowHeight * 0.8)) / 2,
		windowWidth * 0.8,
		windowHeight * 0.8
	};
	g->cellWidth = g->boardDim.w / boardSize;
	g->cellHeight = g->boardDim.h / boardSize;

	g->selectedColor = CELLCOLOR_RED;
	g->piping = false;
	g->endPoint = CELLSTATE_PIPE_END;

	g->pipeSeq = malloc(sizeof(SDL_Point) * boardSize * boardSize);
	g->pipeSeqSize = 0;

	g->isHovered = false;
	g->hoveredCell = (SDL_Point){0, 0};
	g->gameRunning = true;

	while (g->gameRunning)
	{
		if (g->board->isGenerated)
		{
			handleInput(g);
		}
		else
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
					case SDL_QUIT:
						g->gameRunning = false;
						break;
				}
			}
		}
		drawGame(g);
	}

	free(g->pipeSeq);
	boardFree(g->board);
}

void game(i32 argc, char *argv[])
{
	Game game;
	if (!startSDL()
	    || !(game.window     = SDL_CreateWindow("flow", 1920, 0, 800, 600, SDL_WINDOW_SHOWN))
	    || !(game.renderer   = SDL_CreateRenderer(game.window, -1, SDL_RENDERER_ACCELERATED))
	    || !(game.clickSound = Mix_LoadWAV("assets/Audio/click_001.wav"))
	    || !(game.yuhSound   = Mix_LoadWAV("assets/Audio/YUH.wav")))
	{
		fprintf(stderr, "Error initializing\n");
		goto cleanup;
	}

	i32 boardSize = argc>1 ? atoi(argv[1]) : 5;
	i32 seed      = argc>2 ? atoi(argv[2]) : time(NULL);

	if (boardSize < 2) boardSize = 2;
	srand(seed);

	playFlow(&game, boardSize);
	
cleanup:
	SDL_DestroyRenderer(game.renderer);
	SDL_DestroyWindow(game.window);
	Mix_FreeChunk(game.yuhSound);
	Mix_FreeChunk(game.clickSound);
	quitSDL();
}

void testgen()
{
	srand(42);
	Board *b = boardCreate(8, 8);
	boardGenerate(b);
}


int main(int argc, char *argv[])
{
	game(argc, argv);
	return 0;
}
