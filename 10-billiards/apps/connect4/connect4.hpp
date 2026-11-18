#ifndef APPS_CONNECT4_HPP
#define APPS_CONNECT4_HPP

#include "../../textures/connect4/board_frame.hpp"
#include "../../utils/coordinate.hpp"
#include <SDL.h>
#include "imgui.h"
#include <array>
#include <iostream>
#include <cstring>

namespace Apps {

using Color = int;
constexpr Color Color_None = 0;
constexpr Color Color_Red = 1;
constexpr Color Color_Blue = 2;

struct Piece {
    Color color = Color_None;
    int r = -1;
    int c = -1;
    float fr = -1;
};

struct Board {
    using BoardFrame = Textures::Connect4::BoardFrame;
    static constexpr int M = BoardFrame::M;
    static constexpr int N = BoardFrame::N;
    int pieces_l = 0;
    std::array<Piece, M * N> pieces;
    Color board[M][N] {};

    bool addPiece(int col, Color color) {
        if (col < 0 || col >= N || color == Color_None) return false;
        for (int r = 0; r < M; r++) {
            if (board[r][col] != Color_None) continue;
            auto& p = pieces[pieces_l++];
            board[r][col] = color;
            p.color = color;
            p.r = r;
            p.c = col;
            p.fr = M;
            return true;
        }
        return false;
    }

    void clear() {
        pieces_l = 0;
        std::memset(&board, 0, sizeof(board));
    }
};

struct Connect4 {
    using BoardFrame = Textures::Connect4::BoardFrame;
    static constexpr const char* name = "Connect 4";
    static constexpr Color BotColor = Color_Blue;
    static constexpr Color PlayerColor = Color_Red;

    Board board;
    Color turn = Color_Red;

    void drawControls() {
        bool restart = false;
        if (ImGui::Button("Player starts")) {
            turn = Color_Red;
            restart = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Bot starts")) {
            turn = Color_Blue;
            restart = true;
        }
        if (restart) board.clear();
    }

    int colSelected(float x, float originX, float regionW) {
        Utils::map1(x, originX, regionW, 0, BoardFrame::W);
        Utils::map1(x, BoardFrame::br, BoardFrame::W - 2.0f * BoardFrame::br, 0, BoardFrame::N);
        return std::floor(x);
    }

    void drawPieces(ImDrawList* drawList, Uint64 dt, ImVec2 origin, ImVec2 region) {
        using Textures::Connect4::BoardFrame;
        for (int i = 0; i < board.pieces_l; i++) {
            auto& p = board.pieces[i];
            if (p.color == Color_None) continue;
            if (p.fr > p.r) {
                p.fr = std::max<float>(p.r, p.fr - (static_cast<float>(dt) / 300.0f));
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
        }
    }

    void drawBoard(ImDrawList* drawList, ImVec2 origin, ImVec2 region) {
        ImVec2 botright = ImVec2(origin.x + region.x, origin.y + region.y);

        drawList->AddImage((ImTextureID) BoardFrame::get(), origin, botright);

        ImGui::InvisibleButton("", region);
        if (ImGui::IsItemClicked()) {
            ImVec2 pos = ImGui::GetIO().MousePos;
            int col = colSelected(pos.x, origin.x, region.x);
            board.addPiece(col, turn);
            turn = 3 - turn;
        }
    }

    void draw(Uint64 dt) {
        using Textures::Connect4::BoardFrame;

        drawControls();
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 10));

        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImVec2 boardRegion(region.y * BoardFrame::aspect_ratio, region.y);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawPieces(drawList, dt, origin, boardRegion);
        drawBoard(drawList, origin, boardRegion);
    }

    void init() {}
    void deinit() {}
};

}

#endif
