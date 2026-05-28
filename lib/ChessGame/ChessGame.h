#pragma once
#include "SceneGame.h"
#include <stdint.h>

enum class PieceType : uint8_t { None, Pawn, Knight, Bishop, Rook, Queen, King };
enum class PieceColor : uint8_t { White, Black };

struct Piece {
    PieceType type = PieceType::None;
    PieceColor color = PieceColor::White;
    
    bool isEmpty() const { return type == PieceType::None; }
};

struct ChessData {
    uint32_t hiScore = 0; // Maybe tracked wins?
};

class ChessTitleScene : public Scene {
public:
    void setShared(ChessData* data) { _data = data; }
    void onEnter(Console& ctx) override;
    void update(Console& ctx, SceneManager& sm, float dt) override;
    void draw(Console& ctx) override;
private:
    ChessData* _data = nullptr;
};

class ChessPlayScene : public Scene {
public:
    void setShared(ChessData* data) { _data = data; }
    
    void onEnter(Console& ctx) override;
    void update(Console& ctx, SceneManager& sm, float dt) override;
    void draw(Console& ctx) override;
    
private:
    void initBoard();
    bool isPseudoLegalMove(int fx, int fy, int tx, int ty);
    bool isPathClear(int fx, int fy, int tx, int ty);
    void drawPiece(Console& ctx, const Piece& p, int x, int y);
    void doAIMove(Console& ctx);
    int evaluateBoard();
    int minimax(int depth, PieceColor turn, int alpha, int beta);

    ChessData* _data = nullptr;
    
    Piece _board[8][8];
    int _cx = 3; // cursor X
    int _cy = 3; // cursor Y
    int _sx = -1; // selected X
    int _sy = -1; // selected Y
    
    PieceColor _turn = PieceColor::White;
    
    bool _isPvE = true;
    float _aiTimer = 0.0f;
    
    bool _gameOver = false;
    PieceColor _winner = PieceColor::White;
};

class ChessGame : public SceneGame<ChessData> {
public:
    void onEnter(Console& ctx) override;
    void onExit(Console& ctx) override;
    const char* getName() const override;

private:
    ChessTitleScene _title;
    ChessPlayScene  _play;
};
