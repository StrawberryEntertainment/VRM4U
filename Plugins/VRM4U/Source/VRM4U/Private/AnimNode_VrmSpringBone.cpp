// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AnimNode_VrmSpringBone.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"

#include "VrmMetaObject.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone

namespace {

	bool operator<(const FBoneTransform &a, const FBoneTransform &b) {
		return a.BoneIndex < b.BoneIndex;
	}

	int32 GetDirectChildBonesLocal(FReferenceSkeleton &refs, int32 ParentBoneIndex, TArray<int32> & Children)
	{
		Children.Reset();

		const int32 NumBones = refs.GetNum();
		for (int32 ChildIndex = ParentBoneIndex + 1; ChildIndex < NumBones; ChildIndex++)
		{
			if (ParentBoneIndex == refs.GetParentIndex(ChildIndex))
			{
				Children.Add(ChildIndex);
			}
		}

		return Children.Num();
	}

}


namespace VRMSpring {

	class VRMSpring;

	class VRMSpringManager {
	public:

		bool bInit = false;
		USkeletalMesh *skeletalMesh = nullptr;

		void init(const UVrmMetaObject *meta);
		void update(float DeltaTime, FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
		void reset();

		TArray<VRMSpring> spring;
	};

	class VRMSprintData {
	public:
		int boneIndex = -1;
		FString boneName;
		FVector m_currentTail = FVector::ZeroVector;
		FVector m_prevTail = FVector::ZeroVector;
		FTransform m_transform = FTransform::Identity;

		FQuat m_resultQuat = FQuat::Identity;
	};

	class VRMSpring {
	public:
		float stiffiness = 0.f;
		float gravityPower = 0.f;
		FVector gravityDir = { 0,0,0 };
		float dragForce = 0.f;
		float hitRadius = 0.f;

		//int boneNum = 0;
		//int *bones;
		//FString bones_name;

		//int colliderGourpNum = 0;
		//int* colliderGroups = nullptr;

		TArray< TArray<VRMSprintData> > SpringDataChain;
		TArray<VRMSprintData> RootSpringData;

		USkeletalMesh *skeletalMesh = nullptr;
		~VRMSpring() {
			skeletalMesh = nullptr;
		}

		void Update(float DeltaTime, FTransform center,
			//float stiffnessForce, float dragForce, FVector external,
			//int colliders,
			FComponentSpacePoseContext& Output);
	};

	void VRMSpring::Update(float DeltaTime, FTransform center,
		FComponentSpacePoseContext& Output) {

		float stiffnessForce = stiffiness * DeltaTime;
		//FVector external(10, 0, 0);//m_gravityDir * (m_gravityPower * Time.deltaTime);
		FVector external = gravityDir * (gravityPower * DeltaTime);

		if (skeletalMesh == nullptr) {
			return;
		}

		const auto RefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
		const auto &RefSkeletonTransform = RefSkeleton.GetRefBonePose();

		const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
		center = ComponentTransform;


		for (int springCount = 0; springCount < SpringDataChain.Num(); ++springCount) {

			auto &ChainRoot = SpringDataChain[springCount];

			for (int chainCount = 0; chainCount < ChainRoot.Num(); ++chainCount) {

				auto &sData = ChainRoot[chainCount];
				//RootSpringData[springCount].

				FVector currentTail = center.TransformPosition(sData.m_currentTail);
				FVector prevTail = center.TransformPosition(sData.m_prevTail);

				int myBoneIndex = RefSkeleton.FindBoneIndex(*sData.boneName);
				int myParentBoneIndex = RefSkeleton.GetParentIndex(myBoneIndex);

				FQuat ParentRotation = FQuat::Identity;
				if (chainCount == 0) {
					FCompactPoseBoneIndex ParentIndex(myParentBoneIndex);
					FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(ParentIndex);
					ParentRotation = NewBoneTM.GetRotation();
				}else {
					ParentRotation = ChainRoot[chainCount-1].m_resultQuat;
				}
				FQuat m_localRotation = FQuat::Identity;
				//FQuat m_localRotation = center.GetRotation().Inverse();
				FVector m_boneAxis = RefSkeletonTransform[myBoneIndex].GetLocation();
				float m_length = m_boneAxis.Size();


				// verlet積分で次の位置を計算
				FVector nextTail = currentTail
					+ (currentTail - prevTail) * (1.0f - dragForce) // 前フレームの移動を継続する(減衰もあるよ)
					+ ParentRotation * m_localRotation * m_boneAxis * stiffnessForce // 親の回転による子ボーンの移動目標
					+ external // 外力による移動量
					;

				// 長さをboneLengthに強制
				nextTail = sData.m_transform.GetLocation() + (nextTail - sData.m_transform.GetLocation()).GetSafeNormal() * m_length;

				// Collisionで移動
				//nextTail = Collision(colliders, nextTail);

				sData.m_prevTail = center.InverseTransformPosition(currentTail);
				sData.m_currentTail = center.InverseTransformPosition(nextTail);

				FQuat rotation = ParentRotation * m_localRotation;

				sData.m_resultQuat = FQuat::FindBetween(rotation * m_boneAxis,
					nextTail - sData.m_transform.GetLocation()) * rotation;
			
			}
		}
	}

	void VRMSpringManager::reset() {
		spring.Empty();
	}
	void VRMSpringManager::init(const UVrmMetaObject *meta) {
			if (meta == nullptr) return;
		if (bInit) return;

		skeletalMesh = meta->SkeletalMesh;

		spring.SetNum(meta->VRMSprintMeta.Num());

		for (int i = 0; i < spring.Num(); ++i) {
			auto &s = spring[i];
			const auto &metaS = meta->VRMSprintMeta[i];

			s.skeletalMesh = meta->SkeletalMesh;

			s.stiffiness = metaS.stiffiness;
			s.gravityPower = metaS.gravityPower;
			s.gravityDir = metaS.gravityDir;
			s.dragForce = metaS.dragForce;
			s.hitRadius = metaS.hitRadius;

			s.RootSpringData.SetNum(metaS.bones.Num());
			for (int scount = 0; scount < s.RootSpringData.Num(); ++scount) {
				s.RootSpringData[scount].boneName = metaS.boneNames[scount];
				s.RootSpringData[scount].boneIndex = s.skeletalMesh->RefSkeleton.FindBoneIndex(*metaS.boneNames[scount]);
			}
			// TODO add child

			s.SpringDataChain.SetNum(metaS.bones.Num());
			for (int scount = 0; scount < s.RootSpringData.Num(); ++scount) {
				auto &chain = s.SpringDataChain[scount];

				{
					auto &c = chain.AddDefaulted_GetRef();
					c.boneName = metaS.boneNames[scount];
					c.boneIndex = skeletalMesh->RefSkeleton.FindBoneIndex(*metaS.boneNames[scount]);
				}

				for (int chainCount = 0; chainCount < 100; ++chainCount) {
					TArray<int32> Children;
					GetDirectChildBonesLocal(skeletalMesh->RefSkeleton, chain[chainCount].boneIndex, Children);

					if (Children.Num() == 0) {
						break;
					}
					auto &c = chain.AddDefaulted_GetRef();
					c.boneIndex = Children[0];
					c.boneName = skeletalMesh->RefSkeleton.GetBoneName(Children[0]).ToString();
				}
				
			}
			// TODO add child
		}

		bInit = true;
	}
	void VRMSpringManager::update(float DeltaTime, FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
		for (int i = 0; i < spring.Num(); ++i) {
			FTransform c;
			//c = Output.AnimInstanceProxy->GetComponentTransform();
			c = Output.AnimInstanceProxy->GetActorTransform();
			
			spring[i].Update(DeltaTime, c, Output);
		}
	}
}

FAnimNode_VrmSpringBone::FAnimNode_VrmSpringBone()
{
}

//void FAnimNode_VrmSpringBone::Update_AnyThread(const FAnimationUpdateContext& Context) {
//	Super::Update_AnyThread(Context);
//	//Context.GetDeltaTime();
//}
void FAnimNode_VrmSpringBone::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
	Super::Initialize_AnyThread(Context);
	if (SpringManager.Get()) {
		SpringManager.Get()->reset();
	}
}
void FAnimNode_VrmSpringBone::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
	Super::CacheBones_AnyThread(Context);
}

void FAnimNode_VrmSpringBone::ResetDynamics(ETeleportType InTeleportType) {
	Super::ResetDynamics(InTeleportType);
}

void FAnimNode_VrmSpringBone::UpdateInternal(const FAnimationUpdateContext& Context){
	Super::UpdateInternal(Context);

	CurrentDeltaTime = Context.GetDeltaTime();
}


void FAnimNode_VrmSpringBone::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmSpringBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	const auto RefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();

	const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

	//dstRefSkeleton.GetParentIndex

	//auto BoneSpace = EBoneControlSpace::BCS_ParentBoneSpace;
	auto BoneSpace = EBoneControlSpace::BCS_WorldSpace;
	{

		if (VrmMetaObject == nullptr) {
			return;
		}

		const auto &RefSkeletonTransform = RefSkeleton.GetRefBonePose();

		{
			if (SpringManager.Get() == nullptr) {
				SpringManager = MakeShareable(new VRMSpring::VRMSpringManager());
			}
			SpringManager->init(VrmMetaObject);
			SpringManager->update(CurrentDeltaTime, Output, OutBoneTransforms);

			for (auto &springRoot : SpringManager->spring) {
				for (auto &sChain : springRoot.SpringDataChain) {
					int BoneChain = 0;

					FTransform CurrentTransForm;
					for (auto &sData : sChain) {


						FCompactPoseBoneIndex CompactPoseBoneToModify(sData.boneIndex);


						FTransform NewBoneTM;

						NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
						NewBoneTM.SetRotation(sData.m_resultQuat);
						if (BoneChain == 0) {
							NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
							NewBoneTM.SetRotation(sData.m_resultQuat);

							CurrentTransForm = NewBoneTM;
						}else{

							NewBoneTM = CurrentTransForm;
							
							auto c = RefSkeletonTransform[sData.boneIndex];
							c.SetRotation(sData.m_resultQuat);
							NewBoneTM = c * NewBoneTM;

							CurrentTransForm = NewBoneTM;

							for (int i = 1; i < BoneChain; ++i) {

								//FCompactPoseBoneIndex parentIndex(sc[BoneChain - 1].boneIndex);
								//FTransform parentT = Output.Pose.GetComponentSpaceTransform(parentIndex);
								//NewBoneTM = NewBoneTM * parentT;

								//auto c = RefSkeletonTransform[sChain[i].boneIndex];
								//c.SetRotation(sc[i].m_resultQuat);
								//NewBoneTM *= c;

								//FCompactPoseBoneIndex parentIndex(sc[i].boneIndex);
								//FTransform t = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
								//NewBoneTM.SetRotation(sd.m_resultQuat);
							}




							//FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, t, CompactPoseBoneToModify, BoneSpace);
							//NewBoneTM.SetRotation(t.GetRotation());
							//ConvertBoneSpaceTransformToCS(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, FTransform& InOutBoneSpaceTM, FCompactPoseBoneIndex BoneIndex, EBoneControlSpace Space);
						}

						FBoneTransform a(CompactPoseBoneToModify, NewBoneTM);

						bool bFirst = true;
						for (auto &t : OutBoneTransforms) {
							if (t.BoneIndex == a.BoneIndex) {
								bFirst = false;
								break;
							}
						}

						if (bFirst) {
							OutBoneTransforms.Add(a);
						}
						BoneChain++;
					}
				}

			}
			OutBoneTransforms.Sort();

		}
		/*
		{
			static VRMSpring spring[2];

			spring[0].boneName = "J_Bip_L_Thumb1";
			spring[1].boneName = "J_Bip_L_Thumb2";
			//spring[2].boneName = "J_Bip_L_Thumb3";

			for (auto &s : spring) {
				s.skeletalMesh = Output.AnimInstanceProxy->GetSkelMeshComponent()->SkeletalMesh;
			}

			for (auto &s : spring) {
				FTransform center = ComponentTransform;
				float stiffnessForce = 0.5;
				float dragForce = 0;
				FVector external(0, 0, -1);
				int colliders = 0;
				FQuat q;

				const auto dstIndex = RefSkeleton.FindBoneIndex(*s.boneName);
				if (dstIndex < 0) {
					continue;
				}

				
				s.Update(center, stiffnessForce, dragForce, external,
					colliders, Output, q);

				FCompactPoseBoneIndex CompactPoseBoneToModify(dstIndex);

				FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);

				NewBoneTM.SetRotation(q);

				FBoneTransform a(CompactPoseBoneToModify, NewBoneTM);
				OutBoneTransforms.Add(a);
				//break;
			}
		}
		*/


		//const auto& dstRefTrans = dstRefSkeletonTransform[dstIndex];

		//FCompactPoseBoneIndex CompactPoseBoneToModify(dstIndex);

		//auto a = srcCurrentTrans;
		//FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
		//FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, BoneSpace);


		//VRMSpring 
	}
	{
	}
}

bool FAnimNode_VrmSpringBone::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	//return (BoneToModify.IsValidToEvaluate(RequiredBones));
	return true;
}

void FAnimNode_VrmSpringBone::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	//BoneToModify.Initialize(RequiredBones);
}
