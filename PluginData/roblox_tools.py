"""Roblox Tools module

Contains several convenience functions to generate Roblox Avatar compatible character.

Requirements:
    - Python 3+
    - Blender 3.6+
"""
from pathlib import Path
script_dir = str(Path( __file__ ).parent.absolute())

REPOSITION_ATTACHMENTS = True
REPOSITION_CAGES = True
USE_SHRINKWRAP_TARGET = True

import os
import bpy
from mathutils import Matrix
from mathutils import Vector

template_file_name = "/Daz_Cage_Att_Template_v4b.blend"
blend_file_path = script_dir + template_file_name

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

def create_shrinkwrap_target():
    # prepare lists
    geo_list = []
    all_objects_list = []
    for obj in bpy.data.objects:
        all_objects_list.append(obj)
        if obj.type == 'MESH' and obj.name.lower().endswith('_geo'):
            geo_list.append(obj)
    
    # object mode
    bpy.ops.object.mode_set(mode='OBJECT')
    # deselect all
    bpy.ops.object.select_all(action='DESELECT')
    for obj in geo_list:
        obj.select_set(True)
    # set geo_list[0] to active
    bpy.context.view_layer.objects.active = geo_list[0]
    # duplicate selected objects
    bpy.ops.object.duplicate()
    # deselect geo_list objects
    for obj in geo_list:
        obj.select_set(False)
    # create new object list
    new_geo_list = []
    for obj in bpy.data.objects:
        if obj not in all_objects_list:
            new_geo_list.append(obj)
    # set new_geo_list[0] to active
    bpy.context.view_layer.objects.active = new_geo_list[0]
    # join all selected objects    
    bpy.ops.object.join()

    # find new object not in all_objects_list
    new_obj = None
    for obj in bpy.data.objects:
        if obj not in all_objects_list:
            new_obj = obj
            break
    # rename new object to shrinkwrap_target
    new_obj.name = 'shrinkwrap_target'
    # hide new object
    new_obj.hide_set(True)
    # remove all materials from new object
    new_obj.data.materials.clear()
    # remove all modifiers from new object
    new_obj.modifiers.clear()
    # remove all vertex groups from new object
    new_obj.vertex_groups.clear()
    return new_obj

def add_shrinkwrap_modifier(obj, target_obj):
    # add shrinkwrap modifier to obj
    mod = obj.modifiers.new(name='Shrinkwrap', type='SHRINKWRAP')
    mod.target = target_obj
    mod.wrap_method = 'NEAREST_SURFACEPOINT'
    mod.wrap_mode = 'OUTSIDE_SURFACE'
#    mod.offset = 0.5
    mod.offset = 0.0005
    return mod

def calculate_geometric_center(obj_geo):
    # calculate geometric center of obj_geo from vertices
    # get vertices
    vertices = obj_geo.data.vertices
    # calculate geometric center
    geometric_center = sum([v.co for v in vertices], Vector()) / len(vertices)
    # convert to world space
    geometric_center = obj_geo.matrix_world @ geometric_center
    return geometric_center

def calculate_bounding_box(obj_geo):
    return

def add_cage_and_attachments():
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

    if USE_SHRINKWRAP_TARGET:
        shrinkwrap_target = create_shrinkwrap_target()
    for obj in loaded_objects:
        if obj.name.endswith('_OuterCage'):
            # object mode
            bpy.ops.object.mode_set(mode='OBJECT')
            # deselect all
            bpy.ops.object.select_all(action='DESELECT')
            # apply all transforms
            obj.select_set(True)
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
            # calculate cage center
            cage_center = calculate_geometric_center(obj)

            # find corresponding geo object
            geo_name = obj.name.replace('_OuterCage', '_Geo')
            geo_obj = bpy.data.objects[geo_name]
            # calculate geometric center
            geometric_center = calculate_geometric_center(geo_obj)

            # # set 3d cursor to geometric center
            # bpy.context.scene.cursor.location = geometric_center

            # # debugging visualization
            # # create cube at geometric center, named after geo_obj
            # bpy.ops.mesh.primitive_cube_add(size=0.01, location=geometric_center)
            # cube = bpy.context.active_object
            # cube.name = geo_name.replace('_Geo', '_Geo_Center')
            # bpy.ops.mesh.primitive_cube_add(size=0.01, location=cage_center)
            # cube = bpy.context.active_object
            # cube.name = geo_name.replace('_Geo', '_Cage_Center')

            # calculate offset
            offset = cage_center - geometric_center
            strength = 0.25
            # apply offset to cage world matrix translation
            obj.matrix_world.translation -= (offset * strength)
            # apply transform
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

            if USE_SHRINKWRAP_TARGET:
                add_shrinkwrap_modifier(obj, shrinkwrap_target)
            else:
                add_shrinkwrap_modifier(obj, geo_obj)


    # cleanup all unused and unlinked data blocks
    # print("DEBUG: main(): cleaning up unused data blocks...")
    bpy.ops.outliner.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)            