#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "board.h"

bool placeStart(Board *board, i32 *pipes, i32 numPipes, i32 currentPipe, i32 shortestPipeLength);
bool placePipe(Board *board, i32 *pipes, i32 numPipes, i32 currentPipe, i32 shortestPipeLength,
			   i32 currentSize, i32 row, i32 col);

Board* boardCreate(i32 width, i32 height)
{
	Board *board = malloc(sizeof(Board));
	board->width = width;
	board->height = height;
	board->cells = malloc(sizeof(Cell) * width * height);
	i32 size = width * height;
	for (i32 i = 0; i < size; i++)
	{
		board->cells[i].state = CELLSTATE_EMPTY;
		board->cells[i].color = CELLCOLOR_RED;
		board->cells[i].connection = CELLCONNECTION_NONE;
	}
	board->isGenerated = false;
	return board;
}

void boardFree(Board *board)
{
	free(board->cells);
	free(board);
}

void boardSetColor(Board *board, i32 r, i32 c, CellColor color)
{
	boardGet(board, r, c)->color = color;
}

void boardSetState(Board *board, i32 r, i32 c, CellState state)
{
	boardGet(board, r, c)->state = state;
}

Cell* boardGet(Board *board, i32 r, i32 c)
{
	return &board->cells[r * board->width + c];
}

Cell* boardGetI(Board *board, i32 index)
{
	return &board->cells[index];
}

bool boardBoundsCheck(Board *board, i32 r, i32 c)
{
	return r >= 0 && r < board->height && c >= 0 && c < board->width;
}

void boardPrint(Board *board)
{
	for (i32 r = 0; r < board->height; r++)
	{
		for (i32 c = 0; c < board->width; c++)
		{
			if (boardGet(board, r, c)->state != CELLSTATE_EMPTY
				&& boardGet(board, r, c)->state != CELLSTATE_EMPTY_MARKED)
			{
				printf("%u ", boardGet(board, r, c)->color);
			}
			else
			{
				printf("_ ");
			}
		}
		printf("\n");
	}
}

void boardDfsFillState(Board *board, i32 row, i32 col, CellState oldState, CellState newState)
{
	if (!boardBoundsCheck(board, row, col))
		return;

	i32 bi = row * board->width + col;
	if (board->cells[bi].state == oldState)
	{
		board->cells[bi].state = newState;
		boardDfsFillState(board, row - 1, col, oldState, newState);
		boardDfsFillState(board, row + 1, col, oldState, newState);
		boardDfsFillState(board, row, col - 1, oldState, newState);
		boardDfsFillState(board, row, col + 1, oldState, newState);
	}
}

bool boardIsEmptyConnected(Board *board, i32 row, i32 col)
{
	CellState prevState = boardGet(board, row, col)->state;

	board->cells[row * board->width + col].state = CELLSTATE_PIPE_START;

	i32 emptyIndex = 0;
	bool emptyFound = false;

	i32 size = board->width * board->height;
	for (i32 i = 0; i < size; i++)
	{
		if (board->cells[i].state == CELLSTATE_EMPTY)
		{
			emptyIndex = i;
			emptyFound = true;
			break;
		}
	}

	if (!emptyFound)
	{
		boardGet(board, row, col)->state = prevState;
		return true;
	}

	i32 emptyRow = emptyIndex / board->width;
	i32 emptyCol = emptyIndex % board->width;

	boardDfsFillState(board, emptyRow, emptyCol, CELLSTATE_EMPTY, CELLSTATE_EMPTY_MARKED);

	bool allEmptyVisited = true;
	for (i32 i = 0; i < size; i++)
	{
		if (board->cells[i].state == CELLSTATE_EMPTY)
			allEmptyVisited = false;

		else if (board->cells[i].state == CELLSTATE_EMPTY_MARKED)
			board->cells[i].state = CELLSTATE_EMPTY;
	}

	board->cells[row * board->width + col].state = prevState;

	return allEmptyVisited;
}

i32 boardAdjacentWithColor(Board *board, i32 row, i32 col, CellColor color)
{
	Vec2i dirs[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
	i32 count = 0;

	for (i32 d = 0; d < 4; d += 1)
	{
		i32 adjRow = row + dirs[d].y;
		i32 adjCol = col + dirs[d].x;

		if (adjRow >= 0 && adjRow < board->height && adjCol >= 0 && adjCol < board->width
			&& boardGet(board, adjRow, adjCol)->state != CELLSTATE_EMPTY
			&& boardGet(board, adjRow, adjCol)->state != CELLSTATE_EMPTY_MARKED
			&& boardGet(board, adjRow, adjCol)->color == color)
		{
			count += 1;
		}
	}

	return count;
}


bool placePipe(Board *board, i32 *pipes, i32 numPipes, i32 currentPipe, i32 shortestPipeLength,
			   i32 currentSize, i32 row, i32 col)
{
	bool rejected[4] = {true, true, true, true};
	i32 numRejected = 4;
	Vec2i dirs[4] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

	for (i32 d = 0; d < 4; d += 1)
	{
		i32 adjRow = row + dirs[d].y;
		i32 adjCol = col + dirs[d].x;

		if (adjRow >= 0 && adjRow < board->height && adjCol >= 0 && adjCol < board->width
			&& boardGet(board, adjRow, adjCol)->state == CELLSTATE_EMPTY
			&& boardAdjacentWithColor(board, adjRow, adjCol, currentPipe) < 2
			&& boardIsEmptyConnected(board, adjRow, adjCol))
		{
			rejected[d] = false;
			numRejected -= 1;
		}
	}

	if (numRejected == 4)
		return false;

	currentSize += 1;
	while (numRejected < 4)
	{
		i32 direction = 0;
		{
			i32 dir = rand() % (4 - numRejected);
			i32 dirIndex = 0;
			for (i32 d = 0; d < 4; d += 1)
			{
				if (!rejected[d])
				{
					if (dirIndex == dir)
					{
						direction = d;
						break;
					}
					else
					{
						dirIndex += 1;
					}
				}
			}
		}

		i32 adjRow = row + dirs[direction].y;
		i32 adjCol = col + dirs[direction].x;

		CellState pipeState;
		if (currentSize == pipes[currentPipe])
		{
			pipeState = CELLSTATE_PIPE_END;
		}
		else
		{
			switch (direction)
			{
				case DIRECTION_UP:
				case DIRECTION_DOWN:
					pipeState = CELLSTATE_PIPE_V;
					break;
				default:
				case DIRECTION_LEFT:
				case DIRECTION_RIGHT:
					pipeState = CELLSTATE_PIPE_H;
					break;
			}
		}

		boardGet(board, adjRow, adjCol)->state = pipeState;
		boardGet(board, adjRow, adjCol)->color = currentPipe;

		if (currentSize == pipes[currentPipe])
		{
			if (currentPipe == numPipes - 1)
				return true;
			else
			{
				if (placeStart(board, pipes, numPipes, currentPipe + 1, shortestPipeLength))
					return true;
				else
				{
					rejected[direction] = true;
					numRejected += 1;
					boardGet(board, adjRow, adjCol)->state = CELLSTATE_EMPTY;
					boardGet(board, adjRow, adjCol)->color = 0;
				}
			}
		}
		else
		{
			if (placePipe(board, pipes, numPipes, currentPipe, shortestPipeLength, currentSize,
			              adjRow, adjCol))
				return true;
			else
			{
				rejected[direction] = true;
				numRejected += 1;
				boardGet(board, adjRow, adjCol)->state = CELLSTATE_EMPTY;
				boardGet(board, adjRow, adjCol)->color = 0;
			}
		}
	}
	return false;
}

bool placeStart(Board *board, i32 *pipes, i32 numPipes, i32 currentPipe, i32 shortestPipeLength)
{
	i32 rejectedSize = board->width * board->height;

	bool *rejected = malloc(sizeof(bool) * rejectedSize);
	for (i32 i = 0; i < rejectedSize; i++)
	{
		rejected[i] = false;
	}

	while (true)
	{
		i32 nonRejectedEmpty = 0;
		i32 boardSize = board->width * board->height;
		for (i32 i = 0; i < boardSize; i += 1)
		{
			if (boardGetI(board, i)->state == CELLSTATE_EMPTY && !rejected[i])
				nonRejectedEmpty += 1;
		}

		if (nonRejectedEmpty == 0)
		{
			free(rejected);
			return false;
		}

		i32 startIndex = 0;
		i32 startRow = 0;
		i32 startCol = 0;
		while (true)
		{
			i32 offset = rand() % nonRejectedEmpty;
			i32 current = 0;
			for (i32 i = 0; i < boardSize; i += 1)
			{
				if (boardGetI(board, i)->state == CELLSTATE_EMPTY && !rejected[i])
				{
					if (current == offset)
					{
						startIndex = i;
						startRow = i / board->width;
						startCol = i % board->width;
						break;
					}
					current += 1;
				}
			}

			if (boardIsEmptyConnected(board, startRow, startCol))
				break;
			else
			{
				rejected[startIndex] = true;
				nonRejectedEmpty -= 1;

				if (nonRejectedEmpty == 0)
				{
					free(rejected);
					return false;
				}
			}
		}

		boardGetI(board, startIndex)->state = CELLSTATE_PIPE_START;
		boardGetI(board, startIndex)->color = currentPipe;

		bool placed = placePipe(board, pipes, numPipes, currentPipe, shortestPipeLength, 1,
		                        startRow, startCol);
		if (placed)
		{
			free(rejected);
			return true;
		}
		else
		{
			rejected[startIndex] = true;
			boardGetI(board, startIndex)->state = CELLSTATE_EMPTY;
			boardGetI(board, startIndex)->color = 0;
		}
	}

	free(rejected);
	return false;
}

bool boardGenerate(Board *board)
{
	printf("Generating board...\n");
	u64 startTime = SDL_GetPerformanceCounter();

	i32 numPipes = board->width;

	board->isGenerated = false;

	if (numPipes >= CELLCOLOR_COUNT)
		return false;

	i32 size = board->width * board->height;
	i32 *pipes = malloc(sizeof(i32) * numPipes);

	for (i32 i = 0; i < numPipes; i++)
	{
		pipes[i] = 3;
	}

	i32 totalPipeLength = numPipes * 3;

	while (totalPipeLength < size)
	{
		i32 index = rand() % numPipes;
		pipes[index] += 1;
		totalPipeLength += 1;
	}

	i32 shortestPipeLength = INT_MAX;
	for (i32 i = 0; i < numPipes; i++)
	{
		if (pipes[i] < shortestPipeLength)
			shortestPipeLength = pipes[i];
	}

	bool placed = placeStart(board, pipes, numPipes, 0, shortestPipeLength);
	
	for (Cell *c = board->cells; c < board->cells+size; c++)
	{
		if (c->state != CELLSTATE_PIPE_START && c->state != CELLSTATE_PIPE_END)
			c->state = CELLSTATE_EMPTY;
	}

	free(pipes);
	board->isGenerated = true;
	printf("Generated!\n");

	u64 endTime = SDL_GetPerformanceCounter();
	printf("Time taken: %f\n", (endTime - startTime) / (f64)SDL_GetPerformanceFrequency());

	return placed;
}


















