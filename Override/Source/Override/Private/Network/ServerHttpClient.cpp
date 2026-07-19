#include "Network/ServerHttpClient.h"
#include "BlueprintHelpers.h"
#include "HttpModule.h"
#include "GameMode/MatchData.h"
#include "GameMode/MatchPlayerData.h"
#include "GameMode/OverrideGameInstance.h"
#include "Network/HttpRequester.h"
#include "Player/CustomPlayerController.h"

void UServerHttpClient::ResolveMasterServerIp()
{
	FString UriQuery = "http://192.168.140.201:5000/ipcheck";
	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("GET"));

	Request->OnProcessRequestComplete().BindUObject(this, &UServerHttpClient::ResolveMasterServerIpCallback, true);

	Request->ProcessRequest();
}

void UServerHttpClient::ListLobbies(ACustomPlayerController* Requester)
{
	UE_LOG(LogTemp, Log, TEXT("Sending List Lobbies Request to Master Server"));

	FString UriQuery = GetMasterServerFullAddress() + TEXT("/lobbies");
	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("GET"));

	Request->OnProcessRequestComplete().BindUObject(this, &UServerHttpClient::ListLobbiesCallback, Requester);

	Request->ProcessRequest();
}

void UServerHttpClient::CreateLobby(ACustomPlayerController* Requester)
{
	UE_LOG(LogTemp, Log, TEXT("Sending Create Lobby Request to Master Server"));

	FString UriQuery = GetMasterServerFullAddress() + TEXT("/lobby/create");
	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("POST"));

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonBody = MakeShareable(new FJsonObject);
	JsonBody->SetStringField(TEXT("version"), UBlueprintHelpers::GetProjectVersion());

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);

	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindUObject(this, &UServerHttpClient::CreateLobbyCallback, Requester);

	Request->ProcessRequest();
}

void UServerHttpClient::JoinLobby(FString TargetLobbyId, ACustomPlayerController* Requester)
{
	UE_LOG(LogTemp, Log, TEXT("Sending Join Lobby Request to Master Server"));

	FString UriQuery = GetMasterServerFullAddress() + TEXT("/lobby/playerjoin");
	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("POST"));

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonBody = MakeShareable(new FJsonObject);
	JsonBody->SetStringField(TEXT("lobbyId"), TargetLobbyId);
	JsonBody->SetStringField(TEXT("clientVersion"), UBlueprintHelpers::GetProjectVersion());

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);

	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindUObject(this, &UServerHttpClient::JoinLobbyCallback, Requester);

	Request->ProcessRequest();
}

void UServerHttpClient::NotifyLeaveLobby(FString TargetLobbyId, ACustomPlayerController* Requester, bool ServerSide)
{
	UE_LOG(LogTemp, Log, TEXT("Sending Leave %s Request to Master Server"), *TargetLobbyId);

	FString UriQuery = ServerSide
		                   ? TEXT("http://127.0.0.1:5000/lobby/playerleave")
		                   : GetMasterServerFullAddress() + TEXT("/lobby/playerleave");

	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("POST"));

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonBody = MakeShareable(new FJsonObject);
	JsonBody->SetStringField(TEXT("lobbyId"), TargetLobbyId);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);

	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindUObject(this, &UServerHttpClient::NotifyLeaveLobbyCallback, Requester);

	Request->ProcessRequest();
}

void UServerHttpClient::SendLobbyHeartbeat(FString TargetLobbyId)
{
	UE_LOG(LogTemp, Log, TEXT("Sending Heartbeat of %s to Master Server"), *TargetLobbyId);

	FString UriQuery = TEXT("http://127.0.0.1:5000/lobby/heartbeat");
	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("POST"));

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonBody = MakeShareable(new FJsonObject);
	JsonBody->SetStringField(TEXT("lobbyId"), TargetLobbyId);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);

	Request->SetContentAsString(JsonString);

	Request->ProcessRequest();
}

void UServerHttpClient::SetLobbyInGame(FString TargetLobbyId, const bool Value)
{
	if (Value)
	{
		UE_LOG(LogTemp, Log, TEXT("Sending request to set %s IsInGame to true to Master Server"), *TargetLobbyId);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Sending request to set %s IsInGame to false to Master Server"), *TargetLobbyId);
	}

	FString UriQuery = TEXT("http://127.0.0.1:5000/lobby/setingame");
	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("PATCH"));

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonBody = MakeShareable(new FJsonObject);
	JsonBody->SetStringField(TEXT("lobbyId"), TargetLobbyId);
	JsonBody->SetBoolField(TEXT("value"), Value);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);

	Request->SetContentAsString(JsonString);

	Request->ProcessRequest();
}

void UServerHttpClient::FetchMatchesData(IHttpRequester* Requester,
                                         FString VersionId, FString MatchId, FString PlayerId, FString TeamId)
{
	UE_LOG(LogTemp, Log, TEXT("Fetching Match Data from DB"));

	FString UrlQuery = GetTelemetryServerFullAddress() + TEXT("/matches");

	TArray<FString> Params;
	if (!VersionId.IsEmpty()) Params.Add(FString::Printf(TEXT("versionId=%s"), *VersionId));
	if (!MatchId.IsEmpty()) Params.Add(FString::Printf(TEXT("matchId=%s"), *MatchId));
	if (!PlayerId.IsEmpty()) Params.Add(FString::Printf(TEXT("playerId=%s"), *PlayerId));
	if (!TeamId.IsEmpty()) Params.Add(FString::Printf(TEXT("teamId=%s"), *TeamId));
	UrlQuery += FString::Join(Params, TEXT("&"));

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(UrlQuery);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	Request->OnProcessRequestComplete().BindUObject(this, &UServerHttpClient::FetchMatchDataCallback, Requester);

	Request->ProcessRequest();
}

void UServerHttpClient::SetMatchData(FString Version, UOverrideGameInstance* GameInst)
{
	UE_LOG(LogTemp, Error, TEXT("Trying to send match data to DB..."));

	// BUILD JSON
	if (Version.IsEmpty()) Version = TEXT("editor");

	const TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField("versionId", Version);

	TArray<TSharedPtr<FJsonValue>> PlayersArray;

	for (const FMatchPlayerData& Player : GameInst->MatchPlayers)
	{
		TSharedPtr<FJsonObject> PlayerObj = MakeShared<FJsonObject>();
		PlayerObj->SetNumberField(TEXT("playerId"), Player.PlayerId);
		PlayerObj->SetNumberField(TEXT("teamId"), Player.TeamId);
		PlayerObj->SetNumberField(TEXT("kills"), Player.KillsCount);
		PlayerObj->SetNumberField(TEXT("deaths"), Player.DeathsCount);
		PlayerObj->SetNumberField(TEXT("flags"), Player.FlagsCount);
		PlayerObj->SetBoolField(TEXT("hasWon"), Player.HasWon);

		TArray<TSharedPtr<FJsonValue>> PositionsArray;
		for (const FPlayerPosition& Pos : Player.Positions)
		{
			TSharedPtr<FJsonObject> PosObj = MakeShared<FJsonObject>();
			PosObj->SetNumberField(TEXT("posX"), Pos.Position.X);
			PosObj->SetNumberField(TEXT("posY"), Pos.Position.Y);
			PosObj->SetNumberField(TEXT("posZ"), Pos.Position.Z);
			PositionsArray.Add(MakeShared<FJsonValueObject>(PosObj));
		}

		PlayerObj->SetArrayField(TEXT("positions"), PositionsArray);
		PlayersArray.Add(MakeShared<FJsonValueObject>(PlayerObj));
	}

	Json->SetArrayField(TEXT("players"), PlayersArray);

	FString Payload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("http://localhost:6000/matches"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Payload);

	Request->OnProcessRequestComplete().BindUObject(this, &UServerHttpClient::SetMatchDataCallback);

	Request->ProcessRequest();
}

void UServerHttpClient::ListLobbiesCallback(TSharedPtr<IHttpRequest> Request,
                                            TSharedPtr<IHttpResponse> Response, bool bSuccess,
                                            ACustomPlayerController* Requester)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed"));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	FString Body = Response->GetContentAsString();

	UE_LOG(LogTemp, Log, TEXT("List Lobbies Callback: Status: %d | Body: %s"), StatusCode, *Body);

	TArray<TSharedPtr<FJsonValue>> JsonArray;

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
		return;
	}

	TArray<FLobby> Lobbies;

	for (const TSharedPtr<FJsonValue>& Value : JsonArray)
	{
		FLobby Lobby;

		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (!Obj.IsValid()) continue;

		Lobby.Id = Obj->GetStringField(TEXT("Id"));
		Lobby.Version = Obj->GetStringField(TEXT("Version"));
		Lobby.CurrentPlayersCount = Obj->GetIntegerField(TEXT("CurrentPlayersCount"));
		Lobby.MaxPlayers = Obj->GetIntegerField(TEXT("MaxPlayers"));
		Lobby.ServerIp = GetServerIP();
		Lobby.ServerPort = Obj->GetNumberField(TEXT("ServerPort"));
		Lobby.IsInGame = Obj->GetBoolField(TEXT("IsInGame"));

		Lobbies.Add(Lobby);
	}

	Requester->OnLobbyListReceived(Lobbies);
}

void UServerHttpClient::CreateLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
                                            bool bSuccess, ACustomPlayerController* Requester)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed"));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	FString Body = Response->GetContentAsString();

	UE_LOG(LogTemp, Log, TEXT("Create Lobby Callback: Status: %d | Body: %s"), StatusCode, *Body);

	TSharedPtr<FJsonValue> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);

	if (!FJsonSerializer::Deserialize(Reader, Json))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
		return;
	}

	const TSharedPtr<FJsonObject> Obj = Json->AsObject();

	Requester->OnLobbyCreated(Obj->GetStringField(TEXT("Id")));
}

void UServerHttpClient::JoinLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
                                          bool bSuccess, ACustomPlayerController* Requester)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed"));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	FString Body = Response->GetContentAsString();

	UE_LOG(LogTemp, Log, TEXT("Join Lobby Callback: Status: %d | Body: %s"), StatusCode, *Body);

	TSharedPtr<FJsonValue> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);

	if (!FJsonSerializer::Deserialize(Reader, Json))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
		return;
	}

	const TSharedPtr<FJsonObject> Obj = Json->AsObject();

	Requester->ConnectToLobbyServer(Obj->GetStringField(TEXT("Id")), GetServerIP(),
	                                Obj->GetIntegerField(TEXT("serverPort")));
}

void UServerHttpClient::NotifyLeaveLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
                                           bool bSuccess, ACustomPlayerController* Requester)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed"));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	FString Body = Response->GetContentAsString();

	UE_LOG(LogTemp, Log, TEXT("Leave Lobby Callback: Status: %d | Body: %s"), StatusCode, *Body);

	Requester->OnNotifyLobbyLeft();
}

void UServerHttpClient::FetchMatchDataCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
                                               bool bSuccess, IHttpRequester* Requester)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed"));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	FString Body = Response->GetContentAsString();

	UE_LOG(LogTemp, Log, TEXT("Fetch Match Data Callback: Status: %d | Body: %s"), StatusCode, *Body);

	TArray<TSharedPtr<FJsonValue>> MatchesArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);

	if (!FJsonSerializer::Deserialize(Reader, MatchesArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response"));
		return;
	}

	TArray<FMatchData> Matches;

	for (const TSharedPtr<FJsonValue>& MatchValue : MatchesArray)
	{
		TSharedPtr<FJsonObject> MatchObj = MatchValue->AsObject();

		FMatchData Match;

		Match.Id = FString::FromInt(MatchObj->GetNumberField(TEXT("matchId")));
		Match.Version = MatchObj->GetStringField(TEXT("versionId"));

		TArray<FMatchPlayerData> Players;

		const TArray<TSharedPtr<FJsonValue>>& PlayersArray = MatchObj->GetArrayField(TEXT("players"));

		for (const TSharedPtr<FJsonValue>& PlayerValue : PlayersArray)
		{
			TSharedPtr<FJsonObject> PlayerObj = PlayerValue->AsObject();

			FMatchPlayerData PlayerData;

			PlayerData.PlayerId = PlayerObj->GetIntegerField(TEXT("playerId"));
			PlayerData.TeamId = PlayerObj->GetIntegerField(TEXT("teamId"));

			const TArray<TSharedPtr<FJsonValue>>& PositionsArray = PlayerObj->GetArrayField(TEXT("positions"));

			for (const TSharedPtr<FJsonValue>& PosValue : PositionsArray)
			{
				TSharedPtr<FJsonObject> PosObj = PosValue->AsObject();

				FPlayerPosition PlayerPos;

				PlayerPos.Time = PosObj->GetIntegerField(TEXT("posId"));
				PlayerPos.Position.X = PosObj->GetNumberField(TEXT("posX"));
				PlayerPos.Position.Y = PosObj->GetNumberField(TEXT("posY"));
				PlayerPos.Position.Z = PosObj->GetNumberField(TEXT("posZ"));

				PlayerData.Positions.Add(PlayerPos);
			}

			Players.Add(PlayerData);
		}

		Match.Players = Players;

		Matches.Add(Match);
	}

	Requester->OnMatchesDataReceived(Matches);
}

void UServerHttpClient::SetMatchDataCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
                                             bool bSuccess)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("HTTP request failed"));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	FString Body = Response->GetContentAsString();
	
	UE_LOG(LogTemp, Log, TEXT("Set Match Data Callback: Status: %d | Body: %s"), StatusCode, *Body);
}

FString UServerHttpClient::GetServerIP() const
{
	return UseLocalServerIp ? "192.168.140.201" : "185.30.209.201";
}

FString UServerHttpClient::GetMasterServerFullAddress() const
{
	return TEXT("http://") + GetServerIP() + TEXT(":5000");
}

FString UServerHttpClient::GetTelemetryServerFullAddress() const
{
	return TEXT("http://") + GetServerIP() + TEXT(":6000");
}

void UServerHttpClient::ResolveMasterServerIpCallback(TSharedPtr<IHttpRequest> Request,
                                                      TSharedPtr<IHttpResponse> Response, bool bSuccess,
                                                      bool LocalIpTest)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed"));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	FString Body = Response->GetContentAsString();

	UE_LOG(LogTemp, Log, TEXT("Resolve Master Server Ip Callback: Status: %d | Body: %s"), StatusCode, *Body);

	if (StatusCode == 200)
	{
		UE_LOG(LogTemp, Log, TEXT("Ip Check Succeed: Client will use local ip adress (192.168.140.201)"));
		UseLocalServerIp = true;
	}
	else
	{
		UE_LOG(LogTemp, Log,
		       TEXT("Ip Check Failed (Status Code: %d): Client will try use remote ip adress (185.30.209.201)"),
		       StatusCode);
		UseLocalServerIp = false;
	}
}

UServerHttpClient::UServerHttpClient()
{
	UE_LOG(LogTemp, Log, TEXT("HttpClient has been constructed"));

	ResolveMasterServerIp();
}

UServerHttpClient::~UServerHttpClient()
{
	UE_LOG(LogTemp, Log, TEXT("HttpClient has been destroyed"));
}
