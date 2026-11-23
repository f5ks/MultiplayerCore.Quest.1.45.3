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


#ifdef QUEST_AUTH_TRACE
#include "hooking.hpp"
#include "Oculus/Platform/CAPI.hpp"
#include "Oculus/Platform/Message.hpp"
#include "Oculus/Platform/PlatformInitializeResult.hpp"
#include "System/IntPtr.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#endif

namespace MultiplayerCore {
    namespace {
        using namespace System::Threading;
        using namespace System::Threading::Tasks;
        using ProofSource = TaskCompletionSource_1<StringW>;
        using ProofMessage = Oculus::Platform::Message_1<Oculus::Platform::Models::UserProof*>;
#ifdef QUEST_AUTH_TRACE
        using TraceClock = std::chrono::steady_clock;
        using Oculus::Platform::CAPI;
        using Oculus::Platform::Message_MessageType;
        std::atomic_uint64_t nextProofTrace{0};
        std::atomic_uint64_t latestProofRequest{0};

        auto TraceElapsedMs(TraceClock::time_point start) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(TraceClock::now() - start).count();
        }

        // Preserve only known error vocabulary. A URL, identifier, or unfamiliar word is omitted whole.
        std::string RedactSdkMessage(StringW message) {
            if (!message || message.empty()) return "<empty>";

            constexpr std::string_view safeWords[]{
                "a", "access", "account", "already", "an", "and", "app", "application", "auth", "authentication",
                "authorization", "available", "bad", "bind", "blocked", "by", "cache", "call", "caller", "can",
                "cannot", "certificate", "check", "client", "complete", "configuration", "connect", "connected",
                "connection", "consent", "could", "denied", "device", "did", "disabled", "dns", "does", "dsat",
                "entitlement", "entitled", "error", "exception", "expired", "failed", "failure", "feature", "fetch",
                "fetching", "for", "forbidden", "from", "get", "getting", "graphql", "has", "have", "host", "http",
                "in", "initialization", "initialize", "initialized", "internal", "invalid", "is", "java.io.ioexception",
                "java.net.protocolexception", "java.net.sockettimeoutexception", "java.net.unknownhostexception",
                "jwt", "logged", "login", "message", "meta", "mismatch", "missing", "must", "native", "network",
                "no", "nonce", "not", "oculus", "of", "offline", "on", "operation", "or", "out", "package", "permission",
                "permissions", "platform", "policy", "proof", "query", "rejected", "request", "requested", "required",
                "requires", "resolve", "response", "retrieve", "return", "returned", "sdk", "server", "service",
                "session", "signature", "signed", "signing", "ssl", "start", "status", "supported", "the", "this",
                "timed", "timeout", "time-out", "tls", "to", "token", "unavailable", "unauthorized", "unexpected",
                "unknown", "unknown_error", "unsupported", "user", "valid", "validation", "verify", "version",
                "was", "while", "with", "without"
            };

            const auto text = static_cast<std::string>(message);
            std::string output;
            auto remaining = std::string_view(text);
            while (output.size() < 240) {
                const auto start = remaining.find_first_not_of(" \r\n\t");
                if (start == std::string_view::npos) break;
                remaining.remove_prefix(start);
                const auto raw = remaining.substr(0, remaining.find_first_of(" \r\n\t"));
                remaining.remove_prefix(raw.size());

                auto word = std::string(raw);
                if (!word.empty() && (word.back() == ':' || word.back() == '.' || word.back() == ',')) {
                    word.pop_back();
                }
                std::transform(word.begin(), word.end(), word.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (!output.empty()) output.push_back(' ');
                output += std::ranges::find(safeWords, word) != std::end(safeWords) ? word : "<redacted>";
            }
            return output.empty() ? "<empty>" : output;
        }

        void TraceNativeMessage(System::IntPtr handle) {
            if (!handle.m_value) return;

            const auto type = CAPI::ovr_Message_GetType(handle);
            const bool initialization = type == Message_MessageType::Platform_InitializeAndroidAsynchronous ||
                type == Message_MessageType::Platform_InitializeStandaloneOculus ||
                type == Message_MessageType::Platform_InitializeWithAccessToken;
            const auto requestId = CAPI::ovr_Message_GetRequestID(handle);
            if (!initialization && type != Message_MessageType::User_GetUserProof &&
                (requestId == 0 || latestProofRequest.load() != requestId)) return;

            const bool failed = CAPI::ovr_Message_IsError(handle);
            INFO("[QUEST-AUTH] Native {} response request={} type={} error={}",
                initialization ? "initialization" : "proof", requestId, type.value__, failed);
            if (failed) {
                const auto error = CAPI::ovr_Message_GetError(handle);
                if (!error.m_value) {
                    WARNING("[QUEST-AUTH] Native request={} has no error handle", requestId);
                    return;
                }
                INFO("[QUEST-AUTH] Native request={} SDK={} HTTP={}", requestId,
                    CAPI::ovr_Error_GetCode(error), CAPI::ovr_Error_GetHttpCode(error));
                INFO("[QUEST-AUTH] Native request={} technical: {}", requestId,
                    RedactSdkMessage(CAPI::ovr_Error_GetMessage(error)));
                INFO("[QUEST-AUTH] Native request={} displayable: {}", requestId,
                    RedactSdkMessage(CAPI::ovr_Error_GetDisplayableMessage(error)));
            } else if (initialization) {
                const auto data = CAPI::ovr_Message_GetPlatformInitialize(handle);
                if (data.m_value) {
                    INFO("[QUEST-AUTH] Native initialization request={} result={}", requestId,
                        CAPI::ovr_PlatformInitialize_GetResult(data).value__);
                }
            }
        }
#endif
    }

    StringW API::GetQuestAuthenticationUserId(GlobalNamespace::IAuthenticationTokenProvider* provider) {
        if (!provider) return nullptr;

        const auto questProvider = i2c::try_cast<GlobalNamespace::PlatformAuthenticationTokenProvider*>(provider);
        return questProvider ? questProvider->_userId : nullptr;
    }

    StringW API::RequestQuestUserProof(CancellationToken cancellationToken) {
#ifdef QUEST_AUTH_TRACE
        const auto trace = ++nextProofTrace;
        const auto started = TraceClock::now();
        INFO("[QUEST-AUTH] Proof trace={} queued on Unity main thread", trace);
#endif
        auto completion = safe_ptr(ProofSource::New_ctor(TaskCreationOptions::RunContinuationsAsynchronously));
        auto cancellation = safe_ptr(cancellationToken._source);
        BSML::MainThreadScheduler::Schedule([completion, cancellation
#ifdef QUEST_AUTH_TRACE
            , trace, started
#endif
        ] {
            try {
                const bool canceled = CancellationToken(cancellation.unchecked_ptr()).IsCancellationRequested;
                const bool initialized = !canceled && Oculus::Platform::Core::IsInitialized();
#ifdef QUEST_AUTH_TRACE
                INFO("[QUEST-AUTH] Proof trace={} Unity dispatch after {} ms, canceled={}, SDK initialized={}",
                    trace, TraceElapsedMs(started), canceled, initialized);
#endif
                if (!initialized) {
                    completion->TrySetResult(nullptr);
                    return;
                }

                auto request = Oculus::Platform::Users::GetUserProof();
#ifdef QUEST_AUTH_TRACE
                const auto requestId = request ? request->get_RequestID() : 0;
                latestProofRequest.store(requestId);
                INFO("[QUEST-AUTH] Proof trace={} SDK returned request={} after {} ms",
                    trace, requestId, TraceElapsedMs(started));
#endif
                if (!request) {
                    completion->TrySetResult(nullptr);
                    return;
                }

                request->OnComplete(custom_types::MakeDelegate<ProofMessage::Callback*>(
                    std::function<void(ProofMessage*)>([completion
#ifdef QUEST_AUTH_TRACE
                        , trace, started
#endif
                    ](ProofMessage* message) {
                        try {
#ifdef QUEST_AUTH_TRACE
                            INFO("[QUEST-AUTH] Proof trace={} Unity callback request={} after {} ms, message={}, error={}",
                                trace, message ? message->get_RequestID() : 0, TraceElapsedMs(started),
                                message != nullptr, message && message->get_IsError());
#endif
                            if (message && !message->get_IsError()) {
                                auto proof = message->get_Data();
#ifdef QUEST_AUTH_TRACE
                                INFO("[QUEST-AUTH] Proof trace={} nonce present={}", trace, proof && !proof->Value.empty());
#endif
                                completion->TrySetResult(proof ? proof->Value : nullptr);
                                return;
                            }
                            const auto error = message ? message->GetError() : nullptr;
#ifdef QUEST_AUTH_TRACE
                            INFO("[QUEST-AUTH] Proof trace={} Unity SDK={} HTTP={}, technical: {}",
                                trace, error ? error->Code : -1, error ? error->HttpCode : -1,
                                RedactSdkMessage(error ? error->Message : nullptr));
#endif
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
#ifdef QUEST_AUTH_TRACE
        try {
            const bool completed = task->Wait(10000, cancellationToken);
            auto proof = completed ? task->get_Result() : nullptr;
            INFO("[QUEST-AUTH] Proof trace={} worker finished after {} ms, callback complete={}, nonce present={}",
                trace, TraceElapsedMs(started), completed, proof && !proof.empty());
            return proof;
        } catch (...) {
            INFO("[QUEST-AUTH] Proof trace={} worker wait ended after {} ms, canceled={}",
                trace, TraceElapsedMs(started), cancellationToken.IsCancellationRequested);
            throw;
        }
#else
        return task->Wait(10000, cancellationToken) ? task->get_Result() : nullptr;
#endif
    }
}

#ifdef QUEST_AUTH_TRACE
MAKE_AUTO_HOOK_MATCH(QuestAuth_ParseNativeMessage, &Oculus::Platform::Message::ParseMessageHandle,
    Oculus::Platform::Message*, System::IntPtr handle) {
    try {
        MultiplayerCore::TraceNativeMessage(handle);
    } catch (...) {
        WARNING("[QUEST-AUTH] Could not inspect native message");
    }
    return QuestAuth_ParseNativeMessage(handle);
}
#endif\n