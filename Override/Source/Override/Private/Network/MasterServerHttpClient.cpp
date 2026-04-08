#include "Network/MasterServerHttpClient.h"
#include "BlueprintHelpers.h"
#include "Player/CustomPlayerController.h"

void UMasterServerHttpClient::ResolveMasterServerIp()
{
	FString UriQuery = "http://192.168.140.201:5000/ipcheck";
	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("GET"));

	Request->OnProcessRequestComplete().BindUObject(this, &UMasterServerHttpClient::ResolveMasterServerIpCallback, true);

	Request->ProcessRequest();
}

void UMasterServerHttpClient::ListLobbies(ACustomPlayerController* Requester)
{
	UE_LOG(LogTemp, Log, TEXT("Sending List Lobbies Request to Master Server"));

	FString UriQuery = GetServerFullAddress() + TEXT("/lobbies");
	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("GET"));

	Request->OnProcessRequestComplete().BindUObject(this, &UMasterServerHttpClient::ListLobbiesCallback, Requester);

	Request->ProcessRequest();
}

void UMasterServerHttpClient::CreateLobby(ACustomPlayerController* Requester)
{
	UE_LOG(LogTemp, Log, TEXT("Sending Create Lobby Request to Master Server"));

	FString UriQuery = GetServerFullAddress() + TEXT("/lobby/create");
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

	Request->OnProcessRequestComplete().BindUObject(this, &UMasterServerHttpClient::CreateLobbyCallback, Requester);

	Request->ProcessRequest();
}

void UMasterServerHttpClient::JoinLobby(FString TargetLobbyId, ACustomPlayerController* Requester)
{
	UE_LOG(LogTemp, Log, TEXT("Sending Join Lobby Request to Master Server"));

	FString UriQuery = GetServerFullAddress() + TEXT("/lobby/playerjoin");
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

	Request->OnProcessRequestComplete().BindUObject(this, &UMasterServerHttpClient::JoinLobbyCallback, Requester);

	Request->ProcessRequest();
}

void UMasterServerHttpClient::LeaveLobby(FString TargetLobbyId, ACustomPlayerController* Requester, bool ServerSide)
{
	UE_LOG(LogTemp, Log, TEXT("Sending Leave %s Request to Master Server"), *TargetLobbyId);

	FString UriQuery = ServerSide
	? TEXT("http://127.0.0.1:5000/lobby/playerleave")
	: GetServerFullAddress() + TEXT("/lobby/playerleave");
	
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

	Request->OnProcessRequestComplete().BindUObject(this, &UMasterServerHttpClient::LeaveLobbyCallback, Requester);

	Request->ProcessRequest();
}

void UMasterServerHttpClient::SendHeartbeat(FString TargetLobbyId)
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

void UMasterServerHttpClient::SetLobbyInGame(FString TargetLobbyId, const bool Value)
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

void UMasterServerHttpClient::ListLobbiesCallback(TSharedPtr<IHttpRequest> Request,
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

void UMasterServerHttpClient::CreateLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
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

void UMasterServerHttpClient::JoinLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
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

	Requester->ConnectToLobbyServer(Obj->GetStringField(TEXT("Id")), Obj->GetStringField(TEXT("serverIp")), Obj->GetIntegerField(TEXT("serverPort")));
}

void UMasterServerHttpClient::LeaveLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
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

	Requester->OnLobbyLeft();
}

FString UMasterServerHttpClient::GetServerIP() const
{
	return UseLocalServerIp ? "192.168.140.201" : "185.30.209.201";
}

FString UMasterServerHttpClient::GetServerFullAddress() const
{
	return TEXT("http://") + GetServerIP() + TEXT(":5000");
}

void UMasterServerHttpClient::ResolveMasterServerIpCallback(TSharedPtr<IHttpRequest> Request,
	TSharedPtr<IHttpResponse> Response, bool bSuccess, bool LocalIpTest)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed"));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	FString Body = Response->GetContentAsString();

	UE_LOG(LogTemp, Log, TEXT("Resolve Master Server Ip Callback: Status: %d | Body: %s"), StatusCode, *Body);

	if (StatusCode == 200) UseLocalServerIp = LocalIpTest;
}

UMasterServerHttpClient::UMasterServerHttpClient()
{
	UE_LOG(LogTemp, Log, TEXT("HttpClient has been constructed"));
	
	ResolveMasterServerIp();
}

UMasterServerHttpClient::~UMasterServerHttpClient()
{
	UE_LOG(LogTemp, Log, TEXT("HttpClient has been destroyed"));
}
