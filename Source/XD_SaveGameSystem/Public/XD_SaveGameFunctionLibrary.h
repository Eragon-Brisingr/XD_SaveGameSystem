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
	UFUNCTION(BlueprintCallable, Category = "ARPG|Serialize", meta = (AutoCreateRefTerm = "ReferenceCollection"))
	static FARPG_Recorder SerializeObject(UObject* Object, ULevel* Level, UPARAM(Ref)TArray<UObject*>& ReferenceCollection);

	UFUNCTION(BlueprintCallable, Category = "ARPG|Serialize", meta = (AutoCreateRefTerm = "ReferenceCollection"))
	static UObject* DeserializeObject(const FARPG_Recorder& ARPG_Recorder, ULevel* Level, UPARAM(Ref)TArray<UObject*>& ReferenceCollection, const FIntVector& OldWorldOrigin);

	UFUNCTION(BlueprintCallable, Category = "ARPG|Serialize", meta = (AutoCreateRefTerm = "ReferenceCollection", DeterminesOutputType = "Object"))
	static UObject* DeserializeExistObject(const FARPG_Recorder& ARPG_Recorder, UObject* Object, ULevel* Level, UPARAM(Ref)TArray<UObject*>& ReferenceCollection, const FIntVector& OldWorldOrigin);
	
public:
	UFUNCTION(BlueprintCallable, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static class UXD_SaveGameSystemBase* GetSaveGameSystem(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static void InvokeSaveGame(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static void InvokeLoadGame(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static bool IsInvokeLoadGame(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "游戏|存档", meta = (WorldContext = "WorldContextObject"))
	static FString MakeFullSlotName(UObject* WorldContextObject, FString SlotCategory, FString SlotName);

	UFUNCTION(BlueprintPure, Category = "游戏|功能", meta = (WorldContext = "WorldContextObject"))
	static bool IsLevelInitCompleted(ULevel* Level);
};
