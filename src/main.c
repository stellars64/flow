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


void playFlow(SDL_Window *window, SDL_Renderer *renderer, Mix_Chunk *sounds[])
{
	// generate a game board
	Board *board = boardCreate(5, 8);
	boardGenerate(board, 8);

	// dimensions for drawing the board
	i32 windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	i32 boardWidth = windowWidth * 0.8;
	i32 boardHeight = windowHeight * 0.8;
	i32 boardX = (windowWidth - boardWidth) / 2;
	i32 boardY = (windowHeight - boardHeight) / 2;
	i32 cellWidth = boardWidth / 5;
	i32 cellHeight = boardHeight / 5;

	// information related to runnin' pipe
	CellColor selectedColor = CELLCOLOR_RED;
	bool piping = false;
	CellState endPoint = CELLSTATE_PIPE_END; // pretty dumb, but we can start at end and go to start
	i32 lastPipeRow=0, lastPipeCol=0, startPipeRow=0, startPipeCol=0;

	// user input info
	i32 mouseX=0, mouseY=0;
	bool isCellHovered = false;
	i32 hoveredCellRow=0, hoveredCellCol=0;
	SDL_Event event;

	bool gameRunning = true;
	while (gameRunning)
	{
		// input
		SDL_GetMouseState(&mouseX, &mouseY);
		if (mouseX >= boardX && mouseX <= (boardX + boardWidth)
		    && mouseY >= boardY && mouseY <= (boardY + boardHeight))
		{
			isCellHovered = true;
			hoveredCellRow = (mouseY - boardY) / cellHeight;
			hoveredCellCol = (mouseX - boardX) / cellWidth;

			if (piping 
			    && (hoveredCellRow != lastPipeRow || hoveredCellCol != lastPipeCol)
				&& ((abs(hoveredCellRow - lastPipeRow) == 1 && hoveredCellCol == lastPipeCol)
				    || (abs(hoveredCellCol - lastPipeCol) == 1 && hoveredCellRow == lastPipeRow)))
			{
				printf("On a new cell\n");
				if (boardGet(board, hoveredCellRow, hoveredCellCol)->state == CELLSTATE_EMPTY)
				{
					boardSetColor(board, hoveredCellRow, hoveredCellCol, selectedColor);
					boardSetState(board, hoveredCellRow, hoveredCellCol, CELLSTATE_PIPE_H);
					lastPipeRow = hoveredCellRow;
					lastPipeCol = hoveredCellCol;
					Mix_PlayChannel(-1, sounds[CHUNK_CLICK_001], 0);
				}
				else if (boardGet(board, hoveredCellRow, hoveredCellCol)->state == endPoint
				         && boardGet(board, hoveredCellRow, hoveredCellCol)->color == selectedColor)
				{
					printf("We hit the end of the pipe\n");
					piping = false;
					Mix_PlayChannel(-1, sounds[CHUNK_YUH], 0);
				}
			}
		}
		else
		{
			isCellHovered = false;
		}
		
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					gameRunning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (!piping && isCellHovered)
					{
						CellState hoveredState =
							boardGet(board, hoveredCellRow, hoveredCellCol)->state;
						if (hoveredState==CELLSTATE_PIPE_START || hoveredState==CELLSTATE_PIPE_END)
						{
							piping = true;
							selectedColor = boardGet(board, hoveredCellRow, hoveredCellCol)->color;
							lastPipeRow = hoveredCellRow;
							lastPipeCol = hoveredCellCol;
							startPipeRow = hoveredCellRow;
							startPipeCol = hoveredCellCol;
							if (hoveredState == CELLSTATE_PIPE_START)
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
								if (boardGet(board, r, c)->state != CELLSTATE_PIPE_START
									&& boardGet(board, r, c)->state != CELLSTATE_PIPE_END
									&& boardGet(board, r, c)->color == selectedColor)
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
		SDL_Rect boardBackground = {boardX, boardY, boardWidth, boardHeight};
		SDL_RenderFillRect(renderer, &boardBackground);

		// draw cell stuff
		for (i32 r = 0; r < board->height; r++)
		{
			for (i32 c = 0; c < board->width; c++)
			{
				// the cell background
				SDL_Rect cellBg = {
					boardX + (c * cellWidth) + (0.1 * cellWidth),
					boardY + (r * cellHeight) + (0.1 * cellHeight),
					cellWidth - (0.2 * cellWidth),
					cellHeight - (0.2 * cellHeight)
				};
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderFillRect(renderer, &cellBg);

				i32 red, green, blue;
				switch (boardGet(board, r, c)->color)
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
						red=255; green=0; blue=255; break;
					case CELLCOLOR_CYAN:
						red=0; green=255; blue=255; break;
					case CELLCOLOR_WHITE:
						red=255; green=255; blue=255; break;
					case CELLCOLOR_MAGENTA:
						red=255; green=0; blue=255; break;
					case CELLCOLOR_PINK:
						red=255; green=192; blue=203; break;
				}


				// the cell
				switch (boardGet(board, r, c)->state)
				{	
					// here are all the different cell states
					default:
					case CELLSTATE_EMPTY:
					case CELLSTATE_EMPTY_MARKED:
						break;
					case CELLSTATE_PIPE_START:
					case CELLSTATE_PIPE_END:
					{
						SDL_Rect cellFg = {
							cellBg.x + (0.1 * cellBg.w),
							cellBg.y + (0.1 * cellBg.h),
							cellBg.w - (0.2 * cellBg.w),
							cellBg.h - (0.2 * cellBg.h)
						};
						SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
						SDL_RenderFillRect(renderer, &cellFg);
					}
					break;
					case CELLSTATE_PIPE_H:
					{
						SDL_Rect cellFg = {
							cellBg.x + (0.1 * cellBg.w),
							cellBg.y + (0.4 * cellBg.h),
							cellBg.w - (0.2 * cellBg.w),
							cellBg.h - (0.8 * cellBg.h)
						};
						SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
						SDL_RenderFillRect(renderer, &cellFg);
					}
					break;
				}
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
	    || !(window = SDL_CreateWindow("flow", 1920, 0, 1280, 1280, SDL_WINDOW_SHOWN))
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
