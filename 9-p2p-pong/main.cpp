#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif

#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>
#include <shared_mutex>
#include "platform_socket.hpp"
#include "globals.hpp"
#include "discover.hpp"
#include "p2pTcp.hpp"
#include "logger.hpp"

struct ChatState {
    std::vector<std::string> messages;
    mutable std::shared_mutex messagesMutex;

    void open() {
        std::unique_lock lock(messagesMutex);
        messages.clear();
    }

    void close() {
        std::unique_lock lock(messagesMutex);
        messages.clear();
    }

    void add(std::string message) {
        std::unique_lock lock(messagesMutex);
        messages.push_back(std::move(message));
    }

    std::vector<std::string> snapshot() const {
        std::shared_lock lock(messagesMutex);
        return messages;
    }
};

// Main code
int main(int argc, char** args)
{
    Logger logger("Main");
    if (argc != 3) {
        logger.log("Help: prog <tcp_port> <name>");
        return 0;
    }
    u_int16_t gamePort = std::stoi(args[1]);
    char chatInput[128] = {0};

    ChatState chatState;
    Discover discover(Discover::Config(args[2], gamePort));
    P2PTCP::TcpManager manager(P2PTCP::TcpManager::Config{.port = gamePort});

    // Setup SDL
    #ifdef _WIN32
        ::SetProcessDPIAware();
    #endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        logger.log("Error: ", SDL_GetError());
        return 1;
    }

    // From 2.0.18: Enable native IME.
    #ifdef SDL_HINT_IME_SHOW_UI
        SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    #endif

    // Create window with SDL_Renderer graphics context
    float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        logger.log("Error: SDL_CreateWindow(): ", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        logger.log("Error creating SDL_Renderer!");
        return 1;
    }
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    logger.log("Current SDL_Renderer: ", info.name);


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        {
            using namespace P2PTCP;
            Event event;
            while (manager.incomingQueue.try_pop(event)) {
                switch (event.type) {
                    case Event::ConnectEvent:
                        if (event.data.connectEvent.success) {
                            logger.log("Connected to ", event.data.connectEvent.addr);
                            chatState.open();
                            currState.store(AppState_InGame, std::memory_order_release);
                        } else {
                            logger.log("Failed to connect to ", event.data.connectEvent.addr);
                            chatState.add("Failed to connect");
                        }
                        break;
                    case P2PTCP::Event::Message:
                        chatState.add(std::string("Peer: ") + event.data.message.message);
                        break;
                    case P2PTCP::Event::DisconnectEvent:
                        logger.log("Disconnected from ", event.data.disconnectEvent.addr);
                        chatState.close();
                        currState.store(AppState_Available, std::memory_order_release);
                        break;
                }
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("LAN Users");
        ImGui::Text("User: %s", discover.config.ownLoc.name);
        if (ImGui::BeginTable("neighbour_table", 4, ImGuiTableFlags_Borders, ImVec2(-FLT_MIN, 0.0))) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Connect", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableHeadersRow();
            {
                for (auto& p: discover.getNeighbours()) {
                    std::stringstream ss;
                    ss << p;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", p.name);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", ss.str().data());
                    ImGui::TableSetColumnIndex(2);
                    switch (p.state) {
                        case Discover::IsHost:
                            ImGui::Text("Is Host");
                            break;
                        case Discover::Available:
                            ImGui::Text("Available");
                            break;
                        case Discover::Unavailable:
                            ImGui::Text("Unavailable");
                            break;
                        default:
                            break;
                    }
                    ImGui::TableSetColumnIndex(3);
                    float cellWidth = ImGui::GetContentRegionAvail().x;
                    ImGui::PushID(p.id());
                    ImGui::BeginDisabled(p.state != Discover::Available);
                    if (ImGui::Button("Connect", ImVec2{cellWidth, 20.0f})) {
                        logger.log("Click ", ss.str());
                        if (!manager.connect(p.address)) {
                            logger.log("Failed to queue connection to ", p.address);
                        }
                    }
                    ImGui::EndDisabled();
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();

        if (currState.load(std::memory_order_acquire) == AppState_InGame) {
            bool gameWindowOpen = true;
            if (ImGui::Begin("Game on", &gameWindowOpen)) {
                ImGui::Text("Game has started");
                ImGui::SeparatorText("Chat");
                if (ImGui::BeginChild("chat_messages", ImVec2(0.0f, 180.0f), ImGuiChildFlags_Borders)) {
                    for (const auto& message: chatState.snapshot()) {
                        ImGui::TextWrapped("%s", message.c_str());
                    }
                }
                ImGui::EndChild();
                bool sendChat = ImGui::InputText("##chat_input", chatInput, sizeof(chatInput), ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::SameLine();
                sendChat = ImGui::Button("Send") || sendChat;
                if (sendChat && chatInput[0] != '\0') {
                    if (manager.send_message(chatInput)) {
                        chatState.add(std::string("Me: ") + chatInput);
                        chatInput[0] = '\0';
                    } else {
                        chatState.add("Failed to send: no active TCP connection");
                    }
                }
                if (ImGui::Button("Disconnect", ImVec2{90.0f, 24.0f})) {
                    gameWindowOpen = false;
                }
            }
            ImGui::End();

            if (!gameWindowOpen) {
                logger.log("Disconnecting TCP");
                manager.disconnect();
                chatState.close();
                currState.store(AppState_Available, std::memory_order_release);
            }
        }

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
    currState.store(AppState_Closed, std::memory_order_relaxed);
    discover.kill();

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}