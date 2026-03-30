#include "AI.h"
#include <cstdlib>
#include <ctime>
#include <limits>
#include <algorithm>

// ── Random AI ─────────────────────────────────────────────────────────────────

Board::Move randomMove(const Board& board) {
    auto moves = board.getLegalMoves();
    if (moves.empty()) return {-1, -1, -1, -1};
    static bool seeded = false;
    if (!seeded) { std::srand((unsigned)std::time(nullptr)); seeded = true; }
    return moves[std::rand() % moves.size()];
}

// ── Minimax evaluation ────────────────────────────────────────────────────────

// Piece values: normal = 1, king = 2
// Bonus for kings and advancement
static int evaluate(const Board& board, Square aiPlayer) {
    Square humanPlayer = (aiPlayer == Square::Red) ? Square::Black : Square::Red;
    int score = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            Square s = board.getSquareState(x, y);
            if (s == Square::Empty) continue;

            int val = 0;
            if (s == Square::Red || s == Square::Black) {
                val = 100;
                // Advancement bonus (closer to promotion = more valuable)
                int advancement = (s == Square::Red) ? (7 - y) : y;
                val += advancement * 5;
            } else {
                // King
                val = 200;
                // Kings prefer centre
                int cx = std::abs(x - 3), cy = std::abs(y - 3);
                val += (6 - cx - cy) * 3;
            }

            bool isAI = (aiPlayer == Square::Red)
                ? (s == Square::Red || s == Square::RedKing)
                : (s == Square::Black || s == Square::BlackKing);

            score += isAI ? val : -val;
        }
    }
    return score;
}

// Apply a move to a copy of the board and return it
static Board applyMove(Board board, const Board::Move& m) {
    board.movePiece(m.sx, m.sy, m.ex, m.ey);
    return board;
}

// Minimax with alpha-beta pruning
// maximising = true when it's the AI's turn
static int minimax(Board board, int depth, int alpha, int beta,
                   bool maximising, Square aiPlayer) {
    GameResult result = board.checkResult();
    if (result != GameResult::None) {
        if (result == GameResult::Draw) return 0;
        bool aiWins = (aiPlayer == Square::Red   && result == GameResult::RedWins) ||
                      (aiPlayer == Square::Black && result == GameResult::BlackWins);
        return aiWins ? 100000 + depth : -100000 - depth;
    }
    if (depth == 0) return evaluate(board, aiPlayer);

    auto moves = board.getLegalMoves();
    if (moves.empty()) return evaluate(board, aiPlayer);

    if (maximising) {
        int best = std::numeric_limits<int>::min();
        for (auto& m : moves) {
            Board next = applyMove(board, m);
            // If still same turn (multi-jump in progress), stay maximising
            bool stillAI = (next.getCurrentTurn() == aiPlayer);
            int val = minimax(next, depth - 1, alpha, beta, stillAI, aiPlayer);
            best = std::max(best, val);
            alpha = std::max(alpha, best);
            if (beta <= alpha) break; // prune
        }
        return best;
    } else {
        int best = std::numeric_limits<int>::max();
        for (auto& m : moves) {
            Board next = applyMove(board, m);
            bool stillHuman = (next.getCurrentTurn() != aiPlayer);
            int val = minimax(next, depth - 1, alpha, beta, !stillHuman, aiPlayer);
            best = std::min(best, val);
            beta = std::min(beta, best);
            if (beta <= alpha) break; // prune
        }
        return best;
    }
}

// ── Minimax AI entry point ────────────────────────────────────────────────────

Board::Move minimaxMove(Board board, Square aiPlayer) {
    auto moves = board.getLegalMoves();
    if (moves.empty()) return {-1, -1, -1, -1};

    Board::Move bestMove = moves[0];
    int bestVal = std::numeric_limits<int>::min();
    int alpha = std::numeric_limits<int>::min();
    int beta  = std::numeric_limits<int>::max();

    for (auto& m : moves) {
        Board next = applyMove(board, m);
        bool stillAI = (next.getCurrentTurn() == aiPlayer);
        int val = minimax(next, 5, alpha, beta, stillAI, aiPlayer);
        if (val > bestVal) {
            bestVal = val;
            bestMove = m;
        }
        alpha = std::max(alpha, bestVal);
    }
    return bestMove;
}
