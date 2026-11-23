#include "MultiplayerCore.hpp"
#include "logging.hpp"

#include "GlobalNamespace/PlatformAuthenticationTokenProvider.hpp"
#include "Oculus/Platform/Core.hpp"
#include "Oculus/Platform/Message_1.hpp"
#include "Oculus/Platform/Models/Error.hpp"
#include "Oculus/Platform/Models/UserProof.hpp"
#include "Oculus/Platform/Request_1.hpp"
#include "Oculus/Platform/Users.hpp"
#include "System/Threading/CancellationTokenSource.hpp"
#include "System/Threading/Tasks/TaskCompletionSource_1.hpp"
#include "System/Threading/Tasks/TaskCreationOptions.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "beatsaber-hook/shared/safeptr.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "custom-types/shared/delegate.hpp"

namespace MultiplayerCore {
    namespace {
        using namespace System::Threading;
        using namespace System::Threading::Tasks;
        using ProofSource = TaskCompletionSource_1<StringW>;
        using ProofMessage = Oculus::Platform::Message_1<Oculus::Platform::Models::UserProof*>;
    }

    StringW API::GetQuestAuthenticationUserId(GlobalNamespace::IAuthenticationTokenProvider* provider) {
        if (!provider) return nullptr;

        const auto questProvider = i2c::try_cast<GlobalNamespace::PlatformAuthenticationTokenProvider*>(provider);
        return questProvider ? questProvider->_userId : nullptr;
    }

    StringW API::RequestQuestUserProof(CancellationToken cancellationToken) {
        auto completion = safe_ptr(ProofSource::New_ctor(TaskCreationOptions::RunContinuationsAsynchronously));
        auto cancellation = safe_ptr(cancellationToken._source);
        BSML::MainThreadScheduler::Schedule([completion, cancellation] {
            try {
                if (CancellationToken(cancellation.unchecked_ptr()).IsCancellationRequested ||
                    !Oculus::Platform::Core::IsInitialized()) {
                    completion->TrySetResult(nullptr);
                    return;
                }

                auto request = Oculus::Platform::Users::GetUserProof();
                if (!request) {
                    completion->TrySetResult(nullptr);
                    return;
                }

                request->OnComplete(custom_types::MakeDelegate<ProofMessage::Callback*>(
                    std::function<void(ProofMessage*)>([completion](ProofMessage* message) {
                        try {
                            if (message && !message->get_IsError()) {
                                auto proof = message->get_Data();
                                completion->TrySetResult(proof ? proof->Value : nullptr);
                                return;
                            }
                            const auto error = message ? message->GetError() : nullptr;
                            WARNING("Meta Quest user proof request failed, SDK code {}", error ? error->Code : -1);
                        } catch (...) {
                            WARNING("Could not read the Meta Quest user proof response");
                        }
                        completion->TrySetResult(nullptr);
                    })
                ));
            } catch (...) {
                WARNING("Could not start the Meta Quest user proof request");
                completion->TrySetResult(nullptr);
            }
        });

        auto task = completion->get_Task();
        return task->Wait(10000, cancellationToken) ? task->get_Result() : nullptr;
    }
}
