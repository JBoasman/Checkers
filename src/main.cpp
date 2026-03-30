#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "Board.h"
#include "AI.h"
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>

enum class Controller { Player, EasyBot, HardBot };
enum class GameState  { Setup, Playing, GameOver };

struct Anim {
    bool   active   = false;
    float  startX, startY, endX, endY;
    float  elapsed  = 0.f;
    float  duration = 0.2f;
    Square piece    = Square::Empty;
};

void drawBtn(sf::RenderWindow& w, sf::Font& f,
             float x, float y, float bw, float bh,
             const std::string& label, int fs,
             sf::Color fill, sf::Color outline, sf::Color tc) {
    sf::RectangleShape btn(sf::Vector2f(bw, bh));
    btn.setPosition(x, y); btn.setFillColor(fill);
    btn.setOutlineThickness(1); btn.setOutlineColor(outline);
    w.draw(btn);
    sf::Text t(label, f, fs);
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.left + b.width/2.f, b.top + b.height/2.f);
    t.setPosition(x + bw/2.f, y + bh/2.f);
    t.setFillColor(tc); w.draw(t);
}

int main() {
    sf::RenderWindow window(sf::VideoMode(1100, 950), "Checkers Ultra");
    window.setFramerateLimit(60);

    Board board;
    sf::Font font; font.loadFromFile("arial.ttf");
    sf::Font symFont; symFont.loadFromFile("C:\\Windows\\Fonts\\seguisym.ttf");

    sf::SoundBuffer cbuf, ptbuf, rbuf, subuf, wbuf;
    cbuf.loadFromFile("clickSound.wav");
    ptbuf.loadFromFile("pieceTakenSound.wav");
    rbuf.loadFromFile("resetSound.wav");
    subuf.loadFromFile("startUpSound.wav");
    wbuf.loadFromFile("winSound.wav");
    sf::Sound clickSnd(cbuf), pieceSnd(ptbuf), resetSnd(rbuf), startSnd(subuf), winSnd(wbuf);
    startSnd.play();

    Controller redCtrl   = Controller::Player;
    Controller blackCtrl = Controller::Player;
    GameState  state     = GameState::Setup;
    GameResult lastResult = GameResult::None;

    int   selX = -1, selY = -1;
    bool  isSelected     = false;
    bool  resultRecorded = false;
    Anim  anim;
    sf::Clock gameClock;
    float aiDelay = 0.f;
    const float AI_DELAY = 0.5f;

    auto squarePx = [](int gx, int gy) -> sf::Vector2f {
        return { gx*100.f + 10.f, gy*100.f + 10.f };
    };
    auto ctrlName = [](Controller c) -> std::string {
        if (c == Controller::Player)  return "Player";
        if (c == Controller::EasyBot) return "Easy Bot";
        return "Hard Bot";
    };
    auto currentCtrl = [&]() -> Controller {
        return (board.getCurrentTurn() == Square::Red) ? redCtrl : blackCtrl;
    };
    auto isAITurn = [&]() -> bool {
        return currentCtrl() != Controller::Player
            && board.checkResult() == GameResult::None;
    };
    auto doMove = [&](int sx, int sy, int ex, int ey) {
        Square mp = board.getSquareState(sx, sy);
        board.movePiece(sx, sy, ex, ey);
        auto fr = squarePx(sx, sy), to = squarePx(ex, ey);
        anim = { true, fr.x, fr.y, to.x, to.y, 0.f, 0.2f, mp };
        if (board.didCapture()) pieceSnd.play(); else clickSnd.play();
    };
    auto startGame = [&]() {
        board.resetBoard();
        isSelected = false; resultRecorded = false;
        anim.active = false; aiDelay = 0.f;
        state = GameState::Playing;
        lastResult = GameResult::None;
        if (redCtrl != Controller::Player) aiDelay = AI_DELAY;
        clickSnd.play();
    };

    while (window.isOpen()) {
        float dt = gameClock.restart().asSeconds();

        // Tick animation
        if (anim.active) { anim.elapsed += dt; if (anim.elapsed >= anim.duration) anim.active = false; }

        // Check result
        GameResult result = board.checkResult();
        if (result != GameResult::None && state == GameState::Playing) {
            state = GameState::GameOver;
            lastResult = result;
        }
        if (state == GameState::GameOver && !resultRecorded) {
            board.recordResult(lastResult);
            resultRecorded = true;
            winSnd.play();
        }

        // Tick AI
        if (state == GameState::Playing && isAITurn()) {
            if (aiDelay > 0.f) { aiDelay -= dt; }
            else {
                Controller ctrl = currentCtrl();
                Square turn = board.getCurrentTurn();
                Board::Move m = (ctrl == Controller::EasyBot)
                    ? randomMove(board) : minimaxMove(board, turn);
                if (m.sx >= 0) {
                    doMove(m.sx, m.sy, m.ex, m.ey);
                    aiDelay = board.isMultiJump() ? 0.3f : AI_DELAY;
                }
            }
        }

        // Events
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();
            if (ev.type != sf::Event::MouseButtonPressed ||
                ev.mouseButton.button != sf::Mouse::Left) continue;

            int mx = ev.mouseButton.x, my = ev.mouseButton.y;
            if (anim.active) anim.active = false;

            // Board clicks
            if (mx < 800 && my < 800 && state == GameState::Playing && !isAITurn()) {
                int tx = mx/100, ty = my/100;
                if (!isSelected) {
                    Square s = board.getSquareState(tx, ty);
                    bool mine = (board.getCurrentTurn() == Square::Red)
                        ? (s == Square::Red   || s == Square::RedKing)
                        : (s == Square::Black || s == Square::BlackKing);
                    if (mine) {
                        if (board.isMultiJump() &&
                            (tx != board.getRequiredX() || ty != board.getRequiredY())) continue;
                        selX = tx; selY = ty; isSelected = true; clickSnd.play();
                    }
                } else {
                    int px = board.getLastMoveEndX(), py = board.getLastMoveEndY();
                    doMove(selX, selY, tx, ty);
                    bool moved = (board.getLastMoveEndX() != px || board.getLastMoveEndY() != py);
                    if (!moved) clickSnd.play();
                    isSelected = false;
                    if (isAITurn()) aiDelay = AI_DELAY;
                }
            }

            // Bottom buttons
            if (my > 870 && my < 920) {
                if (mx > 20 && mx < 180) {
                    board.resetBoard(); isSelected = false;
                    resultRecorded = false; anim.active = false; aiDelay = 0.f;
                    state = GameState::Setup; lastResult = GameResult::None;
                    resetSnd.play();
                }
                if (mx > 200 && mx < 380) {
                    board.resetScores(); resetSnd.play();
                }
                if (mx > 560 && mx < 750 && state == GameState::Playing) {
                    board.forfeit(); isSelected = false;
                    resultRecorded = true; anim.active = false; aiDelay = 0.f;
                    state = GameState::Setup; lastResult = GameResult::None;
                    resetSnd.play();
                }
            }

            // Side panel controller rows
            if (mx >= 820 && mx <= 1080) {
                float btnW = 260.f / 3.f;
                Controller vals[] = { Controller::Player, Controller::EasyBot, Controller::HardBot };
                if (my >= 55 && my <= 89) {
                    int idx = std::min(2, (int)((mx-820) / btnW));
                    redCtrl = vals[idx];
                    state = GameState::Setup; board.resetBoard();
                    isSelected = false; resultRecorded = false; anim.active = false; aiDelay = 0.f;
                    clickSnd.play();
                }
                if (my >= 145 && my <= 179) {
                    int idx = std::min(2, (int)((mx-820) / btnW));
                    blackCtrl = vals[idx];
                    state = GameState::Setup; board.resetBoard();
                    isSelected = false; resultRecorded = false; anim.active = false; aiDelay = 0.f;
                    clickSnd.play();
                }
            }

            // Play button
            if (state != GameState::Playing && mx > 275 && mx < 525 && my > 360 && my < 440)
                startGame();
        }

        // ── Draw ──────────────────────────────────────────────────────────────
        window.clear(sf::Color(30, 30, 30));

        bool dimBoard = (state != GameState::Playing);

        // Board tiles
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                sf::Color tc = (x+y)%2 == 0 ? sf::Color(222,184,135) : sf::Color(139,69,19);
                if ((x == board.getLastMoveStartX() && y == board.getLastMoveStartY()) ||
                    (x == board.getLastMoveEndX()   && y == board.getLastMoveEndY()))
                    tc = sf::Color(std::min(255,(int)tc.r+40), std::min(255,(int)tc.g+40), tc.b);
                if (dimBoard) tc = sf::Color(tc.r*0.45f, tc.g*0.45f, tc.b*0.45f);
                sf::RectangleShape tile(sf::Vector2f(100,100));
                tile.setPosition(x*100.f, y*100.f); tile.setFillColor(tc);
                window.draw(tile);
                if (!dimBoard && isSelected && board.legalMove(selX, selY, x, y)) {
                    sf::CircleShape h(15); h.setFillColor(sf::Color(255,255,0,120));
                    h.setPosition(x*100.f+35.f, y*100.f+35.f); window.draw(h);
                }
            }
        }

        // Pieces
        sf::Uint8 alpha = dimBoard ? 80 : 255;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                Square s = board.getSquareState(x, y);
                if (s == Square::Empty) continue;
                if (anim.active && x == board.getLastMoveEndX() && y == board.getLastMoveEndY()) continue;
                float px = x*100.f+10.f, py = y*100.f+10.f;
                sf::CircleShape p(40); p.setPosition(px, py);
                p.setFillColor((s==Square::Red||s==Square::RedKing)
                    ? sf::Color(200,30,30,alpha) : sf::Color(40,40,40,alpha));
                p.setOutlineThickness(2); p.setOutlineColor(sf::Color(0,0,0,alpha));
                if (!dimBoard && isSelected && x==selX && y==selY)
                    { p.setOutlineColor(sf::Color(50,150,255)); p.setOutlineThickness(6); }
                else if (s==Square::RedKing||s==Square::BlackKing)
                    { p.setOutlineColor(sf::Color(212,175,55,alpha)); p.setOutlineThickness(5); }
                window.draw(p);
                if (s==Square::RedKing||s==Square::BlackKing) {
                    sf::Text cr(L"\u265B", symFont, 52);
                    cr.setFillColor(sf::Color(212,175,55,alpha));
                    sf::FloatRect b = cr.getLocalBounds();
                    cr.setOrigin(b.left+b.width/2.f, b.top+b.height/2.f);
                    cr.setPosition(px+40.f, py+40.f); window.draw(cr);
                }
            }
        }

        // Animated piece
        if (anim.active) {
            float t = anim.elapsed/anim.duration; t = t*t*(3.f-2.f*t);
            float px = anim.startX+(anim.endX-anim.startX)*t;
            float py = anim.startY+(anim.endY-anim.startY)*t;
            sf::CircleShape p(40); p.setPosition(px, py);
            p.setFillColor((anim.piece==Square::Red||anim.piece==Square::RedKing)
                ? sf::Color(200,30,30) : sf::Color(40,40,40));
            p.setOutlineThickness(2); p.setOutlineColor(sf::Color::Black);
            if (anim.piece==Square::RedKing||anim.piece==Square::BlackKing)
                { p.setOutlineColor(sf::Color(212,175,55)); p.setOutlineThickness(5); }
            window.draw(p);
            if (anim.piece==Square::RedKing||anim.piece==Square::BlackKing) {
                sf::Text cr(L"\u265B", symFont, 52);
                cr.setFillColor(sf::Color(212,175,55));
                sf::FloatRect b = cr.getLocalBounds();
                cr.setOrigin(b.left+b.width/2.f, b.top+b.height/2.f);
                cr.setPosition(px+40.f, py+40.f); window.draw(cr);
            }
        }

        // Win/draw/stalemate overlay
        if (state != GameState::Playing) {
            sf::RectangleShape overlay(sf::Vector2f(800, 800));
            overlay.setFillColor(sf::Color(0,0,0,140)); window.draw(overlay);

            if (state == GameState::GameOver) {
                std::string msg; sf::Color mc;
                if      (lastResult == GameResult::RedWins)   { msg = "RED WINS!";            mc = sf::Color(255,80,80); }
                else if (lastResult == GameResult::BlackWins)  { msg = "BLACK WINS!";          mc = sf::Color(220,220,220); }
                else                                           { msg = "DRAW!";                mc = sf::Color(200,200,100); }
                sf::Text rt(msg, font, 80);
                sf::FloatRect rb = rt.getLocalBounds();
                rt.setOrigin(rb.left+rb.width/2.f, rb.top+rb.height/2.f);
                rt.setPosition(400.f, 300.f); rt.setFillColor(mc); window.draw(rt);

                std::string reason;
                if (lastResult == GameResult::Draw) {
                    if (board.getQuietMoves() >= 40) reason = "40-move rule";
                    else reason = "3-fold repetition";
                } else if (board.isStalemate()) reason = "Stalemate";
                if (!reason.empty()) {
                    sf::Text rr("("+reason+")", font, 26);
                    sf::FloatRect rrb = rr.getLocalBounds();
                    rr.setOrigin(rrb.left+rrb.width/2.f, rrb.top+rrb.height/2.f);
                    rr.setPosition(400.f, 375.f); rr.setFillColor(sf::Color(180,180,180)); window.draw(rr);
                }
            }

            // Play button
            std::string playLabel = (state == GameState::GameOver) ? "PLAY AGAIN" : "PLAY";
            sf::RectangleShape playBtn(sf::Vector2f(250,80));
            playBtn.setPosition(275.f, 360.f);
            playBtn.setFillColor(sf::Color(40,140,40));
            playBtn.setOutlineThickness(2); playBtn.setOutlineColor(sf::Color(80,220,80));
            window.draw(playBtn);
            sf::Text playTxt(playLabel, font, 36);
            sf::FloatRect pb = playTxt.getLocalBounds();
            playTxt.setOrigin(pb.left+pb.width/2.f, pb.top+pb.height/2.f);
            playTxt.setPosition(400.f, 400.f); playTxt.setFillColor(sf::Color::White);
            window.draw(playTxt);

            std::string mu = ctrlName(redCtrl)+"  (Red)  vs  (Black)  "+ctrlName(blackCtrl);
            sf::Text muTxt(mu, font, 18);
            sf::FloatRect mb2 = muTxt.getLocalBounds();
            muTxt.setOrigin(mb2.left+mb2.width/2.f, 0.f);
            muTxt.setPosition(400.f, 455.f); muTxt.setFillColor(sf::Color(180,180,180));
            window.draw(muTxt);
        }

        // Bottom bar
        if (state == GameState::Playing) {
            sf::Text turnTxt(board.getCurrentTurn()==Square::Red ? "RED'S TURN" : "BLACK'S TURN", font, 22);
            turnTxt.setPosition(20.f, 812.f);
            turnTxt.setFillColor(board.getCurrentTurn()==Square::Red ? sf::Color(255,80,80) : sf::Color::White);
            window.draw(turnTxt);
            sf::Text qm("Quiet: "+std::to_string(board.getQuietMoves())+"/40", font, 16);
            qm.setPosition(20.f, 843.f); qm.setFillColor(sf::Color(140,140,140)); window.draw(qm);
        }

        drawBtn(window, font,  20.f, 875.f, 158.f, 42.f, "Reset Board", 17,
                sf::Color(65,65,65),   sf::Color(110,110,110), sf::Color::White);
        drawBtn(window, font, 185.f, 875.f, 158.f, 42.f, "Reset Score", 17,
                sf::Color(90,55,20),   sf::Color(160,100,40),  sf::Color::White);
        if (state == GameState::Playing)
            drawBtn(window, font, 560.f, 875.f, 190.f, 42.f, "FORFEIT", 18,
                    sf::Color(120,30,30), sf::Color(190,60,60), sf::Color::White);

        // ── Side panel ────────────────────────────────────────────────────────
        sf::RectangleShape panel(sf::Vector2f(300,950));
        panel.setPosition(800.f,0.f); panel.setFillColor(sf::Color(20,20,20)); window.draw(panel);
        sf::RectangleShape div2(sf::Vector2f(2,950));
        div2.setPosition(800.f,0.f); div2.setFillColor(sf::Color(60,60,60)); window.draw(div2);

        // Setup title
        sf::Text st("PLAYER SETUP", font, 16);
        st.setPosition(848.f, 16.f); st.setFillColor(sf::Color(150,150,150)); window.draw(st);

        // Controller rows
        auto drawCtrlRow = [&](float ry, const std::string& rl, Controller cur, sf::Color ac) {
            sf::Text lbl(rl, font, 15); lbl.setPosition(820.f, ry-20.f); lbl.setFillColor(ac); window.draw(lbl);
            const char* names[] = { "Player","Easy Bot","Hard Bot" };
            Controller  vals[]  = { Controller::Player, Controller::EasyBot, Controller::HardBot };
            float bw = 260.f/3.f;
            for (int i = 0; i < 3; i++) {
                bool active = (cur == vals[i]);
                drawBtn(window, font, 820.f+i*bw, ry, bw-2.f, 34.f, names[i], 13,
                        active ? sf::Color(50,95,50)  : sf::Color(38,38,38),
                        active ? sf::Color(75,190,75) : sf::Color(65,65,65),
                        sf::Color::White);
            }
        };
        drawCtrlRow(55.f,  "RED",   redCtrl,   sf::Color(220,80,80));
        drawCtrlRow(145.f, "BLACK", blackCtrl, sf::Color(180,180,180));

        // Matchup summary
        std::string mu2 = ctrlName(redCtrl)+" vs "+ctrlName(blackCtrl);
        sf::Text muT(mu2, font, 13);
        sf::FloatRect mub = muT.getLocalBounds();
        muT.setOrigin(mub.left+mub.width/2.f, 0.f);
        muT.setPosition(950.f, 200.f); muT.setFillColor(sf::Color(90,90,90)); window.draw(muT);

        // Scores
        sf::Text scTitle("SCORES", font, 16);
        scTitle.setPosition(873.f, 225.f); scTitle.setFillColor(sf::Color(140,140,140)); window.draw(scTitle);
        sf::Text sc1("Red:   "+std::to_string(board.getRedWins())+" wins", font, 15);
        sc1.setPosition(820.f, 252.f); sc1.setFillColor(sf::Color(220,80,80)); window.draw(sc1);
        sf::Text sc2("Black: "+std::to_string(board.getBlackWins())+" wins", font, 15);
        sc2.setPosition(820.f, 275.f); sc2.setFillColor(sf::Color(180,180,180)); window.draw(sc2);
        sf::Text sc3("Draws: "+std::to_string(board.getDraws()), font, 15);
        sc3.setPosition(820.f, 298.f); sc3.setFillColor(sf::Color(180,180,80)); window.draw(sc3);

        // Graveyard
        sf::Text gravT("GRAVEYARD", font, 15);
        gravT.setPosition(858.f, 330.f); gravT.setFillColor(sf::Color(120,120,120)); window.draw(gravT);

        sf::Text rcl("Red captured:", font, 13);
        rcl.setPosition(815.f, 354.f); rcl.setFillColor(sf::Color(200,80,80)); window.draw(rcl);
        {
            const auto& caps = board.getRedCaptured();
            for (int i = 0; i < (int)caps.size(); i++) {
                int col=i%5, row=i/5;
                sf::CircleShape m(11); m.setPosition(820.f+col*40.f, 374.f+row*40.f);
                m.setFillColor(sf::Color(40,40,40));
                bool k = caps[i]==Square::BlackKing;
                m.setOutlineThickness(k?2:1); m.setOutlineColor(k?sf::Color(212,175,55):sf::Color(80,80,80));
                window.draw(m);
            }
        }

        sf::Text bcl("Black captured:", font, 13);
        bcl.setPosition(815.f, 510.f); bcl.setFillColor(sf::Color(160,160,160)); window.draw(bcl);
        {
            const auto& caps = board.getBlackCaptured();
            for (int i = 0; i < (int)caps.size(); i++) {
                int col=i%5, row=i/5;
                sf::CircleShape m(11); m.setPosition(820.f+col*40.f, 530.f+row*40.f);
                m.setFillColor(sf::Color(200,30,30));
                bool k = caps[i]==Square::RedKing;
                m.setOutlineThickness(k?2:1); m.setOutlineColor(k?sf::Color(212,175,55):sf::Color(80,80,80));
                window.draw(m);
            }
        }

        // Piece counts
        int rl=0, bl=0;
        for (int y=0;y<8;y++) for (int x=0;x<8;x++) {
            Square s = board.getSquareState(x,y);
            if (s==Square::Red||s==Square::RedKing)   rl++;
            if (s==Square::Black||s==Square::BlackKing) bl++;
        }
        sf::Text cntT("Red: "+std::to_string(rl)+"    Black: "+std::to_string(bl), font, 14);
        cntT.setPosition(815.f, 690.f); cntT.setFillColor(sf::Color(160,160,160)); window.draw(cntT);
        sf::Text qmT("Quiet: "+std::to_string(board.getQuietMoves())+"/40", font, 14);
        qmT.setPosition(815.f, 710.f); qmT.setFillColor(sf::Color(120,120,120)); window.draw(qmT);

        // Turn bar
        sf::RectangleShape tbar(sf::Vector2f(260.f,38.f));
        tbar.setPosition(820.f, 760.f);
        tbar.setFillColor(board.getCurrentTurn()==Square::Red ? sf::Color(120,20,20):sf::Color(40,40,40));
        window.draw(tbar);
        std::string ts = (board.getCurrentTurn()==Square::Red) ? "RED TO MOVE" : "BLACK TO MOVE";
        if (state==GameState::Playing && isAITurn()) ts = ctrlName(currentCtrl())+" THINKING";
        if (state==GameState::Setup)    ts = "PRESS PLAY";
        if (state==GameState::GameOver) ts = "GAME OVER";
        sf::Text tl(ts, font, 14);
        sf::FloatRect tlb = tl.getLocalBounds();
        tl.setOrigin(tlb.left+tlb.width/2.f, tlb.top+tlb.height/2.f);
        tl.setPosition(950.f, 779.f); tl.setFillColor(sf::Color::White); window.draw(tl);

        window.display();
    }
    return 0;
}
