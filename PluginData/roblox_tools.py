"""Roblox Tools module
2024-05-10 - new function to bind attachments
2024-05-30 - copy facial animations

Contains several convenience functions to generate Roblox Avatar compatible character.

Requirements:
    - Python 3+
    - Blender 3.6+
"""
from pathlib import Path
script_dir = str(Path( __file__ ).parent.absolute())

REPOSITION_ATTACHMENTS = True
REPOSITION_CAGES = True
ADD_SHRINKWRAP = False
USE_SHRINKWRAP_TARGET = False

logFilename = "roblox_tools.log"

import os
import mathutils
from mathutils import Matrix
from mathutils import Vector
try:
    import bpy
except:
    print("DEBUG: blender python libraries not detected, continuing for pydoc mode.")

def _add_to_log(sMessage):
    print(str(sMessage))
    with open(logFilename, "a") as file:
        file.write(sMessage + "\n")

template_file_name = "/Daz_Cage_Att_Template.blend"
blend_file_path = script_dir + template_file_name

_animation_template_filename = script_dir + "/Genesis9Action.blend"

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

# returns the geometric center of obj_geo in worldspace
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

# DB-2024-05-10: Old function to add cage and attachments, uses separate blend template file.
#      New export from Daz Studio adds cage and attachments in C++ with FbxTools and MvcTools,
#      and relies on blender python to reposition cages and attachments.  See functions below:
#      bind_attachment_to_bone_inplace() and relocalize_attachment()
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

            if ADD_SHRINKWRAP:
                if USE_SHRINKWRAP_TARGET:
                    add_shrinkwrap_modifier(obj, shrinkwrap_target)
                else:
                    add_shrinkwrap_modifier(obj, geo_obj)

    # cleanup all unused and unlinked data blocks
    # print("DEBUG: main(): cleaning up unused data blocks...")
    bpy.ops.outliner.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)            

# DB-2024-05-10: New function to bind attachments to bones in place, and relocalize attachments
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

# DB-2024-05-10: New function to relocalize attachments. Specifically,
#      this function will calculate the worldspace center of the attachment,
#      and reposition vertices to use the local object origin and then
#      reposition the object to the calculated center.
def relocalize_attachment(obj):
    if obj.name not in lookup_table:
        return
    print("DEBUG: relocalizing :" + obj.name)

    center = calculate_geometric_center(obj)
    # Decompose matrix for object's parent_bone
    parent_bone_name = obj.parent_bone
    if parent_bone_name is None:
        return
    print("DEBUG: parent_bone = " + parent_bone_name)
    parent_bone_matrix = obj.parent.pose.bones[parent_bone_name].matrix
    parent_bone_translation, parent_bone_rotation, parent_bone_scale = parent_bone_matrix.decompose()
    # Decompose the matrix
    translation, rotation, scale = obj.matrix_world.decompose()

    # set worldspace geometric center as new worldspace position
    new_translation = center
    # Create new rotation, zero local rotation is equal to parent bone world rotation
    # DB 2024-06-01: ...but we want zero world rotation? So use identity matrix
    ###new_rotation = parent_bone_rotation
    new_rotation = mathutils.Quaternion()
    # Create new scale, multiply desired local scale by parent bone scale
    new_scale = parent_bone_scale * 0.1
    # Construct new matrix
    new_matrix = Matrix.Translation(new_translation) @ new_rotation.to_matrix().to_4x4() @ Matrix.Diagonal(new_scale).to_4x4()

    # In order to keep same worldspace position and orientation of vertices...
    # ...bake inverse of new matrix to vertices
    bake_transform_to_vertices(obj, new_matrix.inverted())
    # Assign the new matrix to the object
    obj.matrix_world = new_matrix

# DB-2024-05-10: New function to bake worldspace offset to vertex buffer
def bake_transform_to_vertices(obj, transform_matrix):
    if obj is None:
        return
    # if offset is not matrix, return
    if not isinstance(transform_matrix, Matrix):
        return   
    print("DEBUG: baking offset to vertices for: " + obj.name)

    # iterate through all vertices and multiply by offset matrix
    vertices = obj.data.vertices
    for v in vertices:
        v.co = transform_matrix @ v.co

# DB-2024-05-30: Copy facial animations from template file
def copy_facial_animations(animation_template_filename=""):
    armature_name = "Genesis9"
    action_name = "Genesis9Action"

    if animation_template_filename == "":
        animation_template_filename = _animation_template_filename

    # Load action from template file
    with bpy.data.libraries.load(animation_template_filename, link=False) as (data_from, data_to):
        data_to.actions = [name for name in data_from.actions if name == action_name]

    # Asign loaded action to existing armature
    armature_object = bpy.data.objects[armature_name]
    action = bpy.data.actions[action_name]
    armature_object.animation_data_create()  # Create animation data if not already present
    armature_object.animation_data.action = action

    # # Create a temporary NLA track to ensure the action is exported
    # nla_track = armature_object.animation_data.nla_tracks.new()
    # nla_strip = nla_track.strips.new(name=action_name, start=0, action=action)

    # Deselect all objects
    bpy.ops.object.select_all(action='DESELECT')
    # make armature active object
    armature_object.select_set(True)
    bpy.context.view_layer.objects.active = armature_object

    # switch to "Edit Mode"
    bpy.ops.object.mode_set(mode='EDIT')

    # Get edit bone "Head"
    head_bone = armature_object.data.edit_bones.get('Head')

    # Duplicate head bone
    dynamichead_bone = armature_object.data.edit_bones.new(name="DynamicHead")
    dynamichead_bone.head = head_bone.head
    dynamichead_bone.tail = head_bone.tail
    dynamichead_bone.roll = head_bone.roll
    dynamichead_bone.use_connect = False

    dynamichead_bone.parent = head_bone

    # move all children from head to dynamichead
    for child in head_bone.children:
        child.parent = dynamichead_bone

    # Switch back to Object Mode
    bpy.ops.object.mode_set(mode='OBJECT')

    # set up custom properties for Head_Geo
    head_geo_obj = bpy.data.objects['Head_Geo']
    head_geo_obj["RootFaceJoint"] = "DynamicHead"
    head_geo_obj["Frame0"] = "Neutral"
    head_geo_obj["Frame1"] = "EyesLookDown"
    head_geo_obj["Frame2"] = "EyesLookLeft"
    head_geo_obj["Frame3"] = "EyesLookRight"
    head_geo_obj["Frame4"] = "EyesLookUp"
    head_geo_obj["Frame5"] = "JawDrop"
    head_geo_obj["Frame6"] = "LeftEyeClosed"
    head_geo_obj["Frame7"] = "LeftLipCornerPuller"
    head_geo_obj["Frame8"] = "LeftLipStretcher"
    head_geo_obj["Frame9"] = "LeftLowerLipDepressor"
    head_geo_obj["Frame10"] = "LeftUpperLipRaiser"
    head_geo_obj["Frame11"] = "LipsTogether"
    head_geo_obj["Frame12"] = "Pucker"
    head_geo_obj["Frame14"] = "RightEyeClosed"
    head_geo_obj["Frame15"] = "RightLipCornerPuller"
    head_geo_obj["Frame16"] = "RightLipStretcher"
    head_geo_obj["Frame17"] = "RightLowerLipDepressor"
    head_geo_obj["Frame18"] = "RightUpperLipRaiser"

    bpy.context.scene.frame_start = 0
    bpy.context.scene.frame_end = 18

    _add_to_log("Facial animations copied successfully.")

# DB 2024-06-05: Fix UGC Validation issues
def ugc_validation_fixes():
    # switch to bone edit mode
    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action='DESELECT')
    bpy.data.objects["Genesis9"].select_set(True)
    bpy.context.view_layer.objects.active = bpy.data.objects["Genesis9"]
    bpy.ops.object.mode_set(mode="EDIT")

    # fix lowertorso
    bpy.ops.armature.select_all(action='DESELECT')
    print("DEBUG: FIXING LowerTorso bone...")
    bpy.data.objects["Genesis9"].data.edit_bones["LowerTorso"].select = True
    old_z = bpy.data.objects["Genesis9"].data.edit_bones["LowerTorso"].head.z
    new_z = old_z - 0.1
    bpy.data.objects["Genesis9"].data.edit_bones["LowerTorso"].head.z = new_z

    # fix left hip
    bpy.ops.armature.select_all(action='DESELECT')
    print("DEBUG: FIXING LeftUpperLeg bone...")
    bpy.data.objects["Genesis9"].data.edit_bones["LeftUpperLeg"].select = True
    old_z = bpy.data.objects["Genesis9"].data.edit_bones["LeftUpperLeg"].head.z
    new_z = old_z - 0.05
    tail_x = bpy.data.objects["Genesis9"].data.edit_bones["LeftLowerLeg"].tail.x
    bpy.data.objects["Genesis9"].data.edit_bones["LeftUpperLeg"].head.z = new_z
    bpy.data.objects["Genesis9"].data.edit_bones["LeftUpperLeg"].head.x = tail_x

    # fix right hip
    bpy.ops.armature.select_all(action='DESELECT')
    print("DEBUG: FIXING RightUpperLeg bone...")
    bpy.data.objects["Genesis9"].data.edit_bones["RightUpperLeg"].select = True
    old_z = bpy.data.objects["Genesis9"].data.edit_bones["RightUpperLeg"].head.z
    new_z = old_z - 0.05
    tail_x = bpy.data.objects["Genesis9"].data.edit_bones["RightLowerLeg"].tail.x
    bpy.data.objects["Genesis9"].data.edit_bones["RightUpperLeg"].head.z = new_z
    bpy.data.objects["Genesis9"].data.edit_bones["RightUpperLeg"].head.x = tail_x

    bpy.ops.object.mode_set(mode="OBJECT")
