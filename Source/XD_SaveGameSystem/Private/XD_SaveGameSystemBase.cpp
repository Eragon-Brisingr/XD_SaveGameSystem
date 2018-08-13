// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveGameSystemBase.h"
#include "XD_SaveGameSystemInterface.h"
#include "XD_SG_WorldSettingsComponent.h"
#include "XD_LevelFunctionLibrary.h"
#include <GameFramework/WorldSettings.h>
#include "XD_SaveGameSystemUtility.h"
#include "XD_SaveLevelBase.h"
#include <GameFramework/GameModeBase.h>
#include <Kismet/KismetSystemLibrary.h>
#include <Kismet/GameplayStatics.h>


UXD_SaveGameSystemBase::UXD_SaveGameSystemBase()
	:SaveLevelClass(UXD_SaveLevelBase::StaticClass()),
	bShouldInitSpawnActor(true)
{

}

UXD_SaveGameSystemBase* UXD_SaveGameSystemBase::Get(UObject* WorldContextObject)
{
	UGameInstance* GameInstance = WorldContextObject->GetWorld()->GetGameInstance();
	if (GameInstance->Implements<UXD_SaveGame_GameInstanceInterface>())
	{
		return IXD_SaveGame_GameInstanceInterface::Execute_GetSaveGameSystem(GameInstance);
	}

	return nullptr;
}

FString UXD_SaveGameSystemBase::MakeFullSlotName(FString SlotCategory, FString SlotName) const
{
	if (SlotCategory.Len() > 0)
	{
		return SaveGameFilePerfix + TEXT("/") + SlotCategory + TEXT("/") + SlotName;
	}
	else
	{
		return SaveGameFilePerfix + TEXT("/") + SlotName;
	}
}

void UXD_SaveGameSystemBase::InitAutoSaveLoadSystem(class AGameModeBase* GameMode)
{
	UWorld* World = GameMode->GetWorld();

	SaveGameSystem_Display_Log("---------------------------------------------初始化主世界[%s]-------------------------------------------------------", *World->GetName());

	for (ULevel* Level : World->GetLevels())
	{
		LoadLevelOrInitLevel(Level, false);
	}

	OnLevelAdd_DelegateHandle = FWorldDelegates::LevelAddedToWorld.AddLambda([this](ULevel* Level, UWorld* World)
	{
		//由于绑的是全局回调，PIE内没法分辨是不是为Server，在这里再做判断
		if (Level->GetWorld()->IsServer() == false)
			return;

		LoadLevelOrInitLevel(Level, true);
	});

	WhenActorSpawnInitActor(World);
}

void UXD_SaveGameSystemBase::ShutDownAutoSaveLoadSystem(class AARPGGameMode* ARPG_GameMode)
{
	FWorldDelegates::LevelAddedToWorld.Remove(OnLevelAdd_DelegateHandle);
}

void UXD_SaveGameSystemBase::StopAutoSave(UObject* WorldContextObject)
{
	for (ULevel* Level : WorldContextObject->GetWorld()->GetLevels())
	{
		if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = UXD_LevelFunctionLibrary::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
		{
			WorldSettingsComponent->bActiveAutoSave = false;
		}
	}
}

void UXD_SaveGameSystemBase::LoadGame(UObject* WorldContextObject)
{
	StopAutoSave(WorldContextObject);

	UXD_SaveGameSystemBase::Get(WorldContextObject)->bInvokeLoadGame = true;
	UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("restartlevel"));
}

void UXD_SaveGameSystemBase::SaveGame(UObject* WorldContextObject)
{
	//TODO 保存版本号

	//保存所有加载的关卡
	for (ULevel* Level : WorldContextObject->GetWorld()->GetLevels())
	{
		if (CanSaveLevel(Level))
		{
			SaveLevel(Level);
		}
	}

// 	//保存所有玩家
// 	if (AARPGGameMode* ARPGGameMode = Cast<AARPGGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
// 	{
// 		ARPGGameMode->SaveAllPlayer();
// 	}
}

bool UXD_SaveGameSystemBase::CanCompletedSaveGame(UObject* WorldContextObject) const
{
	for (ULevel* Level : WorldContextObject->GetWorld()->GetLevels())
	{
		if (CanSaveLevel(Level) == false)
		{
			return false;
		}
	}
	return true;
}

void UXD_SaveGameSystemBase::LoadLevelOrInitLevel(ULevel* Level, const bool SplitFrameLoadOrInitActors)
{
	if (bInvokeLoadGame && UGameplayStatics::DoesSaveGameExist(MakeLevelSlotName(Level), UserIndex))
	{
		UXD_SaveLevelBase* SaveLevel = Cast<UXD_SaveLevelBase>(UGameplayStatics::LoadGameFromSlot(MakeLevelSlotName(Level), UserIndex));
		SaveLevel->LoadLevel(Level, SplitFrameLoadOrInitActors);
	}
	else
	{
		if (SplitFrameLoadOrInitActors)
		{
			struct FSplitFrameInitActorsHelper
			{
				FSplitFrameInitActorsHelper(ULevel* Level)
					:Level(Level)
				{
					StartInitLevel(Level);
				}

				TWeakObjectPtr<ULevel> Level;

				TArray<TWeakObjectPtr<AActor>> InvokeToInitActors;

				bool InitActorTick(float DeltaSecond)
				{
					constexpr double TimeLimit = 0.005;

					double StartTime = FPlatformTime::Seconds();
					int InitCount = 0;
					while (InitCount < InvokeToInitActors.Num())
					{
						if (AActor* Actor = InvokeToInitActors[InitCount].Get())
						{
							IXD_SaveGameInterface::GameInit(Actor);
						}
						++InitCount;

						if (FPlatformTime::Seconds() - StartTime > TimeLimit)
						{
							break;
						}
					}
					InvokeToInitActors.RemoveAt(0, InitCount);

					if (InvokeToInitActors.Num() != 0)
					{
						return true;
					}
					else
					{
						if (Level.IsValid())
						{
							EndInitLevel(Level.Get());
						}

						delete this;
						return false;
					}
				}
			};

			FSplitFrameInitActorsHelper* SplitInitActorsHelper = new FSplitFrameInitActorsHelper(Level);
			for (AActor* Actor : TArray<AActor*>(Level->Actors))
			{
				if (Actor && Actor->Implements<UXD_SaveGameInterface>())
				{
					SplitInitActorsHelper->InvokeToInitActors.Add(Actor);
				}
			}

			FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(SplitInitActorsHelper, &FSplitFrameInitActorsHelper::InitActorTick));
		}
		else
		{
			StartInitLevel(Level);
			for (AActor* Actor : TArray<AActor*>(Level->Actors))
			{
				if (Actor && Actor->Implements<UXD_SaveGameInterface>())
				{
					IXD_SaveGameInterface::GameInit(Actor);
				}
			}
			EndInitLevel(Level);
		}
	}

	//或许有更好的时机去保存关卡
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = UXD_LevelFunctionLibrary::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		WorldSettingsComponent->bActiveAutoSave = true;
	}
	else
	{
		SaveGameSystem_Error_Log("关卡[%s]不存在ARPG_WorldSettings，自动保存该关卡将失效", *UXD_LevelFunctionLibrary::GetLevelName(Level));
	}
}

void UXD_SaveGameSystemBase::WhenActorSpawnInitActor(UWorld* World)
{
	World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([this](AActor* Actor)
	{
		if (bShouldInitSpawnActor && Actor->Implements<UXD_SaveGameInterface>())
		{
			IXD_SaveGameInterface::GameInit(Actor);
		}
	}));
}

FString UXD_SaveGameSystemBase::MakeLevelSlotName(ULevel* Level)
{
	return Get(Level)->MakeFullSlotName(TEXT("Level"), UXD_LevelFunctionLibrary::GetLevelName(Level));
}

void UXD_SaveGameSystemBase::SaveLevel(ULevel* Level) const
{
	UXD_SaveLevelBase* SaveLevelObject = NewObject<UXD_SaveLevelBase>(GetTransientPackage(), SaveLevelClass);
	SaveLevelObject->SaveLevel(Level);
}

bool UXD_SaveGameSystemBase::CanSaveLevel(ULevel* Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = UXD_LevelFunctionLibrary::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		if (WorldSettingsComponent->bIsLoadingLevel)
		{
			SaveGameSystem_Warning_LOG("不能保存关卡[%s]，因为当前关卡还在分帧读取Actor", *UXD_LevelFunctionLibrary::GetLevelName(Level));
			return false;
		}
		else if (WorldSettingsComponent->bIsInitingLevel)
		{
			SaveGameSystem_Warning_LOG("不能保存关卡[%s]，因为当前关卡还在分帧初始化Actor", *UXD_LevelFunctionLibrary::GetLevelName(Level));
			return false;
		}
		else if (Level->bIsBeingRemoved)
		{
			SaveGameSystem_Warning_LOG("不能保存关卡[%s]，因为当前关卡正在移除", *UXD_LevelFunctionLibrary::GetLevelName(Level));
			return false;
		}
		return true;
	}
	return false;
}

void UXD_SaveGameSystemBase::StartLoadLevel(ULevel* Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = UXD_LevelFunctionLibrary::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		WorldSettingsComponent->bIsLoadingLevel = true;
		WorldSettingsComponent->bIsInitingLevel = false;
	}
}

void UXD_SaveGameSystemBase::EndLoadLevel(ULevel* Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = UXD_LevelFunctionLibrary::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		WorldSettingsComponent->bIsLoadingLevel = false;
	}
}

void UXD_SaveGameSystemBase::StartInitLevel(ULevel* Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = UXD_LevelFunctionLibrary::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		WorldSettingsComponent->bIsInitingLevel = true;
		WorldSettingsComponent->bIsLoadingLevel = false;
	}
}

void UXD_SaveGameSystemBase::EndInitLevel(ULevel* Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = UXD_LevelFunctionLibrary::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		WorldSettingsComponent->bIsInitingLevel = false;
	}
}

bool UXD_SaveGameSystemBase::IsLevelInitCompleted(ULevel* Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = UXD_LevelFunctionLibrary::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		return WorldSettingsComponent->bIsLoadingLevel == false && WorldSettingsComponent->bIsInitingLevel == false;
	}
	return false;
}



