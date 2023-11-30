#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include "board.h"

#define INITIALIZE(call) \
	if (!(call)) \
	{ \
		fprintf(stderr, "Error " #call "\n"); \
		goto cleanup; \
	}

#define DEFAULT_BOARD_SIZE 6

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

typedef enum Sound
{
	SOUND_CLICK,
	SOUND_YUH,
	SOUND_COUNT
} Sound;

typedef enum MenuMusic
{
	MENUMUSIC_001,
	MENUMUSIC_002,
	MENUMUSIC_COUNT
} MenuMusic;

typedef enum GameMusic
{
	GAMEMUSIC_001,
	GAMEMUSIC_002,
	GAMEMUSIC_COUNT
} GameMusic;

typedef enum GameState
{
	GAMESTATE_NONE,
	GAMESTATE_INTRO,
	GAMESTATE_MENU,
	GAMESTATE_PLAY,
	GAMESTATE_EXIT,
	GAMESTATE_COUNT
} GameState;

typedef struct Sprite
{
	SDL_Texture *texture;
	SDL_Rect source;
	SDL_Rect destination;
	SDL_Point origin;
} Sprite;

typedef struct Game
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	Mix_Chunk *sound[SOUND_COUNT];
	Mix_Music *menuMusic[MENUMUSIC_COUNT];
	Mix_Music *gameMusic[GAMEMUSIC_COUNT];
	TTF_Font *font;
	SDL_Texture *menuTexture;
	SDL_Rect menuTextureRect;

	// mc ride
	SDL_Texture *rideTexture;
	u64 rideTime;
	SDL_Rect rideRect;
	SDL_Point rideDirection;


	u64 dt;
	u64 lastTime;

	i32 boardSize;
	Board *board;
	bool gameRunning;
	GameState gameState;
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

void switchState(Game *g, GameState state);

void introInit(Game *g);
void introInput(Game *g);
void introDraw(Game *g);
void introExit(Game *g);

void menuInit(Game *g);
void menuInput(Game *g);
void menuDraw(Game *g);
void menuExit(Game *g);

void playInit(Game *g);
void playInput(Game *g);
void playDraw(Game *g);
void playExit(Game *g);

void exitInit(Game *g);
void exitInput(Game *g);
void exitDraw(Game *g);
void exitExit(Game *g);

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
	if (TTF_Init() < 0)
	{
		fprintf(stderr, "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
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

	if (cell->state != CELLSTATE_EMPTY && cell->connection != CELLCONNECTION_NONE)
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

void switchState(Game *g, GameState state)
{
	switch (g->gameState)
	{
		default:
		case GAMESTATE_NONE:
		case GAMESTATE_COUNT:
			break;
		case GAMESTATE_INTRO:
			introExit(g);
			break;
		case GAMESTATE_MENU:
			menuExit(g);
			break;
		case GAMESTATE_PLAY:
			playExit(g);
			break;
		case GAMESTATE_EXIT:
			exitExit(g);
			break;
	}
	g->gameState = state;
	switch (g->gameState)
	{
		default:
		case GAMESTATE_NONE:
		case GAMESTATE_COUNT:
			break;
		case GAMESTATE_INTRO:
			introInit(g);
			break;
		case GAMESTATE_MENU:
			menuInit(g);
			break;
		case GAMESTATE_PLAY:
			playInit(g);
			break;
		case GAMESTATE_EXIT:
			exitInit(g);
			break;
	}
}

void introInit(Game *g)
{

}

void introInput(Game *g)
{

}

void introDraw(Game *g)
{

}

void introExit(Game *g)
{

}

void menuInit(Game *g)
{
	SDL_Color white = {255, 255, 255, 255};
	SDL_Surface *ts = TTF_RenderText_Solid(g->font, "Welcome to Flow", white); 
	g->menuTexture = SDL_CreateTextureFromSurface(g->renderer, ts);
	i32 ww, wh;
	SDL_GetWindowSize(g->window, &ww, &wh);
	g->menuTextureRect = (SDL_Rect){
		(ww / 2) - (ts->w / 2),
		(wh / 2) - (ts->h / 2),
		ts->w,
		ts->h
	};
	SDL_FreeSurface(ts);
	Mix_PlayMusic(g->menuMusic[MENUMUSIC_001], -1);
}

void menuInput(Game *g)
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				g->gameRunning = false;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						g->gameRunning = false;
						break;
					case SDLK_RETURN:
						switchState(g, GAMESTATE_PLAY);
						break;
					default:
						break;
				}
				break;
		}
	}

}

void menuDraw(Game *g)
{
	SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 255);
	SDL_RenderClear(g->renderer);
	SDL_RenderCopy(g->renderer, g->menuTexture, NULL, &g->menuTextureRect);
	SDL_RenderPresent(g->renderer);
}

void menuExit(Game *g)
{
	SDL_DestroyTexture(g->menuTexture);
}

void playInit(Game *g)
{
	g->board = boardCreate(g->boardSize, g->boardSize);
	SDL_CreateThread((SDL_ThreadFunction)boardGenerate, "boardGenThread", g->board);

	i32 windowWidth, windowHeight;
	SDL_GetWindowSize(g->window, &windowWidth, &windowHeight);
	g->boardDim = (SDL_Rect){
		(windowWidth - (windowWidth * 0.8)) / 2,
		(windowHeight - (windowHeight * 0.8)) / 2,
		windowWidth * 0.8,
		windowHeight * 0.8
	};
	g->cellWidth = g->boardDim.w / g->boardSize;
	g->cellHeight = g->boardDim.h / g->boardSize;

	g->selectedColor = CELLCOLOR_RED;
	g->piping = false;
	g->endPoint = CELLSTATE_PIPE_END;

	g->pipeSeq = malloc(sizeof(SDL_Point) * g->boardSize * g->boardSize);
	g->pipeSeqSize = 0;

	g->isHovered = false;
	g->hoveredCell = (SDL_Point){0, 0};
	g->gameRunning = true;

	g->lastTime = SDL_GetTicks64();

	Mix_PlayMusic(g->gameMusic[GAMEMUSIC_001], -1);
}

void playInput(Game *g)
{
	// get delta time
	u64 currTime = SDL_GetTicks64();
	g->dt = (currTime - g->lastTime);
	g->lastTime = currTime;

	// decrement timers
	if (g->rideTime > 0)
	{
		g->rideTime -= g->dt;
		if (g->rideTime < 0)
		{
			g->rideTime = 0;
		}
		else
		{
			g->rideRect.x += g->rideDirection.x * (g->dt / 1000.0f);
			g->rideRect.y += g->rideDirection.y * (g->dt / 1000.0f);
		}
	}

	if (g->board->genState != GENSTATE_IDLE)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					g->gameRunning = false;
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_r)
					{
						g->board->genState = GENSTATE_STOPREQUESTED;
						printf("Stopping board generation\n");
					}
					break;
			}
		}
		return;
	}

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
					Mix_PlayChannel(-1, g->sound[SOUND_YUH], 0);
					// mc ride time
					g->rideTime = 1000;
					i32 rideW = g->cellWidth * 2;
					i32 rideH = g->cellHeight * 2;
					i32 rideX = rand() % (g->boardDim.w - rideW);
					i32 rideY = rand() % (g->boardDim.h - rideH);
					g->rideRect = (SDL_Rect){rideX, rideY, rideW, rideH};
					// random direction, from -1, -1 to 1, 1
					g->rideDirection = (SDL_Point){rand() % 3 - 1, rand() % 3 - 1};
					if (g->rideDirection.x == 0 && g->rideDirection.y == 0)
					{
						g->rideDirection.x = 1;
					}
				}
				else
				{
					hovered->color = g->selectedColor;
					boardSetState(g->board, g->hoveredCell.y, g->hoveredCell.x, CELLSTATE_PIPE);
					Mix_PlayChannel(-1, g->sound[SOUND_CLICK], 0);
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

					boardGet(g->board, (*prevPipe).y, (*prevPipe).x)->connection
						= CELLCONNECTION_NONE;

					g->pipeSeqSize -= 1;
					Mix_PlayChannel(-1, g->sound[SOUND_CLICK], 0);
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

void playDraw(Game *g)
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

	// draw ride
	if (g->rideTime > 0)
	{
		SDL_RenderCopy(g->renderer, g->rideTexture, NULL, &g->rideRect);
	}

	SDL_RenderPresent(g->renderer);

}

void playExit(Game *g)
{
	free(g->pipeSeq);
	boardFree(g->board);
}

void exitInit(Game *g)
{
	g->gameRunning = false;
}

void exitInput(Game *g)
{

}

void exitDraw(Game *g)
{

}

void exitExit(Game *g)
{

}



int main(int argc, char *argv[])
{
	Game game;
	INITIALIZE(startSDL());
	INITIALIZE(game.window = SDL_CreateWindow("flow", 1920, 0, 800, 600, SDL_WINDOW_SHOWN));
	INITIALIZE(game.renderer = SDL_CreateRenderer(game.window, -1, SDL_RENDERER_ACCELERATED));
	INITIALIZE(game.sound[SOUND_CLICK] = Mix_LoadWAV("assets/Audio/click_001.wav"));
	INITIALIZE(game.sound[SOUND_YUH] = Mix_LoadWAV("assets/Audio/YUH.wav"));
	INITIALIZE(game.menuMusic[MENUMUSIC_001] = Mix_LoadMUS("assets/Audio/flow-song1.wav"));
	INITIALIZE(game.menuMusic[MENUMUSIC_002] = Mix_LoadMUS("assets/Audio/flow-song-3-intro.wav"));
	INITIALIZE(game.gameMusic[GAMEMUSIC_001] = Mix_LoadMUS("assets/Audio/flow-song2.wav"));
	INITIALIZE(game.gameMusic[GAMEMUSIC_002] = Mix_LoadMUS("assets/Audio/flow-song-3-game.wav"));
	INITIALIZE(game.font = TTF_OpenFont("assets/Fonts/IosevkaTermNerdFont-Bold.ttf", 24));
	INITIALIZE(game.rideTexture = IMG_LoadTexture(game.renderer, "assets/Sprites/ride.png"));

	i32 boardSize = argc>1 ? atoi(argv[1]) : 5;
	i32 seed      = argc>2 ? atoi(argv[2]) : time(NULL);

	if (boardSize < 2) boardSize = 2;
	srand(seed);

	game.boardSize = boardSize < 2 ? DEFAULT_BOARD_SIZE : boardSize;

	game.gameState = GAMESTATE_NONE;
	switchState(&game, GAMESTATE_MENU);

	while (game.gameRunning)
	{
		switch (game.gameState)
		{
			default:
			case GAMESTATE_COUNT:
			case GAMESTATE_INTRO:
				introInput(&game);
				introDraw(&game);
				break;
			case GAMESTATE_MENU:
				menuInput(&game);
				menuDraw(&game);
				break;
			case GAMESTATE_PLAY:
				playInput(&game);
				playDraw(&game);
				break;
			case GAMESTATE_EXIT:
				exitInput(&game);
				exitDraw(&game);
				break;
		}
	}

cleanup:
	SDL_DestroyRenderer(game.renderer);
	SDL_DestroyWindow(game.window);
	for (i32 i = 0; i < SOUND_COUNT; i++)
		Mix_FreeChunk(game.sound[i]);
	for (i32 i = 0; i < MENUMUSIC_COUNT; i++)
		Mix_FreeMusic(game.menuMusic[i]);
	for (i32 i = 0; i < GAMEMUSIC_COUNT; i++)
		Mix_FreeMusic(game.gameMusic[i]);
	quitSDL();
	return 0;
}
