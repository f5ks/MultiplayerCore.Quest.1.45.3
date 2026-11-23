#include "Players/MpPlayerManager.hpp"
#include "GlobalNamespace/IMultiplayerSessionManager_1.hpp"
#include "GlobalNamespace/IConnectedPlayer.hpp"

DEFINE_TYPE(MultiplayerCore::Players, MpPlayerManager);

namespace MultiplayerCore::Players {
    void MpPlayerManager::ctor(MultiplayerCore::Networking::MpPacketSerializer* packetSerializer, GlobalNamespace::IBeatSaberMultiplayerSessionManager* sessionManager) {
        INVOKE_CTOR();

        _playerData = ConcurrentPlayerDataDictionary::New_ctor();

        _packetSerializer = packetSerializer;
        _sessionManager = sessionManager;
    }

    void MpPlayerManager::Initialize() {
        _sessionManager->i___GlobalNamespace__IMultiplayerSessionManager_1___GlobalNamespace__IBeatSaberConnectedPlayer__()->SetLocalPlayerState("modded", true);
        _packetSerializer->RegisterCallback<MpPlayerData*>(
            std::bind(&MpPlayerManager::HandlePlayerData, this, std::placeholders::_1, std::placeholders::_2)
        );
    }

    void MpPlayerManager::Dispose() {
        _packetSerializer->UnregisterCallback<MpPlayerData*>();
    }

    void MpPlayerManager::HandlePlayerData(MpPlayerData* packet, GlobalNamespace::IBeatSaberConnectedPlayer* player) {
        _playerData->set_Item(player->i___GlobalNamespace__IConnectedPlayer()->get_userId(), packet);
        PlayerConnectedEvent.invoke(player, packet);
    }

    bool MpPlayerManager::TryGetPlayer(StringW userId, MpPlayerData*& outplayer) {
        if (_playerData->ContainsKey(userId)) {
            outplayer = _playerData->get_Item(userId);
            return true;
        }
        return false;
    }

    MpPlayerData* MpPlayerManager::GetPlayer(StringW userId) {
        MpPlayerData* player = nullptr;
        TryGetPlayer(userId, player);
        return player;
    }
}
