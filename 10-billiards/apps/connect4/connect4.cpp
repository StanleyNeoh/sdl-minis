#include "connect4.hpp"
#include "../../utils/coordinate.hpp"

namespace Apps {
namespace _Connect4 {

constexpr int searchDepth = 7;

int App::colSelected(float x, float originX, float regionW) {
    Utils::map1(x, originX, regionW, 0, BoardFrame::W);
    Utils::map1(x, BoardFrame::br, BoardFrame::W - 2.0f * BoardFrame::br, 0, BoardFrame::N);
    return std::floor(x);
}

void App::handleEngineData(EngineData& res) {
    switch (res.type) {
        case TypeSearch: {
            Action& action = res.data.search.res.action;
            if (res.data.search.play) {
                board.reqAddPiece(engine, action.bestMove, turn);
                turn = switchTurn(turn);
            } else {
                std::cout << "BestMove: " << action.bestMove << "\n";
                for (int i = 0; i < N; i++) {
                    std::cout << i << ": " << action.scores[i] << "\n";
                }
            }
            return;
        }
        case TypeMove: {
            board.addPiece(res.data.move.col, res.data.move.turn, res.data.move.res.r);
            block = false;
            return;
        }
        case TypeWin: {
            board.setWinner(res.data.win.res.color, res.data.win.res.marked);
            block = true;
            return;
        }
    }
}

bool App::reqSearchMove(Color turn, bool play, int depth) {
    if (play && turn != BotColor) return false;
    EngineData req = {
        .type = TypeSearch,
        .data = { .search = {
            .turn = turn,
            .depth = depth,
            .play = play,
        }}
    };
    engine.reqPipe.block_push(req);
    return true;
}


void App::drawControls() {
    bool restart = false;
    Color _turn = turn;
    bool _botPlays = botPlays;
    if (ImGui::Button("Player starts")) {
        _turn = Color_Red;
        restart = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Bot starts")) {
        _turn = Color_Blue;
        restart = true;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Bot Plays", &_botPlays);

    if (ImGui::Button("Best Move")) {
        reqSearchMove(turn, false, searchDepth);
    }

    if (restart) {
        deinit();
        init();
    }
    turn = _turn;
    botPlays = _botPlays;
}

void App::drawPieces(ImDrawList* drawList, Uint64 dt, ImVec2 origin, ImVec2 region) {
    using Textures::Connect4::BoardFrame;
    for (int i = 0; i < board.pieces_l; i++) {
        auto& p = board.pieces[i];
        if (p.color == Color_None) continue;
        if (p.fr > p.r) {
            p.fr = std::max<float>(p.r, p.fr - (static_cast<float>(dt) / 100.0f));
        }

        ImU32 _color = p.color == Color_Blue ? IM_COL32(0, 0, 255, 255) : IM_COL32(255, 0, 0, 255);

        float gx = p.c * BoardFrame::cd + BoardFrame::cr + BoardFrame::br;
        float gy = (BoardFrame::M - 1 - p.fr) * BoardFrame::cd + BoardFrame::cr + BoardFrame::br;
        Utils::map1(gx, 0, BoardFrame::W, origin.x, region.x);
        Utils::map1(gy, 0, BoardFrame::H, origin.y, region.y);
        drawList->AddCircleFilled(
            ImVec2(gx, gy), 
            Utils::scale1(BoardFrame::ccr, BoardFrame::W, region.x),
            _color
        );

        if (board.marked[p.r][p.c]) {
            constexpr ImU32 mark_color = IM_COL32(255, 255, 0, 255);
            drawList->AddCircleFilled(
                ImVec2(gx, gy), 
                10,
                mark_color
            );
        }
    }
}

void App::drawBoard(ImDrawList* drawList, ImVec2 origin, ImVec2 region) {
    ImVec2 botright = ImVec2(origin.x + region.x, origin.y + region.y);

    drawList->AddImage((ImTextureID) BoardFrame::get(), origin, botright);

    ImGui::InvisibleButton("", region);
    if (ImGui::IsItemClicked()) {
        ImVec2 pos = ImGui::GetIO().MousePos;
        int col = colSelected(pos.x, origin.x, region.x);
        if (!block) {
            board.reqAddPiece(engine, col, turn);
            block = true;
            turn = switchTurn(turn);
        }
    }
}

void App::draw(Uint64 dt) {
    using Textures::Connect4::BoardFrame;

    EngineData res;
    while (engine.resPipe.pop(res)) {
        handleEngineData(res);
    }

    if (botPlays && !block && reqSearchMove(turn, true, searchDepth)) {
        block = true;
    }

    drawControls();
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 10));

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 region = ImGui::GetContentRegionAvail();
    ImVec2 boardRegion(region.y * BoardFrame::aspect_ratio, region.y);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawPieces(drawList, dt, origin, boardRegion);
    drawBoard(drawList, origin, boardRegion);
}

void App::init() {
    engine.startThread();
}

void App::deinit() {
    board.clear();
    engine.clear();
    turn = PlayerColor;
    botPlays = true;
    block = false;
}


}}