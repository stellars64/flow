#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include "board.h"

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
	GAMESTATE_EXIT,
	GAMESTATE_INTRO,
	GAMESTATE_MENU,
	GAMESTATE_PLAY,
	GAMESTATE_PAUSE
} GameState;

typedef struct Sprite
{
	SDL_Texture *texture;
	SDL_Rect source;
	SDL_Rect destination;
	SDL_Point origin;
} Sprite;

typedef struct MenuButton
{
	SDL_Rect bounds;
	SDL_Texture *texture;
	bool hovered;
	bool clicked;
} MenuButton;

typedef struct Menu
{
	MenuButton *welcomeMsg;
	MenuButton *play8_8;
	MenuButton *play9_9;
	MenuButton *play10_10;
	MenuButton *exit;
} Menu;

// TODO: break up this monolithic struct
typedef struct Game
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	Mix_Chunk *sound[SOUND_COUNT];
	Mix_Music *menuMusic[MENUMUSIC_COUNT];
	Mix_Music *gameMusic[GAMEMUSIC_COUNT];
	TTF_Font *font;
	Menu menu;
	u64 dt;
	u64 lastTime;
	bool isTimerStarted;
	u64 gameTimer;
	u64 introTimer;
	Sprite splash;
	i32 boardSize;
	Board *board;
	bool running;
	GameState state;
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
void clearPipe(Board *b, CellColor color);
SDL_Texture* createSDLText(SDL_Renderer*, const char*, TTF_Font*, SDL_Color);
MenuButton* createMenuButton(SDL_Texture *texture, int x, int y);
void drawMenuButton(SDL_Renderer *renderer, MenuButton *button);
void destroyMenuButton(MenuButton *button);
void switchState(Game *g, GameState state);
void introInit(Game *g);
void introLoop(Game *g);
void introInput(Game *g);
void introDraw(Game *g);
void introExit(Game *g);
void menuInit(Game *g);
void menuLoop(Game *g);
void menuInput(Game *g);
void menuDraw(Game *g);
void menuExit(Game *g);
void playInit(Game *g);
void playLoop(Game *g);
void playInput(Game *g);
void playDraw(Game *g);
void playExit(Game *g);
void pauseInit(Game *g);
void pauseLoop(Game *g);
void pauseInput(Game *g);
void pauseDraw(Game *g);
void pauseExit(Game *g);

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
	TTF_Quit();
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


void clearPipe(Board *b, CellColor color)
{
	for (i32 i = 0; i < b->width * b->height; i++)
	{
		Cell *cell = &b->cells[i];
		if (cell->color == color)
		{
			cell->connection = CELLCONNECTION_NONE;
			if (cell->state != CELLSTATE_PIPE_START && cell->state != CELLSTATE_PIPE_END)
			{
				cell->state = CELLSTATE_EMPTY;
			}
		}
	}
}

SDL_Texture* createSDLText(SDL_Renderer *rdr, const char *text, TTF_Font *font, SDL_Color color)
{
	SDL_Surface *surf = TTF_RenderText_Blended(font, text, color);
	if (!surf)
	{
		TTF_CloseFont(font);
		fprintf(stderr, "TTF_RenderText: %s\n", TTF_GetError());
		return NULL;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(rdr, surf);
	if (!texture)
	{
		fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
	}
	SDL_FreeSurface(surf);
	return texture;
}

MenuButton* createMenuButton(SDL_Texture *texture, int x, int y)
{
	if (!texture)
		return NULL;

	MenuButton *button = malloc(sizeof(MenuButton));
	if (!button)
		return NULL;

	button->texture = texture;
	SDL_QueryTexture(texture, NULL, NULL, &button->bounds.w, &button->bounds.h);
	button->bounds.x = x - (button->bounds.w / 2);
	button->bounds.y = y - (button->bounds.h / 2);
	return button;
}

void drawMenuButton(SDL_Renderer *renderer, MenuButton *button)
{
	if (button->hovered)
		SDL_SetTextureColorMod(button->texture, 255, 255, 255);
	else
		SDL_SetTextureColorMod(button->texture, 128, 128, 128);

	SDL_RenderCopy(renderer, button->texture, NULL, &button->bounds);

	SDL_SetTextureColorMod(button->texture, 255, 255, 255);
}

void destroyMenuButton(MenuButton *button)
{
	if (!button)
		return;
	SDL_DestroyTexture(button->texture);
	free(button);
}

void switchState(Game *g, GameState state)
{
	if (state == g->state)
	{
		return;
	}

	if (state > g->state)
	{
		for (int gs = g->state + 1; gs <= state; gs++)
		{
			switch (gs)
			{
				default:
				case GAMESTATE_EXIT:
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
				case GAMESTATE_PAUSE:
					pauseInit(g);
					break;
			}
		}
	}

	if (state < g->state)
	{
		for (int gs = g->state; gs > state; gs--)
		{
			switch (gs)
			{
				default:
				case GAMESTATE_EXIT:
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
				case GAMESTATE_PAUSE:
					pauseExit(g);
					break;
			}
		}
	}

	g->state = state;
}

void introInit(Game *g)
{
	if (!startSDL()
		|| !(g->window = SDL_CreateWindow("flow", 1920, 0, 800, 600, SDL_WINDOW_SHOWN))
		|| !(g->renderer = SDL_CreateRenderer(g->window, -1, SDL_RENDERER_ACCELERATED))
		|| !(g->sound[SOUND_CLICK] = Mix_LoadWAV("assets/Audio/click_001.wav"))
		|| !(g->sound[SOUND_YUH] = Mix_LoadWAV("assets/Audio/YUH.wav"))
		|| !(g->menuMusic[MENUMUSIC_001] = Mix_LoadMUS("assets/Audio/flow-song1.wav"))
		|| !(g->menuMusic[MENUMUSIC_002] = Mix_LoadMUS("assets/Audio/flow-song-3-intro.wav"))
		|| !(g->gameMusic[GAMEMUSIC_001] = Mix_LoadMUS("assets/Audio/flow-song2.wav"))
		|| !(g->gameMusic[GAMEMUSIC_002] = Mix_LoadMUS("assets/Audio/flow-song-3-game.wav"))
		|| !(g->font = TTF_OpenFont("assets/Fonts/IosevkaTermNerdFont-Bold.ttf", 24)))
	{
		fprintf(stderr, "Failed to initialize\n");
		fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
		fprintf(stderr, "SDL_mixer Error: %s\n", Mix_GetError());
		fprintf(stderr, "SDL_ttf Error: %s\n", TTF_GetError());
		switchState(g, GAMESTATE_EXIT);
		return;
	}

	srand(time(NULL));
	g->boardSize = DEFAULT_BOARD_SIZE;
	g->running = true;
	g->introTimer = 3000;
	g->splash.texture = IMG_LoadTexture(g->renderer, "assets/Sprites/splash.png");
	if (!g->splash.texture)
	{
		fprintf(stderr, "Failed to load splash texture\n");
		fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
		switchState(g, GAMESTATE_EXIT);
		return;
	}
}
void introLoop(Game *g)
{
	u64 currTime = SDL_GetTicks64();
	u64 lastTime = currTime;

	while (g->running && g->state == GAMESTATE_INTRO)
	{
		currTime = SDL_GetTicks64();
		u64 dt = currTime - lastTime;
		lastTime = currTime;

		g->introTimer -= dt;
		if (g->introTimer <= 0)
			switchState(g, GAMESTATE_MENU);

		introInput(g);
		introDraw(g);
	}
	
}

void introInput(Game *g)
{

}

void introDraw(Game *g)
{
	SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 255);
	SDL_RenderClear(g->renderer);
	SDL_RenderCopy(g->renderer, g->splash.texture, NULL, NULL);
	SDL_RenderPresent(g->renderer);
}

void introExit(Game *g)
{
	SDL_DestroyTexture(g->splash.texture);
	TTF_CloseFont(g->font);
	for (i32 i = 0; i < SOUND_COUNT; i++)
		Mix_FreeChunk(g->sound[i]);
	for (i32 i = 0; i < MENUMUSIC_COUNT; i++)
		Mix_FreeMusic(g->menuMusic[i]);
	for (i32 i = 0; i < GAMEMUSIC_COUNT; i++)
		Mix_FreeMusic(g->gameMusic[i]);
	SDL_DestroyRenderer(g->renderer);
	SDL_DestroyWindow(g->window);
	quitSDL();
}

void menuInit(Game *g)
{
	SDL_Texture *welcomeMsgt, *play8x8t, *play9x9t, *play10x10t, *exit;
	SDL_Color white = {255, 255, 255, 255};
	SDL_Color green = {  0, 255,   0, 255};

	welcomeMsgt= createSDLText(g->renderer, "Welcome to Flow", g->font, green);
	play8x8t   = createSDLText(g->renderer, "Play 8x8",        g->font, white);
	play9x9t   = createSDLText(g->renderer, "Play 9x9",        g->font, white);
	play10x10t = createSDLText(g->renderer, "Play 10x10",      g->font, white);
	exit       = createSDLText(g->renderer, "Exit",            g->font, white);

	i32 ww, wh;
	SDL_GetWindowSize(g->window, &ww, &wh);

	g->menu.welcomeMsg = createMenuButton(welcomeMsgt, ww / 2, wh*1 / 11);
	g->menu.play8_8    = createMenuButton(play8x8t,    ww / 2, wh*2 / 11);
	g->menu.play9_9    = createMenuButton(play9x9t,    ww / 2, wh*3 / 11);
	g->menu.play10_10  = createMenuButton(play10x10t,  ww / 2, wh*4 / 11);
	g->menu.exit       = createMenuButton(exit,        ww / 2, wh*5 / 11);

	Mix_PlayMusic(g->menuMusic[MENUMUSIC_001], -1);
}

void menuLoop(Game *g)
{
	menuInput(g);
	while (g->running && g->state == GAMESTATE_MENU)
	{
		menuDraw(g);
		SDL_RenderPresent(g->renderer);
		menuInput(g);
	}
}

void menuInput(Game *g)
{
	i32 mx, my;
	u32 mb = SDL_GetMouseState(&mx, &my);

	g->menu.welcomeMsg->hovered = true;
	g->menu.play8_8->hovered    = inBounds(mx, my, g->menu.play8_8->bounds);
	g->menu.play9_9->hovered    = inBounds(mx, my, g->menu.play9_9->bounds);
	g->menu.play10_10->hovered  = inBounds(mx, my, g->menu.play10_10->bounds);
	g->menu.exit->hovered       = inBounds(mx, my, g->menu.exit->bounds);

	if (mb & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		if (g->menu.play8_8->hovered)
		{
			g->boardSize = 8;
			switchState(g, GAMESTATE_PLAY);
		}
		else if (g->menu.play9_9->hovered)
		{
			g->boardSize = 9;
			switchState(g, GAMESTATE_PLAY);
		}
		else if (g->menu.play10_10->hovered)
		{
			g->boardSize = 10;
			switchState(g, GAMESTATE_PLAY);
		}
		else if (g->menu.exit->hovered)
		{
			switchState(g, GAMESTATE_EXIT);
		}
	}

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				g->running = false;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						switchState(g, GAMESTATE_EXIT);
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
	drawMenuButton(g->renderer, g->menu.welcomeMsg);
	drawMenuButton(g->renderer, g->menu.play8_8);
	drawMenuButton(g->renderer, g->menu.play9_9);
	drawMenuButton(g->renderer, g->menu.play10_10);
	drawMenuButton(g->renderer, g->menu.exit);
}

void menuExit(Game *g)
{
	destroyMenuButton(g->menu.welcomeMsg);
	destroyMenuButton(g->menu.play8_8);
	destroyMenuButton(g->menu.play9_9);
	destroyMenuButton(g->menu.play10_10);
	destroyMenuButton(g->menu.exit);
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
	g->running = true;

	g->lastTime = SDL_GetTicks64();

	g->isTimerStarted = false;

	Mix_PlayMusic(g->gameMusic[GAMEMUSIC_001], -1);
}

void playLoop(Game *g)
{
	playInput(g);
	while (g->running && g->state == GAMESTATE_PLAY)
	{
		playDraw(g);
		SDL_RenderPresent(g->renderer);
		playInput(g);
	}
}

void playInput(Game *g)
{
	if (!g->running)
		return;

	u64 currTime = SDL_GetTicks64();
	g->dt = (currTime - g->lastTime);
	g->lastTime = currTime;

	if (g->board->genState != GENSTATE_IDLE)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					g->running = false;
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_r)
					{
						g->board->genState = GENSTATE_STOPREQUESTED;
					}
					if (event.key.keysym.sym == SDLK_p)
					{
						switchState(g, GAMESTATE_PAUSE);
					}
					break;
			}
		}
		return;
	}
	else
	{
		if (!g->isTimerStarted)
		{
			g->isTimerStarted = true;
			g->gameTimer = 1000 * 60 * 1;
			g->gameTimer += 15000;
		}
	}

	g->gameTimer -= g->dt;
	if (g->gameTimer <= 0)
	{
		switchState(g, GAMESTATE_MENU);
	}

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
					
					// TODO: if player solves board with less than all pipes, will not trigger
					bool solved = true;
					for (i32 i = 0; i < g->board->width * g->board->height; i++)
					{
						if (g->board->cells[i].state == CELLSTATE_EMPTY)
						{
							solved = false;
							break;
						}
					}
					if (solved)
					{
						switchState(g, GAMESTATE_MENU);
						switchState(g, GAMESTATE_PLAY);
						return;
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
				g->running = false;
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
						g->endPoint = (hovered->state==CELLSTATE_PIPE_START)
							? CELLSTATE_PIPE_END
							: CELLSTATE_PIPE_START;
						clearPipe(g->board, g->selectedColor);
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (g->piping)
				{
					clearPipe(g->board, g->selectedColor);
					g->piping = false;
				}
				break;
			
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_p)
				{
					switchState(g, GAMESTATE_PAUSE);
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

	i32 ww, wh;
	SDL_GetWindowSize(g->window, &ww, &wh);
	char timerText[15];
	i32 minutes = (g->gameTimer / 1000) / 60;
	i32 seconds = (g->gameTimer / 1000) % 60;
	snprintf(timerText, 15, "%02i:%02i", minutes, seconds);
	SDL_Color white = {255,255,255,255};
	SDL_Texture *timerTexture = createSDLText(g->renderer, timerText, g->font, white);
	MenuButton *timer = createMenuButton(timerTexture, ww / 2, 10);
	drawMenuButton(g->renderer, timer);
	destroyMenuButton(timer);
}

void playExit(Game *g)
{
	free(g->pipeSeq);
	boardFree(g->board);
}

void pauseInit(Game *g)
{
	if (g->board->genState == GENSTATE_GENERATING)
	{
		g->board->genState = GENSTATE_PAUSE;
	}
}

void pauseLoop(Game *g)
{
	pauseInput(g);
	while (g->running && g->state == GAMESTATE_PAUSE)
	{
		pauseDraw(g);
		SDL_RenderPresent(g->renderer);
		pauseInput(g);
	}
}

void pauseInput(Game *g)
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				g->running = false;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_p:
						switchState(g, GAMESTATE_PLAY);
						break;
				}
				break;
		}
	}
}

void pauseDraw(Game *g)
{
	playDraw(g);
	SDL_SetRenderDrawBlendMode(g->renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 150);
	SDL_RenderFillRect(g->renderer, NULL);
}

void pauseExit(Game *g)
{
	if (g->board->genState == GENSTATE_PAUSE)
	{
		g->board->genState = GENSTATE_GENERATING;
	}
}

int main(int argc, char *argv[])
{
	Game game;
	switchState(&game, GAMESTATE_INTRO);

	while (game.running)
	{
		switch (game.state)
		{
			default:
			case GAMESTATE_EXIT:
				game.running = false;
				break;
			case GAMESTATE_INTRO:
				introLoop(&game);
				break;
			case GAMESTATE_MENU:
				menuLoop(&game);
				break;
			case GAMESTATE_PLAY:
				playLoop(&game);
				break;
			case GAMESTATE_PAUSE:
				pauseLoop(&game);
				break;
		}
	}
	return 0;
}
