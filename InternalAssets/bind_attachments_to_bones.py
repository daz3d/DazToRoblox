import bpy

# Lookup table for object-to-bone mapping
lookup_table = {
    # Head
    'FaceCenter_Att': 'Head',
    'FaceFront_Att': 'Head',
    'Hat_Att': 'Head',
    'Hair_Att': 'Head',
    # UpperTorso
    'Neck_Att': 'UpperTorso',
    'LeftCollar_Att': 'UpperTorso',
    'RightCollar_Att': 'UpperTorso',
    'BodyBack_Att': 'UpperTorso',
    'BodyFront_Att': 'UpperTorso',
    # LowerTorso
    'Root_Att': 'LowerTorso',
    'WaistFront_Att': 'LowerTorso',
    'WaistBack_Att': 'LowerTorso',
    'WaistCenter_Att': 'LowerTorso',
    # LeftUpperArm
    'LeftShoulder_Att': 'LeftUpperArm',
    # RightUpperArm
    'RightShoulder_Att': 'RightUpperArm',
    # LeftHand
    'LeftGrip_Att': 'LeftHand',
    # RightHand
    'RightGrip_Att': 'RightHand',
    # LeftFoot
    'LeftFoot_Att': 'LeftFoot',
    # RightFoot
    'RightFoot_Att': 'RightFoot',
}

def bind_attachment_to_bone_inplace(obj):
    if obj.name not in lookup_table:
        return

    arm = bpy.data.armatures[0]
    arm_obj = bpy.data.objects[arm.name]

    world_matrix = obj.matrix_world.copy()
    # calc local bone offset
    bone_name = lookup_table[obj.name]
    bone = arm_obj.pose.bones[bone_name]
    # reparent to destination armature
    obj.parent = arm_obj
    obj.parent_bone = bone_name
    obj.parent_type = 'BONE'
    obj.matrix_world = world_matrix

def main():
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and obj.name.endswith('_Att'):
            bind_attachment_to_bone_inplace(obj)

if __name__ == '__main__':
    main()