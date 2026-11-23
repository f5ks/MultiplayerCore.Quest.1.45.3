#pragma once

#include "beatsaber-hook/shared/callback.hpp"
#include "custom-types/shared/macros.hpp"
#include "Zenject/IInitializable.hpp"
#include "System/IDisposable.hpp"
#include "MpPlayerData.hpp"

#include "../Networking/MpPacketSerializer.hpp"
#include "GlobalNamespace/IBeatSaberMultiplayerSessionManager.hpp"

#include "System/Collections/Concurrent/ConcurrentDictionary_2.hpp"

using ConcurrentPlayerDataDictionary = System::Collections::Concurrent::ConcurrentDictionary_2<StringW, MultiplayerCore::Players::MpPlayerData*>;

DECLARE_CLASS_CODEGEN_INTERFACES(MultiplayerCore::Players, MpPlayerManager, System::Object, ::Zenject::IInitializable*, ::System::IDisposable*) {
    DECLARE_INSTANCE_FIELD_PRIVATE(MultiplayerCore::Networking::MpPacketSerializer*, _packetSerializer);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::IBeatSaberMultiplayerSessionManager*, _sessionManager);
    DECLARE_INSTANCE_FIELD_PRIVATE(ConcurrentPlayerDataDictionary*, _playerData);

    DECLARE_OVERRIDE_METHOD_MATCH(void, Initialize, &::Zenject::IInitializable::Initialize);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &::System::IDisposable::Dispose);

    DECLARE_CTOR(ctor, MultiplayerCore::Networking::MpPacketSerializer* packetSerializer, GlobalNamespace::IBeatSaberMultiplayerSessionManager* sessionManager);
    DECLARE_INSTANCE_METHOD(void, HandlePlayerData, MpPlayerData* packet, GlobalNamespace::IBeatSaberConnectedPlayer* player);

    public:
        bool TryGetPlayer(StringW userId, MpPlayerData*& outplayer);
        MpPlayerData* GetPlayer(StringW userId);

        unordered_event_callback<GlobalNamespace::IBeatSaberConnectedPlayer*, MpPlayerData*> PlayerConnectedEvent;
};