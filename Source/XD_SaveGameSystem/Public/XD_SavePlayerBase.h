// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <GameFramework/SaveGame.h>
#include "XD_SavePlayerBase.generated.h"

/**
 * 
 */
UCLASS()
class XD_SAVEGAMESYSTEM_API UXD_SavePlayerBase : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "存档")
	FXD_SaveGameRecorder PlayerCharacterData;

	UPROPERTY(BlueprintReadWrite, Category = "存档")
	FXD_SaveGameRecorder PlayerControllerData;

	UPROPERTY(BlueprintReadWrite, Category = "存档")
	FIntVector OldWorldOrigin;

public:
	UFUNCTION(BlueprintNativeEvent, Category = "游戏|存档")
	class APawn* GetControlledPawn(APlayerController* PlayerController) const;
	virtual class APawn* GetControlledPawn_Implementation(APlayerController* PlayerController) const;

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|存档")
	FString GetPlayerSaveSlotName(APlayerController* PlayerController) const;
	virtual FString GetPlayerSaveSlotName_Implementation(APlayerController* PlayerController) const;

	FString GetFullPlayerSlotName(APlayerController* PlayerController) const;

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|存档")
	bool SavePlayer(APlayerController* PlayerController);
	virtual bool SavePlayer_Implementation(APlayerController* PlayerController);

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|存档")
	class APawn* LoadPlayer(APlayerController* PlayerController);
	virtual class APawn* LoadPlayer_Implementation(APlayerController* PlayerController);
	
};
