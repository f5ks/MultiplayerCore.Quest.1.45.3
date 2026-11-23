#include "hooking.hpp"
#include "logging.hpp"
#include "tasks.hpp"

#include "Hooks/NetworkConfigHooks.hpp"
#include "Models/MpStatusData.hpp"

#include "GlobalNamespace/MultiplayerStatusModel.hpp"
#include "System/Net/Http/HttpClient.hpp"
#include "System/Uri.hpp"

using namespace MultiplayerCore;

MAKE_AUTO_HOOK_ORIG_MATCH(MultiplayerStatusModel_GetMultiplayerStatusAsyncInternal,
    &GlobalNamespace::MultiplayerStatusModel::GetMultiplayerStatusAsyncInternal,
    Task<GlobalNamespace::MultiplayerStatusData*>*, GlobalNamespace::MultiplayerStatusModel* self) {
    if (!Hooks::NetworkConfigHooks::IsOverridingAPI())
        return MultiplayerStatusModel_GetMultiplayerStatusAsyncInternal(self);

    auto server = Hooks::NetworkConfigHooks::GetCurrentServer();
    auto url = System::Uri::New_ctor(server->masterServerStatusUrl);
    auto request = safe_ptr(self->_client->GetStringAsync(url));
    return StartTask<GlobalNamespace::MultiplayerStatusData*>([request] {
        try {
            auto body = request->get_Result();
            if (body && !body.empty())
                return Models::MpStatusData::New_ctor(body);
        } catch (const std::exception& error) {
            ERROR("Could not fetch multiplayer server status: {}", error.what());
        }
        return static_cast<Models::MpStatusData*>(nullptr);
    });
}
