#ifndef CHECKERS_BOARD_H
#define CHECKERS_BOARD_H

#include <vector>
#include <map>
#include <string>

enum class Square { Empty, Red, Black, RedKing, BlackKing };

enum class GameResult { None, RedWins, BlackWins, Draw };

class Board {
private:
    Square grid[8][8];
    Square currentTurn;

    // Scores
    int redWins   = 0;
    int blackWins = 0;
    int draws     = 0;

    // Draw tracking
    int  quietMoves = 0;
    std::map<std::string, int> positionHistory;

    bool multiJumpInProgress = false;
    int  lastX = -1, lastY = -1;
    bool justCaptured = false;

    // Graveyard
    std::vector<Square> redCaptured;
    std::vector<Square> blackCaptured;

    // Last move highlight
    int lastMoveStartX = -1, lastMoveStartY = -1;
    int lastMoveEndX   = -1, lastMoveEndY   = -1;

    std::string positionKey() const;

public:
    Board();
    void resetBoard();
    void resetScores();
    void forfeit();

    Square getSquareState(int x, int y) const;
    Square getCurrentTurn() const { return currentTurn; }

    int getRedWins()    const { return redWins;    }
    int getBlackWins()  const { return blackWins;  }
    int getDraws()      const { return draws;      }
    int getQuietMoves() const { return quietMoves; }

    bool isMultiJump()  const { return multiJumpInProgress; }
    int  getRequiredX() const { return lastX; }
    int  getRequiredY() const { return lastY; }
    bool didCapture()   const { return justCaptured; }

    const std::vector<Square>& getRedCaptured()   const { return redCaptured;   }
    const std::vector<Square>& getBlackCaptured() const { return blackCaptured; }

    int getLastMoveStartX() const { return lastMoveStartX; }
    int getLastMoveStartY() const { return lastMoveStartY; }
    int getLastMoveEndX()   const { return lastMoveEndX;   }
    int getLastMoveEndY()   const { return lastMoveEndY;   }

    GameResult checkResult() const;

    bool legalMove(int startX, int startY, int endX, int endY) const;
    bool canPieceJump(int x, int y) const;
    bool hasAnyJump(Square player) const;
    bool hasAnyMove(Square player) const;
    bool isStalemate() const;
    void movePiece(int startX, int startY, int endX, int endY);
    void switchTurn();
    void recordResult(GameResult r);

    struct Move { int sx, sy, ex, ey; };
    std::vector<Move> getLegalMoves() const;
};

#endif
