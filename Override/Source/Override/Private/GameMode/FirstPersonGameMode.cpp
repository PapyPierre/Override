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

void AFirstPersonGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	HttpClient = NewObject<UServerHttpClient>();
}

void AFirstPersonGameMode::SendDataToDB()
{
	FString Version = GetVersionFromFile("C:/Users/SIG5-PROJ05/Desktop/Tchoupi_Tools/VersionInfo.txt");
	UOverrideGameInstance* GameInst = Cast<UOverrideGameInstance>(GetGameInstance());

	HttpClient->SetMatchData(Version, GameInst);
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
