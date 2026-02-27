#include "GameMode/FirstPersonGameMode.h"
#include "SocketTypes.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "GameMode/OverrideGameInstance.h"
#include "winsock.h"
#include "Network/FMatchDataFetcher.h"

void AFirstPersonGameMode::SendDataToDB()
{
	UE_LOG(LogTemp, Error, TEXT("Trying to send match data to DB..."));
	
	FMatchDataFetcher MatchDataFetcher;
	
	FSocket* Socket = MatchDataFetcher.CreateSocket("127.0.0.1",6000);

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

	MatchDataFetcher.SendData(Socket, Payload);

	FPlatformProcess::Sleep(0.05f);
	
	FString Response;
	MatchDataFetcher.RecvAll(Socket, Response);

	if (Response.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Empty response from DB server"));
		MatchDataFetcher.CloseSocket(Socket);
	}
	
	TSharedPtr<FJsonValue> ResponseJson;
	
	if (!MatchDataFetcher.ParseJsonSafe(Response, ResponseJson))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid JSON from DB server"));
	}

	MatchDataFetcher.CloseSocket(Socket);
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
		else if (Key.Equals(TEXT("Patch"), ESearchCase::IgnoreCase))
		{
			Patch = FCString::Atoi(*Value);
		}
	}

	return FString::Printf(TEXT("%d.%d.%d"), Major, Minor, Patch);
}
