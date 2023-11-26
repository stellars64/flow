#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "board.h"


bool placeStart(Board *board, i32 *pipes, i32 numPipes, i32 currentPipe);


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


void boardSetColorAll(Board *board, CellColor color)
{
	i32 size = board->width * board->height;
	for (i32 i = 0; i < size; i++)
	{
		board->cells[i].color = color;
	}
}


void boardSetState(Board *board, i32 r, i32 c, CellState state)
{
	boardGet(board, r, c)->state = state;
}


void boardSetStateAll(Board *board, CellState state)
{
	i32 size = board->width * board->height;
	for (i32 i = 0; i < size; i++)
	{
		board->cells[i].state = state;
	}
}


Cell* boardGet(Board *board, i32 r, i32 c)
{
	return &board->cells[r * board->width + c];
}


Cell* boardGetI(Board *board, i32 index)
{
	return &board->cells[index];
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
	// if out of bounds, return
	if (row < 0 || row >= board->height || col < 0 || col >= board->width)
	{
		return;
	}

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

	// place temp pipe
	board->cells[row * board->width + col].state = CELLSTATE_PIPE_START;

	// find empty cell
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

	i32 emptyRow = emptyIndex / board->width;
	i32 emptyCol = emptyIndex % board->width;

	// trivially true
	if (!emptyFound)
	{
		boardGet(board, row, col)->state = prevState;
		return true;
	}

	// recursively fill the empty cells with CELLSTATE_EMPTY_MARKED
	boardDfsFillState(board, emptyRow, emptyCol, CELLSTATE_EMPTY, CELLSTATE_EMPTY_MARKED);

	// check if any non-marked empty cells exist, and while we are at it unmark the marked cells
	bool allEmptyVisited = true;
	for (i32 i = 0; i < size; i++)
	{
		if (board->cells[i].state == CELLSTATE_EMPTY)
		{
			allEmptyVisited = false;
		}
		else if (board->cells[i].state == CELLSTATE_EMPTY_MARKED)
		{
			board->cells[i].state = CELLSTATE_EMPTY;
		}
	}

	// reset temp pipe
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


bool placePipe(Board *board, i32 *pipes, i32 numPipes, i32 currentPipe, i32 currentSize, i32 row, i32 col)
{
	// order: U,D,L,R
	bool rejected[4] = {true, true, true, true};
	i32 numRejected = 4;
	Vec2i dirs[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

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
	{
		return false;
	}

	currentSize += 1;
	while (numRejected < 4)
	{

		// This section picks a random non-rejected direction, while avoiding
		// calling rand() over and over again
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

		// get row, column of the adjacent cell we are trying
		i32 adjRow = row + dirs[direction].y;
		i32 adjCol = col + dirs[direction].x;

		// for now, the pipe will be done -p[
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

		// attempt to place the pipe
		boardGet(board, adjRow, adjCol)->state = pipeState;
		boardGet(board, adjRow, adjCol)->color = currentPipe;

		// if we are finished with the current pipe
		if (currentSize == pipes[currentPipe])
		{
			// if we are finished with all pipes, we are done with recursion
			if (currentPipe == numPipes - 1)
			{
				return true;
			}
			// attempt to place the next pipe
			else
			{
				// return back up the call stack on success
				if (placeStart(board, pipes, numPipes, currentPipe + 1))
				{
					return true;
				}
				// could not place the next pipe, so we must backtrack
				else
				{
					rejected[direction] = true;
					numRejected += 1;
					boardGet(board, adjRow, adjCol)->state = CELLSTATE_EMPTY;
					boardGet(board, adjRow, adjCol)->color = 0;
				}
			}
		}
		// we need to place the next segment of the pipe
		else
		{
			// return back up the call stack on success
			if (placePipe(board, pipes, numPipes, currentPipe, currentSize, adjRow, adjCol))
			{
				// TODO
				// Here is where we would alter the pipe state to be a corner of some sort
				return true;
			}
			// could not place the next segment, so we must backtrack
			else
			{
				rejected[direction] = true;
				numRejected += 1;
				boardGet(board, adjRow, adjCol)->state = CELLSTATE_EMPTY;
				boardGet(board, adjRow, adjCol)->color = 0;
			}
		}
	}

	// all directions were rejected, so backtrack by returning false
	return false;
}

bool placeStart(Board *board, i32 *pipes, i32 numPipes, i32 currentPipe)
{
	i32 rejectedSize = board->width * board->height;
	bool *rejected = malloc(sizeof(bool) * rejectedSize);
	for (i32 i = 0; i < rejectedSize; i++)
	{
		rejected[i] = false;
	}

	while (true)
	{
		// get number of non-rejected empty cells
		i32 nonRejectedEmpty = 0;
		i32 boardSize = board->width * board->height;
		for (i32 i = 0; i < boardSize; i += 1)
		{
			if (boardGetI(board, i)->state == CELLSTATE_EMPTY && !rejected[i])
			{
				nonRejectedEmpty += 1;
			}
		}

		// no valid way to place start
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
			// pick a random non-rejected empty cell
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

			// Check to see if emptyConnected, if so then index is a good start
			if (boardIsEmptyConnected(board, startRow, startCol))
			{
				break;
			}
			// otherwise, mark as rejected. Check if we have any non-rejected empty cells left
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

		// place the start
		boardGetI(board, startIndex)->state = CELLSTATE_PIPE_START;
		boardGetI(board, startIndex)->color = currentPipe;

		bool placed = placePipe(board, pipes, numPipes, currentPipe, 1, startRow, startCol);
		if (placed)
		{
			free(rejected);
			return true;
		}
		else
		{
			// mark cell as rejected, clear it off board and try again
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
	{
		return false;
	}

	i32 size = board->width * board->height;
	i32 *pipes = malloc(sizeof(i32) * numPipes);

	for (i32 i = 0; i < numPipes; i++)
	{
		pipes[i] = 3; // 3 is the absolute minimum length
	}

	i32 totalPipeLength = numPipes * 3;

	while (totalPipeLength < size)
	{
		i32 index = rand() % numPipes;
		pipes[index] += 1;
		totalPipeLength += 1;
	}

	bool placed = placeStart(board, pipes, numPipes, 0);
	for (i32 i = 0; i < size; i++)
	{
		if (boardGetI(board, i)->state != CELLSTATE_PIPE_START
		    && boardGetI(board, i)->state != CELLSTATE_PIPE_END)
		{
			boardGetI(board, i)->state = CELLSTATE_EMPTY;
		}
	}

	free(pipes);
	board->isGenerated = true;
	printf("Generated!\n");
	u64 endTime = SDL_GetPerformanceCounter();
	printf("Time taken: %f\n", (endTime - startTime) / (f64)SDL_GetPerformanceFrequency());
	return placed;
}



















