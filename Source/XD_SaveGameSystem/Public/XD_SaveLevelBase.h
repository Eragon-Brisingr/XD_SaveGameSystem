// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <GameFramework/SaveGame.h>
#include "XD_SaveLevelBase.generated.h"

/**
 * 
 */
UCLASS()
class XD_SAVEGAMESYSTEM_API UXD_SaveLevelBase : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "存档")
	TArray<FARPG_Recorder> ActorRecorders;

	UPROPERTY(BlueprintReadWrite, Category = "存档")
	FIntVector OldWorldOrigin;

	UFUNCTION(BlueprintCallable, Category = "游戏|存档")
	bool SaveLevel(ULevel* OuterLevel);

	UFUNCTION(BlueprintCallable, Category = "游戏|读档")
	void LoadLevel(ULevel* OuterLevel, const bool SplitFrameLoadActors);
	
	
};
