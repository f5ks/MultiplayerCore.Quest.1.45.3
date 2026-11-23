#include "scotland2/shared/loader.hpp"
#include "MultiplayerCore.hpp"
#include "System/String.hpp"
#include "System/Exception.hpp"
#include "System/Net/Http/HttpClient.hpp"
#include "System/Net/Http/HttpContent.hpp"
#include "System/Net/Http/HttpRequestMessage.hpp"
#include "System/Net/Http/HttpResponseMessage.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Uri.hpp"
#include "hooking.hpp"
#include "logging.hpp"

#include "Utils/EnumUtils.hpp"
#include "tasks.hpp"

using namespace System::Net::Http;
using namespace System::Threading::Tasks;
using namespace System::Threading;

MAKE_AUTO_HOOK_MATCH(HttpClient_SendAsync, &HttpClient::SendAsync,
    Task_1<HttpResponseMessage *> *, HttpClient *self,
    HttpRequestMessage *request, HttpCompletionOption completionOption,
    CancellationToken cancellationToken) {

  auto result =
      HttpClient_SendAsync(self, request, completionOption, cancellationToken);
  auto currentServer = MultiplayerCore::API::GetCurrentServer();
  if (currentServer &&
      !System::String::IsNullOrWhiteSpace(currentServer->graphUrl) &&
      (request->RequestUri->Host ==
           System::Uri::New_ctor(currentServer->graphUrl)->Host ||
       request->RequestUri->OriginalString->StartsWith(
           currentServer->graphUrl) ||
       (!System::String::IsNullOrWhiteSpace(
            currentServer->masterServerStatusUrl) &&
        request->RequestUri->OriginalString->StartsWith(
            currentServer->masterServerStatusUrl)))) {

    il2cpp_thread([result, request]() {
      try {
        while (!result->IsCompleted && !result->IsCanceled)
          std::this_thread::yield();

        auto response = result->Result;
        if (!response->IsSuccessStatusCode) {
          ERROR("An error occurred while attempting to post to the Graph API: "
                "Uri '{}' StatusCode: {}: {}",
                request->RequestUri->ToString(),
                static_cast<int>(response->StatusCode),
                MultiplayerCore::Utils::EnumUtils::GetEnumName(
                    response->StatusCode));
          auto responseContent = response->Content->ReadAsStringAsync();
          while (!responseContent->IsCompleted && !responseContent->IsCanceled)
            std::this_thread::yield();
          DEBUG("Response: {}", responseContent->Result);
        }
      } 
      catch (const std::exception &ex) {
      ERROR("An error occurred while attempting to post to the Graph API: "
            "Exception Message: {}",
            ex.what());
      // DEBUG(ex.what());
      }
  }).detach();
}
return result;
}
