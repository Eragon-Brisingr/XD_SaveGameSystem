// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveGameInterface.h"
#include "XD_SaveGameSystemUtility.h"
#include "XD_DebugFunctionLibrary.h"


const FGuid FXD_SaveGameVersion::Guid = FGuid(0x717F9EE7, 0xE9B0493A, 0x88B39123, 0x1B382222);

const FName FXD_SaveGameVersion::FriendlyName = TEXT("XD_SaveGame");

int32 FXD_SaveGameVersion::Version = FXD_SaveGameVersion::Original;


// Add default functionality here for any IXD_SaveGameInterface functions that are not pure virtual.

//Init的时候可能会SpawnActor，不能用bool
int32 WhenGameInitIsOverride;

void IXD_SaveGameInterface::GameInit(UObject* Object)
{
	WhenGameInitIsOverride += 1;
	int32 ChachWhenGameInitIsOverride = WhenGameInitIsOverride;

	IXD_SaveGameInterface::Execute_WhenGameInit(Object);

	if (ChachWhenGameInitIsOverride == WhenGameInitIsOverride)
	{
		SaveGameSystem_Display_Log("初始化%s", *UXD_DebugFunctionLibrary::GetDebugName(Object));
		WhenGameInitIsOverride -= 1;
	}
}

void IXD_SaveGameInterface::WhenGameInit_Implementation()
{
	WhenGameInitIsOverride -= 1;
}

bool IXD_SaveGameInterface::NeedSave_Implementation() const
{
	if (const AActor* Actor = Cast<AActor>(this))
	{
		return Actor->GetOwner() ? false : true;
	}
	else
	{
		return true;
	}
}

void IXD_SaveGameInterface::WhenPostLoad_Implementation()
{

}

int32 IXD_SaveGameInterface::GetSaveGameVersion(FArchive& Archive)
{
	return Archive.CustomVer(FXD_SaveGameVersion::Guid);
}
