#include "Network/MasterServerHttpClient.h"
#include "BlueprintHelpers.h"
#include "Player/CustomPlayerController.h"

void UMasterServerHttpClient::ListLobbies(ACustomPlayerController* Requester)
{
	UE_LOG(LogTemp, Log, TEXT("Sending List Lobbies Request to Master Server"));
	
	FString UriQuery = GetServerFullAddress(true);
	FHttpModule& HttpModule = FHttpModule::Get();

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();

	Request->SetURL(UriQuery);
	Request->SetVerb(TEXT("GET"));
	
	Request->OnProcessRequestComplete().BindUObject(this, &UMasterServerHttpClient::ListLobbiesCallback, Requester);

	Request->ProcessRequest();
}

void UMasterServerHttpClient::CreateLobby()
{
	UE_LOG(LogTemp, Log, TEXT("Sending Create Lobby Request to Master Server"));

	FString UriQuery = GetServerFullAddress(true);
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

	Request->OnProcessRequestComplete().BindUObject(this, &UMasterServerHttpClient::CreateLobbyCallback);

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

	TSharedPtr<FJsonObject> JsonResponse;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	FJsonSerializer::Deserialize(Reader, JsonResponse);

	if (!JsonResponse.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
		return;
	}
	
	TArray<FLobby> Lobbies;

	if (const TArray<TSharedPtr<FJsonValue>>* Array; JsonResponse->TryGetArrayField(TEXT("lobbies"), Array))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Array)
		{
			FLobby Lobby;
			
			const TSharedPtr<FJsonObject> Obj = Value->AsObject();
			if (!Obj.IsValid()) continue;

			Lobby.Id = Obj->GetStringField(TEXT("Id"));
			Lobby.Version = Obj->GetStringField(TEXT("Version"));
			Lobby.ServerIp = GetServerIP(true);
			Lobby.ServerPort = Obj->GetNumberField(TEXT("ServerPort"));
			
			Lobbies.Add(Lobby);
		}
	}

	Requester->OnLobbyListReceived(Lobbies);
}

void UMasterServerHttpClient::CreateLobbyCallback(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response,
	bool bSuccess)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed"));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	FString Body = Response->GetContentAsString();

	UE_LOG(LogTemp, Log, TEXT("Create Lobby Callback: Status: %d | Body: %s"), StatusCode, *Body);
}

FString UMasterServerHttpClient::GetServerIP(bool UseLocalServerIP) const
{
	return UseLocalServerIP ? "192.168.140.201" : "185.30.209.201";
}

FString UMasterServerHttpClient::GetServerFullAddress(bool UseLocalServerIP) const
{
	return UseLocalServerIP ? "http://192.168.140.201:5000" : "http://185.30.209.201:5000";
}
