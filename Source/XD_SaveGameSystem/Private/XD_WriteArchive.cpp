// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_WriteArchive.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_SaveGameSystemUtility.h"
#include "XD_GameTypeEx.h"
#include "Components/ActorComponent.h"

FArchive& FXD_WriteArchive::operator<<(class UObject*& Obj)
{
	struct FXD_WriteArchiveHelper
	{
		static EObjectArchiveType GetObjectArchiveType(UObject* Obj)
		{
			if (IsValid(Obj))
			{
				//资源
				if (Obj->IsAsset() || Obj->IsA<UClass>())
				{
					return EObjectArchiveType::Asset;
				}
				else if (Obj->IsA<AActor>())
				{
					// Component非本Actor不可持有引用
					check(!Obj->IsA<UActorComponent>());

					if (Obj->HasAnyFlags(RF_InPackageFlags))
					{
						return EObjectArchiveType::InPackageActor;
					}
					else
					{
						return EObjectArchiveType::DynamicActor;
					}
				}
				else
				{
					if (Obj->HasAnyFlags(RF_InPackageFlags))
					{
						return EObjectArchiveType::InPackageObject;
					}
					else
					{
						return EObjectArchiveType::DynamicObject;
					}
				}
			}
			else
			{
				return EObjectArchiveType::NullObject;
			}
		}

		static void SerilizeActorSpecialInfo(FXD_WriteArchive& Ar, AActor* Actor)
		{
			if (Actor->Implements<UXD_SaveGameInterface>())
			{
				IXD_SaveGameInterface::WhenPreSave(Actor);
			}
			Actor->Serialize(Ar);

			TArray<UActorComponent*> NeedSaveComponents;
			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (Component->Implements<UXD_SaveGameInterface>() && IXD_SaveGameInterface::NeedNotSave(Component) == false)
				{
					NeedSaveComponents.Add(Component);
				}
			}
			uint8 ComponentNumber = NeedSaveComponents.Num();
			Ar << ComponentNumber;
			for (UActorComponent* Component : NeedSaveComponents)
			{
				FXD_DynamicSaveData DynamicSaveData;
				DynamicSaveData.ClassPath = Component->GetClass();
				DynamicSaveData.Name = Component->GetName();
				FXD_DynamicSaveData::StaticStruct()->SerializeBin(Ar, &DynamicSaveData);

				Ar.ObjectReferenceCollection.Add(Component);

				IXD_SaveGameInterface::WhenPreSave(Component);
				Component->Serialize(Ar);
			}
		}
	};

	int32 ObjectIndex = ObjectReferenceCollection.IndexOfByKey(Obj);
	if (ObjectIndex == INDEX_NONE)
	{
		int32 AddIndex = ObjectReferenceCollection.Add(Obj);
		*this << AddIndex;

		//确定Obj类型
		EObjectArchiveType ObjectArchiveType = FXD_WriteArchiveHelper::GetObjectArchiveType(Obj);
		*this << ObjectArchiveType;

		switch (ObjectArchiveType)
		{
		case EObjectArchiveType::NullObject:
			return *this;
		case EObjectArchiveType::Asset:
		{
			FXD_AssetSaveData AssetSaveData;
			AssetSaveData.Path = Obj;
			FXD_AssetSaveData::StaticStruct()->SerializeBin(*this, &AssetSaveData);
		}
		return *this;
		case EObjectArchiveType::InPackageObject:
		{
			check(Obj->IsA<UActorComponent>() == false);

			FXD_InPackageSaveData InPackageSaveData;
			InPackageSaveData.ClassPath = Obj->GetClass();
			InPackageSaveData.Path = Obj;
			FXD_InPackageSaveData::StaticStruct()->SerializeBin(*this, &InPackageSaveData);

			Obj->Serialize(*this);
		}
		return *this;
		case EObjectArchiveType::DynamicObject:
		{
			FXD_DynamicSaveData DynamicSaveData;
			DynamicSaveData.ClassPath = Obj->GetClass();
			DynamicSaveData.Name = Obj->GetName();
			FXD_DynamicSaveData::StaticStruct()->SerializeBin(*this, &DynamicSaveData);

#if WITH_EDITOR
			CheckDynamicObjectError(Obj);
#endif

			//先保存Outer
			UObject* Outer = Obj->GetOuter();
			if (Outer != GetTransientPackage())
			{
				int32 OuterIndex = ObjectReferenceCollection.IndexOfByKey(Outer);
				if (OuterIndex == INDEX_NONE)
				{
					OuterIndex = ObjectReferenceCollection.Num();
					*this << OuterIndex;

					*this << Outer;
				}
				else
				{
					*this << OuterIndex;
				}
			}
			else
			{
				int32 NullOuter = INDEX_NONE;
				*this << NullOuter;
			}

			Obj->Serialize(*this);
		}
		return *this;
		case EObjectArchiveType::InPackageActor:
		{
			AActor* Actor = CastChecked<AActor>(Obj);

#if WITH_EDITOR
			CheckActorError(Actor);
			TopActor.Push(Actor);
#endif
			FXD_InPackageSaveData InPackageSaveData;
			InPackageSaveData.ClassPath = Actor->GetClass()->GetPathName();
			InPackageSaveData.Path = Actor->GetPathName();
			FXD_InPackageSaveData::StaticStruct()->SerializeBin(*this, &InPackageSaveData);

			FXD_ActorExtraSaveData ActorExtraSaveData;
			ActorExtraSaveData.Transform = Actor->GetTransform();
			FXD_ActorExtraSaveData::StaticStruct()->SerializeBin(*this, &ActorExtraSaveData);

			FXD_WriteArchiveHelper::SerilizeActorSpecialInfo(*this, Actor);

#if WITH_EDITOR
			TopActor.Pop();
#endif
		}
		return *this;
		case EObjectArchiveType::DynamicActor:
		{
			AActor* Actor = CastChecked<AActor>(Obj);

#if WITH_EDITOR
			CheckActorError(Actor);
			TopActor.Push(Actor);
#endif
			FXD_DynamicSaveData DynamicSaveData;
			DynamicSaveData.ClassPath = Actor->GetClass();
			DynamicSaveData.Name = Actor->GetName();
			FXD_DynamicSaveData::StaticStruct()->SerializeBin(*this, &DynamicSaveData);

			FXD_ActorExtraSaveData ActorExtraSaveData;
			ActorExtraSaveData.Transform = Actor->GetTransform();
			FXD_ActorExtraSaveData::StaticStruct()->SerializeBin(*this, &ActorExtraSaveData);

			//保存Owner
			AActor* Owner = Actor->GetOwner();
			if (Owner != nullptr)
			{
				int32 OwnerIndex = ObjectReferenceCollection.IndexOfByKey(Owner);
				if (OwnerIndex == INDEX_NONE)
				{
					//设置下一个即为Owner
					OwnerIndex = ObjectReferenceCollection.Num();
					*this << OwnerIndex;

					*this << Owner;
				}
				else
				{
					*this << OwnerIndex;
				}
			}
			else
			{
				int32 NullOwner = INDEX_NONE;
				*this << NullOwner;
			}

			FXD_WriteArchiveHelper::SerilizeActorSpecialInfo(*this, Actor);

#if WITH_EDITOR
			TopActor.Pop();
#endif
		}
		return *this;
		}


		return *this;
	}
	else
	{
		*this << ObjectIndex;
		return *this;
	}
}

#if WITH_EDITOR
void FXD_WriteArchive::CheckActorError(AActor* Actor)
{
	if (ensure(Actor->GetLevel() == Level.Get()) == false)
	{
		SaveGameSystem_Error_Log("存档系统将保存关卡[%s]之外的Actor%s，应属于关卡[%s]，存档系统反序列化可能出现问题", *UXD_LevelFunctionLibrary::GetLevelName(Level.Get()), *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(Actor->GetLevel()));
	}
}

void FXD_WriteArchive::CheckDynamicObjectError(const UObject* Object) const
{
	if (TopActor.Num() > 0)
	{
		if (AActor* MainActor = TopActor.Top().Get())
		{
			for (UObject* NextOuter = Object->GetOuter(); NextOuter != NULL; NextOuter = NextOuter->GetOuter())
			{
				if (MainActor == NextOuter)
				{
					return;
				}
			}
			SaveGameSystem_Error_Log("%s的Outer链中不存在%s的Actor，请使用SoftObjectPtr代替直接引用", *UXD_DebugFunctionLibrary::GetDebugName(Object), *UXD_DebugFunctionLibrary::GetDebugName(MainActor));
		}
	}
}
#endif
