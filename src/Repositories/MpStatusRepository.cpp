#include "Hooks/NetworkConfigHooks.hpp"
#include "Repositories/MpStatusRepository.hpp"
#include "logging.hpp"

DEFINE_TYPE(MultiplayerCore::Repositories, MpStatusRepository);

namespace MultiplayerCore::Repositories {
    MpStatusRepository* MpStatusRepository::_instance = nullptr;

    void MpStatusRepository::New(GlobalNamespace::INetworkConfig* networkConfig) {
        _statusByUrl = System::Collections::Generic::Dictionary_2<StringW, MultiplayerCore::Models::MpStatusData*>::New_ctor();
        _networkConfig = networkConfig;
        _instance = this;
    }

    MpStatusRepository* MpStatusRepository::get_Instance() {
        if (!_instance) {
            ERROR("MpStatusRepository instance is null! Did you call too early?");
        }
        return _instance;
    }

    void MpStatusRepository::ReportStatus(MultiplayerCore::Models::MpStatusData* statusData) {
        _lastStatus = statusData;
        auto server = Hooks::NetworkConfigHooks::GetCurrentServer();
        if (!server) return;
        _statusByUrl->set_Item(server->masterServerStatusUrl, statusData);
        statusUpdatedForUrlEvent.invoke(server->masterServerStatusUrl, statusData);
    }

    MultiplayerCore::Models::MpStatusData* MpStatusRepository::GetLastStatusData() {
        return _lastStatus;
    }
}