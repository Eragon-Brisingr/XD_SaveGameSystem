// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveGameFunctionLibrary.h"
#include "XD_SaveGameSystemBase.h"
#include "XD_ReadArchive.h"
#include <MemoryReader.h>
#include "XD_WriteArchive.h"
#include <BufferArchive.h>


FARPG_Recorder UXD_SaveGameFunctionLibrary::SerializeObject(UObject* Object, ULevel* Level, TArray<UObject*>& ReferenceCollection)
{
	TArray<UObject*> ObjectList;

	FBufferArchive BufferArchive;

	FXD_WriteArchive ARPG_WriteArchive(BufferArchive, Level, ReferenceCollection);
	ARPG_WriteArchive << Object;

	return BufferArchive;
}

UObject* UXD_SaveGameFunctionLibrary::DeserializeObject(const FARPG_Recorder& ARPG_Recorder, ULevel* Level, TArray<UObject*>& ReferenceCollection, const FIntVector& OldWorldOrigin)
{
	FMemoryReader MemoryReader(ARPG_Recorder.Data);

	FXD_ReadArchive ARPG_ReadArchive(MemoryReader, Level, ReferenceCollection, OldWorldOrigin);
	UObject* Object = nullptr;
	ARPG_ReadArchive << Object;
	return Object;
}

UObject* UXD_SaveGameFunctionLibrary::DeserializeExistObject(const FARPG_Recorder& ARPG_Recorder, UObject* Object, ULevel* Level, TArray<UObject*>& ReferenceCollection, const FIntVector& OldWorldOrigin)
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

void UXD_SaveGameFunctionLibrary::InvokeSaveGame(UObject* WorldContextObject)
{
	if (WorldContextObject->GetWorld()->IsServer())
	{
		GetSaveGameSystem(WorldContextObject)->SaveGame(WorldContextObject);
	}
}

void UXD_SaveGameFunctionLibrary::InvokeLoadGame(UObject* WorldContextObject)
{
	if (WorldContextObject->GetWorld()->IsServer())
	{
		GetSaveGameSystem(WorldContextObject)->LoadGame(WorldContextObject);
	}
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
