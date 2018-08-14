// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "XD_SaveGameSystemBase.generated.h"

/**
 * 
 */

class ULevel;

UCLASS()
class XD_SAVEGAMESYSTEM_API UXD_SaveGameSystemBase : public UObject
{
	GENERATED_BODY()
	
	friend class UXD_SaveLevelBase;
	friend class UXD_SavePlayerBase;
	friend class UXD_SaveGameFunctionLibrary;
	friend class UXD_SG_WorldSettingsComponent;
	friend struct FXD_ReadArchive;
public:
	UXD_SaveGameSystemBase();
	
	static UXD_SaveGameSystemBase* Get(const UObject* WorldContextObject);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "游戏|存档")
	uint8 bInvokeLoadGame : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "游戏|存档")
	FString SaveGameFilePerfix = TEXT("SaveData1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "游戏|存档")
	int32 UserIndex;

	FString MakeFullSlotName(FString SlotCategory, FString SlotName) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "游戏|存档")
	TSubclassOf<class UXD_SaveLevelBase> SaveLevelClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "游戏|存档")
	TSubclassOf<class UXD_SavePlayerBase> SavePlayerClass;
private:
	//只有读取时Spawn的Actor不执行Init
	uint8 bShouldInitSpawnActor : 1;

	void StartSpawnActorWithoutInit() { bShouldInitSpawnActor = false; }

	void EndSpawnActorWithoutInit() { bShouldInitSpawnActor = true; }
private:
	void LoadGame(UObject* WorldContextObject);

	void SaveGame(UObject* WorldContextObject);

	bool CanCompletedSaveGame(UObject* WorldContextObject) const;

	FDelegateHandle OnLevelAdd_DelegateHandle;

	void InitAutoSaveLoadSystem(class AGameModeBase* GameMode);

	void ShutDownAutoSaveLoadSystem(class AGameModeBase* GameMode);

	void StopAutoSave(UObject* WorldContextObject);
private:
	void LoadLevelOrInitLevel(ULevel* Level, const bool SplitFrameLoadOrInitActors);

	void WhenActorSpawnInitActor(UWorld* World);

	static FString MakeLevelSlotName(ULevel* Level);

	bool SaveLevel(ULevel* Level) const;

	static bool CanSaveLevel(ULevel* Level);

	static void StartLoadLevel(ULevel* Level);

	static void EndLoadLevel(ULevel* Level);

	static void StartInitLevel(ULevel* Level);

	static void EndInitLevel(ULevel* Level);

	static bool IsLevelInitCompleted(ULevel* Level);

private:
	bool SavePlayer(class APlayerController* Player);

	bool SaveAllPlayer(const UObject* WorldContextObject);

	APawn* TryLoadPlayer(class APlayerController* Player);
};
