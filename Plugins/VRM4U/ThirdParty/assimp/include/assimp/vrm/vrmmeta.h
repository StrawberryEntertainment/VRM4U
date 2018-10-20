#pragma once

#include <map>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <stdexcept>

#pragma pack(1)

#ifdef __cplusplus
extern "C" {
#endif

    namespace VRM {
        typedef float(vec3)[3];

        struct ASSIMP_API VRMBlendShapeBind {
            aiString blendShapeName;
            aiString nodeName;
            aiString meshName;
            int meshID=0;
            int shapeIndex=0;
            int weight=100;
        };

        struct ASSIMP_API VRMBlendShapeGroup {
            aiString groupName;

            int bindNum=0;
            VRMBlendShapeBind *bind = nullptr;
        };

        struct ASSIMP_API VRMHumanoid {
            aiString humanBoneName;
            aiString nodeName;
        };

        struct ASSIMP_API VRMSpring {
            float stiffiness = 0.f;
            float gravityPower = 0.f;
            vec3 gravityDir = { 0,0,0 };
            float dragForce = 0.f;
            float hitRadius = 0.f;

            int boneNum = 0;
            int *bones;
            aiString *bones_name = nullptr;

            int colliderGourpNum = 0;
            int* colliderGroups = nullptr;
        };
        struct ASSIMP_API VRMCollider {
            vec3 offset = { 0,0,0 };
            float radius = 0.f;
        };
        struct ASSIMP_API VRMColliderGroup {
            int node = 0;
            aiString node_name;

            int colliderNum = 0;
            VRMCollider *colliders = nullptr;
        };

        struct ASSIMP_API VRMMetadata
        {
            int sprintNum = 0;
            VRMSpring *springs = nullptr;

            int colliderGroupNum = 0;
            VRMColliderGroup *colliderGroups = nullptr;

            VRMHumanoid humanoidBone[55];

            int blensShapeGroupNum = 0;
            VRMBlendShapeGroup *blensShapeGourp = nullptr;

            VRMMetadata() {
            }

        };
    }

#ifdef __cplusplus
}
#endif

#pragma pack()

