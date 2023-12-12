#include "MTSReader.h"
#include "Runtime/JsonUtilities/Public/JsonObjectConverter.h"


TSharedPtr<FMTSHandler> FMTSHandler::MTSInst;

TSharedPtr<FMTSHandler> FMTSHandler::Get()
{
	if (!MTSInst.IsValid()) {
		MTSInst = MakeShareable(new FMTSHandler);
	}
	return MTSInst;
}

void FMTSHandler::GetMTSData(FString& ReceivedJson)
{
	UE_LOG(LogTemp, Warning, TEXT("Received Json : %s"), *ReceivedJson);
	FJsonObjectConverter::JsonObjectStringToUStruct(ReceivedJson, &FMTSHandler::Get()->MTSJson, 0, 0);
	FString ConvertedJson;
	FJsonObjectConverter::UStructToJsonObjectString(FMTSHandler::Get()->MTSJson, ConvertedJson, 0, 0);
	UE_LOG(LogTemp, Warning, TEXT("Converted Json : %s"), *ConvertedJson);
}






