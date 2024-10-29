#include <wx/wx.h>
#include <wx/listctrl.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <json/json.h>
#include <string>
#include <map>

typedef websocketpp::client<websocketpp::config::asio_client> WebsocketClient;

class MatchmakingFrame : public wxFrame {
public:
    MatchmakingFrame() : wxFrame(nullptr, wxID_ANY, "Matchmaking Client") {
        InitializeUI();
        ConnectWebSocket();
    }

private:
    WebsocketClient wsClient;
    websocketpp::connection_hdl wsConnection;
    wxListCtrl* queueList;
    wxListBox* mapList;
    wxButton* joinQueueBtn;
    wxButton* voteMapBtn;
    wxStaticText* statusText;
    bool inQueue = false;

    void InitializeUI() {
        // Create main panel and sizer
        auto* panel = new wxPanel(this);
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        
        // Status text
        statusText = new wxStaticText(panel, wxID_ANY, "Not connected");
        mainSizer->Add(statusText, 0, wxALL, 5);

        // Queue list
        queueList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, 
                                 wxSize(300, 200), wxLC_REPORT);
        queueList->InsertColumn(0, "Player");
        queueList->InsertColumn(1, "ELO");
        mainSizer->Add(queueList, 1, wxEXPAND | wxALL, 5);

        // Map voting section
        auto* mapSizer = new wxBoxSizer(wxHORIZONTAL);
        mapList = new wxListBox(panel, wxID_ANY);
        mapList->Append("dust2");
        mapList->Append("mirage");
        mapList->Append("inferno");
        mapList->Append("overpass");
        mapList->Append("nuke");
        mapSizer->Add(mapList, 1, wxEXPAND | wxALL, 5);

        voteMapBtn = new wxButton(panel, wxID_ANY, "Vote Map");
        mapSizer->Add(voteMapBtn, 0, wxALL, 5);
        mainSizer->Add(mapSizer, 0, wxEXPAND | wxALL, 5);

        // Join queue button
        joinQueueBtn = new wxButton(panel, wxID_ANY, "Join Queue");
        mainSizer->Add(joinQueueBtn, 0, wxALL, 5);

        panel->SetSizer(mainSizer);

        // Bind events
        joinQueueBtn->Bind(wxEVT_BUTTON, &MatchmakingFrame::OnJoinQueue, this);
        voteMapBtn->Bind(wxEVT_BUTTON, &MatchmakingFrame::OnVoteMap, this);

        // Set minimum size
        SetMinSize(wxSize(400, 500));
        Center();
    }

    void ConnectWebSocket() {
        try {
            wsClient.init_asio();
            wsClient.clear_access_channels(websocketpp::log::alevel::all);
            
            // Set up callbacks
            wsClient.set_message_handler([this](auto hdl, auto msg) {
                HandleWebSocketMessage(msg->get_payload());
            });

            wsClient.set_open_handler([this](auto hdl) {
                wsConnection = hdl;
                wxGetApp().CallAfter([this]() {
                    statusText->SetLabel("Connected to server");
                });
            });

            websocketpp::lib::error_code ec;
            auto conn = wsClient.get_connection("ws://localhost:3000", ec);
            if (ec) {
                wxMessageBox("Could not connect to server", "Error");
                return;
            }

            wsClient.connect(conn);

            // Start WebSocket client thread
            std::thread([this]() {
                wsClient.run();
            }).detach();

        } catch (const std::exception& e) {
            wxMessageBox(e.what(), "Error");
        }
    }

    void HandleWebSocketMessage(const std::string& message) {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(message, root)) {
            return;
        }

        wxGetApp().CallAfter([this, root]() {
            if (root.isMember("type")) {
                std::string type = root["type"].asString();
                
                if (type == "queueUpdate") {
                    UpdateQueueList(root["players"]);
                }
                else if (type == "matchCreated") {
                    HandleMatchCreated(root);
                }
                else if (type == "mapVotesUpdate") {
                    UpdateMapVotes(root["votes"]);
                }
            }
        });
    }

    void OnJoinQueue(wxCommandEvent& evt) {
        if (!inQueue) {
            Json::Value data;
            data["username"] = "Player" + std::to_string(rand() % 1000);
            data["elo"] = 1000;

            std::string message = Json::FastWriter().write(data);
            wsClient.send(wsConnection, message, websocketpp::frame::opcode::text);
            
            joinQueueBtn->SetLabel("Leave Queue");
            inQueue = true;
        } else {
            // Leave queue logic
            wsClient.send(wsConnection, "leaveQueue", websocketpp::frame::opcode::text);
            joinQueueBtn->SetLabel("Join Queue");
            inQueue = false;
        }
    }

    void OnVoteMap(wxCommandEvent& evt) {
        int selection = mapList->GetSelection();
        if (selection != wxNOT_FOUND) {
            std::string mapName = mapList->GetString(selection).ToStdString();
            
            Json::Value data;
            data["type"] = "voteMap";
            data["map"] = mapName;

            std::string message = Json::FastWriter().write(data);
            wsClient.send(wsConnection, message, websocketpp::frame::opcode::text);
        }
    }

    void UpdateQueueList(const Json::Value& players) {
        queueList->DeleteAllItems();
        for (const auto& player : players) {
            long index = queueList->GetItemCount();
            queueList->InsertItem(index, player["username"].asString());
            queueList->SetItem(index, 1, std::to_string(player["elo"].asInt()));
        }
    }

    void HandleMatchCreated(const Json::Value& matchData) {
        wxMessageBox("Match found! Get ready to play!", "Match Created");
        inQueue = false;
        joinQueueBtn->SetLabel("Join Queue");
    }

    void UpdateMapVotes(const Json::Value& votes) {
        for (int i = 0; i < mapList->GetCount(); i++) {
            std::string mapName = mapList->GetString(i).ToStdString();
            if (votes.isMember(mapName)) {
                mapList->SetString(i, 
                    wxString::Format("%s (%d votes)", 
                        mapName, votes[mapName].asInt()));
            }
        }
    }
};

class MatchmakingApp : public wxApp {
public:
    bool OnInit() {
        auto* frame = new MatchmakingFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(MatchmakingApp);