#include "Board.h"
#include <cmath>
#include <sstream>

Board::Board() {
    redWins = blackWins = draws = 0;
    resetBoard();
}

// ── Position key for 3-fold repetition ───────────────────────────────────────
std::string Board::positionKey() const {
    std::ostringstream ss;
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            ss << (int)grid[x][y];
    ss << (int)currentTurn;
    return ss.str();
}

void Board::resetBoard() {
    currentTurn = Square::Red;
    multiJumpInProgress = false;
    justCaptured = false;
    quietMoves = 0;
    positionHistory.clear();
    redCaptured.clear();
    blackCaptured.clear();
    lastMoveStartX = lastMoveStartY = lastMoveEndX = lastMoveEndY = -1;
    lastX = lastY = -1;
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            grid[x][y] = Square::Empty;
            if ((x + y) % 2 != 0) {
                if (y < 3) grid[x][y] = Square::Black;
                if (y > 4) grid[x][y] = Square::Red;
            }
        }
    // Record starting position
    positionHistory[positionKey()]++;
}

void Board::resetScores() {
    redWins = blackWins = draws = 0;
}

void Board::recordResult(GameResult r) {
    if (r == GameResult::RedWins)   redWins++;
    if (r == GameResult::BlackWins) blackWins++;
    if (r == GameResult::Draw)      draws++;
}

void Board::forfeit() {
    if (currentTurn == Square::Red) blackWins++; else redWins++;
    resetBoard();
}

Square Board::getSquareState(int x, int y) const {
    if (x >= 0 && x < 8 && y >= 0 && y < 8) return grid[x][y];
    return Square::Empty;
}

// ── Result check ─────────────────────────────────────────────────────────────
GameResult Board::checkResult() const {
    int redCount = 0, blackCount = 0;
    for (int x = 0; x < 8; x++)
        for (int y = 0; y < 8; y++) {
            if (grid[x][y] == Square::Red   || grid[x][y] == Square::RedKing)   redCount++;
            if (grid[x][y] == Square::Black || grid[x][y] == Square::BlackKing) blackCount++;
        }
    if (blackCount == 0) return GameResult::RedWins;
    if (redCount   == 0) return GameResult::BlackWins;

    // Stalemate: current player has no moves → opponent wins
    if (!hasAnyMove(currentTurn))
        return (currentTurn == Square::Red) ? GameResult::BlackWins : GameResult::RedWins;

    // 40-move rule (no capture or promotion)
    if (quietMoves >= 40) return GameResult::Draw;

    // 3-fold repetition
    auto it = positionHistory.find(positionKey());
    if (it != positionHistory.end() && it->second >= 3) return GameResult::Draw;

    return GameResult::None;
}

bool Board::isStalemate() const {
    return !hasAnyMove(currentTurn);
}

bool Board::legalMove(int startX, int startY, int endX, int endY) const {
    if (endX < 0 || endX > 7 || endY < 0 || endY > 7) return false;
    if (grid[endX][endY] != Square::Empty) return false;

    Square s = grid[startX][startY];
    int dx = endX - startX, dy = endY - startY;
    if (abs(dx) != abs(dy)) return false;

    if (s == Square::Red   && dy >= 0) return false;
    if (s == Square::Black && dy <= 0) return false;

    if (abs(dx) == 1) return true;
    if (abs(dx) == 2) {
        Square mid = grid[startX + dx/2][startY + dy/2];
        if (s == Square::Red   || s == Square::RedKing)
            return (mid == Square::Black || mid == Square::BlackKing);
        if (s == Square::Black || s == Square::BlackKing)
            return (mid == Square::Red   || mid == Square::RedKing);
    }
    return false;
}

bool Board::canPieceJump(int x, int y) const {
    int dx[] = {2,2,-2,-2}, dy[] = {2,-2,2,-2};
    for (int i = 0; i < 4; i++)
        if (legalMove(x, y, x+dx[i], y+dy[i])) return true;
    return false;
}

bool Board::hasAnyJump(Square player) const {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            Square s = grid[x][y];
            if ((player == Square::Red   && (s == Square::Red   || s == Square::RedKing)) ||
                (player == Square::Black && (s == Square::Black || s == Square::BlackKing)))
                if (canPieceJump(x, y)) return true;
        }
    return false;
}

bool Board::hasAnyMove(Square player) const {
    int dx[] = {1,1,-1,-1,2,2,-2,-2};
    int dy[] = {1,-1,1,-1,2,-2,2,-2};
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            Square s = grid[x][y];
            if ((player == Square::Red   && (s == Square::Red   || s == Square::RedKing)) ||
                (player == Square::Black && (s == Square::Black || s == Square::BlackKing)))
                for (int i = 0; i < 8; i++)
                    if (legalMove(x, y, x+dx[i], y+dy[i])) return true;
        }
    return false;
}

void Board::movePiece(int startX, int startY, int endX, int endY) {
    if (!legalMove(startX, startY, endX, endY)) return;
    int dx = abs(endX - startX);
    if (dx == 1 && hasAnyJump(currentTurn)) return;

    justCaptured = false;
    lastMoveStartX = startX; lastMoveStartY = startY;
    lastMoveEndX   = endX;   lastMoveEndY   = endY;

    Square movingPiece = grid[startX][startY];
    grid[endX][endY]   = movingPiece;
    grid[startX][startY] = Square::Empty;

    bool isPromotion = false;
    if (dx == 2) {
        int midX = startX + (endX-startX)/2;
        int midY = startY + (endY-startY)/2;
        Square captured = grid[midX][midY];
        if (currentTurn == Square::Red) redCaptured.push_back(captured);
        else                            blackCaptured.push_back(captured);
        grid[midX][midY] = Square::Empty;
        justCaptured = true;

        // Promote mid-jump
        if (grid[endX][endY] == Square::Red   && endY == 0) { grid[endX][endY] = Square::RedKing;   isPromotion = true; }
        if (grid[endX][endY] == Square::Black && endY == 7) { grid[endX][endY] = Square::BlackKing; isPromotion = true; }

        if (canPieceJump(endX, endY)) {
            multiJumpInProgress = true;
            lastX = endX; lastY = endY;
            // reset quiet counter on capture
            quietMoves = 0;
            positionHistory[positionKey()]++;
            return;
        }
    }

    // Promote at end of turn
    if (grid[endX][endY] == Square::Red   && endY == 0) { grid[endX][endY] = Square::RedKing;   isPromotion = true; }
    if (grid[endX][endY] == Square::Black && endY == 7) { grid[endX][endY] = Square::BlackKing; isPromotion = true; }

    // Update quiet move counter
    if (justCaptured || isPromotion) quietMoves = 0;
    else                              quietMoves++;

    multiJumpInProgress = false;
    switchTurn();
    positionHistory[positionKey()]++;
}

void Board::switchTurn() {
    currentTurn = (currentTurn == Square::Red) ? Square::Black : Square::Red;
}

std::vector<Board::Move> Board::getLegalMoves() const {
    std::vector<Move> moves;
    int ddx[] = {1,1,-1,-1,2,2,-2,-2};
    int ddy[] = {1,-1,1,-1,2,-2,2,-2};
    bool mustJump = hasAnyJump(currentTurn);

    if (multiJumpInProgress) {
        for (int i = 4; i < 8; i++) {
            int ex = lastX + ddx[i], ey = lastY + ddy[i];
            if (legalMove(lastX, lastY, ex, ey))
                moves.push_back({lastX, lastY, ex, ey});
        }
        return moves;
    }

    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            Square s = grid[x][y];
            bool mine = (currentTurn == Square::Red)
                ? (s == Square::Red   || s == Square::RedKing)
                : (s == Square::Black || s == Square::BlackKing);
            if (!mine) continue;
            int start = mustJump ? 4 : 0;
            for (int i = start; i < 8; i++) {
                int ex = x+ddx[i], ey = y+ddy[i];
                if (legalMove(x, y, ex, ey))
                    moves.push_back({x, y, ex, ey});
            }
        }
    return moves;
}
