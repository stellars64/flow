#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include "board.h"

#define CHUNK_CLICK_001 0
#define CHUNK_YUH 1

bool startSDL();
void quitSDL();
bool inBounds(i32 x, i32 y, SDL_Rect rect);
bool pointsEqual(SDL_Point a, SDL_Point b);
bool pointsAdjacent(SDL_Point a, SDL_Point b);
void drawPipeSection(SDL_Renderer *renderer, SDL_Rect *cellBg, Direction dir, i32 r, i32 g, i32 b);
void drawCell(SDL_Renderer *renderer, SDL_Rect *cellBg, Cell *cell);
void playFlow(SDL_Window *window, SDL_Renderer *renderer, Mix_Chunk *sounds[]);

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

void drawPipeSection(SDL_Renderer *renderer, SDL_Rect *cellBg, Direction dir, i32 r, i32 g, i32 b)
{
	SDL_Rect dest;
	switch (dir)
	{
		default:
		case DIRECTION_UP:
			dest.x = cellBg->x + (cellBg->w / 2);
			dest.y = cellBg->y;
			dest.h = cellBg->h / 2;
			dest.w = cellBg->w / 10;
			break;
		case DIRECTION_DOWN:
			dest.x = cellBg->x + (cellBg->w / 2);
			dest.y = cellBg->y + (cellBg->h / 2);
			dest.h = cellBg->h / 2;
			dest.w = cellBg->w / 10;
			break;
		case DIRECTION_LEFT:
			dest.x = cellBg->x;
			dest.y = cellBg->y + (cellBg->h / 2);
			dest.h = cellBg->h / 10;
			dest.w = cellBg->w / 2;
			break;
		case DIRECTION_RIGHT:
			dest.x = cellBg->x + (cellBg->w / 2);
			dest.y = cellBg->y + (cellBg->h / 2);
			dest.h = cellBg->h / 10;
			dest.w = cellBg->w / 2;
			break;
	}
	SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	SDL_RenderFillRect(renderer, &dest);
}

void drawCell(SDL_Renderer *renderer, SDL_Rect *cellBg, Cell *cell)
{
	i32 red, green, blue;
	switch (cell->color)
	{
		case CELLCOLOR_COUNT:
		case CELLCOLOR_RED:
			red=255; green=0; blue=0; break;
		case CELLCOLOR_GREEN:
			red=0; green=255; blue=0; break;
		case CELLCOLOR_BLUE:
			red=0; green=0; blue=255; break;
		case CELLCOLOR_YELLOW:
			red=255; green=255; blue=0; break;
		case CELLCOLOR_PURPLE:
			red=125; green=0; blue=255; break;
		case CELLCOLOR_CYAN:
			red=0; green=255; blue=255; break;
		case CELLCOLOR_WHITE:
			red=255; green=255; blue=255; break;
		case CELLCOLOR_MAGENTA:
			red=255; green=0; blue=255; break;
		case CELLCOLOR_PINK:
			red=255; green=192; blue=203; break;
		case CELLCOLOR_DARK_RED:
			red=139; green=0; blue=0; break;
		case CELLCOLOR_DARK_GREEN:
			red=0; green=100; blue=0; break;
		case CELLCOLOR_DARK_BLUE:
			red=0; green=0; blue=139; break;
	}
	
	switch (cell->state)
	{	
		// here are all the different cell states
		default:
		case CELLSTATE_EMPTY:
		case CELLSTATE_EMPTY_MARKED:
			break;
		case CELLSTATE_PIPE_START:
		case CELLSTATE_PIPE_END:
		{
			SDL_Rect circle = {
				cellBg->x + (0.1 * cellBg->w),
				cellBg->y + (0.1 * cellBg->h),
				cellBg->w - (0.2 * cellBg->w),
				cellBg->h - (0.2 * cellBg->h)
			};
			SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
			SDL_RenderFillRect(renderer, &circle);
		}
		break;
		case CELLSTATE_PIPE_H:
		{
			SDL_Rect hbar = {
				cellBg->x + (0.1 * cellBg->w),
				cellBg->y + (0.4 * cellBg->h),
				cellBg->w - (0.2 * cellBg->w),
				cellBg->h - (0.8 * cellBg->h)
			};
			SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
			SDL_RenderFillRect(renderer, &hbar);
		}
		break;
		case CELLSTATE_PIPE_V:
		{
			SDL_Rect vbar = {
				cellBg->x + (0.4 * cellBg->w),
				cellBg->y + (0.1 * cellBg->h),
				cellBg->w - (0.8 * cellBg->w),
				cellBg->h - (0.2 * cellBg->h)
			};
			SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
			SDL_RenderFillRect(renderer, &vbar);
		}
		break;
		case CELLSTATE_PIPE_LU:
			drawPipeSection(renderer, cellBg, DIRECTION_LEFT, red, green, blue);
			drawPipeSection(renderer, cellBg, DIRECTION_UP, red, green, blue);
			break;
		case CELLSTATE_PIPE_LD:
			drawPipeSection(renderer, cellBg, DIRECTION_LEFT, red, green, blue);
			drawPipeSection(renderer, cellBg, DIRECTION_DOWN, red, green, blue);
		    break;
		case CELLSTATE_PIPE_RU:
			drawPipeSection(renderer, cellBg, DIRECTION_RIGHT, red, green, blue);
			drawPipeSection(renderer, cellBg, DIRECTION_UP, red, green, blue);
			break;
		case CELLSTATE_PIPE_RD:
			drawPipeSection(renderer, cellBg, DIRECTION_RIGHT, red, green, blue);
			drawPipeSection(renderer, cellBg, DIRECTION_DOWN, red, green, blue);
		    break;
	}
}

void setCellStateAngle(Board *board, SDL_Point *cellPos)
{
	Direction dir[2];
	i32 di = 0;

	// x is diff, so we moved horizontally
	for (SDL_Point *c = cellPos - 1; c < cellPos + 1; c += 1)
	{
		SDL_Point *prev = c;
		SDL_Point *curr = c + 1;

		if (curr->y == prev->y)
		{
			if (curr->x > prev->x)
			{
				dir[di] = DIRECTION_RIGHT;
			}
			else
			{
				dir[di] = DIRECTION_LEFT;
			}	
		}
		else
		{
			if (curr->y > prev->y)
			{
				dir[di] = DIRECTION_DOWN;
			}
			else
			{
				dir[di] = DIRECTION_UP;
			}
		}
		di += 1;
	}

	Cell *cell = boardGet(board, cellPos->y, cellPos->x);
	switch (dir[0])
	{
		case DIRECTION_UP:
			switch (dir[1])
			{
				default:
				case DIRECTION_UP:
					cell->state = CELLSTATE_PIPE_V;
					break;
				case DIRECTION_LEFT:
					cell->state = CELLSTATE_PIPE_LD;
					break;
				case DIRECTION_RIGHT:
					cell->state = CELLSTATE_PIPE_RD;
					break;
			}
			break;
		case DIRECTION_DOWN:
			switch (dir[1])
			{
				default:
				case DIRECTION_DOWN:
					cell->state = CELLSTATE_PIPE_V;
					break;
				case DIRECTION_LEFT:
					cell->state = CELLSTATE_PIPE_LU;
					break;
				case DIRECTION_RIGHT:
					cell->state = CELLSTATE_PIPE_RU;
					break;
			}
			break;
		case DIRECTION_LEFT:
			switch (dir[1])
			{
				default:
				case DIRECTION_LEFT:
					cell->state = CELLSTATE_PIPE_H;
					break;
				case DIRECTION_UP:
					cell->state = CELLSTATE_PIPE_RU;
					break;
				case DIRECTION_DOWN:
					cell->state = CELLSTATE_PIPE_RD;
					break;
			}
			break;
		case DIRECTION_RIGHT:
			switch (dir[1])
			{
				default:
				case DIRECTION_RIGHT:
					cell->state = CELLSTATE_PIPE_H;
					break;
				case DIRECTION_UP:
					cell->state = CELLSTATE_PIPE_LU;
					break;
				case DIRECTION_DOWN:
					cell->state = CELLSTATE_PIPE_LD;
					break;
			}
			break;
	}
}

#define BOARD_SIZE 8
void playFlow(SDL_Window *window, SDL_Renderer *renderer, Mix_Chunk *sounds[])
{
	// generate a game board
	Board *board = boardCreate(BOARD_SIZE, BOARD_SIZE);
	printf("Generating....\n");
	boardGenerate(board, BOARD_SIZE);
	printf("Generated!\n");

	// dimensions for drawing the board
	i32 windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	SDL_Rect boardDim = {
		(windowWidth - (windowWidth * 0.8)) / 2,
		(windowHeight - (windowHeight * 0.8)) / 2,
		windowWidth * 0.8,
		windowHeight * 0.8
	};
	SDL_Point cellDim = {
		boardDim.w / BOARD_SIZE,
		boardDim.h / BOARD_SIZE
	};
	i32 cellWidth = boardDim.w / BOARD_SIZE;
	i32 cellHeight = boardDim.h / BOARD_SIZE;

	// information related to runnin' pipe
	CellColor selectedColor = CELLCOLOR_RED;
	bool piping = false;
	CellState endPoint = CELLSTATE_PIPE_END; // pretty dumb, but we can start at end and go to start
	
	SDL_Point pipeSeq[BOARD_SIZE * BOARD_SIZE];
	i32 pipeSeqSize = 0;
	
	// user input info
	SDL_Point mouse = {0, 0};
	bool isHovered = false;
	SDL_Point hoveredCell = {0, 0};
	SDL_Event event;

	bool gameRunning = true;
	while (gameRunning)
	{
		// input
		SDL_GetMouseState(&mouse.x, &mouse.y);
		/*
		if (mouse.x >= boardX && mouse.x <= (boardX + boardWidth)
		    && mouse.y >= boardY && mouse.y <= (boardY + boardHeight))
		*/
		if (inBounds(mouse.x, mouse.y, boardDim))
		{
			isHovered = true;
			hoveredCell.y = (mouse.y - boardDim.y) / cellHeight;
			hoveredCell.x = (mouse.x - boardDim.x) / cellWidth;

			if (piping && pointsAdjacent(hoveredCell, pipeSeq[pipeSeqSize - 1]))
				//&& !pointsEqual(hoveredCell, pipeSeq[pipeSeqSize - 1])
				//&& pointsAdjacent(hoveredCell, pipeSeq[pipeSeqSize - 1]))
			{
				Cell* hovered = boardGet(board, hoveredCell.y, hoveredCell.x);
				if (hovered->state == CELLSTATE_EMPTY)
				{
					hovered->color = selectedColor;
					if (hoveredCell.y == pipeSeq[pipeSeqSize - 1].y)
					{
						boardSetState(board, hoveredCell.y, hoveredCell.x, CELLSTATE_PIPE_H);
					}
					else
					{
						boardSetState(board, hoveredCell.y, hoveredCell.x, CELLSTATE_PIPE_V);
					}
					pipeSeq[pipeSeqSize] = hoveredCell;
					pipeSeqSize += 1;

					// set correct cellstate for previous cell, which is now sandwiched
					// between the current cell and the cell before it
					if (pipeSeqSize > 2)
					{
						setCellStateAngle(board, &pipeSeq[pipeSeqSize - 2]);
					}

					/*
					// set the correct cellstate for the previous cell
					if (pipeSeqSize > 2)
					{
						Direction dir[2];
						i32 di = 0;

						// x is diff, so we moved horizontally
						for (i32 i = pipeSeqSize - 3; i < pipeSeqSize - 1; i += 1)
						{
							SDL_Point prev = pipeSeq[i];
							SDL_Point curr = pipeSeq[i+1];
							// moved left/right
							if (curr.y == prev.y)
							{
								// moved right
								if (curr.x > prev.x)
								{
									dir[di] = DIRECTION_RIGHT;
								}
								// moved left
								else
								{
									dir[di] = DIRECTION_LEFT;
								}
							}
							else
							{
								// moved down
								if (curr.y > prev.y)
								{
									dir[di] = DIRECTION_DOWN;
								}
								// moved up
								else
								{
									dir[di] = DIRECTION_UP;
								}
							}
							di += 1;
						}

						// get previous cell, and set the correct state
						Cell *prev = boardGet(board, 
							pipeSeq[pipeSeqSize - 2].y, pipeSeq[pipeSeqSize - 2].x);

						// state depends on dir[0] & dir[1]
						// this is really fucking confusing
						if (dir[0] == DIRECTION_LEFT)
						{
							if (dir[1] == DIRECTION_LEFT)
							{
								prev->state = CELLSTATE_PIPE_H;
							}
							else if (dir[1] == DIRECTION_UP)
							{
								prev->state = CELLSTATE_PIPE_RU;
							}
							else if (dir[1] == DIRECTION_DOWN)
							{
								prev->state = CELLSTATE_PIPE_RD;
							}
						}
						else if (dir[0] == DIRECTION_RIGHT)
						{
							if (dir[1] == DIRECTION_RIGHT)
							{
								prev->state = CELLSTATE_PIPE_H;
							}
							else if (dir[1] == DIRECTION_UP)
							{
								prev->state = CELLSTATE_PIPE_LU;
							}
							else if (dir[1] == DIRECTION_DOWN)
							{
								prev->state = CELLSTATE_PIPE_LD;
							}
						}
						else if (dir[0] == DIRECTION_UP)
						{
							if (dir[1] == DIRECTION_LEFT)
							{
								prev->state = CELLSTATE_PIPE_LD;
							}
							else if (dir[1] == DIRECTION_RIGHT)
							{
								prev->state = CELLSTATE_PIPE_RD;
							}
							else if (dir[1] == DIRECTION_UP)
							{
								prev->state = CELLSTATE_PIPE_V;
							}
						}
						else if (dir[0] == DIRECTION_DOWN)
						{
							if (dir[1] == DIRECTION_LEFT)
							{
								prev->state = CELLSTATE_PIPE_LU;
							}
							else if (dir[1] == DIRECTION_RIGHT)
							{
								prev->state = CELLSTATE_PIPE_RU;
							}
							else if (dir[1] == DIRECTION_DOWN)
							{
								prev->state = CELLSTATE_PIPE_V;
							}
						}
					}
					*/


					Mix_PlayChannel(-1, sounds[CHUNK_CLICK_001], 0);
				}
				else if (boardGet(board, hoveredCell.y, hoveredCell.x)->state == endPoint
				         && boardGet(board, hoveredCell.y, hoveredCell.x)->color == selectedColor)
				{
					pipeSeq[pipeSeqSize] = hoveredCell;
					pipeSeqSize += 1;

					// set correct cellstate for previous cell, which is now sandwiched
					// between the current cell and the cell before it
					if (pipeSeqSize > 2)
					{
						setCellStateAngle(board, &pipeSeq[pipeSeqSize - 2]);
					}

					piping = false;
					pipeSeqSize = 0;
					Mix_PlayChannel(-1, sounds[CHUNK_YUH], 0);
				}
			}
		}
		else
		{
			isHovered = false;
		}
		
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					gameRunning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (!piping && isHovered)
					{
						Cell *hovered = boardGet(board, hoveredCell.y, hoveredCell.x);
						if (hovered->state == CELLSTATE_PIPE_START 
						    || hovered->state == CELLSTATE_PIPE_END)
						{
							piping = true;
							selectedColor = hovered->color;
							pipeSeqSize = 1;
							pipeSeq[0] = hoveredCell;
							if (hovered->state == CELLSTATE_PIPE_START)
							{
								endPoint = CELLSTATE_PIPE_END;
							}
							else
							{
								endPoint = CELLSTATE_PIPE_START;
							}
						}
							
					}
					break;
				case SDL_MOUSEBUTTONUP:
					// if we are still piping, we haven't completed it yet, completely undo for now
					if (piping)
					{
						for (i32 r = 0; r < board->height; r++)	
						{
							for (i32 c = 0; c < board->width; c++)
							{
								Cell* cell = boardGet(board, r, c);
								if (cell->state != CELLSTATE_PIPE_START
									&& cell->state != CELLSTATE_PIPE_END
									&& cell->color == selectedColor)
								{
									boardSetState(board, r, c, CELLSTATE_EMPTY);
								}
							}
						}
						piping = false;
					}
					break;
			}
		}

		// drawing
		// clear screen with gray
		SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
		SDL_RenderClear(renderer);

		// draw the background
		SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
		SDL_RenderFillRect(renderer, &boardDim);

		// draw cell stuff
		for (i32 r = 0; r < board->height; r++)
		{
			for (i32 c = 0; c < board->width; c++)
			{
				// the cell background
				SDL_Rect cellBg = {
					boardDim.x + (c * cellWidth) + (0.1 * cellWidth),
					boardDim.y + (r * cellHeight) + (0.1 * cellHeight),
					cellWidth - (0.2 * cellWidth),
					cellHeight - (0.2 * cellHeight)
				};
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderFillRect(renderer, &cellBg);

				// the cell
				drawCell(renderer, &cellBg, boardGet(board, r, c));
			}
		}

		SDL_RenderPresent(renderer);
	}
	boardFree(board);
}


int main(int argc, char *argv[])
{
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	Mix_Chunk *sounds[2];
	Mix_Chunk *placePipeSound = Mix_LoadWAV("assets/Audio/click_001.wav");
	Mix_Chunk *yuhSound = Mix_LoadWAV("assets/Audio/YUH.wav");

	if (!startSDL() 
	    || !(window = SDL_CreateWindow("flow", 1920, 0, 800, 600, SDL_WINDOW_SHOWN))
	    || !(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED))
	    || !(sounds[CHUNK_CLICK_001] = Mix_LoadWAV("assets/Audio/click_001.wav"))
	    || !(sounds[CHUNK_YUH] = Mix_LoadWAV("assets/Audio/YUH.wav")))
	{
		fprintf(stderr, "Error initializing\n");
		goto cleanup;
	}

	playFlow(window, renderer, sounds);


cleanup:
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	Mix_FreeChunk(sounds[CHUNK_YUH]);
	Mix_FreeChunk(sounds[CHUNK_CLICK_001]);
	quitSDL();
	return 0;
}
