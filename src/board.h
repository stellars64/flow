#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include <stdbool.h>
#include "common.h"

typedef enum
{
	DIRECTION_UP = 0,
	DIRECTION_DOWN = 1,
	DIRECTION_LEFT = 2,
	DIRECTION_RIGHT = 3
} Direction;

typedef enum
{
	CELLCOLOR_RED,
	CELLCOLOR_GREEN,
	CELLCOLOR_BLUE,
	CELLCOLOR_YELLOW,
	CELLCOLOR_PURPLE,
	CELLCOLOR_CYAN,
	CELLCOLOR_WHITE,
	CELLCOLOR_LIGHT_GRAY,
	CELLCOLOR_MEDIUM_GRAY,
	CELLCOLOR_DARK_GRAY,
	CELLCOLOR_MAGENTA,
	CELLCOLOR_PINK,
	CELLCOLOR_DARK_RED,
	CELLCOLOR_DARK_GREEN,
	CELLCOLOR_DARK_BLUE,
	CELLCOLOR_ULTRADARK_RED,
	CELLCOLOR_ULTRADARK_GREEN,
	CELLCOLOR_ULTRADARK_BLUE,
	CELLCOLOR_ULTRADARK_YELLOW,
	CELLCOLOR_ULTRADARK_PURPLE,
	CELLCOLOR_ULTRADARK_CYAN,
	CELLCOLOR_FUCHSIA,
	CELLCOLOR_COUNT
} CellColor;

typedef enum
{
	CELLSTATE_EMPTY,
	CELLSTATE_EMPTY_MARKED,
	CELLSTATE_PIPE_START,
	CELLSTATE_PIPE_END,
	CELLSTATE_PIPE,
	CELLSTATE_COUNT
} CellState;

typedef enum
{
	CELLCONNECTION_UP,
	CELLCONNECTION_DOWN,
	CELLCONNECTION_LEFT,
	CELLCONNECTION_RIGHT,
	CELLCONNECTION_NONE,
	CELLCONNECTION_COUNT
} CellConnection;

typedef enum
{
	GENSTATE_IDLE,
	GENSTATE_GENERATING,
	GENSTATE_PAUSE,
	GENSTATE_STOPREQUESTED,
	GENSTATE_STOPPING
} GenState;

typedef struct
{
	CellState state;
	CellColor color;
	CellConnection connection;
} Cell;

typedef struct
{
	i32 x;
	i32 y;
} Vec2i;


typedef struct
{
	Cell *cells;
	i32 width;
	i32 height;
	GenState genState;
} Board;

Board* boardCreate(i32 width, i32 height);

void boardFree(Board *board);

void boardSetColor(Board *board, i32 r, i32 c, CellColor color);

void boardSetColorAll(Board *board, CellColor color);

void boardSetState(Board *board, i32 r, i32 c, CellState state);

void boardSetStateAll(Board *board, CellState state);

Cell* boardGet(Board *board, i32 r, i32 c);

Cell* boardGetI(Board *board, i32 index);

bool boardBoundsCheck(Board *board, i32 r, i32 c);

void boardPrint(Board *board);

bool boardGenerate(Board *board);

bool boardEmptyPathExists(Board *board, i32 r1, i32 c1, i32 r2, i32 c2);

#endif
