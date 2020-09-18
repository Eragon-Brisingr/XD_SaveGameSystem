// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SavePlayerBase.h"
#include <Kismet/GameplayStatics.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <TimerManager.h>

#include "XD_SaveGameFunctionLibrary.h"
#include "XD_SaveGameSystemBase.h"
#include "XD_SaveGameSystemUtility.h"
#include "XD_SaveGameInterface.h"




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
	PlayerControllerRotation = PlayerController->GetControlRotation();

	FString PlayerSaveSlotName = GetPlayerSaveSlotName(PlayerState);

	UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(PlayerController);

	bool Result = UGameplayStatics::SaveGameToSlot(this, GetFullPlayerSlotName(PlayerState), SaveGameSystem->UserIndex);

	SaveGameSystem_Display_Log("保存玩家[%s]，角色%s，控制器%s", *PlayerSaveSlotName, *Pawn->GetName(), *PlayerController->GetName());

	return Result;
}

class APawn* UXD_SavePlayerBase::LoadPlayer_Implementation(APlayerController* PlayerController)
{
	TArray<UObject*> ReferenceCollection;
	TArray<UObject*> ObjectExecuteWhenLoadOrder;
	UXD_SaveGameFunctionLibrary::DeserializeExistObject(PlayerControllerData, PlayerController, PlayerController->GetLevel(), OldWorldOrigin, ReferenceCollection, ObjectExecuteWhenLoadOrder);
	APawn* PlayerCharacter = CastChecked<APawn>(UXD_SaveGameFunctionLibrary::DeserializeObject(PlayerCharacterData, PlayerController->GetLevel(), OldWorldOrigin, ReferenceCollection, ObjectExecuteWhenLoadOrder));
	PlayerController->Possess(PlayerCharacter);

	PlayerController->GetWorld()->GetTimerManager().SetTimerForNextTick([=]
	{
		PlayerController->SetControlRotation(PlayerControllerRotation);
	});

	for (UObject* Object : ObjectExecuteWhenLoadOrder)
	{
		if (Object && Object->Implements<UXD_SaveGameInterface>())
		{
			IXD_SaveGameInterface::WhenPostLoad(Object);
		}
	}

	SaveGameSystem_Display_Log("读取玩家[%s]，角色%s，控制器%s", *GetPlayerSaveSlotName(PlayerController->PlayerState), *PlayerCharacter->GetName(), *PlayerController->GetName());

	return PlayerCharacter;
}
