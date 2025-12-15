#include "CustomGameInstance.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"

void UCustomGameInstance::Init()
{
	Super::Init();

	if (IsRunningDedicatedServer())
	{
		StartHealthCheck();
	}
}

void UCustomGameInstance::StartHealthCheck()
{
	FHttpServerModule& HttpServer = FHttpServerModule::Get();
	TSharedPtr<IHttpRouter> Router = HttpServer.GetHttpRouter(8080);

	Router->BindRoute(
		FHttpPath(TEXT("/health")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda(
			[](const FHttpServerRequest& Request,
			   const FHttpResultCallback& OnComplete)
			{
				TUniquePtr<FHttpServerResponse> Response =
					FHttpServerResponse::Create(
						FString(TEXT("OK")),
						TEXT("text/plain")
					);

				Response->Code = EHttpServerResponseCodes::Ok;
				OnComplete(MoveTemp(Response));
				return true;
			}
		)
	);

	HttpServer.StartAllListeners();
}
