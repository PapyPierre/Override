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

	void CreateLobby(ACustomPlayerController* Requester);

	void JoinLobby(FString TargetLobbyId, ACustomPlayerController* Requester);

	void LeaveLobby(FString TargetLobbyId, ACustomPlayerController* Requester, bool ServerSide = false);

	void SendHeartbeat(FString TargetLobbyId);

	void SetLobbyInGame(FString TargetLobbyId, bool Value);
	
	//void QuickSearchLobby();

private:
	bool UseLocalServerIp = false;

	UMasterServerHttpClient();
	~UMasterServerHttpClient();

	void ResolveMasterServerIp();
	
	FString GetServerIP() const;
	FString GetServerFullAddress() const;

	void ResolveMasterServerIpCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
		bool bSuccess, bool LocalIpTest);
	
	void ListLobbiesCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess,
	                         ACustomPlayerController* Requester);

	void CreateLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess,
		ACustomPlayerController* Requester);

	void JoinLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess,
		ACustomPlayerController* Requester);

	void LeaveLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess,
		ACustomPlayerController* Requester);

	void SendHeartbeatCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess,
		ACustomPlayerController* Requester);
};
