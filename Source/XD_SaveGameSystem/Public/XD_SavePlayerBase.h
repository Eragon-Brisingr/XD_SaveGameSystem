// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <GameFramework/SaveGame.h>
#include "XD_SavePlayerBase.generated.h"

/**
 * 
 */
USTRUCT()
struct XD_SAVEGAMESYSTEM_API FXD_SavePlayerData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	APawn* Pawn;

	UPROPERTY(SaveGame)
	APlayerController* PlayerController;
};

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
	FRotator PlayerControllerRotation;

	UPROPERTY(BlueprintReadWrite, Category = "存档")
	FIntVector OldWorldOrigin;

public:
	UFUNCTION(BlueprintNativeEvent, Category = "游戏|存档")
	FString GetPlayerSaveSlotName(class APlayerState* PlayerState) const;
	virtual FString GetPlayerSaveSlotName_Implementation(class APlayerState* PlayerState) const;

	FString GetFullPlayerSlotName(class APlayerState* PlayerState) const;

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|存档")
	bool SavePlayer(APlayerController* PlayerController, APawn* Pawn, class APlayerState* PlayerState);
	virtual bool SavePlayer_Implementation(APlayerController* PlayerController, APawn* Pawn, class APlayerState* PlayerState);

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|存档")
	class APawn* LoadPlayer(APlayerController* PlayerController);
	virtual class APawn* LoadPlayer_Implementation(APlayerController* PlayerController);
	
};
