#include "ChessGame.h"
#include "ChessSprites.h"
#include "GameRegistry.h"
#include <math.h>

// ═════════════════════════════════════════════════════════════════════════════
// ChessTitleScene
// ═════════════════════════════════════════════════════════════════════════════

void ChessTitleScene::onEnter(Console& ctx) {
}

void ChessTitleScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::A)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // Custom event to transition to play
    }
    if (ctx.justPressed(Btn::B)) {
        sm.emit(ctx, Event::QUIT); // Exit game
    }
}

void ChessTitleScene::draw(Console& ctx) {
    ctx.setFont(u8g2_font_6x10_tf);
    ctx.drawStrCentered(20, "CHESS");
    ctx.drawStrCentered(40, "Press A to Start");
}

// ═════════════════════════════════════════════════════════════════════════════
// ChessPlayScene
// ═════════════════════════════════════════════════════════════════════════════

void ChessPlayScene::initBoard() {
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            _board[y][x].type = PieceType::None;
        }
    }
    
    // Pawns
    for (int x = 0; x < 8; ++x) {
        _board[1][x] = {PieceType::Pawn, PieceColor::Black};
        _board[6][x] = {PieceType::Pawn, PieceColor::White};
    }
    
    PieceType backRow[8] = {PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen, 
                            PieceType::King, PieceType::Bishop, PieceType::Knight, PieceType::Rook};
                            
    for (int x = 0; x < 8; ++x) {
        _board[0][x] = {backRow[x], PieceColor::Black};
        _board[7][x] = {backRow[x], PieceColor::White};
    }
    
    _turn = PieceColor::White;
    _sx = -1;
    _sy = -1;
    _cx = 3;
    _cy = 6;
}

void ChessPlayScene::onEnter(Console& ctx) {
    initBoard();
}

bool ChessPlayScene::isPathClear(int fx, int fy, int tx, int ty) {
    int dx = (tx > fx) ? 1 : ((tx < fx) ? -1 : 0);
    int dy = (ty > fy) ? 1 : ((ty < fy) ? -1 : 0);
    
    int x = fx + dx;
    int y = fy + dy;
    
    while (x != tx || y != ty) {
        if (!_board[y][x].isEmpty()) return false;
        x += dx;
        y += dy;
    }
    return true;
}

bool ChessPlayScene::isPseudoLegalMove(int fx, int fy, int tx, int ty) {
    if (fx == tx && fy == ty) return false; // same square
    
    Piece p = _board[fy][fx];
    Piece target = _board[ty][tx];
    
    if (!target.isEmpty() && target.color == p.color) return false; // can't capture own
    
    int dx = tx - fx;
    int dy = ty - fy;
    int adx = abs(dx);
    int ady = abs(dy);
    
    switch (p.type) {
        case PieceType::Pawn: {
            int dir = (p.color == PieceColor::White) ? -1 : 1;
            int startY = (p.color == PieceColor::White) ? 6 : 1;
            
            // Move forward
            if (dx == 0) {
                if (dy == dir && target.isEmpty()) return true;
                if (dy == 2 * dir && fy == startY && target.isEmpty() && _board[fy + dir][fx].isEmpty()) return true;
            }
            // Capture
            else if (adx == 1 && dy == dir && !target.isEmpty()) {
                return true; // regular capture
            }
            return false;
        }
        case PieceType::Knight:
            return (adx == 1 && ady == 2) || (adx == 2 && ady == 1);
        case PieceType::Bishop:
            if (adx != ady) return false;
            return isPathClear(fx, fy, tx, ty);
        case PieceType::Rook:
            if (adx != 0 && ady != 0) return false;
            return isPathClear(fx, fy, tx, ty);
        case PieceType::Queen:
            if (adx != ady && adx != 0 && ady != 0) return false;
            return isPathClear(fx, fy, tx, ty);
        case PieceType::King:
            return adx <= 1 && ady <= 1;
        default: return false;
    }
}

void ChessPlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::UP) && _cy > 0) _cy--;
    if (ctx.justPressed(Btn::DOWN) && _cy < 7) _cy++;
    if (ctx.justPressed(Btn::LEFT) && _cx > 0) _cx--;
    if (ctx.justPressed(Btn::RIGHT) && _cx < 7) _cx++;
    
    if (ctx.justPressed(Btn::UP) || ctx.justPressed(Btn::DOWN) || ctx.justPressed(Btn::LEFT) || ctx.justPressed(Btn::RIGHT)) {
        ctx.sfxMenuNav();
    }
    
    if (ctx.justPressed(Btn::B)) {
        if (_sx != -1) {
            _sx = -1; // Deselect
            _sy = -1;
            ctx.sfxMenuBack();
        } else {
            sm.emit(ctx, Event::QUIT);
        }
    }
    
    if (ctx.justPressed(Btn::A)) {
        if (_sx == -1) {
            // Select
            if (!_board[_cy][_cx].isEmpty() && _board[_cy][_cx].color == _turn) {
                _sx = _cx;
                _sy = _cy;
                ctx.sfxMenuEnter();
            }
        } else {
            // Move
            if (isPseudoLegalMove(_sx, _sy, _cx, _cy)) {
                _board[_cy][_cx] = _board[_sy][_sx];
                _board[_sy][_sx] = {PieceType::None, PieceColor::White};
                _sx = -1;
                _sy = -1;
                _turn = (_turn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
                ctx.sfxPoint(); // move sound
            } else {
                ctx.sfxDeath(); // error sound
            }
        }
    }
}

void ChessPlayScene::drawPiece(Console& ctx, const Piece& p, int x, int y) {
    const uint8_t* sprite = nullptr;
    switch (p.type) {
        case PieceType::Pawn:   sprite = ChessSprites::pawn; break;
        case PieceType::Knight: sprite = ChessSprites::knight; break;
        case PieceType::Bishop: sprite = ChessSprites::bishop; break;
        case PieceType::Rook:   sprite = ChessSprites::rook; break;
        case PieceType::Queen:  sprite = ChessSprites::queen; break;
        case PieceType::King:   sprite = ChessSprites::king; break;
        default: return;
    }
    
    if (p.color == PieceColor::White) {
        // Draw normal
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawBitmap(x, y, 1, 8, sprite);
    } else {
        // Draw inverted? The background might be dark or light.
        // Actually, just drawing it with XOR or making a 8x8 filled box first
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBitmap(x, y, 1, 8, sprite);
        // We will need to make sure black pieces can be seen on dark squares.
        // Let's rely on XOR trick or just draw a white background for black pieces.
        // Wait, drawing it black on a black square won't be seen.
        // What if we draw a 1-pixel white outline? Our sprites don't have that.
        // Instead, let's use the DitherBox or just invert the sprite drawing.
    }
}

void ChessPlayScene::draw(Console& ctx) {
    int boardX = 0;
    int boardY = 0;
    int tileSize = 8;
    
    // Draw Board
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            int px = boardX + x * tileSize;
            int py = boardY + y * tileSize;
            
            bool isDark = (x + y) % 2 != 0;
            if (isDark) {
                ctx.setDrawColor(Console::COLOR_WHITE); // or use DitherBox
                ctx.drawBox(px, py, tileSize, tileSize); // wait, if dark square is white, then piece drawing must account for it
                ctx.drawDitherBox(px, py, tileSize, tileSize, 1); // 25% grey for dark squares
            }
            
            if (!_board[y][x].isEmpty()) {
                Piece p = _board[y][x];
                
                // Draw piece background so it's always visible
                if (p.color == PieceColor::Black) {
                    ctx.setDrawColor(Console::COLOR_WHITE);
                    // ctx.drawBox(px+1, py+1, 6, 6);
                } else {
                    ctx.setDrawColor(Console::COLOR_BLACK);
                    // ctx.drawBox(px+1, py+1, 6, 6);
                }
                
                ctx.pushDrawState();
                if (p.color == PieceColor::White) {
                    ctx.setDrawColor(Console::COLOR_WHITE);
                    ctx.drawBitmap(px, py, 1, 8, (p.type == PieceType::Pawn) ? ChessSprites::pawn :
                                                 (p.type == PieceType::Knight) ? ChessSprites::knight :
                                                 (p.type == PieceType::Bishop) ? ChessSprites::bishop :
                                                 (p.type == PieceType::Rook) ? ChessSprites::rook :
                                                 (p.type == PieceType::Queen) ? ChessSprites::queen : ChessSprites::king);
                } else {
                    // Draw black piece by XORing a solid white square and drawing the piece?
                    // Actually U8g2 drawBitmap draws '1' bits in the current draw color.
                    // If we want black piece, we set color to XOR, draw a solid box, then draw the bitmap?
                    // Let's just draw white bits using COLOR_XOR, which might invert the background.
                    ctx.setDrawColor(Console::COLOR_XOR);
                    ctx.drawBitmap(px, py, 1, 8, (p.type == PieceType::Pawn) ? ChessSprites::pawn :
                                                 (p.type == PieceType::Knight) ? ChessSprites::knight :
                                                 (p.type == PieceType::Bishop) ? ChessSprites::bishop :
                                                 (p.type == PieceType::Rook) ? ChessSprites::rook :
                                                 (p.type == PieceType::Queen) ? ChessSprites::queen : ChessSprites::king);
                }
                ctx.popDrawState();
            }
        }
    }
    
    // Draw selected highlights
    if (_sx != -1) {
        ctx.setDrawColor(Console::COLOR_XOR);
        ctx.drawFrame(boardX + _sx * tileSize, boardY + _sy * tileSize, tileSize, tileSize);
    }
    
    // Draw cursor
    ctx.setDrawColor(Console::COLOR_XOR);
    ctx.drawFrame(boardX + _cx * tileSize, boardY + _cy * tileSize, tileSize, tileSize);
    ctx.drawFrame(boardX + _cx * tileSize + 1, boardY + _cy * tileSize + 1, tileSize - 2, tileSize - 2);

    // Sidebar
    ctx.setDrawColor(Console::COLOR_WHITE);
    ctx.setFont(u8g2_font_4x6_tf);
    ctx.drawStr(66, 10, "Turn:");
    ctx.drawStr(66, 20, _turn == PieceColor::White ? "White" : "Black");
}

// ═════════════════════════════════════════════════════════════════════════════
// ChessGame Boilerplate
// ═════════════════════════════════════════════════════════════════════════════

void ChessGame::onEnter(Console& ctx) {
    _data.hiScore = ctx.loadHiScore();

    _title.setShared(&_data);
    _play.setShared(&_data);

    useDefaultEvents(nullptr, nullptr); 
    _sm.onEvent(Event::CUSTOM_1, SceneManager::REPLACE, &_play);

    _sm.replace(&_title, ctx);
}

const char* ChessGame::getName() const {
    return "Chess";
}

REGISTER_GAME(ChessGame);
