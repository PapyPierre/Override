#pragma once

#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "ServerHttpClient.generated.h"

class UOverrideGameInstance;
class IHttpRequester;
class ACustomPlayerController;

UCLASS()
class OVERRIDE_API UServerHttpClient : public UObject
{
	GENERATED_BODY()

public:
#pragma region Lobbies
	void ListLobbies(ACustomPlayerController* Requester);

	void CreateLobby(ACustomPlayerController* Requester);

	void JoinLobby(FString TargetLobbyId, ACustomPlayerController* Requester);

	void LeaveLobby(FString TargetLobbyId, ACustomPlayerController* Requester, bool ServerSide = false);

	void SendLobbyHeartbeat(FString TargetLobbyId);

	void SetLobbyInGame(FString TargetLobbyId, bool Value);

	//void QuickSearchLobby();
#pragma endregion

#pragma region Telemetry
	void FetchMatchesData(IHttpRequester* Requester,
		FString VersionId, FString MatchId, FString PlayerId, FString TeamId);

	void SetMatchData(FString Version, UOverrideGameInstance* GameInst);
#pragma endregion

private:
	bool UseLocalServerIp = false;

	UServerHttpClient();
	~UServerHttpClient();

	void ResolveMasterServerIp();

	FString GetServerIP() const;
	FString GetMasterServerFullAddress() const;
	FString GetTelemetryServerFullAddress() const;

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

	void FetchMatchListCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess,
	                            IHttpRequester* Requester);

	void FetchMatchDataCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess,
	                            IHttpRequester* Requester);

	void SetMatchDataCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bSuccess);
};
