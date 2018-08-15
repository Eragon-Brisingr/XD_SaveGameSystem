// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_WriteArchive.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_SaveGameSystemUtility.h"
#include "XD_GameTypeEx.h"


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
					if (Obj->HasAnyFlags(RF_XD_WasLoaded))
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
					if (Obj->HasAnyFlags(RF_XD_WasLoaded))
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
			TArray<UActorComponent*> NeedSaveComponents;
			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (Component->Implements<UXD_SaveGameInterface>() && IXD_SaveGameInterface::Execute_NeedNotSave(Component) == false)
				{
					NeedSaveComponents.Add(Component);
				}
			}
			uint8 ComponentNumber = NeedSaveComponents.Num();
			Ar << ComponentNumber;
			for (UActorComponent* Component : NeedSaveComponents)
			{
				FString ComponentClassPath = Component->GetClass()->GetPathName();
				Ar << ComponentClassPath;
				FString ComponentName = Component->GetName();
				Ar << ComponentName;

				Ar.ObjectReferenceCollection.Add(Component);

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
			FString AssetPath = Obj->GetPathName();
			*this << AssetPath;
		}
		return *this;
		case EObjectArchiveType::InPackageObject:
		{
			check(Obj->IsA<UActorComponent>() == false);

			FString ClassPath = Obj->GetClass()->GetPathName();
			*this << ClassPath;
			FString ObjectPath = Obj->GetPathName();
			*this << ObjectPath;
			Obj->Serialize(*this);
		}
		return *this;
		case EObjectArchiveType::DynamicObject:
		{
			FString ClassPath = Obj->GetClass()->GetPathName();
			*this << ClassPath;
			FString ObjectName = Obj->GetName();
			*this << ObjectName;

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

			CheckActorError(Actor);

			FString ClassPath = Actor->GetClass()->GetPathName();
			*this << ClassPath;
			FString ActorPath = Actor->GetPathName();
			*this << ActorPath;
			FTransform ActorTransForm = Actor->GetTransform();

			*this << ActorTransForm;

			Actor->Serialize(*this);

			FXD_WriteArchiveHelper::SerilizeActorSpecialInfo(*this, Actor);
		}
		return *this;
		case EObjectArchiveType::DynamicActor:
		{
			AActor* Actor = CastChecked<AActor>(Obj);

			CheckActorError(Actor);

			FString ClassPath = Actor->GetClass()->GetPathName();
			*this << ClassPath;
			FString ActorName = Actor->GetName();
			*this << ActorName;
			FTransform ActorTransForm = Actor->GetTransform();
			*this << ActorTransForm;

			//保存Owner
			AActor* Owner = Actor->GetOwner();
			if (Owner != nullptr)
			{
				int32 OwnerIndex = ObjectReferenceCollection.IndexOfByKey(Owner);
				if (OwnerIndex == INDEX_NONE)
				{
					//下一个即为Owner
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

			Actor->Serialize(*this);

			FXD_WriteArchiveHelper::SerilizeActorSpecialInfo(*this, Actor);
		}
		return *this;
		case EObjectArchiveType::InPackageComponent:
			return *this;
		case EObjectArchiveType::DynamicComponent:
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

void FXD_WriteArchive::CheckActorError(AActor* Actor)
{
	if (ensure(Actor->GetLevel() == Level.Get()) == false)
	{
		SaveGameSystem_Error_Log("存档系统将保存关卡[%s]之外的Actor%s，应属于关卡[%s]，存档系统反序列化可能出现问题", *UXD_LevelFunctionLibrary::GetLevelName(Level.Get()), *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(Actor->GetLevel()));
	}
}
