"""Merge Cage and Attachment Objects from a Template File

Will load the cage and attachment objects from a template file and merge them into the current scene.

- Developed and tested with Blender 3.6.1 (Python 3.10.12)
- Requires Blender 3.6 or later

Usage:
    1. Open the .blend file where you want to merge the cage and attachment objects.
    2. Open this script in the text editor and run it.

"""

REPOSITION_ATTACHMENTS = True
REPOSITION_CAGES = True

import os
import bpy
from mathutils import Matrix

# Path to the .blend template file
try:
    current_text = bpy.context.space_data.text
    if current_text and current_text.filepath:
        # If the script is being run from a text editor, use the directory of the text file
        script_path = os.path.dirname(current_text.filepath)
except:
    # If the script is being run from the console, use the directory of the python file
    script_path = os.path.dirname(os.path.realpath(__file__))
    # running from command line, a character blend file and then the template...
    # TODO: get character blend file and template file from command line arguments
    print("Please run from Blender Scripting Panel.  Running from command line is not yet implemented.")
    exit()

current_text = bpy.context.space_data.text
if current_text and current_text.filepath:
    # If the script is being run from a text editor, use the directory of the text file
    script_path = os.path.dirname(current_text.filepath)
else:
    # If the script is being run from the console, use the directory of the python file
    script_path = os.path.dirname(os.path.realpath(__file__))

template_file_name = "/Daz_Cage_Att_Template_2.blend"
blend_file_path = script_path + template_file_name

# Directory inside the blend file (use '/' for root)
blend_directory = "/Object/"
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

# Function to load objects from a .blend file
def load_objects_from_blend(file_path, directory):
    return_list = set()
    with bpy.data.libraries.load(file_path, link=False) as (data_from, data_to):
        import_list = ['Cage'] + [name for name in data_from.objects if name.endswith('_OuterCage')]
        import_list2 = [name for name in data_from.objects if name.startswith(tuple(lookup_table.keys()))]
        return_list.update(import_list)
        return_list.update(import_list2)
        data_to.objects = list(return_list)
    return list(return_list)

def get_bone_world_matrix(bone, arm=None):
    if arm is None:
        arm = bone.id_data
    arm_matrix = arm.matrix_world
    world_matrix_bone = arm_matrix @ bone.matrix
    return world_matrix_bone

def calc_local_bone_offset(obj, bone):
    if bone is None:
        return None
    arm = bone.id_data
    arm_matrix = arm.matrix_world
    world_matrix_bone = arm_matrix @ bone.matrix
    loc,rot,scale = world_matrix_bone.decompose()
    world_matrix_bone = Matrix.Translation(loc)
    if obj is not None:
        loc, rot, scale = obj.matrix_world.decompose()
        world_matrix_obj = Matrix.Translation(loc)
        local_matrix = world_matrix_bone.inverted() @ world_matrix_obj
    else:
        local_matrix = world_matrix_bone

    return local_matrix

def main():
    # Load the objects
    import_list = load_objects_from_blend(blend_file_path, blend_directory)

    # List to keep track of loaded objects
    loaded_objects = []

    # Link loaded objects to the current scene and add them to the loaded_objects list
    for obj in bpy.data.objects:
        if obj.name in import_list:
            bpy.context.collection.objects.link(obj)
            loaded_objects.append(obj)
            obj.select_set(True)
        else:
            obj.select_set(False)

    # Assume the armature is already in the scene
    arm = bpy.data.armatures[0]
    arm_obj = bpy.data.objects[arm.name]
    source_arm = bpy.data.armatures[1]
    source_arm_obj = bpy.data.objects[source_arm.name]

    source_obj_matrix = {}
    source_bone_matrix = {}
    source_bone_offset = {}

    for obj in loaded_objects:
        if obj.name not in lookup_table:
            continue
        world_matrix = obj.matrix_world.copy()
        source_obj_matrix[obj.name] = world_matrix
        # calc local bone offset
        bone_name = obj.parent_bone
        bone = source_arm_obj.pose.bones[bone_name]
        bone_matrix = get_bone_world_matrix(bone, source_arm_obj)
        source_bone_matrix[obj.name] = bone_matrix
        local_matrix = calc_local_bone_offset(obj, bone)
        # print("DEBUG: merge_cage_att_from_template.py: main(): local_matrix = \n", local_matrix)
        source_bone_offset[obj.name] = local_matrix
        # unparent from source armature
        obj.parent = None
        obj.matrix_world = world_matrix
    
    # Parent loaded objects to the armature, keeping transform
    for obj in loaded_objects:
        if obj.name not in lookup_table:
            continue
        world_matrix = obj.matrix_world.copy()
        # reparent to destination armature
        obj.parent = arm_obj
        obj.matrix_world = world_matrix

    # Now, parent each object to the specific bone mentioned in the lookup table
    for obj in loaded_objects:
        if obj.name not in lookup_table:
            continue
        world_matrix = obj.matrix_world.copy()
        # attachment world matrix
        loc_obj,rot_obj,scale_obj = world_matrix.decompose()
        bone_name = lookup_table[obj.name]
        bone = arm_obj.pose.bones[bone_name]

        # target bone location
        world_bone_matrix = get_bone_world_matrix(bone)
        loc_bone,rot_bone,scale_bone = world_bone_matrix.decompose()
        # source offset location
        local_bone_offset = source_bone_offset[obj.name]
        loc_offset,rot_offset,scale_offset = local_bone_offset.decompose()

        obj.parent_bone = bone_name
        obj.parent_type = 'BONE'

        # new attachment world matrix (target bone location + source offset location)
        new_world_matrix = Matrix.Translation(loc_bone + loc_offset) @ rot_obj.to_matrix().to_4x4() @ Matrix.Diagonal(scale_obj).to_4x4()

        if REPOSITION_ATTACHMENTS:
            obj.matrix_world = new_world_matrix
        else:
            obj.matrix_world = world_matrix

    # cleanup all unused and unlinked data blocks
    # print("DEBUG: main(): cleaning up unused data blocks...")
    bpy.ops.outliner.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)            

if __name__ == "__main__":
    main()

# End of file merge_cage_att_from_template.py