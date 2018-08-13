// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SavePlayerBase.h"
#include <Kismet/GameplayStatics.h>
#include "XD_SaveGameFunctionLibrary.h"
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>




void UXD_SavePlayerBase::SavePlayer(APawn* PlayerCharacter)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController()))
	{
		TArray<UObject*> ReferenceCollection;
		PlayerControllerData = UXD_SaveGameFunctionLibrary::SerializeObject(PlayerController, PlayerController->GetLevel(), ReferenceCollection);
		PlayerCharacterData = UXD_SaveGameFunctionLibrary::SerializeObject(PlayerCharacter, PlayerCharacter->GetLevel(), ReferenceCollection);
		OldWorldOrigin = UGameplayStatics::GetWorldOriginLocation(PlayerCharacter);
	}
}

class APawn* UXD_SavePlayerBase::LoadPlayer(APlayerController* PlayerController)
{
	TArray<UObject*> ReferenceCollection;
	UXD_SaveGameFunctionLibrary::DeserializeExistObject(PlayerControllerData, PlayerController, PlayerController->GetLevel(), ReferenceCollection, OldWorldOrigin);
	APawn* PlayerCharacter = CastChecked<APawn>(UXD_SaveGameFunctionLibrary::DeserializeObject(PlayerCharacterData, PlayerController->GetLevel(), ReferenceCollection, OldWorldOrigin));
	PlayerController->Possess(PlayerCharacter);

	return PlayerCharacter;
}
