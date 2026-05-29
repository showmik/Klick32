#include "ChessGame.h"
#include "ChessSprites.h"
#include "GameRegistry.h"
#include <math.h>
#include <stdlib.h>

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


}

void ChessTitleScene::draw(Console& ctx) {
    ctx.setDrawColor(Console::COLOR_WHITE);
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

int ChessPlayScene::evaluateBoard() {
    int score = 0;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            if (_board[y][x].isEmpty()) continue;
            
            Piece p = _board[y][x];
            int val = 0;
            switch(p.type) {
                case PieceType::Pawn: val = 10; break;
                case PieceType::Knight: val = 30; break;
                case PieceType::Bishop: val = 30; break;
                case PieceType::Rook: val = 50; break;
                case PieceType::Queen: val = 90; break;
                case PieceType::King: val = 900; break;
                default: break;
            }
            
            // Positional bonus: encourage center control
            int centerDist = abs(x - 3) + abs(y - 3); 
            val -= centerDist; 
            
            if (p.type == PieceType::Knight) {
                val -= centerDist * 2; // Knights need the center
            } 
            
            if (p.color == PieceColor::Black) {
                val += y; // encourage moving forward
                score += val;
            } else {
                val += (7 - y);
                score -= val;
            }
        }
    }
    return score;
}

int ChessPlayScene::minimax(int depth, PieceColor turn, int alpha, int beta) {
    if (depth == 0) return evaluateBoard();
    
    int bestScore = (turn == PieceColor::Black) ? -999999 : 999999;
    
    for (int fy = 0; fy < 8; ++fy) {
        for (int fx = 0; fx < 8; ++fx) {
            if (_board[fy][fx].color != turn || _board[fy][fx].isEmpty()) continue;
            
            for (int ty = 0; ty < 8; ++ty) {
                for (int tx = 0; tx < 8; ++tx) {
                    if (isPseudoLegalMove(fx, fy, tx, ty)) {
                        Piece captured = _board[ty][tx];
                        Piece moved = _board[fy][fx];
                        
                        if (captured.type == PieceType::King) {
                            return (turn == PieceColor::Black) ? 900000 : -900000;
                        }
                        
                        _board[ty][tx] = moved;
                        _board[fy][fx] = {PieceType::None, PieceColor::White};
                        
                        if (moved.type == PieceType::Pawn) {
                            if (turn == PieceColor::Black && ty == 7) _board[ty][tx].type = PieceType::Queen;
                            if (turn == PieceColor::White && ty == 0) _board[ty][tx].type = PieceType::Queen;
                        }
                        
                        int score = minimax(depth - 1, (turn == PieceColor::Black) ? PieceColor::White : PieceColor::Black, alpha, beta);
                        
                        _board[fy][fx] = moved;
                        _board[ty][tx] = captured;
                        
                        if (turn == PieceColor::Black) {
                            if (score > bestScore) bestScore = score;
                            if (bestScore > alpha) alpha = bestScore;
                            if (beta <= alpha) return bestScore;
                        } else {
                            if (score < bestScore) bestScore = score;
                            if (bestScore < beta) beta = bestScore;
                            if (beta <= alpha) return bestScore;
                        }
                    }
                }
            }
        }
    }
    return bestScore;
}

void ChessPlayScene::doAIMove(Console& ctx) {
    int bestScore = -999999;
    int bestFx = -1, bestFy = -1, bestTx = -1, bestTy = -1;
    
    // 2-ply Minimax search
    for (int fy = 0; fy < 8; ++fy) {
        for (int fx = 0; fx < 8; ++fx) {
            if (_board[fy][fx].color != PieceColor::Black || _board[fy][fx].isEmpty()) continue;
            
            for (int ty = 0; ty < 8; ++ty) {
                for (int tx = 0; tx < 8; ++tx) {
                    if (isPseudoLegalMove(fx, fy, tx, ty)) {
                        Piece captured = _board[ty][tx];
                        Piece moved = _board[fy][fx];
                        
                        _board[ty][tx] = moved;
                        _board[fy][fx] = {PieceType::None, PieceColor::White};
                        
                        if (moved.type == PieceType::Pawn && ty == 7) {
                            _board[ty][tx].type = PieceType::Queen;
                        }
                        
                        int score;
                        if (captured.type == PieceType::King) {
                            score = 900000;
                        } else {
                            score = minimax(2, PieceColor::White, -999999, 999999);
                        }
                        
                        _board[fy][fx] = moved;
                        _board[ty][tx] = captured;
                        
                        score += (rand() % 3); // tiny noise for variety
                        
                        if (score > bestScore) {
                            bestScore = score;
                            bestFx = fx;
                            bestFy = fy;
                            bestTx = tx;
                            bestTy = ty;
                        }
                    }
                }
            }
        }
    }
    
    if (bestFx != -1) {
        Piece captured = _board[bestTy][bestTx];
        _board[bestTy][bestTx] = _board[bestFy][bestFx];
        _board[bestFy][bestFx] = {PieceType::None, PieceColor::White};
        
        // Pawn promotion
        if (_board[bestTy][bestTx].type == PieceType::Pawn && bestTy == 7) {
            _board[bestTy][bestTx].type = PieceType::Queen;
        }
        
        ctx.sfxPoint();
        
        // Game Over check
        if (captured.type == PieceType::King) {
            _gameOver = true;
            _winner = PieceColor::Black;
        }
    }
    
    _turn = PieceColor::White;
    _aiTimer = 0;
}

void ChessPlayScene::update(Console& ctx, SceneManager& sm, float dt) {


    if (_gameOver) {
        if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::B)) {
            initBoard();
            _gameOver = false;
        }
        return;
    }

    if (_isPvE && _turn == PieceColor::Black) {
        _aiTimer += dt;
        if (_aiTimer > 1.0f) { // 1 second delay
            doAIMove(ctx);
        }
        return; // Block input
    }

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
            ctx.sfxMenuBack();
            sm.emit(ctx, Event::CUSTOM_2); // Return to title scene instead of quitting
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
                Piece captured = _board[_cy][_cx];
                _board[_cy][_cx] = _board[_sy][_sx];
                _board[_sy][_sx] = {PieceType::None, PieceColor::White};
                
                // Pawn promotion
                if (_board[_cy][_cx].type == PieceType::Pawn && _cy == 0) {
                    _board[_cy][_cx].type = PieceType::Queen;
                }
                
                // Game over
                if (captured.type == PieceType::King) {
                    _gameOver = true;
                    _winner = PieceColor::White;
                }
                
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
    
    // Draw outline
    ctx.setDrawColor(p.color == PieceColor::White ? Console::COLOR_BLACK : Console::COLOR_WHITE);
    ctx.drawBitmap(x - 1, y, 1, 8, sprite);
    ctx.drawBitmap(x + 1, y, 1, 8, sprite);
    ctx.drawBitmap(x, y - 1, 1, 8, sprite);
    ctx.drawBitmap(x, y + 1, 1, 8, sprite);
    
    // Draw piece interior
    ctx.setDrawColor(p.color == PieceColor::White ? Console::COLOR_WHITE : Console::COLOR_BLACK);
    ctx.drawBitmap(x, y, 1, 8, sprite);
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
                ctx.setDrawColor(Console::COLOR_WHITE);
                ctx.drawDitherBox(px, py, tileSize, tileSize, 2); // 50% grey for dark squares
            }
            
            if (!_board[y][x].isEmpty()) {
                ctx.pushDrawState();
                drawPiece(ctx, _board[y][x], px, py);
                ctx.popDrawState();
            }
        }
    }
    
    // Draw selected highlights and valid moves
    if (_sx != -1) {
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawFrame(boardX + _sx * tileSize, boardY + _sy * tileSize, tileSize, tileSize);
        
        // Draw valid moves
        for (int ty = 0; ty < 8; ++ty) {
            for (int tx = 0; tx < 8; ++tx) {
                if (isPseudoLegalMove(_sx, _sy, tx, ty)) {
                    ctx.setDrawColor(Console::COLOR_WHITE);
                    // Draw a 2x2 dot in the center of the tile
                    ctx.drawBox(boardX + tx * tileSize + 3, boardY + ty * tileSize + 3, 2, 2);
                }
            }
        }
    }
    
    // Draw cursor
    // White outer box, black inner box for high contrast
    ctx.setDrawColor(Console::COLOR_WHITE);
    ctx.drawFrame(boardX + _cx * tileSize, boardY + _cy * tileSize, tileSize, tileSize);
    ctx.setDrawColor(Console::COLOR_BLACK);
    ctx.drawFrame(boardX + _cx * tileSize + 1, boardY + _cy * tileSize + 1, tileSize - 2, tileSize - 2);

    // Sidebar
    int wMat = 0;
    int bMat = 0;
    for (int ty = 0; ty < 8; ++ty) {
        for (int tx = 0; tx < 8; ++tx) {
            Piece p = _board[ty][tx];
            if (p.isEmpty()) continue;
            int val = 0;
            switch(p.type) {
                case PieceType::Pawn: val=1; break;
                case PieceType::Knight: val=3; break;
                case PieceType::Bishop: val=3; break;
                case PieceType::Rook: val=5; break;
                case PieceType::Queen: val=9; break;
                default: break;
            }
            if (p.color == PieceColor::White) wMat += val;
            else bMat += val;
        }
    }

    ctx.setDrawColor(Console::COLOR_WHITE);
    // Vertical separator
    ctx.drawVLine(65, 0, 64);
    
    ctx.setFont(u8g2_font_5x8_tf);
    ctx.drawStr(69, 9, "TURN:");
    
    // Inverted box for active turn
    if (_turn == PieceColor::White) {
        ctx.drawBox(69, 12, 48, 11);
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawStr(74, 21, "WHITE");
    } else {
        ctx.drawFrame(69, 12, 48, 11);
        ctx.drawStr(74, 21, "BLACK");
    }
    
    // Draw Material Advantage
    ctx.setDrawColor(Console::COLOR_WHITE);
    if (wMat > bMat) {
        ctx.drawPrintf(69, 36, "W +%d", wMat - bMat);
    } else if (bMat > wMat) {
        ctx.drawPrintf(69, 36, "B +%d", bMat - wMat);
    } else {
        ctx.drawStr(69, 36, "Even");
    }
    
    // Controls at bottom right
    ctx.drawStr(69, 52, "[A] Move");
    ctx.drawStr(69, 61, "[B] Back");
    
    // Evaluation Bar (far right, 6 pixels wide)
    int eval = evaluateBoard();
    if (eval > 150) eval = 150;
    if (eval < -150) eval = -150;
    int splitY = 32 + (eval * 30 / 150);
    
    ctx.drawFrame(121, 1, 6, 62);
    ctx.drawBox(121, splitY, 6, 63 - splitY);
    ctx.setDrawColor(Console::COLOR_BLACK);
    ctx.drawHLine(121, 32, 6);

    if (_gameOver) {
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(14, 24, 100, 16);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawFrame(14, 24, 100, 16);
        ctx.setFont(u8g2_font_6x10_tf);
        if (_winner == PieceColor::White) {
            ctx.drawStrCentered(35, "WHITE WINS!");
        } else {
            ctx.drawStrCentered(35, "BLACK WINS!");
        }
    }
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
    _sm.onEvent(Event::CUSTOM_2, SceneManager::REPLACE, &_title);

    _sm.replace(&_title, ctx);
}

void ChessGame::onExit(Console& ctx) {
    ctx.setDrawColor(Console::COLOR_WHITE);
    SceneGame::onExit(ctx);
}

const char* ChessGame::getName() const {
    return "Chess";
}

REGISTER_GAME(ChessGame);
