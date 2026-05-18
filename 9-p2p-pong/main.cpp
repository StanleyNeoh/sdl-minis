#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include "platform_socket.hpp"
#include <unordered_map>

using Clock = std::chrono::system_clock;

struct LocData {
    in_addr_t address = 0;
    in_port_t port = 0;
    char name[32] = {0};
    time_t timestamp = 0;

    LocData(int port, std::string_view _name, in_addr_t address): port(port), address(address) {
        _name = _name.substr(0, 31);
        memcpy(name, _name.data(), _name.size());
    }
    LocData(int port, std::string_view _name): LocData(port, _name, 0) {}
    LocData(): LocData(0, "", 0) {};

    void refresh() {
        timestamp = Clock::to_time_t(Clock::now());
    }

    bool operator==(const LocData& other) const {
        return address == other.address && port == other.port;
    }

    std::size_t id() const {
        return address << 16 | port;
    }
};

template <>
struct std::hash<LocData> {
    std::size_t operator()(const LocData& locData) const noexcept {
        return locData.id();
    }
};

std::atomic<bool> is_running;
std::shared_mutex neighbour_ips_mut;
std::unordered_map<size_t, LocData> neighbour_ips;

void broadcast_thread(int udpSocket, int udpPort, LocData myloc) {
    std::cout << "Starting broadcast thread\n";

    sockaddr_in broadcastAddress{};
    broadcastAddress.sin_family = AF_INET;
    broadcastAddress.sin_port = htons(udpPort); // converts to network byte order
    broadcastAddress.sin_addr.s_addr = INADDR_BROADCAST; // Socket listens to all available IPs (Main TCP socket to start handshake)
    while (is_running.load(std::memory_order_relaxed)) {
        myloc.refresh();
        ssize_t n = sendto(udpSocket, &myloc, sizeof(myloc), 0, reinterpret_cast<sockaddr*>(&broadcastAddress), sizeof(broadcastAddress));
        std::cout << "Sending UDP n = " << n << " to broadcast port " << udpPort << ". Error: " << socket_error() <<"\n";
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    std::cout << "Closed broadcast thread\n";
}


void listen_thread(int clientSocket, int udpPort) {
    std::cout << "Starting listen thread\n";
    sockaddr_in senderAddress;
    socklen_t addressSize = sizeof(senderAddress);
    LocData recvloc;
    while (is_running.load(std::memory_order_relaxed)) {
        ssize_t n = recvfrom(clientSocket, &recvloc, sizeof(recvloc), 0, reinterpret_cast<sockaddr*>(&senderAddress), &addressSize);
        std::cout << "Received n = " << n << " bytes \n";
        if (n <= 0) {
            std::cerr << "Failed to receive UDP gateway response: " << socket_error() << "\n";
            break;
        }
        recvloc.address = senderAddress.sin_addr.s_addr;
        {
            std::unique_lock lock(neighbour_ips_mut);
            neighbour_ips[recvloc.id()] =  recvloc;
            auto it = neighbour_ips.begin();
            time_t now = Clock::to_time_t(Clock::now());
            while (it != neighbour_ips.end()) {
                if (std::difftime(now, it->second.timestamp) > 10.0f) {
                    it = neighbour_ips.erase(it);
                } else {
                    it++;
                }
            }
        }
    }
    std::cout << "Closed listen thread\n";
}

struct SearchThreads {
    int clientSocket = -1;

    SearchThreads(int udpPort, int tcpPort, std::string_view name) {
        clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (clientSocket < 0) {
            std::cerr << "Failed to create UDP socket: " << socket_error() << "\n";
            return;
        } 

        int is_broadcast = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&is_broadcast), sizeof(is_broadcast)) != 0) {
            std::cout << "Failed to set SO_BROADCAST: " << socket_error() << "\n";
            return;
        }

        int reuse = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&reuse), sizeof(reuse)) != 0) {
            std::cout << "Failed to set SO_REUSEPORT: " << socket_error() << "\n";
            return;
        }

        sockaddr_in listenAddress{};
        listenAddress.sin_family = AF_INET;
        listenAddress.sin_port = htons(udpPort); // converts to network byte order
        listenAddress.sin_addr.s_addr = INADDR_ANY; // Socket listens to all available IPs (Main TCP socket to start handshake)
        if (bind(clientSocket, reinterpret_cast<sockaddr*>(&listenAddress), sizeof(listenAddress))) {
            std::cout << "Failed to bind listen address: " << socket_error() << "\n";
            return;
        }

        LocData locData(tcpPort, name);
        is_running.store(true, std::memory_order_relaxed);
        std::thread _broadcast_thread(broadcast_thread, clientSocket, udpPort, locData);
        _broadcast_thread.detach();

        std::thread _listen_thread(listen_thread, clientSocket, udpPort);
        _listen_thread.detach();
    }

    ~SearchThreads() {
        is_running.store(false, std::memory_order_relaxed);
        if (clientSocket >= 0) close(clientSocket);
    }
};

// Main code
int main(int argc, char** args)
{
    if (argc != 3) {
        std::cout << "Help: prog <tcp_port> <name>\n";
        return 0;
    }
    int tcpPort = std::stoi(args[1]);
    std::string_view name = args[2];
    SearchThreads search_thread(12345, tcpPort, name);

    // Setup SDL
    #ifdef _WIN32
        ::SetProcessDPIAware();
    #endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
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
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return 1;
    }
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    SDL_Log("Current SDL_Renderer: %s", info.name);


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

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("LAN Users");
        if (ImGui::BeginTable("neighbour_table", 3, ImGuiTableFlags_Borders, ImVec2(-FLT_MIN, 0.0))) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn("Connect", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableHeadersRow();
            {
                std::shared_lock lock(neighbour_ips_mut);
                for (auto& p: neighbour_ips) {
                    char clientIp[INET_ADDRSTRLEN] = {0};
                    inet_ntop(AF_INET, &p.second.address, clientIp, sizeof(clientIp));
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", p.second.name);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s:%u", clientIp, p.second.port);
                    ImGui::TableSetColumnIndex(2);
                    float cellWidth = ImGui::GetContentRegionAvail().x;
                    ImGui::PushID(p.second.id());
                    if (ImGui::Button("Connect", ImVec2{cellWidth, 20.0f})) {
                        std::cout << "Click " << clientIp << ":" << p.second.port << "\n";
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
    is_running.store(false, std::memory_order_relaxed);

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}