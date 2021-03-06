﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <Subsystems/GameInstanceSubsystem.h>
#include "XD_SaveGameSystemBase.generated.h"

/**
 * 
 */

class ULevel;

UCLASS()
class XD_SAVEGAMESYSTEM_API UXD_SaveGameSystemBase : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	friend class UXD_SaveLevelBase;
	friend class UXD_SavePlayerBase;
	friend class UXD_SaveGameFunctionLibrary;
	friend class UXD_SG_WorldSettingsComponent;
	friend struct FXD_ReadArchive;
	friend class UXD_AutoSavePlayerLambda;

protected:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;
public:
	UXD_SaveGameSystemBase();
	
	static UXD_SaveGameSystemBase* Get(const UObject* WorldContextObject);

	UPROPERTY(EditAnywhere, Category = "存档系统")
	uint8 bEnableAutoSave : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "存档系统")
	uint8 bInvokeLoadGame : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "存档系统")
	FString SaveGameFilePerfix = TEXT("SaveData1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "存档系统")
	int32 UserIndex;

	FString MakeFullSlotName(FString SlotCategory, FString SlotName) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "存档系统")
	TSubclassOf<class UXD_SaveLevelBase> SaveLevelClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "存档系统")
	TSubclassOf<class UXD_SavePlayerBase> SavePlayerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "存档系统")
	float SplitFrameLoadActorLimitSeconds = 0.005f;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreLevelUnload, ULevel*);
	FOnPreLevelUnload OnPreLevelUnload;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadLevelCompleted, ULevel*);
	FOnLoadLevelCompleted OnLoadLevelCompleted;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnInitLevelCompleted, ULevel*);
	FOnInitLevelCompleted OnInitLevelCompleted;
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

	static bool IsAutoSaveLevel(UObject* WorldContextObject);
private:
	void LoadLevelOrInitLevel(ULevel* Level, const bool SplitFrameLoadOrInitActors);

	void WhenActorSpawnInitActor(UWorld* World);

	static void NotifyActorAndComponentInit(AActor* Actor);

	static FString MakeLevelSlotName(ULevel* Level);

	bool SaveLevel(ULevel* Level) const;

	static bool CanSaveLevel(ULevel* Level);

	struct FLoadLevelGuard
	{
		FLoadLevelGuard(ULevel* Level);
		~FLoadLevelGuard();
		TWeakObjectPtr<ULevel> Level;
	};
	struct FInitLevelGuard
	{
		FInitLevelGuard(ULevel* Level);
		~FInitLevelGuard();
		TWeakObjectPtr<ULevel> Level;
	};

	static bool IsLevelInitCompleted(ULevel* Level);

private:
	bool SavePlayer(class APlayerController* Player, APawn* Pawn = nullptr, class APlayerState* PlayerState = nullptr);

	bool SaveAllPlayer(const UObject* WorldContextObject);

	APawn* TryLoadPlayer(class APlayerController* Player);

	void RegisterAutoSavePlayer(class APawn* Pawn);

protected:
	void SetSaveGameVersion(int32 Version);
};

UCLASS()
class UXD_AutoSavePlayerLambda : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	class APlayerController* PlayerContoller;

	UPROPERTY()
	class APlayerState* PlayerState;

	UFUNCTION()
	void WhenPlayerLeaveGame(AActor* Actor, EEndPlayReason::Type EndPlayReason);
};
