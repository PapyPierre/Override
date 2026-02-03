#pragma once

struct FMatchPlayerData;

class FMatchDataFetcher
{
public:
	static bool FetchMatch(FString VersionId, FString MatchId, FString PlayerId, FString TeamId,
		TArray<FMatchPlayerData>& OutPlayers);
	static void CloseSocket(FSocket* Socket);
	static bool SendDataToDB(FSocket* Socket, FString Payload);
	static bool RecvAll(FSocket* Socket, FString& OutResponse);
	static bool RecvData(FSocket* Socket, uint8* Data, int32 Size);
	static bool ParseJsonSafe(const FString& JsonString, TSharedPtr<FJsonValue>& OutRoot);
	static bool ParseJsonArraySafe(const FString& JsonString, TArray<TSharedPtr<FJsonValue>>& OutArray);
};


