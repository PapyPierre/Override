#include "GameMode/FirstPersonGameMode.h"

#include "HttpModule.h"
#include "SocketTypes.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "GameMode/OverrideGameInstance.h"
#include "winsock.h"
#include "GameFramework/PlayerState.h"
#include "Network/FMatchDataFetcher.h"
#include "Player/CustomPlayerController.h"

void AFirstPersonGameMode::SendDataToDB()
{
	UE_LOG(LogTemp, Error, TEXT("Trying to send match data to DB..."));

	// BUILD JSON
	FString Version = GetVersionFromFile("C:/Users/SIG5-PROJ05/Desktop/Tchoupi_Tools/VersionInfo.txt");
	if (Version.IsEmpty()) Version = TEXT("editor");

	UOverrideGameInstance* GameInst = Cast<UOverrideGameInstance>(GetGameInstance());

	const TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField("versionId", Version);

	TArray<TSharedPtr<FJsonValue>> PlayersArray;

	for (const FMatchPlayerData& Player : GameInst->MatchPlayers)
	{
		TSharedPtr<FJsonObject> PlayerObj = MakeShared<FJsonObject>();
		PlayerObj->SetNumberField(TEXT("playerId"), Player.PlayerId);
		PlayerObj->SetNumberField(TEXT("teamId"), Player.TeamId);

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
	Request->SetURL(TEXT("http://localhost:5000/matches"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Payload);
	
	Request->OnProcessRequestComplete().BindLambda(
		[](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bSuccess)
		{
			if (!bSuccess || !Response.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("HTTP request failed"));
				return;
			}
			UE_LOG(LogTemp, Log, TEXT("Response: %s"), *Response->GetContentAsString());
		}
	);

	Request->ProcessRequest();
}

void AFirstPersonGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (ACustomPlayerController* PC = Cast<ACustomPlayerController>(Exiting))
	{
		UE_LOG(LogTemp, Warning, TEXT("Player disconnected: %s"),
			*PC->GetPlayerState<APlayerState>()->GetPlayerName());

		PC->OnLogout();
	}
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
