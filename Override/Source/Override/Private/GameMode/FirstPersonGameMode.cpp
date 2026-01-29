#include "GameMode/FirstPersonGameMode.h"
#include "SocketTypes.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Networking.h"
#include "GameMode/OverrideGameInstance.h"
#include "winsock.h"

void AFirstPersonGameMode::SendDataToDB()
{
	FSocket* Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)
		->CreateSocket(NAME_Stream, TEXT("StatsSocket"), false);

	FIPv4Address IP;
	FIPv4Address::Parse(TEXT("10.51.1.111"), IP);

	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

#undef SetPort // In winsock.h to avoid conflicts

	Addr->SetIp(IP.Value);
	Addr->SetPort(5000);

	if (Socket->Connect(*Addr))
	{
		UE_LOG(LogTemp, Log, TEXT("Connected to %s successfully in order to send match data"),
		       *Addr->ToString(true));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to connected to %s, will not send match data"),
		       *Addr->ToString(true));
		return;
	}

	// BUILD JSON
	FString Version = GetVersionFromFile("C:/Users/SIG5-PROJ05/Desktop/Tchoupi_Tools/VersionInfo.txt");
	if (Version.IsEmpty()) Version = TEXT("editor");

	const TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField("action", "set_match_info");
	Json->SetStringField("version_id", Version);
	Json->SetStringField("token", "override_db_token");

	UOverrideGameInstance* GameInst = Cast<UOverrideGameInstance>(GetGameInstance());

	TArray<TSharedPtr<FJsonValue>> PlayersArray;

	for (const FMatchPlayerData& Player : GameInst->MatchPlayers)
	{
		TSharedPtr<FJsonObject> PlayerObj = MakeShared<FJsonObject>();
		PlayerObj->SetNumberField(TEXT("player_id"), Player.PlayerId);
		PlayerObj->SetNumberField(TEXT("team_id"), Player.TeamId);

		TArray<TSharedPtr<FJsonValue>> PositionsArray;
		for (const FPlayerPosition& Pos : Player.Positions)
		{
			TArray<TSharedPtr<FJsonValue>> PosArray;
			PosArray.Add(MakeShared<FJsonValueNumber>(Pos.Time));
			PosArray.Add(MakeShared<FJsonValueNumber>(Pos.Position.X));
			PosArray.Add(MakeShared<FJsonValueNumber>(Pos.Position.Y));
			PosArray.Add(MakeShared<FJsonValueNumber>(Pos.Position.Z));

			PositionsArray.Add(MakeShared<FJsonValueArray>(PosArray));
		}

		PlayerObj->SetArrayField(TEXT("positions"), PositionsArray);
		PlayersArray.Add(MakeShared<FJsonValueObject>(PlayerObj));
	}

	Json->SetArrayField(TEXT("players"), PlayersArray);

	FString Payload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	// SEND
	FTCHARToUTF8 Convert(*Payload);
	int32 PayloadSize = Convert.Length();

	uint32 NetSize = htonl(PayloadSize);

	int32 Sent = 0;
	Socket->Send((uint8*)&NetSize, sizeof(uint32), Sent);
	Socket->Send((uint8*)Convert.Get(), PayloadSize, Sent);

	FPlatformProcess::Sleep(0.05f);

	// RECEIVE
	FString Response;
	RecvAll(Socket, Response);

	if (Response.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Empty response from DB server"));
		goto cleanup;
	}

	UE_LOG(LogTemp, Log, TEXT("DB server response:\n%s"), *Response);

	if (TSharedPtr<FJsonObject> ResponseJson; !ParseJsonSafe(Response, ResponseJson))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid JSON from DB server"));
	}

cleanup:
	Socket->Close();
	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
}

bool AFirstPersonGameMode::RecvAll(FSocket* Socket, FString& OutResponse)
{
	OutResponse.Empty();

	uint32 PendingSize = 0;
	while (Socket->HasPendingData(PendingSize))
	{
		TArray<uint8> Buffer;
		Buffer.SetNumUninitialized(FMath::Min(PendingSize, 65536u));

		int32 BytesRead = 0;
		if (!Socket->Recv(Buffer.GetData(), Buffer.Num(), BytesRead))
		{
			return false;
		}

		if (BytesRead > 0)
		{
			OutResponse.Append(FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer.GetData()))));
		}
	}

	return true;
}

bool AFirstPersonGameMode::ParseJsonSafe(const FString& JsonString, TSharedPtr<FJsonObject>& OutJson)
{
	if (JsonString.IsEmpty())return false;

	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	return FJsonSerializer::Deserialize(Reader, OutJson) && OutJson.IsValid();
}

FString AFirstPersonGameMode::GetVersionFromFile(const FString& FilePath)
{
	FString FileContent;

	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		return FString();
	}

	int32 Major = 0;
	int32 Minor = 0;
	int32 Revision = 0;
	int32 Patch = 0;

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines);

	for (const FString& Line : Lines)
	{
		FString Key, Value;

		if (!Line.Split(TEXT("="), &Key, &Value))
		{
			continue;
		}

		if (Key.Equals(TEXT("Major"), ESearchCase::IgnoreCase))
		{
			Major = FCString::Atoi(*Value);
		}
		else if (Key.Equals(TEXT("Minor"), ESearchCase::IgnoreCase))
		{
			Minor = FCString::Atoi(*Value);
		}
		else if (Key.Equals(TEXT("Revision"), ESearchCase::IgnoreCase))
		{
			Revision = FCString::Atoi(*Value);
		}
		else if (Key.Equals(TEXT("Patch"), ESearchCase::IgnoreCase))
		{
			Patch = FCString::Atoi(*Value);
		}
	}

	return FString::Printf(TEXT("%d.%d.%d.%d"), Major, Minor, Revision, Patch);
}
