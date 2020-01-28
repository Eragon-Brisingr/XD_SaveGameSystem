// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveLevelBase.h"
#include <Engine/Level.h>
#include <Kismet/GameplayStatics.h>
#include <Containers/Ticker.h>

#include "XD_SaveGameInterface.h"
#include "XD_SaveGameSystemBase.h"
#include "XD_SaveGameFunctionLibrary.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_LevelFunctionLibrary.h"
#include "XD_SaveGameSystemUtility.h"

bool UXD_SaveLevelBase::SaveLevel(ULevel* OuterLevel)
{
	if (OuterLevel)
	{
		OldWorldOrigin = UGameplayStatics::GetWorldOriginLocation(OuterLevel);

		TArray<UObject*> ObjectReferenceCollection;

		TArray<AActor*> SortedNeedSaveActors;
		{
			for (AActor* Actor : OuterLevel->Actors)
			{
				if (Actor && Actor->Implements<UXD_SaveGameInterface>() && IXD_SaveGameInterface::NeedSave(Actor))
				{
					SortedNeedSaveActors.Add(Actor);
				}
			}
			SortedNeedSaveActors.Sort([](AActor& LHS, AActor& RHS) {return IXD_SaveGameInterface::GetActorSerializePriority(&LHS) < IXD_SaveGameInterface::GetActorSerializePriority(&RHS); });
		}
		for (AActor* Actor : SortedNeedSaveActors)
		{
			ActorRecorders.Add(UXD_SaveGameFunctionLibrary::SerializeObject(Actor, OuterLevel, ObjectReferenceCollection));
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
			UXD_SaveGameSystemBase::FLoadLevelGuard LoadLevelGuard;

			FSplitFrameLoadActorHelper(ULevel* Level, UXD_SaveLevelBase* SaveLevelBase, bool IsNoSplitFrameLoadUse)
				:LoadLevelGuard(Level), Level(Level), SaveLevelBase(SaveLevelBase), IsNoSplitFrameLoadUse(IsNoSplitFrameLoadUse)
			{
				if (IsNoSplitFrameLoadUse)
				{
					TimeLimit = TNumericLimits<double>::Max();
				}
				else
				{
					TimeLimit = UXD_SaveGameSystemBase::Get(Level)->SplitFrameLoadActorLimitSeconds;
				}

				SaveLevelBase->SetFlags(RF_StrongRefOnFrame);

				//记录下需要被读取的Actor
				for (AActor* Actor : Level->Actors)
				{
					if (Actor && Actor->Implements<UXD_SaveGameInterface>() && IXD_SaveGameInterface::NeedSave(Actor))
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

			double TimeLimit;

			TWeakObjectPtr<ULevel> Level;
			TWeakObjectPtr<UXD_SaveLevelBase> SaveLevelBase;
			bool IsNoSplitFrameLoadUse;

			TSet<TWeakObjectPtr<AActor>> NeedBeLoadActors;
			TSet<TWeakObjectPtr<AActor>> RemainExistActors;

			TArray<UObject*> ObjectReferenceCollection;
			struct FWhenLoadUnit
			{
				AActor* MainActor;
				TArray<UObject*> Objects;
			};
			TArray<FWhenLoadUnit> WhenLoadUnits;

			int32 LoadDataIdx = 0;
			int32 ExecutePostLoadIdx = 0;

			enum class EStepType
			{
				Deserialize,
				PostLoad,
				Destroy
			};
			EStepType CurrentStep = FSplitFrameLoadActorHelper::EStepType::Deserialize;

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

				switch (CurrentStep)
				{
				case FSplitFrameLoadActorHelper::EStepType::Deserialize:
				{
					// 分帧执行反序列化
					const TArray<FXD_SaveGameRecorder>& ActorRecorders = SaveLevelBase->ActorRecorders;
					for (; LoadDataIdx < ActorRecorders.Num(); ++LoadDataIdx)
					{
						const FXD_SaveGameRecorder& XD_Recorder = ActorRecorders[LoadDataIdx];
						FWhenLoadUnit& WhenLoadUnit = WhenLoadUnits.AddDefaulted_GetRef();
						WhenLoadUnit.MainActor = CastChecked<AActor>(UXD_SaveGameFunctionLibrary::DeserializeObject(XD_Recorder, Level.Get(), SaveLevelBase->OldWorldOrigin, ObjectReferenceCollection, WhenLoadUnit.Objects));

						if (FPlatformTime::Seconds() - StartTime > TimeLimit)
						{
							return true;
						}
					}

					WhenLoadUnits.Sort([](const FWhenLoadUnit& LHS, const FWhenLoadUnit& RHS)
						{
							return IXD_SaveGameInterface::GetActorSerializePriority(LHS.MainActor) > IXD_SaveGameInterface::GetActorSerializePriority(LHS.MainActor);
						});
					CurrentStep = EStepType::PostLoad;
				}
				case FSplitFrameLoadActorHelper::EStepType::PostLoad:
				{
					// 分帧执行读取
					for (; ExecutePostLoadIdx < WhenLoadUnits.Num(); ++ExecutePostLoadIdx)
					{
						FWhenLoadUnit& WhenLoadUnit = WhenLoadUnits[ExecutePostLoadIdx];
						for (UObject* Object : WhenLoadUnit.Objects)
						{
							if (Object && Object->Implements<UXD_SaveGameInterface>())
							{
								IXD_SaveGameInterface::WhenPostLoad(Object);
							}
						}

						if (FPlatformTime::Seconds() - StartTime > TimeLimit)
						{
							return true;
						}
					}

					CurrentStep = EStepType::Destroy;
				}
				case FSplitFrameLoadActorHelper::EStepType::Destroy:
				{
					//------------------当读取完毕
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

					if (!IsNoSplitFrameLoadUse)
					{
						//最后删除自己防止内存泄露
						delete this;
					}
					return false;
				}
				}

				checkNoEntry();
				return true;
			}
		};

		if (SplitFrameLoadActors)
		{
			FSplitFrameLoadActorHelper* SplitFrameLoadHelper = new FSplitFrameLoadActorHelper(OuterLevel, this, false);
			FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(SplitFrameLoadHelper, &FSplitFrameLoadActorHelper::LoadActorsTick));
		}
		else
		{
			FSplitFrameLoadActorHelper SplitFrameLoadHelper(OuterLevel, this, true);
			SplitFrameLoadHelper.LoadActorsTick(0.f);
		}
	}

}



