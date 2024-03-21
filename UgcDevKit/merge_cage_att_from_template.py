"""Merge Cage and Attachment Objects from a Template File

Will load the cage and attachment objects from a template file and merge them into the current scene.

Requirements:
- Blender 3.6 or later

Usage:
    1. Open the .blend file where you want to merge the cage and attachment objects.
    2. Open this script in the text editor and run it.

"""

import os
import bpy

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

    # Assume the armature is already in the scene and named 'Armature'
    arm = bpy.data.armatures[0]
    arm_obj = bpy.data.objects[arm.name]

    for obj in loaded_objects:
        if obj.name not in lookup_table:
            continue
        world_matrix = obj.matrix_world.copy()
        obj.parent = None
        obj.matrix_world = world_matrix
    
    # Parent loaded objects to the armature, keeping transform
    for obj in loaded_objects:
        if obj.name not in lookup_table:
            continue
        world_matrix = obj.matrix_world.copy()
        obj.parent = arm_obj
        obj.matrix_world = world_matrix
        # obj.matrix_parent_inverse = arm_obj.matrix_world.inverted()

    # Now, parent each object to the specific bone mentioned in the lookup table
    for obj in loaded_objects:
        if obj.name not in lookup_table:
            continue
        world_matrix = obj.matrix_world.copy()
        bone_name = lookup_table[obj.name]
        bone = arm_obj.pose.bones[bone_name]
        obj.parent_bone = bone_name
        obj.parent_type = 'BONE'
        obj.matrix_world = world_matrix
        # obj.matrix_parent_inverse = arm_obj.matrix_world.inverted()

    # remove missing or unused images
    print("DEBUG: deleting missing or unused images...")
    for image in bpy.data.images:
        is_missing = False
        if image.filepath:
            imagePath = bpy.path.abspath(image.filepath)
            if (not os.path.exists(imagePath)):
                is_missing = True

        is_unused = False
        if image.users == 0:
            is_unused = True

        if is_missing or is_unused:
            bpy.data.images.remove(image)

    # cleanup all unused and unlinked data blocks
    print("DEBUG: main(): cleaning up unused data blocks...")
    bpy.ops.outliner.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)            

if __name__ == "__main__":
    main()

# End of file merge_cage_att_from_template.py