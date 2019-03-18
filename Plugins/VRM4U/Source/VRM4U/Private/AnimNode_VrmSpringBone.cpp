// Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.


/*

// VRMSpringBone.cs
MIT License

Copyright (c) 2018 DWANGO Co., Ltd. for UniVRM
Copyright (c) 2018 ousttrue for UniGLTF, UniHumanoid
Copyright (c) 2018 Masataka SUMI for MToon

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "AnimNode_VrmSpringBone.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"

#include "VrmMetaObject.h"
#include "VrmUtil.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone

namespace {
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

		void init(const UVrmMetaObject *meta, FComponentSpacePoseContext& Output);
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
		FVector m_boneAxis = FVector::ForwardVector;
		float m_length = 1.f;

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
		center = ComponentTransform.Inverse();


		for (int springCount = 0; springCount < SpringDataChain.Num(); ++springCount) {

			auto &ChainRoot = SpringDataChain[springCount];
			FTransform currentTransform = FTransform::Identity;

			for (int chainCount = 0; chainCount < ChainRoot.Num(); ++chainCount) {

				auto &sData = ChainRoot[chainCount];

				FVector currentTail = center.TransformPosition(sData.m_currentTail);
				FVector prevTail = center.TransformPosition(sData.m_prevTail);

				int myBoneIndex = RefSkeleton.FindBoneIndex(*sData.boneName);
				int myParentBoneIndex = RefSkeleton.GetParentIndex(myBoneIndex);

				//currentTransform = FTransform::Identity;
				FQuat ParentRotation = FQuat::Identity;
				if (chainCount == 0) {
					FCompactPoseBoneIndex uu(myBoneIndex);
					FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(uu);
					ParentRotation = NewBoneTM.GetRotation();

					currentTransform = NewBoneTM;
				}else {
					auto c = RefSkeletonTransform[sData.boneIndex];
					auto t = c * currentTransform;

					ParentRotation = t.GetRotation();
					currentTransform = t;
				}
				FQuat m_localRotation = FQuat::Identity;


				// verlet積分で次の位置を計算
				FVector nextTail = currentTail
					+ (currentTail - prevTail) * (1.0f - dragForce) // 前フレームの移動を継続する(減衰もあるよ)
					+ ParentRotation * m_localRotation * sData.m_boneAxis * stiffnessForce // 親の回転による子ボーンの移動目標
					+ external // 外力による移動量
					;

				// 長さをboneLengthに強制
				//nextTail = sData.m_transform.GetLocation() + (nextTail - sData.m_transform.GetLocation()).GetSafeNormal() * sData.m_length;
				nextTail = currentTransform.GetLocation() + (nextTail - currentTransform.GetLocation()).GetSafeNormal() * sData.m_length;

				// Collisionで移動
				//nextTail = Collision(colliders, nextTail);

				{
					FCompactPoseBoneIndex uu(myBoneIndex);
					//sData.m_transform = Output.Pose.GetComponentSpaceTransform(uu);//RefSkeletonTransform[myBoneIndex];
					//sData.m_transform = currentTransform;
				}

				//nextTail.Set(100, 0, 0);

				sData.m_prevTail = center.InverseTransformPosition(currentTail);
				sData.m_currentTail = center.InverseTransformPosition(nextTail);

				FQuat rotation = ParentRotation * m_localRotation;

				//sData.m_resultQuat = FQuat::FindBetween((rotation * sData.m_boneAxis).GetSafeNormal(),
				//	(nextTail - sData.m_transform.GetLocation()).GetSafeNormal()) * rotation;

				sData.m_resultQuat = FQuat::FindBetween((rotation * sData.m_boneAxis).GetSafeNormal(),
					(nextTail - currentTransform.GetLocation()).GetSafeNormal()) * rotation;
				//sData.m_resultQuat = FQuat::FindBetween(rotation * sData.m_boneAxis,
				//	nextTail) * rotation;

				currentTransform.SetRotation(sData.m_resultQuat);

			}
		}
	}

	void VRMSpringManager::reset() {
		spring.Empty();
	}
	void VRMSpringManager::init(const UVrmMetaObject *meta, FComponentSpacePoseContext& Output) {
		if (meta == nullptr) return;
		if (bInit) return;

		skeletalMesh = meta->SkeletalMesh;
		const FReferenceSkeleton &RefSkeleton = skeletalMesh->RefSkeleton;
		const auto &RefSkeletonTransform = RefSkeleton.GetRefBonePose();

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
				s.RootSpringData[scount].boneIndex = RefSkeleton.FindBoneIndex(*metaS.boneNames[scount]);
			}
			// TODO add child

			s.SpringDataChain.SetNum(metaS.bones.Num());
			for (int scount = 0; scount < s.RootSpringData.Num(); ++scount) {
				auto &chain = s.SpringDataChain[scount];

				//root
				{
					auto &sData = chain.AddDefaulted_GetRef();
					sData.boneName = metaS.boneNames[scount];
					sData.boneIndex = skeletalMesh->RefSkeleton.FindBoneIndex(*metaS.boneNames[scount]);

					TArray<int32> Children;
					GetDirectChildBonesLocal(skeletalMesh->RefSkeleton, sData.boneIndex, Children);
					if (Children.Num() > 0) {
						sData.m_boneAxis = RefSkeletonTransform[Children[0]].GetLocation();
					} else {
						sData.m_boneAxis = RefSkeletonTransform[sData.boneIndex].GetLocation() * 0.7f;
					}
				}

				// child
				if (1) {
					for (int chainCount = 0; chainCount < 100; ++chainCount) {
						bool bLast = false;
						TArray<int32> Children;

						GetDirectChildBonesLocal(skeletalMesh->RefSkeleton, chain[chainCount].boneIndex, Children);
						if (Children.Num() <= 0) {
							break;
						}

						auto &sData = chain.AddDefaulted_GetRef();

						sData.boneIndex = Children[0];
						sData.boneName = RefSkeleton.GetBoneName(sData.boneIndex).ToString();

						GetDirectChildBonesLocal(skeletalMesh->RefSkeleton, sData.boneIndex, Children);
						if (Children.Num() > 0) {
							sData.m_boneAxis = RefSkeletonTransform[Children[0]].GetLocation();
						}
						else {
							sData.m_boneAxis = RefSkeletonTransform[sData.boneIndex].GetLocation() * 0.7f;
						}
					}
				}
			}
			// TODO add child
		}

		// init default transform
		const FTransform center = Output.AnimInstanceProxy->GetComponentTransform();
		for (auto &s : spring) {
			for (auto &root : s.SpringDataChain) {
				for (auto &sData : root) {
					sData.m_length = sData.m_boneAxis.Size();

					
					//FCompactPoseBoneIndex uu(RefSkeleton.GetParentIndex(sData.boneIndex));
					//FTransform t = Output.Pose.GetComponentSpaceTransform(uu);
					//sData.m_transform = RefSkeletonTransform[(sData.boneIndex)] * t * center;
					//t = RefSkeletonTransform[(sData.boneIndex)] * t;

					FCompactPoseBoneIndex uu(sData.boneIndex);
					sData.m_transform = Output.Pose.GetComponentSpaceTransform(uu) * center;
					FTransform t = Output.Pose.GetComponentSpaceTransform(uu);

					sData.m_currentTail = sData.m_prevTail = t.TransformPosition(sData.m_boneAxis);
				}
			}
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
	} else {
		SpringManager = MakeShareable(new VRMSpring::VRMSpringManager());
	}
	if (VrmMetaObject) {
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

	CurrentDeltaTime = Context.GetDeltaTime() * 10;
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
				return;
			}
			SpringManager->init(VrmMetaObject, Output);

			SpringManager->update(CurrentDeltaTime, Output, OutBoneTransforms);

			for (auto &springRoot : SpringManager->spring) {
				for (auto &sChain : springRoot.SpringDataChain) {
					int BoneChain = 0;

					FTransform CurrentTransForm = FTransform::Identity;
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
							NewBoneTM = c * NewBoneTM;
							NewBoneTM.SetRotation(sData.m_resultQuat);


							//const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
							//NewBoneTM.SetLocation(ComponentTransform.TransformPosition(sData.m_currentTail));

							CurrentTransForm = NewBoneTM;
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
