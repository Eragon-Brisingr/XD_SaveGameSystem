// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SavePlayerBase.h"
#include <Kismet/GameplayStatics.h>
#include "XD_SaveGameFunctionLibrary.h"
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include "XD_DebugFunctionLibrary.h"
#include "XD_SaveGameSystemBase.h"
#include "XD_SaveGameSystemUtility.h"




FString UXD_SavePlayerBase::GetPlayerSaveSlotName_Implementation(class APlayerState* PlayerState) const
{
	FString SlotName = PlayerState->GetPlayerName().Left(PlayerState->GetPlayerName().Find(TEXT("-"), ESearchCase::IgnoreCase, ESearchDir::FromEnd));
	return SlotName;
}

FString UXD_SavePlayerBase::GetFullPlayerSlotName(class APlayerState* PlayerState) const
{
	return UXD_SaveGameSystemBase::Get(PlayerState)->MakeFullSlotName(TEXT("Players"), GetPlayerSaveSlotName(PlayerState));
}

bool UXD_SavePlayerBase::SavePlayer_Implementation(APlayerController* PlayerController, APawn* Pawn, class APlayerState* PlayerState)
{
	TArray<UObject*> ReferenceCollection;
	PlayerControllerData = UXD_SaveGameFunctionLibrary::SerializeObject(PlayerController, PlayerController->GetLevel(), ReferenceCollection);
	PlayerCharacterData = UXD_SaveGameFunctionLibrary::SerializeObject(Pawn, Pawn->GetLevel(), ReferenceCollection);
	OldWorldOrigin = UGameplayStatics::GetWorldOriginLocation(Pawn);

	FString PlayerSaveSlotName = GetPlayerSaveSlotName(PlayerState);

	UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(PlayerController);

	bool Result = UGameplayStatics::SaveGameToSlot(this, GetFullPlayerSlotName(PlayerState), SaveGameSystem->UserIndex);

	SaveGameSystem_Display_Log("保存玩家[%s]，角色%s，控制器%s", *PlayerSaveSlotName, *UXD_DebugFunctionLibrary::GetDebugName(Pawn), *UXD_DebugFunctionLibrary::GetDebugName(PlayerController));

	return Result;
}

class APawn* UXD_SavePlayerBase::LoadPlayer_Implementation(APlayerController* PlayerController)
{
	TArray<UObject*> ReferenceCollection;
	UXD_SaveGameFunctionLibrary::DeserializeExistObject(PlayerControllerData, PlayerController, PlayerController->GetLevel(), ReferenceCollection, OldWorldOrigin);
	APawn* PlayerCharacter = CastChecked<APawn>(UXD_SaveGameFunctionLibrary::DeserializeObject(PlayerCharacterData, PlayerController->GetLevel(), ReferenceCollection, OldWorldOrigin));
	PlayerController->Possess(PlayerCharacter);

	SaveGameSystem_Display_Log("读取玩家[%s]，角色%s，控制器%s", *GetPlayerSaveSlotName(PlayerController->PlayerState), *UXD_DebugFunctionLibrary::GetDebugName(PlayerCharacter), *UXD_DebugFunctionLibrary::GetDebugName(PlayerController));

	return PlayerCharacter;
}
