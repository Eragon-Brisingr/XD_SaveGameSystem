// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveGameFunctionLibrary.h"
#include "XD_SaveGameSystemBase.h"
#include "XD_ReadArchive.h"
#include <MemoryReader.h>
#include "XD_WriteArchive.h"
#include <BufferArchive.h>
#include <GameFramework/GameModeBase.h>


FXD_SaveGameRecorder UXD_SaveGameFunctionLibrary::SerializeObject(UObject* Object, ULevel* Level, TArray<UObject*>& ReferenceCollection)
{
	TArray<UObject*> ObjectList;

	FBufferArchive BufferArchive;

	FXD_WriteArchive ARPG_WriteArchive(BufferArchive, Level, ReferenceCollection);
	ARPG_WriteArchive << Object;

	return BufferArchive;
}

UObject* UXD_SaveGameFunctionLibrary::DeserializeObject(const FXD_SaveGameRecorder& ARPG_Recorder, ULevel* Level, TArray<UObject*>& ReferenceCollection, const FIntVector& OldWorldOrigin)
{
	FMemoryReader MemoryReader(ARPG_Recorder.Data);

	FXD_ReadArchive ARPG_ReadArchive(MemoryReader, Level, ReferenceCollection, OldWorldOrigin);
	UObject* Object = nullptr;
	ARPG_ReadArchive << Object;
	return Object;
}

UObject* UXD_SaveGameFunctionLibrary::DeserializeExistObject(const FXD_SaveGameRecorder& ARPG_Recorder, UObject* Object, ULevel* Level, TArray<UObject*>& ReferenceCollection, const FIntVector& OldWorldOrigin)
{
	FMemoryReader MemoryReader(ARPG_Recorder.Data);

	FXD_ReadArchive ARPG_ReadArchive(MemoryReader, Level, ReferenceCollection, OldWorldOrigin);
	ARPG_ReadArchive << Object;
	return Object;
}

UXD_SaveGameSystemBase* UXD_SaveGameFunctionLibrary::GetSaveGameSystem(UObject* WorldContextObject)
{
	return UXD_SaveGameSystemBase::Get(WorldContextObject);
}

bool UXD_SaveGameFunctionLibrary::InitAutoSaveLoadSystem(class AGameModeBase* GameMode)
{
	if (GameMode)
	{
		if (UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(GameMode))
		{
			SaveGameSystem->InitAutoSaveLoadSystem(GameMode);
			return true;
		}
	}
	return false;
}

bool UXD_SaveGameFunctionLibrary::ShutDownAutoSaveLoadSystem(class AGameModeBase* GameMode)
{
	if (GameMode)
	{
		if (UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(GameMode))
		{
			SaveGameSystem->ShutDownAutoSaveLoadSystem(GameMode);
			return true;
		}
	}
	return false;
}

bool UXD_SaveGameFunctionLibrary::InvokeSaveGame(UObject* WorldContextObject)
{
	if (WorldContextObject->GetWorld()->IsServer())
	{
		if (UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(WorldContextObject))
		{
			SaveGameSystem->SaveGame(WorldContextObject);
			return true;
		}
	}
	return false;
}

bool UXD_SaveGameFunctionLibrary::InvokeLoadGame(UObject* WorldContextObject)
{
	if (WorldContextObject->GetWorld()->IsServer())
	{
		if (UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(WorldContextObject))
		{
			SaveGameSystem->LoadGame(WorldContextObject);
			return true;
		}
	}
	return false;
}

bool UXD_SaveGameFunctionLibrary::IsInvokeLoadGame(UObject* WorldContextObject)
{
	return GetSaveGameSystem(WorldContextObject)->bInvokeLoadGame;
}

FString UXD_SaveGameFunctionLibrary::MakeFullSlotName(UObject* WorldContextObject, FString SlotCategory, FString SlotName)
{
	return GetSaveGameSystem(WorldContextObject)->MakeFullSlotName(SlotCategory, SlotName);
}

bool UXD_SaveGameFunctionLibrary::IsLevelInitCompleted(ULevel* Level)
{
	return UXD_SaveGameSystemBase::IsLevelInitCompleted(Level);
}

bool UXD_SaveGameFunctionLibrary::InvokeSavePlayer(class APlayerController* Player)
{
	if (Player)
	{
		UXD_SaveGameSystemBase::Get(Player)->SavePlayer(Player);
	}
	return false;
}

bool UXD_SaveGameFunctionLibrary::InvokeSaveAllPlayer(const UObject* WorldContextObject)
{
	return UXD_SaveGameSystemBase::Get(WorldContextObject)->SaveAllPlayer(WorldContextObject);
}

class APawn* UXD_SaveGameFunctionLibrary::TryLoadPlayer(class APlayerController* Player)
{
	if (Player)
	{
		return UXD_SaveGameSystemBase::Get(Player)->TryLoadPlayer(Player);
	}
	return nullptr;
}

void UXD_SaveGameFunctionLibrary::RegisterAutoSavePlayer(class APawn* Pawn)
{
	if (Pawn)
	{
		UXD_SaveGameSystemBase::Get(Pawn)->RegisterAutoSavePlayer(Pawn);
	}
}
