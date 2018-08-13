// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "XD_SavePlayerBase.generated.h"

/**
 * 
 */
UCLASS()
class XD_SAVEGAMESYSTEM_API UXD_SavePlayerBase : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "存档")
	FARPG_Recorder PlayerCharacterData;

	UPROPERTY(BlueprintReadWrite, Category = "存档")
	FARPG_Recorder PlayerControllerData;

	UPROPERTY(BlueprintReadWrite, Category = "存档")
	FIntVector OldWorldOrigin;

	UFUNCTION(BlueprintCallable, Category = "游戏|存档")
	void SavePlayer(APawn* PlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "游戏|存档")
	class APawn* LoadPlayer(APlayerController* PlayerController);
	
	
};
