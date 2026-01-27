#include "GameMode/FirstPersonGameMode.h"
#include "SocketTypes.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Networking.h"

void AFirstPersonGameMode::SendDataToDB()
{
	FSocket* Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)
		->CreateSocket(NAME_Stream, TEXT("StatsSocket"), false);

	FIPv4Address IP;
	FIPv4Address::Parse(TEXT("10.51.1.68"), IP);

	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	Addr->SetIp(IP.Value);
	Addr->SetPort(5000);

	UE_LOG(LogTemp, Log, TEXT("Trying to connect to %s in order to send match data"),
	       *Addr->ToString(true));
	
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

	FString Version = GetVersionFromFile("C:/Users/SIG5-PROJ05/Desktop/Tchoupi_Tools/VersionInfo.txt");

	if (Version == "") Version = TEXT("editor");
	
	const TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField("action", "set_match_info");
	Json->SetStringField("version_id", Version);
	Json->SetStringField("token", "override_db_token");
	

	FString Payload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	TArray<uint8> Data;
	const FTCHARToUTF8 Convert(*Payload);
	Data.Append((uint8*)Convert.Get(), Convert.Length());

	int32 Sent = 0;
	Socket->Send(Data.GetData(), Data.Num(), Sent);

	uint32 Size;
	Socket->HasPendingData(Size);

	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(Size);

	int32 Read = 0;
	Socket->Recv(Buffer.GetData(), Buffer.Num(), Read);

	const FString Response = FString(UTF8_TO_TCHAR(Buffer.GetData()));

	TSharedPtr<FJsonObject> ResponseJson;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);

	FJsonSerializer::Deserialize(Reader, ResponseJson);

	int32 MatchId = ResponseJson->Values["match_id"].Get()->AsNumber();

	UE_LOG(LogTemp, Log, TEXT("Match %i data successfully sent to database server"), MatchId);

	Socket->Close();
	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
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
