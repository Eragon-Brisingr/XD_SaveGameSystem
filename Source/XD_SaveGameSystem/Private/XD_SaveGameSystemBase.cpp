﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveGameSystemBase.h"
#include <GameFramework/WorldSettings.h>
#include <GameFramework/GameModeBase.h>
#include <Kismet/KismetSystemLibrary.h>
#include <GameFramework/PlayerState.h>
#include <Engine/World.h>
#include <Kismet/GameplayStatics.h>
#include <Containers/Ticker.h>

#include "XD_SG_WorldSettingsComponent.h"
#include "XD_SaveGameSystemUtility.h"
#include "XD_SaveLevelBase.h"
#include "XD_SavePlayerBase.h"
#include "XD_SaveGameInterface.h"


void UXD_SaveGameSystemBase::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

}

void UXD_SaveGameSystemBase::Deinitialize()
{
	Super::Deinitialize();

}

UXD_SaveGameSystemBase::UXD_SaveGameSystemBase()
	:bEnableAutoSave(true),
	SaveLevelClass(UXD_SaveLevelBase::StaticClass()),
	SavePlayerClass(UXD_SavePlayerBase::StaticClass()),
	bShouldInitSpawnActor(true)
{

}

UXD_SaveGameSystemBase* UXD_SaveGameSystemBase::Get(const UObject* WorldContextObject)
{
	UGameInstance* GameInstance = WorldContextObject->GetWorld()->GetGameInstance();
	return GameInstance->GetSubsystem<UXD_SaveGameSystemBase>();
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

	OnLevelAdd_DelegateHandle = FWorldDelegates::LevelAddedToWorld.AddWeakLambda(this, [this](ULevel* Level, UWorld* World)
	{
		//由于绑的是全局回调，PIE内没法分辨是不是为Server，在这里再做判断
		if (Level->GetWorld()->IsServer() == false)
			return;

		LoadLevelOrInitLevel(Level, true);
	});

	WhenActorSpawnInitActor(World);
}

void UXD_SaveGameSystemBase::ShutDownAutoSaveLoadSystem(class AGameModeBase* GameMode)
{
	FWorldDelegates::LevelAddedToWorld.Remove(OnLevelAdd_DelegateHandle);
}

void UXD_SaveGameSystemBase::StopAutoSave(UObject* WorldContextObject)
{
	for (ULevel* Level : WorldContextObject->GetWorld()->GetLevels())
	{
		if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = SaveGameSystemUtility::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
		{
			WorldSettingsComponent->bActiveAutoSave = false;
		}
	}
}

bool UXD_SaveGameSystemBase::IsAutoSaveLevel(UObject* WorldContextObject)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = SaveGameSystemUtility::GetCurrentLevelWorldSettings(WorldContextObject->GetWorld()->PersistentLevel)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		return WorldSettingsComponent->bActiveAutoSave;
	}
	return false;
}

void UXD_SaveGameSystemBase::LoadGame(UObject* WorldContextObject)
{
	StopAutoSave(WorldContextObject);

	UXD_SaveGameSystemBase::Get(WorldContextObject)->bInvokeLoadGame = true;
	UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("restartlevel"));
}

void UXD_SaveGameSystemBase::SaveGame(UObject* WorldContextObject)
{
	//保存所有加载的关卡
	//主关卡最后保存（可能有全局数据），所以倒着保存关卡
	const TArray<ULevel*>& Levels = WorldContextObject->GetWorld()->GetLevels();
	for (int32 Idx = Levels.Num() - 1; Idx >= 0; --Idx)
	{
		ULevel* Level = Levels[Idx];
		if (CanSaveLevel(Level))
		{
			SaveLevel(Level);
		}
	}

	SaveAllPlayer(WorldContextObject);
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
					:Level(Level), InitLevelGuard(Level)
				{
					for (AActor* Actor : TArray<AActor*>(Level->Actors))
					{
						if (Actor && Actor->Implements<UXD_SaveGameInterface>())
						{
							InvokeToInitActors.Add(Actor);
						}
					}
					InvokeToInitActors.Sort([](const TWeakObjectPtr<AActor>& LHS, const TWeakObjectPtr<AActor>& RHS) 
						{
							return IXD_SaveGameInterface::GetActorSerializePriority(LHS.Get()) > IXD_SaveGameInterface::GetActorSerializePriority(RHS.Get());
						});
					TimeLimit = UXD_SaveGameSystemBase::Get(Level)->SplitFrameLoadActorLimitSeconds;
				}
				TWeakObjectPtr<ULevel> Level;
				FInitLevelGuard InitLevelGuard;

				TArray<TWeakObjectPtr<AActor>> InvokeToInitActors;
				int32 InitIterator = 0;
				double TimeLimit;

				bool InitActorTick(float DeltaSecond)
				{
					double StartTime = FPlatformTime::Seconds();
					while (InitIterator < InvokeToInitActors.Num())
					{
						if (AActor* Actor = InvokeToInitActors[InitIterator].Get())
						{
							UXD_SaveGameSystemBase::NotifyActorAndComponentInit(Actor);
						}
						InitIterator += 1;

						if (FPlatformTime::Seconds() - StartTime > TimeLimit)
						{
							break;
						}
					}

					if (InitIterator != InvokeToInitActors.Num())
					{
						return true;
					}
					else
					{
						delete this;
						return false;
					}
				}
			};

			FSplitFrameInitActorsHelper* SplitInitActorsHelper = new FSplitFrameInitActorsHelper(Level);
			FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(SplitInitActorsHelper, &FSplitFrameInitActorsHelper::InitActorTick));
		}
		else
		{
			FInitLevelGuard InitLevelGuard(Level);
			TArray<AActor*> SortedNeedInitActors;
			{
				for (AActor* Actor : Level->Actors)
				{
					if (Actor && Actor->Implements<UXD_SaveGameInterface>())
					{
						SortedNeedInitActors.Add(Actor);
					}
				}
				SortedNeedInitActors.Sort([](AActor& LHS, AActor& RHS) {return IXD_SaveGameInterface::GetActorSerializePriority(&LHS) > IXD_SaveGameInterface::GetActorSerializePriority(&RHS); });
			}
			for (AActor* Actor : SortedNeedInitActors)
			{
				NotifyActorAndComponentInit(Actor);
			}
		}
	}

	//或许有更好的时机去保存关卡
	if (AWorldSettings* WorldSettings = SaveGameSystemUtility::GetCurrentLevelWorldSettings(Level))
	{
		//子层级的WorldSettings不设置BegunPlay状态，手动调用
		if (WorldSettings->HasActorBegunPlay() == false)
		{
			WorldSettings->DispatchBeginPlay();
		}

		UXD_SG_WorldSettingsComponent* SG_WorldSettingsComponent = NewObject<UXD_SG_WorldSettingsComponent>(WorldSettings, TEXT("SG_WorldSettingsComponent"));
		WorldSettings->AddOwnedComponent(SG_WorldSettingsComponent);
		SG_WorldSettingsComponent->RegisterComponent();
		SG_WorldSettingsComponent->OnWorldSettingsComponentEndPlay.AddWeakLambda(this, [=](const EEndPlayReason::Type EndPlayReason)
		{
			OnPreLevelUnload.Broadcast(Level);
		});
	}
	else
	{
		SaveGameSystem_Error_Log("关卡[%s]不存在WorldSettings，自动保存该关卡将失效", *SaveGameSystemUtility::GetLevelName(Level));
	}
}

void UXD_SaveGameSystemBase::WhenActorSpawnInitActor(UWorld* World)
{
	World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateWeakLambda(this, [this](AActor* Actor)
	{
		if (bShouldInitSpawnActor && Actor->Implements<UXD_SaveGameInterface>())
		{
			NotifyActorAndComponentInit(Actor);
		}
	}));
}

void UXD_SaveGameSystemBase::NotifyActorAndComponentInit(AActor* Actor)
{
	IXD_SaveGameInterface::GameInit(Actor);
	for (UActorComponent* Component : Actor->GetComponents())
	{
		if (Component->Implements<UXD_SaveGameInterface>())
		{
			IXD_SaveGameInterface::GameInit(Component);
		}
	}
}

FString UXD_SaveGameSystemBase::MakeLevelSlotName(ULevel* Level)
{
	return Get(Level)->MakeFullSlotName(TEXT("Level"), SaveGameSystemUtility::GetLevelName(Level));
}

bool UXD_SaveGameSystemBase::SaveLevel(ULevel* Level) const
{
	UXD_SaveLevelBase* SaveLevelObject = NewObject<UXD_SaveLevelBase>(GetTransientPackage(), SaveLevelClass);
	return SaveLevelObject->SaveLevel(Level);
}

bool UXD_SaveGameSystemBase::CanSaveLevel(ULevel* Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = SaveGameSystemUtility::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		if (WorldSettingsComponent->bIsLoadingLevel)
		{
			SaveGameSystem_Warning_Log("不能保存关卡[%s]，因为当前关卡还在分帧读取Actor", *SaveGameSystemUtility::GetLevelName(Level));
			return false;
		}
		else if (WorldSettingsComponent->bIsInitingLevel)
		{
			SaveGameSystem_Warning_Log("不能保存关卡[%s]，因为当前关卡还在分帧初始化Actor", *SaveGameSystemUtility::GetLevelName(Level));
			return false;
		}
		else if (Level->bIsBeingRemoved)
		{
			SaveGameSystem_Warning_Log("不能保存关卡[%s]，因为当前关卡正在移除", *SaveGameSystemUtility::GetLevelName(Level));
			return false;
		}
		return true;
	}
	return false;
}

bool UXD_SaveGameSystemBase::IsLevelInitCompleted(ULevel* Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = SaveGameSystemUtility::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		return WorldSettingsComponent->bIsLoadingLevel == false && WorldSettingsComponent->bIsInitingLevel == false;
	}
	return false;
}

bool UXD_SaveGameSystemBase::SavePlayer(class APlayerController* Player, APawn* Pawn /*= nullptr*/, class APlayerState* PlayerState /*= nullptr*/)
{
	if (Pawn == nullptr)
	{
		Pawn = Player->GetPawn();
	}
	if (PlayerState == nullptr)
	{
		PlayerState = Player->PlayerState;
	}

	UXD_SavePlayerBase* SavePlayerObject = NewObject<UXD_SavePlayerBase>(GetTransientPackage(), SavePlayerClass);

	return SavePlayerObject->SavePlayer(Player, Pawn, PlayerState);
}

bool UXD_SaveGameSystemBase::SaveAllPlayer(const UObject* WorldContextObject)
{
	bool Result = true;
	UWorld* World = WorldContextObject->GetWorld();
	for (auto PlayerControllerIterator = World->GetPlayerControllerIterator(); PlayerControllerIterator; ++PlayerControllerIterator)
	{
		APlayerController* PlayerController = PlayerControllerIterator->Get();
		Result &= SavePlayer(PlayerController);
	}
	return Result;
}

APawn* UXD_SaveGameSystemBase::TryLoadPlayer(class APlayerController* Player)
{
	if (bInvokeLoadGame)
	{
		FString SlotName = GetDefault<UXD_SavePlayerBase>(SavePlayerClass)->GetFullPlayerSlotName(Player->PlayerState);
		if (UXD_SavePlayerBase* SavePlayerObject = Cast<UXD_SavePlayerBase>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex)))
		{
			APawn* Pawn = SavePlayerObject->LoadPlayer(Player);
			return Pawn;
		}
	}
	return nullptr;
}

void UXD_SaveGameSystemBase::RegisterAutoSavePlayer(class APawn* Pawn)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
	{
		UXD_AutoSavePlayerLambda* AutoSavePlayerLamdba = NewObject<UXD_AutoSavePlayerLambda>(Pawn);
		AutoSavePlayerLamdba->SetFlags(RF_StrongRefOnFrame);
		AutoSavePlayerLamdba->PlayerContoller = PlayerController;
		AutoSavePlayerLamdba->PlayerState = PlayerController->PlayerState;
		SaveGameSystem_Display_Log("玩家[%s]登记为自动保存", *PlayerController->PlayerState->GetPlayerName());
		Pawn->OnEndPlay.AddDynamic(AutoSavePlayerLamdba, &UXD_AutoSavePlayerLambda::WhenPlayerLeaveGame);
	}
}

void UXD_SaveGameSystemBase::SetSaveGameVersion(int32 Version)
{
	FXD_SaveGameVersion::Version = Version;
}

void UXD_AutoSavePlayerLambda::WhenPlayerLeaveGame(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	ClearFlags(RF_StrongRefOnFrame);

	if (PlayerContoller)
	{
		SaveGameSystem_Display_Log("玩家[%s]登出", *PlayerState->GetPlayerName());

		UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(PlayerContoller);
		if (SaveGameSystem->bEnableAutoSave && UXD_SaveGameSystemBase::IsAutoSaveLevel(Actor))
		{
			SaveGameSystem->SavePlayer(PlayerContoller, Cast<APawn>(Actor), PlayerState);
		}
	}
}

UXD_SaveGameSystemBase::FInitLevelGuard::FInitLevelGuard(ULevel* Level) 
	:Level(Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = SaveGameSystemUtility::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		check(WorldSettingsComponent->bIsLoadingLevel == false);

		WorldSettingsComponent->bIsInitingLevel = true;
	}
}

UXD_SaveGameSystemBase::FInitLevelGuard::~FInitLevelGuard()
{
	if (ULevel* LevelRef = Level.Get())
	{
		if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = SaveGameSystemUtility::GetCurrentLevelWorldSettings(LevelRef)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
		{
			WorldSettingsComponent->bIsInitingLevel = false;
		}
		if (UXD_SaveGameSystemBase* SaveGameSystem = Get(LevelRef))
		{
			SaveGameSystem->OnInitLevelCompleted.Broadcast(LevelRef);
		}
	}
}

UXD_SaveGameSystemBase::FLoadLevelGuard::FLoadLevelGuard(ULevel* Level)
	:Level(Level)
{
	if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = SaveGameSystemUtility::GetCurrentLevelWorldSettings(Level)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
	{
		check(WorldSettingsComponent->bIsInitingLevel == false);

		WorldSettingsComponent->bIsLoadingLevel = true;
	}
}

UXD_SaveGameSystemBase::FLoadLevelGuard::~FLoadLevelGuard()
{
	if (ULevel* LevelRef = Level.Get())
	{
		if (UXD_SG_WorldSettingsComponent* WorldSettingsComponent = SaveGameSystemUtility::GetCurrentLevelWorldSettings(LevelRef)->FindComponentByClass<UXD_SG_WorldSettingsComponent>())
		{
			WorldSettingsComponent->bIsLoadingLevel = false;
		}
		if (UXD_SaveGameSystemBase* SaveGameSystem = Get(LevelRef))
		{
			SaveGameSystem->OnLoadLevelCompleted.Broadcast(LevelRef);
		}
	}
}
