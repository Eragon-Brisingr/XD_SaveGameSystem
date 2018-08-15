// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XD_SaveGameRecorder.h"
#include "XD_SaveGameFunctionLibrary.generated.h"

/**
 * 
 */

UCLASS()
class XD_SAVEGAMESYSTEM_API UXD_SaveGameFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	//序列化与反序列化的顺序必须一致
public:
	UFUNCTION(BlueprintCallable, Category = "游戏|存档", meta = (AutoCreateRefTerm = "ReferenceCollection"))
	static FXD_SaveGameRecorder SerializeObject(UObject* Object, ULevel* Level, UPARAM(Ref)TArray<UObject*>& ReferenceCollection);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档", meta = (AutoCreateRefTerm = "ReferenceCollection"))
	static UObject* DeserializeObject(const FXD_SaveGameRecorder& XD_Recorder, ULevel* Level, UPARAM(Ref)TArray<UObject*>& ReferenceCollection, const FIntVector& OldWorldOrigin);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档", meta = (AutoCreateRefTerm = "ReferenceCollection", DeterminesOutputType = "Object"))
	static UObject* DeserializeExistObject(const FXD_SaveGameRecorder& XD_Recorder, UObject* Object, ULevel* Level, UPARAM(Ref)TArray<UObject*>& ReferenceCollection, const FIntVector& OldWorldOrigin);
	
public:
	UFUNCTION(BlueprintPure, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static class UXD_SaveGameSystemBase* GetSaveGameSystem(UObject* WorldContextObject);

	//在GameMode执行BeginPlay时执行
	UFUNCTION(BlueprintCallable, Category = "游戏|存档")
	static bool InitAutoSaveLoadSystem(class AGameModeBase* GameMode);

	//在GameMode执行EndPlay时执行
	UFUNCTION(BlueprintCallable, Category = "游戏|存档")
	static bool ShutDownAutoSaveLoadSystem(class AGameModeBase* GameMode);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static bool InvokeSaveGame(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static bool InvokeLoadGame(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static bool IsInvokeLoadGame(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static FString MakeFullSlotName(UObject* WorldContextObject, FString SlotCategory, FString SlotName);

	UFUNCTION(BlueprintPure, Category = "游戏|功能", meta = (WorldContext = "WorldContextObject"))
	static bool IsLevelInitCompleted(ULevel* Level);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档")
	static bool InvokeSavePlayer(class APlayerController* Player);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档")
	static bool InvokeSaveAllPlayer(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档")
	static class APawn* TryLoadPlayer(class APlayerController* Player);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档")
	static void RegisterAutoSavePlayer(class APawn* Pawn);
};
