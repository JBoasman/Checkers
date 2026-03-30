#ifndef CHECKERS_AI_H
#define CHECKERS_AI_H

#include "Board.h"

// Returns a random legal move for the current player
Board::Move randomMove(const Board& board);

// Returns the best move using minimax with alpha-beta pruning (depth 5)
Board::Move minimaxMove(Board board, Square aiPlayer);

#endif
