#pragma once

#include "Lobby.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "MasterServerHttpClient.generated.h"

class ACustomPlayerController;

UCLASS()
class OVERRIDE_API UMasterServerHttpClient : public UObject
{
	GENERATED_BODY()
	
public:
	void ListLobbies(ACustomPlayerController* Requester);

	void CreateLobby();

	/*
	void QuickSearchLobby();

	void JoinLobby(FLobby TargetLobby);

	void LeaveLobby(FLobby TargetLobby);
	*/

private:
	FString GetServerIP(bool UseLocalServerIP) const;
	FString GetServerFullAddress(bool UseLocalServerIP) const;

	void ListLobbiesCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess,
		ACustomPlayerController* Requester);

	void CreateLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess);
	
};
