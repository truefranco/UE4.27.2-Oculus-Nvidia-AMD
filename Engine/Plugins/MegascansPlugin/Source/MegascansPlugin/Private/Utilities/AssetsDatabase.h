// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"


class FSQLiteDatabase;
class FSQLiteDatabaseConnection;

struct AssetRecord {
	FString ID;
	FString Name;
	FString Path;
	FString Type;
};

class FAssetsDatabase
{
private:
	FAssetsDatabase() ;
	
	static TSharedPtr<FAssetsDatabase> DBInst;
	void CreateAssetsDatabase(const FString& DatabasePath);
	void CreateMaterialsDatabase(const FString& DatabasePath);
	TSharedPtr<FSQLiteDatabaseConnection> SQLiteDatabase;

public:
	~FAssetsDatabase();
	static TSharedPtr<FAssetsDatabase> Get();
	void AddRecord(const AssetRecord& Record);
	void AddMaterialRecord(const FString& MaterialName, float MaterialVersion);
	bool RecordExists(const FString& AssetID, AssetRecord& Record);
	float GetMaterialVersion(const FString& MaterialName);

};
