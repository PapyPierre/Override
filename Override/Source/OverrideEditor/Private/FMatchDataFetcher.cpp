#include "FMatchDataFetcher.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "winsock.h"
#include "GameMode/MatchPlayerData.h"

bool FMatchDataFetcher::FetchMatch(FString VersionId, FString MatchId, FString PlayerId, FString TeamId,
                                   TArray<FMatchPlayerData>& OutPlayers)
{
	FSocket* Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)
		->CreateSocket(NAME_Stream, TEXT("StatsSocket"), false);

	FIPv4Address IP;
	FIPv4Address::Parse(TEXT("10.51.0.140"), IP);

	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

#undef SetPort // In winsock.h to avoid conflicts

	Addr->SetIp(IP.Value);
	Addr->SetPort(5000);

	if (Socket->Connect(*Addr))
	{
		UE_LOG(LogTemp, Log, TEXT("Connected to %s successfully in order to fetch match data"),
		       *Addr->ToString(true));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to connected to %s, will not fetch match data"),
		       *Addr->ToString(true));
		return false;
	}

	// BUILD JSON
	const TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField("action", "get_match_info");
	Json->SetStringField("version_id", VersionId);
	Json->SetStringField("match_id", MatchId);
	Json->SetStringField("player_id", PlayerId);
	Json->SetStringField("team_id", TeamId);
	Json->SetStringField("token", "override_db_token");

	FString Payload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	SendDataToDB(Socket, Payload);

	FPlatformProcess::Sleep(0.05f);

	FString Response;
	RecvAll(Socket, Response);
	
	if (Response.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Empty response from DB server"));
		CloseSocket(Socket);
		return false;
	}

	TSharedPtr<FJsonValue> ResponseJson;

	if (!ParseJsonSafe(Response, ResponseJson))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid JSON"));
	}

	const TArray<TSharedPtr<FJsonValue>>& Array = ResponseJson->AsArray();
	TMap<int32, FMatchPlayerData> PlayersById;
	
	for (const TSharedPtr<FJsonValue>& Value : Array)
	{
		TSharedPtr<FJsonObject> Obj = Value->AsObject();

		const int32 PlayerId = Obj->GetIntegerField(TEXT("player_id"));
		const int32 TeamId   = Obj->GetIntegerField(TEXT("team_id"));

		FMatchPlayerData& PlayerData = PlayersById.FindOrAdd(PlayerId);
		PlayerData.PlayerId = PlayerId;
		PlayerData.TeamId   = TeamId;

		FPlayerPosition Pos;
		Pos.Time = Obj->GetIntegerField(TEXT("pos_id")); 
		Pos.Position = FVector(
			Obj->GetNumberField(TEXT("pos_x")),
			Obj->GetNumberField(TEXT("pos_y")),
			Obj->GetNumberField(TEXT("pos_z"))
		);

		PlayerData.Positions.Add(Pos);
	}

	PlayersById.GenerateValueArray(OutPlayers);

	CloseSocket(Socket);
	
	return true;
}

void  FMatchDataFetcher::CloseSocket(FSocket* Socket)
{
	Socket->Close();
	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
}

bool FMatchDataFetcher::SendDataToDB(FSocket* Socket, FString Payload)
{
	FTCHARToUTF8 Convert(*Payload);
	int32 PayloadSize = Convert.Length();

	uint32 NetSize = htonl(PayloadSize);

	int32 Sent = 0;
	Socket->Send((uint8*)&NetSize, sizeof(uint32), Sent);
	Socket->Send((uint8*)Convert.Get(), PayloadSize, Sent);

	return true;
}

bool FMatchDataFetcher::RecvAll(FSocket* Socket, FString& OutResponse)
{
	OutResponse.Empty();

	uint32 NetSize = 0;
	if (!RecvData(Socket, (uint8*)&NetSize, sizeof(uint32)))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read response size"));
		return false;
	}

	const uint32 PayloadSize = ntohl(NetSize);

	if (PayloadSize == 0 || PayloadSize > 5 * 1024 * 1024)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid payload size: %u"), PayloadSize);
		return false;
	}

	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(PayloadSize);

	if (!RecvData(Socket, Buffer.GetData(), PayloadSize))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read response payload"));
		return false;
	}

	OutResponse = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer.GetData())));

	return true;
}

bool FMatchDataFetcher::RecvData(FSocket* Socket, uint8* Data, int32 Size)
{
	int32 Total = 0;

	while (Total < Size)
	{
		int32 Read = 0;
		if (!Socket->Recv(Data + Total, Size - Total, Read))
			return false;

		if (Read <= 0)
			return false;

		Total += Read;
	}
	return true;
}

bool FMatchDataFetcher::ParseJsonSafe(const FString& JsonString, TSharedPtr<FJsonValue>& OutRoot)
{
	if (JsonString.IsEmpty()) return false;
	
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	return FJsonSerializer::Deserialize(Reader, OutRoot);
}

bool FMatchDataFetcher::ParseJsonArraySafe(const FString& JsonString, TArray<TSharedPtr<FJsonValue>>& OutArray)
{
	if (JsonString.IsEmpty()) return false;

	const TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(JsonString);

	return FJsonSerializer::Deserialize(Reader, OutArray);
}
