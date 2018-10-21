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

        enum VRMLicenseList{
            LIC_version,
            LIC_author,
            LIC_contactInformation,
            LIC_reference,
            LIC_title,
            LIC_texture,
            LIC_allowedUserName,
            LIC_violentUssageName,
            LIC_sexualUssageName,
            LIC_commercialUssageName,
            LIC_otherPermissionUrl,
            LIC_licenseName,
            LIC_otherLicenseUrl,

            LIC_futter,
            LIC_max,
        };

        struct ASSIMP_API VRMLicensePair {
            aiString Key;
            aiString Value;
        };

        struct ASSIMP_API VRMLicense
        {
            int licensePairNum = 0;
            VRMLicensePair *licensePair = nullptr;
        };
        struct ASSIMP_API VRMMetadata
        {
            VRMLicense license;

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

