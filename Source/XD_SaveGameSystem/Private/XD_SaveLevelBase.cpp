// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveLevelBase.h"
#include "XD_SaveGameInterface.h"

bool UXD_SaveLevelBase::SaveLevel(ULevel* OuterLevel)
{
	if (OuterLevel)
	{
		OldWorldOrigin = UGameplayStatics::GetWorldOriginLocation(OuterLevel);

		TArray<UObject*> ObjectReferenceCollection;

		for (AActor* Actor : OuterLevel->Actors)
		{
			if (Actor &&  Actor->Implements<UXD_SaveGameInterface>() && !IXD_SaveGameInterface::Execute_NeedNotSave(Actor))
			{
				ActorRecorders.Add(UXD_SaveGameFunctionLibrary::SerializeObject(Actor, OuterLevel, ObjectReferenceCollection));
			}
		}

		bool Result = UGameplayStatics::SaveGameToSlot(this, UXD_SaveGameSystemBase::MakeLevelSlotName(OuterLevel), UXD_SaveGameSystemBase::Get(OuterLevel)->UserIndex);

		//LOG
		{
			FString SavedObjectsDesc;
			for (UObject* Object : ObjectReferenceCollection)
			{
				SavedObjectsDesc += UXD_DebugFunctionLibrary::GetDebugName(Object) + " | ";
			}

			if (Result)
			{
				//SaveGameSystem_Display_Log("\n保存关卡[%s]成功\n保存的Object列表：%s", *UARPG_FunctionLibrary::GetLevelName(OuterLevel), *SavedObjectsDesc);

				SaveGameSystem_Display_Log("--------------------------------------保存关卡[%s]成功--------------------------------------", *UXD_LevelFunctionLibrary::GetLevelName(OuterLevel));

				SaveGameSystem_Display_Log("保存的Object列表：%s", *SavedObjectsDesc);
			}
			else
			{
				SaveGameSystem_Error_Log("\n保存关卡[%s]失败", *UXD_LevelFunctionLibrary::GetLevelName(OuterLevel));
			}
		}

		return Result;
	}
	return false;
}

void UXD_SaveLevelBase::LoadLevel(ULevel* OuterLevel, const bool SplitFrameLoadActors)
{
	if (OuterLevel)
	{
		struct FSplitFrameLoadActorHelper
		{
			FSplitFrameLoadActorHelper(ULevel* Level, UXD_SaveLevelBase* SaveLevelBase, bool IsNoSplitFrameLoadUse = false)
				:Level(Level), SaveLevelBase(SaveLevelBase), IsNoSplitFrameLoadUse(IsNoSplitFrameLoadUse)
			{
				UXD_SaveGameSystemBase::StartLoadLevel(Level);

				if (IsNoSplitFrameLoadUse)
				{
					TimeLimit = TNumericLimits<double>::Max();
				}

				SaveLevelBase->SetFlags(RF_StrongRefOnFrame);

				//记录下需要被读取的Actor
				for (AActor* Actor : Level->Actors)
				{
					if (Actor && Actor->Implements<UXD_SaveGameInterface>() && IXD_SaveGameInterface::Execute_NeedNotSave(Actor) == false)
					{
						NeedBeLoadActors.Add(Actor);
					}
				}
			}

			~FSplitFrameLoadActorHelper()
			{
				if (SaveLevelBase.IsValid())
				{
					SaveLevelBase->ClearFlags(RF_StrongRefOnFrame);
				}
			}

			bool IsNoSplitFrameLoadUse;

			double TimeLimit = 0.005;

			TWeakObjectPtr<ULevel> Level;
			TWeakObjectPtr<UXD_SaveLevelBase> SaveLevelBase;

			TSet<TWeakObjectPtr<AActor>> NeedBeLoadActors;

			TSet<TWeakObjectPtr<AActor>> RemainExistActors;

			TArray<UObject*> ObjectReferenceCollection;

			bool LoadActorsTick(float DeltaSceonds)
			{
				if (Level.IsValid() == false || SaveLevelBase.IsValid() == false)
				{
					if (!IsNoSplitFrameLoadUse)
					{
						delete this;
					}
					return false;
				}

				double StartTime = FPlatformTime::Seconds();

				int LoadCount = 0;
				while (LoadCount < SaveLevelBase->ActorRecorders.Num())
				{
					FARPG_Recorder& ARPG_Recorder = SaveLevelBase->ActorRecorders[LoadCount];
					UXD_SaveGameFunctionLibrary::DeserializeObject(ARPG_Recorder, Level.Get(), ObjectReferenceCollection, SaveLevelBase->OldWorldOrigin);

					++LoadCount;

					if (FPlatformTime::Seconds() - StartTime > TimeLimit)
					{
						break;
					}
				}
				SaveLevelBase->ActorRecorders.RemoveAt(0, LoadCount);

				if (SaveLevelBase->ActorRecorders.Num() != 0)
				{
					return true;
				}
				else
				{
					//过滤出被读取的Actor
					for (UObject* Object : ObjectReferenceCollection)
					{
						if (AActor* Actor = Cast<AActor>(Object))
						{
							RemainExistActors.Add(Actor);
						}
					}

					TSet<TWeakObjectPtr<AActor>> NeedDestroyActors(NeedBeLoadActors.Difference(RemainExistActors));
					//LOG
					{
						FString LoadedObjectsDesc;
						for (UObject* Object : ObjectReferenceCollection)
						{
							LoadedObjectsDesc += UXD_DebugFunctionLibrary::GetDebugName(Object) + TEXT(" | ");
						}

						FString DestroyedActorsDesc;
						for (const TWeakObjectPtr<AActor>& Actor : NeedDestroyActors)
						{
							if (Actor.IsValid())
							{
								DestroyedActorsDesc += UXD_DebugFunctionLibrary::GetDebugName(Actor.Get()) + TEXT(" | ");
							}
						}

						SaveGameSystem_Display_Log("--------------------------------------读取关卡[%s]--------------------------------------", *UXD_LevelFunctionLibrary::GetLevelName(Level.Get()));

						SaveGameSystem_Display_Log("读取的Object列表：%s", *LoadedObjectsDesc);

						SaveGameSystem_Display_Log("删除的Actor列表：%s", *DestroyedActorsDesc);
					}

					//删除没被读取的Actor
					for (const TWeakObjectPtr<AActor>& Actor : NeedDestroyActors)
					{
						if (Actor.IsValid())
						{
							Actor->Destroy();
						}
					}

					if (Level.IsValid())
					{
						UXD_SaveGameSystemBase::EndLoadLevel(Level.Get());
					}

					//最后删除自己防止内存泄露
					if (!IsNoSplitFrameLoadUse)
					{
						delete this;
					}
					return false;
				}
			}
		};

		if (SplitFrameLoadActors)
		{
			FSplitFrameLoadActorHelper* SplitFrameLoadHelper = new FSplitFrameLoadActorHelper(OuterLevel, this);

			FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(SplitFrameLoadHelper, &FSplitFrameLoadActorHelper::LoadActorsTick));
		}
		else
		{
			FSplitFrameLoadActorHelper SplitFrameLoadHelper(OuterLevel, this, true);
			SplitFrameLoadHelper.LoadActorsTick(0.f);
		}
	}

}



