"""Blender Convert DTU to Roblox Blend (R15)

This is a command-line script to import a dtu/fbx intermediate file pair into
Blender and convert it to a format compatible with direct import into
Roblox Studio, aka R15.

- Developed and tested with Blender 3.6.1 (Python 3.10.12)
- Uses modified blender_tools.py module
- Requires Blender 3.6 or later

USAGE: blender.exe --background --python blender_dtu_to_roblox_R15.py <fbx file>

EXAMPLE:

    C:/Blender3.6/blender.exe --background --python blender_dtu_to_roblox_R15.py C:/Users/dbui/Documents/DazToRoblox/Amelia9YoungAdult/Amelia9YoungAdult.fbx

"""
do_experimental_remove_materials = True


logFilename = "blender_dtu_to_roblox_R15.log"

## Do not modify below
def _print_usage():
    # print("Python version: " + str(sys.version))
    print("\nUSAGE: blender.exe --background --python blender_dtu_to_roblox_R15.py <fbx file>\n")

from pathlib import Path
script_dir = str(Path( __file__ ).parent.absolute())

import sys
import os
import json
import re
import shutil
import mathutils
from mathutils import Matrix
try:
    import bpy
except:
    print("DEBUG: blender python libraries not detected, continuing for pydoc mode.")

try:
    import blender_tools
    blender_tools.logFilename = logFilename
    import roblox_tools
    import game_readiness_tools
    from game_readiness_roblox_data import *
except:
    sys.path.append(script_dir)
    import blender_tools
    import roblox_tools
    import game_readiness_tools
    from game_readiness_roblox_data import *

decimation_lookup = {
                    "Skullcap_DecimationGroup": 0.905,
                    "NonFace_DecimationGroup": 0.5,
                    "Face_DecimationGroup": 0.6,
                    "UpperTorso_DecimationGroup": 0.15,
                    "LowerTorso_DecimationGroup": 0.223,
                    "RightUpperArm_DecimationGroup": 0.15,
                    "LeftUpperArm_DecimationGroup": 0.15,
                    "RightLowerArm_DecimationGroup": 0.15,
                    "LeftLowerArm_DecimationGroup": 0.15,
                    "RightHand_DecimationGroup": 0.1,
                    "LeftHand_DecimationGroup": 0.1,
                    "RightUpperLeg_DecimationGroup": 0.15,
                    "LeftUpperLeg_DecimationGroup": 0.15,
                    "RightLowerLeg_DecimationGroup": 0.15,
                    "LeftLowerLeg_DecimationGroup": 0.15,
                    "RightFoot_DecimationGroup": 0.14,
                    "LeftFoot_DecimationGroup": 0.14,
}


def _add_to_log(sMessage):
    print(str(sMessage))
    with open(logFilename, "a") as file:
        file.write(sMessage + "\n")

def _main(argv):
    try:
        line = str(argv[-1])
    except:
        _print_usage()
        return

    try:
        start, stop = re.search("#([0-9]*)\.", line).span(0)
        token_id = int(line[start+1:stop-1])
        print(f"DEBUG: token_id={token_id}")
    except:
        print(f"ERROR: unable to parse token_id from '{line}'")
        token_id = 0

    blender_tools.delete_all_items()
    blender_tools.switch_to_layout_mode()

    fbxPath = line.replace("\\","/").strip()
    if (not os.path.exists(fbxPath)):
        _add_to_log("ERROR: main(): fbx file not found: " + str(fbxPath))
        exit(1)
        return

    # prepare intermediate folder paths
    blenderFilePath = fbxPath.replace(".fbx", ".blend")
    intermediate_folder_path = os.path.dirname(fbxPath)

    # load FBX
    _add_to_log("DEBUG: main(): loading fbx file: " + str(fbxPath))
    blender_tools.import_fbx(fbxPath)

    bpy.ops.object.select_all(action="SELECT")
    # Loop through all objects in the scene
    cage_obj_list = []
    for obj in bpy.data.objects:
        print(f"Processing object: {obj.name}")
        # if cage, transfer weights and parent to armature
        if obj.type == 'MESH' and "_OuterCage" in obj.name:
            cage_obj_list.append(obj)
            transfer_weights("Genesis9.Shape", obj.name)
        # if attachment, parent to bone
        if obj.type == 'MESH' and "_Att" in obj.name:
            roblox_tools.bind_attachment_to_bone_inplace(obj)

    blender_tools.fix_eyes()
    blender_tools.fix_scalp()

    blender_tools.center_all_viewports()
    jsonPath = fbxPath.replace(".fbx", ".dtu")
    _add_to_log("DEBUG: main(): loading json file: " + str(jsonPath))
    dtu_dict = blender_tools.process_dtu(jsonPath)

    if "Has Animation" in dtu_dict:
        bHasAnimation = dtu_dict["Has Animation"]
        # FUTURE TODO: import and process facial animation
    else:
        bHasAnimation = False


    # clear all animation data
    # Iterate over all objects
    print("DEBUG: main(): clearing animation data")
    for obj in bpy.data.objects:
        # Check if the object has animation data
        if obj.animation_data:
            # Clear all animation data
            obj.animation_data_clear()        


    # move root node to origin
    print("DEBUG: main(): moving root node to origin")
    move_root_node_to_origin()

    daz_generation = dtu_dict["Asset Id"]
    if (bHasAnimation == False):
        # if ("Genesis8" in daz_generation):
        #     blender_tools.apply_tpose_for_g8_g9()
        # elif ("Genesis9" in daz_generation):
        #     blender_tools.apply_tpose_for_g8_g9()
        apply_i_pose()

    # # add decimate modifier
    # add_decimate_modifier()

    # # separate by materials (and delete unwanted meshes)
    # separate_by_materials()

    # # separate by loose parts
    # separate_by_loose_parts()

    # # separate by bone influence
    # separate_by_bone_influence()

    main_obj = None
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            if obj.name == "Genesis9.Shape":
                print("DEBUG: main(): obj.name=" + obj.name)
                main_obj = obj
                break
    if main_obj is None:
        _add_to_log("ERROR: main(): main_obj not found.")
        return
    bpy.context.view_layer.objects.active = main_obj

    if True:
        # Create duplicate of original Genesis9.Shape to perform baking later
        obj = main_obj
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.duplicate()
        if obj != bpy.context.object and obj.name in bpy.context.object.name:
            bpy.context.object.name = "HD_Genesis9.Shape"
            # repeat for eyes
            for obj in bpy.data.objects:
                if obj.type == 'MESH' and obj.name == "Genesis9Eyes.Shape":
                    bpy.ops.object.select_all(action='DESELECT')
                    obj.select_set(True)
                    bpy.context.view_layer.objects.active = obj
                    bpy.ops.object.duplicate()
                    if obj != bpy.context.object and obj.name in bpy.context.object.name:
                        bpy.context.object.name = "HD_Genesis9Eyes.Shape"
                        # join to hd body
                        bpy.ops.object.select_all(action='DESELECT')
                        bpy.data.objects["HD_Genesis9.Shape"].select_set(True)
                        bpy.data.objects["HD_Genesis9Eyes.Shape"].select_set(True)
                        bpy.context.view_layer.objects.active = bpy.data.objects["HD_Genesis9.Shape"]
                        # join
                        bpy.ops.object.join()
                    else:
                        print(f"DEBUG: HD eyes duplication failed, bpy.context.object.name={bpy.context.object.name}")
                        exit(-1)
                    break
        else:
            print(f"DEBUG: HD duplication failed, bpy.context.object.name={bpy.context.object.name}")
            exit(-1)

    # decimate mouth
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and obj.name == "Genesis9Mouth.Shape":
            print("DEBUG: main(): obj.name=" + obj.name)
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            new_modifier = obj.modifiers.new(name="Mouth", type='DECIMATE')
            new_modifier.name = "Mouth"
            new_modifier.decimate_type = 'COLLAPSE'
            new_modifier.ratio = 0.08
            new_modifier.use_collapse_triangulate = True
            new_modifier.use_symmetry = True
            bpy.ops.object.modifier_apply(modifier="Mouth")
            break

    # decimate eyes
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and obj.name == "Genesis9Eyes.Shape":
            print("DEBUG: main(): obj.name=" + obj.name)
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            new_modifier = obj.modifiers.new(name="Eyes", type='DECIMATE')
            new_modifier.name = "Eyes"
            new_modifier.decimate_type = 'COLLAPSE'
            new_modifier.ratio = 0.182
            new_modifier.use_collapse_triangulate = True
            new_modifier.use_symmetry = True
            bpy.ops.object.modifier_apply(modifier="Eyes")
            break

    figure_list = ["genesis9.shape", "genesis9mouth.shape", "genesis9eyes.shape", "hd_genesis9.shape"]
    # add any custom eyes or eyelashes to safe list
    head_accessories_list = []
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and (
            "eyebrow" in obj.name.lower() or
            "eyelash" in obj.name.lower()
            ):
            head_accessories_list.append(obj.name.lower())
    cage_list = []
    cage_obj_list = []
    att_list = []
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            game_readiness_tools.remove_moisture_materials(obj)
            if "_OuterCage" in obj.name:
                print("DEBUG: cage obj.name=" + obj.name)
                cage_list.append(obj.name.lower())
                cage_obj_list.append(obj)
            elif "_Att" in obj.name:
                print("DEBUG: attachment obj.name=" + obj.name)
                att_list.append(obj.name.lower())
    game_readiness_tools.remove_extra_meshes(figure_list + cage_list + att_list + head_accessories_list)
    game_readiness_tools.remove_extra_materials(["body", "cage_material", "attachment_material"], head_accessories_list)

    # create new collection for cages
    cage_collection = bpy.data.collections.new(name="Cages")
    # Link the new collection to the scene
    bpy.context.scene.collection.children.link(cage_collection)
    # move cages to new collection
    for obj in cage_obj_list:
        # Unlink the object from its current collection(s)
        for col in obj.users_collection:
            col.objects.unlink(obj)
        # Link the object to the new collection
        cage_collection.objects.link(obj)

    # set up game_engine eyelashes and eyebrows if present
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and (
            "eyebrow" in obj.name.lower() or
            "eyelash" in obj.name.lower()
            ):
            if "_innercage" in obj.name.lower() or "_outercage" in obj.name.lower():
                continue
            # unparent and keep transforms
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
            # apply all transforms
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
            # rename object
            obj.name = obj.name.replace("game_engine_", "Genesis9_").replace("_0", "").replace(".Shape", "")
            # add custom properties
            roblox_tools.add_custom_dynamichead_properties(obj)
            # create inner/outer cages
            cagename_template = obj.name
            # get source cage ("Head_OuterCage")
            source_cage = bpy.data.objects.get("Head_OuterCage")
            if source_cage is not None:
                cage_name = cagename_template + "_OuterCage"
                if bpy.data.objects.get(cage_name) is None:
                    bpy.ops.object.select_all(action='DESELECT')
                    source_cage.select_set(True)
                    bpy.context.view_layer.objects.active = source_cage
                    bpy.ops.object.duplicate()
                    if source_cage != bpy.context.object and source_cage.name in bpy.context.object.name:
                        bpy.context.object.name = cage_name

                cage_name = cagename_template + "_InnerCage"
                if bpy.data.objects.get(cage_name) is None:
                    bpy.ops.object.select_all(action='DESELECT')
                    source_cage.select_set(True)
                    bpy.context.view_layer.objects.active = source_cage
                    bpy.ops.object.duplicate()
                    if source_cage != bpy.context.object and source_cage.name in bpy.context.object.name:
                        bpy.context.object.name = cage_name

    # read from python data file (import vertex_indices_for_daztoroblox.py)
    for group_name in geo_group_names + decimation_group_names:
        print("DEBUG: creating vertex group: " + group_name)
        # evaluate group_name to python variable name
        vertex_index_buffer = globals()[group_name]
        game_readiness_tools.create_vertex_group(main_obj, group_name, vertex_index_buffer)

    # separate by vertex group
    for group_name in geo_group_names:
        print("DEBUG: separating by vertex group: " + group_name)
        game_readiness_tools.separate_by_vertexgroup(main_obj, group_name)

    # add eyes and mouth to head_geo
    # deselect all
    bpy.ops.object.select_all(action='DESELECT')
    for obj_name in ["Genesis9Eyes.Shape", "Genesis9Mouth.Shape", "Head_Geo"]:
        obj = bpy.data.objects.get(obj_name)
        if obj is not None:
            obj.select_set(True)
    # head_geo to active
    bpy.context.view_layer.objects.active = bpy.data.objects.get("Head_Geo")
    bpy.ops.object.join()
    
    # delete genesis9.shape
    bpy.ops.object.select_all(action='DESELECT')
    obj = bpy.data.objects.get("Genesis9.Shape")
    if obj is not None:
        obj.select_set(True)
        bpy.ops.object.delete()

    # for each obj, select correct decimation group and add decimate modifier
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            if obj.name == "Head_Geo":
                # custom add decimate modifier
                for group_name in ["Skullcap_DecimationGroup", "NonFace_DecimationGroup", "Face_DecimationGroup"]:
                    try:
                        decimate_ratio = decimation_lookup[group_name]
                    except:
                        decimate_ratio = 0.91
                    game_readiness_tools.add_decimate_modifier_per_vertex_group(obj, group_name, decimate_ratio)
                continue
            for group_name in decimation_group_names:
                print("DEBUG: decimation check: " + obj.name.lower() + ", group_name=" + group_name.lower())
                if obj.name.lower().replace("_geo","") in group_name.lower():
                    print("DEBUG: decimation match: " + obj.name + ", group_name=" + group_name)
                    print("DEBUG: adding decimate modifier for: " + obj.name + ", group_name=" + group_name)
                    try:
                        decimate_ratio = decimation_lookup[group_name]
                    except:
                        decimate_ratio = 0.91
                    game_readiness_tools.add_decimate_modifier_per_vertex_group(obj, group_name, decimate_ratio)
                    break

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

    # pack all images
    print("DEBUG: main(): packing all images...")
    bpy.ops.file.pack_all()

    # select all objects
    bpy.ops.object.select_all(action="SELECT")
    # set active object
    bpy.context.view_layer.objects.active = bpy.context.selected_objects[0]

    # switch to object mode before saving (pre-processing save)
    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.wm.save_as_mainfile(filepath=blenderFilePath)

    # unparent attachments
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and "_Att" in obj.name:
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
    # select all
    bpy.ops.object.select_all(action="SELECT")
    # set origin to 3d cursor
    bpy.context.scene.cursor.location = (0.0, 0.0, 0.0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')

    # NEW SCALING CODE
    bpy.context.scene.unit_settings.scale_length = 1/28

    # apply using "All Transforms to Deltas"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # re-bind attachments to bones
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and "_Att" in obj.name:
            roblox_tools.bind_attachment_to_bone_inplace(obj)
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and "_Att" in obj.name:
            roblox_tools.relocalize_attachment(obj)
    # Fix orientation of Grip attachments
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and "_Att" in obj.name and "Grip" in obj.name:
            # set rotation axes separately with Quaternions to avoid gimbal lock
            euler_x = mathutils.Euler((1.5708, 0, 0))
            euler_y = mathutils.Euler((0, -0.17453292, 0))
            euler_z = mathutils.Euler((0, 0, -1.5708))
            new_rotation = mathutils.Quaternion(euler_x) @ mathutils.Quaternion(euler_y)
            new_matrix = obj.matrix_world @ new_rotation.to_matrix().to_4x4()
            obj.matrix_world = new_matrix

    # add cage and attachments
    #roblox_tools.add_cage_and_attachments()

    # copy facial animations
    #roblox_tools.copy_facial_animations(script_dir + "/Genesis9Action.blend")
    roblox_tools.copy_facs50_animations(script_dir + "/Genesis9facs50.blend")

    # roblox UGC Validation Fixes
    roblox_tools.ugc_validation_fixes()

    # hide HD_Genesis9.Shape if present
    obj = bpy.data.objects.get("HD_Genesis9.Shape")
    if obj is not None:
        obj.hide_viewport = True

    # mesh naming fix so Roblox Studio GLB importer works
    # rename mesh data to object name
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            mesh_data = obj.data
            mesh_data.name = obj.name

    # prepare destination folder paths
    roblox_asset_name = dtu_dict["Asset Name"]
    roblox_output_path = dtu_dict["Output Folder"]
    destinationPath = roblox_output_path.replace("\\","/")
    if (not os.path.exists(destinationPath)):
        os.makedirs(destinationPath)
    fbx_base_name = os.path.basename(fbxPath)
    fbx_output_name = fbx_base_name.replace(".fbx", "_R15_avatar.fbx")
    fbx_output_file_path = os.path.join(destinationPath, fbx_output_name).replace("\\","/")
    blender_output_file_path = fbx_output_file_path.replace(".fbx", ".blend")
    glb_output_file_path = fbx_output_file_path.replace(".fbx", ".glb")

    # save blender file to destination
    bpy.ops.wm.save_as_mainfile(filepath=blender_output_file_path)

    # select armature
    bpy.ops.object.select_all(action="DESELECT")
    for obj in bpy.data.objects:
        if obj.type == 'ARMATURE':
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            armature = obj
            break

    if armature is None:
        _add_to_log("ERROR: main(): armature not found, unable to perform GLB scaling.")
    else:
        scale_factor = float(100/28)
        # Apply the scale work-around to animation keyframes
        blender_tools.propagate_scale_to_animation(armature, scale_factor)

    _add_to_log("DEBUG: saving Roblox FBX file to destination: " + fbx_output_file_path)
    # export to fbx
    try:
        bpy.ops.export_scene.fbx(filepath=fbx_output_file_path, 
                                 add_leaf_bones = False,
                                 path_mode = "COPY",
                                 embed_textures = True,
                                 use_visible = True,
                                 use_custom_props = True,
                                 bake_anim_use_nla_strips = False,
                                 bake_anim_use_all_actions = False
                                 )
        _add_to_log("DEBUG: save completed.")
    except Exception as e:
        _add_to_log("ERROR: unable to save Roblox FBX file: " + fbx_output_file_path)
        _add_to_log("EXCEPTION: " + str(e))

    # Reload blender file
    bpy.ops.wm.open_mainfile(filepath=blender_output_file_path)

    # unparent all geometry from armature (inlcuding attachments)
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            bpy.ops.object.select_all(action="DESELECT")
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')

    # select armature
    bpy.ops.object.select_all(action="DESELECT")
    for obj in bpy.data.objects:
        if obj.type == 'ARMATURE':
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            armature = obj
            break    

    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            obj.select_set(True)

    if armature is None:
       _add_to_log("ERROR: main(): armature not found, unable to perform GLB scaling.")
    else:
        # scale work-around because Blender GLTF exporter ignores scene scale
        scale_factor = float(100/28)
        bpy.ops.transform.resize(value=(scale_factor, scale_factor, scale_factor))
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        # Apply the scale work-around to animation keyframes
        blender_tools.propagate_scale_to_animation(armature, scale_factor)
        # re-bind attachments to bones
        for obj in bpy.data.objects:
            if obj.type == 'MESH' and "_Att" in obj.name:
                roblox_tools.bind_attachment_to_bone_inplace(obj)
        for obj in bpy.data.objects:
            if obj.type == 'MESH' and "_Att" in obj.name:
                roblox_tools.relocalize_attachment(obj)

        # Blender GLTF exporter is hardcoded to 24 fps, so set it here so that keyframes are not lost
        bpy.context.scene.render.fps = 24

    # # save debug blender file
    # blender_debug_file_path = blender_output_file_path.replace(".blend", "_debug.blend")
    # bpy.ops.wm.save_as_mainfile(filepath=blender_debug_file_path)

    # Apply all modifiers
    for obj in bpy.data.objects:
        blender_tools.apply_mesh_modifiers(obj)

    generate_final_glb = True
    if generate_final_glb:
        try:
            bpy.ops.export_scene.gltf(filepath=glb_output_file_path,
                                      export_format="GLB", 
                                      use_visible=True,
                                      use_selection=False, 
                                      export_extras=True,
                                      export_yup=True,
                                      export_texcoords=True,
                                      export_normals=True,
                                      export_rest_position_armature=True,
                                      export_skins=True,
                                      export_influence_nb=4,
                                      export_animations=True,
                                    #   export_animation_mode='SCENE',
                                    #   export_animation_mode='ACTIONS',
                                      export_animation_mode='ACTIVE_ACTIONS',
                                      export_nla_strips_merged_animation_name='FACS',
                                      export_current_frame=False,
                                      export_bake_animation=False,
                                      export_frame_range=False,
                                      export_frame_step=1,
                                      export_optimize_animation_size=False,
                                      export_optimize_animation_keep_anim_armature=False,
                                      export_optimize_animation_keep_anim_object=False,
                                      export_morph=False,
                                    )
            _add_to_log("DEBUG: save completed.")
        except Exception as e:
            _add_to_log("ERROR: unable to save Final GLB file: " + glb_output_file_path)
            _add_to_log("EXCEPTION: " + str(e))
            raise e

    _add_to_log("DEBUG: main(): completed conversion for: " + str(fbxPath))


def apply_i_pose():
    print("DEBUG: apply_i_pose()")
    # Object Mode
    bpy.ops.object.mode_set(mode="OBJECT")       
    #retrieve armature name
    armature_name = bpy.data.armatures[0].name
    for arm in bpy.data.armatures:
        if "genesis" in arm.name.lower():
            armature_name = arm.name
            print("DEBUG: armature_name=" + armature_name)
            break

    # create a list of objects with armature modifier
    armature_modifier_list = []
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            for mod in obj.modifiers:
                if mod.type == "ARMATURE" and mod.name == armature_name:
                    armature_modifier_list.append([obj, mod])
    print("DEBUG: armature_modifier_list=" + str(armature_modifier_list))

    # apply i-pose
    for obj in bpy.data.objects:
        if obj.type == 'ARMATURE':
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode="POSE")
            bpy.ops.pose.select_all(action='SELECT')
            bpy.ops.pose.rot_clear()
            bpy.ops.object.mode_set(mode="OBJECT")

    # select all objects
    bpy.ops.object.select_all(action="SELECT")
    # switch to pose mode
    bpy.ops.object.mode_set(mode="POSE")
    # go to frame 0
    bpy.context.scene.frame_set(0)
    # clear all pose transforms
    bpy.ops.pose.transforms_clear()
    # set tpose values for shoulders and hips
    if "LeftUpperArm" in bpy.context.object.pose.bones:
        _add_to_log("DEBUG: applying t-pose rotations...")
        # rotate hip "LowerTorso"
        # bpy.context.object.pose.bones["LowerTorso"].rotation_mode= "XYZ"
        # bpy.context.object.pose.bones["LowerTorso"].rotation_euler[0] = 0.17
        # UpperTorso
        # bpy.context.object.pose.bones["UpperTorso"].rotation_mode= "XYZ"
        # bpy.context.object.pose.bones["UpperTorso"].rotation_euler[0] = -0.17
        # rotate left shoulder 50 degrees along global y
        bpy.context.object.pose.bones["LeftUpperArm"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["LeftUpperArm"].rotation_euler[0] = 0.02731336
        bpy.context.object.pose.bones["LeftUpperArm"].rotation_euler[2] = -0.5127
        bpy.context.object.pose.bones["RightUpperArm"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["RightUpperArm"].rotation_euler[0] = 0.02731336
        bpy.context.object.pose.bones["RightUpperArm"].rotation_euler[2] = 0.5127
        # elbows
        bpy.context.object.pose.bones["LeftLowerArm"].rotation_mode= "XYZ"
        # bpy.context.object.pose.bones["LeftLowerArm"].rotation_euler[0] = 0.115
        # bpy.context.object.pose.bones["LeftLowerArm"].rotation_euler[1] = 0.079
        bpy.context.object.pose.bones["RightLowerArm"].rotation_mode= "XYZ"
        # bpy.context.object.pose.bones["RightLowerArm"].rotation_euler[0] = 0.115
        # bpy.context.object.pose.bones["RightLowerArm"].rotation_euler[1] = -0.079
        # wrists
        bpy.context.object.pose.bones["LeftHand"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["LeftHand"].rotation_euler[0] = -0.204557824
        bpy.context.object.pose.bones["LeftHand"].rotation_euler[1] = -0.113851841
        bpy.context.object.pose.bones["RightHand"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["RightHand"].rotation_euler[0] = -0.204557824
        bpy.context.object.pose.bones["RightHand"].rotation_euler[1] = 0.113851841
        # L and R hips to 5 degrees
        bpy.context.object.pose.bones["LeftUpperLeg"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["LeftUpperLeg"].rotation_euler[0] = -0.00296797
        bpy.context.object.pose.bones["LeftUpperLeg"].rotation_euler[1] = -0.09181723
        bpy.context.object.pose.bones["LeftUpperLeg"].rotation_euler[2] = -0.0448015547
        bpy.context.object.pose.bones["RightUpperLeg"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["RightUpperLeg"].rotation_euler[0] = -0.00296797
        bpy.context.object.pose.bones["RightUpperLeg"].rotation_euler[1] = 0.09181723
        bpy.context.object.pose.bones["RightUpperLeg"].rotation_euler[2] = 0.0448015547

    # if shapes are present in mesh, then return without baking t-pose since blender can not apply armature modifier
    for obj, mod in armature_modifier_list:
        if obj.data.shape_keys is not None:
            _add_to_log("DEBUG: shape keys found, skipping t-pose bake for G8/G9...")
            return

    # Object Mode
    bpy.ops.object.mode_set(mode="OBJECT")
    # duplicate and apply armature modifier
    for obj, mod in armature_modifier_list:
        _add_to_log("DEBUG: Duplicating armature modifier: " + obj.name + "." + mod.name)
        # select object
        _add_to_log("DEBUG: Selecting object: " + obj.name)
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        num_mods = len(obj.modifiers)
        _add_to_log("DEBUG: num_mods = " + str(num_mods))
        result = bpy.ops.object.modifier_copy(modifier=mod.name)
        _add_to_log("DEBUG: result=" + str(result) + ", mod.name=" + mod.name)
        if len(obj.modifiers) > num_mods:
            new_mod = obj.modifiers[num_mods]
            _add_to_log("DEBUG: Applying armature modifier: " + new_mod.name)
            try:
                result = bpy.ops.object.modifier_apply(modifier=new_mod.name)
            except Exception as e:
                _add_to_log("ERROR: Unable to apply armature modifier: " + str(e))
                _add_to_log("DEBUG: result=" + str(result) + ", mod.name=" + new_mod.name)
                bpy.ops.object.modifier_remove(modifier=new_mod.name)     
                return
            _add_to_log("DEBUG: result=" + str(result) + ", mod.name=" + new_mod.name)
        else:
            _add_to_log("DEBUG: Unable to retrieve duplicate, applying original: " + mod.name)
            result = bpy.ops.object.modifier_apply(modifier=mod.name)
            _add_to_log("DEBUG: result=" + str(result) + ", mod.name=" + mod.name)

    # pose mode
    bpy.ops.object.select_all(action="DESELECT")
    armature_obj = bpy.data.objects.get(armature_name)
    armature_obj.select_set(True)
    bpy.context.view_layer.objects.active = armature_obj
    bpy.ops.object.mode_set(mode="POSE")
    # apply pose as rest pose
    _add_to_log("DEBUG: Applying pose as rest pose...")
    bpy.ops.pose.armature_apply(selected=False)
    # Object Mode
    bpy.ops.object.mode_set(mode="OBJECT")
    # select all before returning
    bpy.ops.object.select_all(action="SELECT")

def move_root_node_to_origin():
    print("DEBUG: move_root_node_to_origin(): bpy.data.objects=" + str(bpy.data.objects))
    # move root node to origin
    for obj in bpy.data.objects:
        print("DEBUG: move_root_node_to_origin(): obj.name=" + obj.name + ", obj.type=" + obj.type)
        if obj.type == 'ARMATURE':
            # deselect all objects
            bpy.ops.object.select_all(action='DESELECT')
            # select armature object
            obj.select_set(True)
            # select "LowerTorso" bone
            bpy.context.view_layer.objects.active = obj            
            bpy.ops.object.mode_set(mode="EDIT")
            bpy.context.object.data.bones["LowerTorso"].select = True
            bone_head_pos_y = bpy.context.object.data.bones["LowerTorso"].head.y
            bone_head_pos_z = bpy.context.object.data.bones["LowerTorso"].head.z            
            print("DEBUG: move_root_node_to_origin(): bone_head_pos_y=" + str(bone_head_pos_y) + ", bone_head_pos_z=" + str(bone_head_pos_z))
            bpy.ops.object.mode_set(mode="OBJECT")
            # select all objects in object mode
            bpy.ops.object.select_all(action='SELECT')
            # move all objects by the inverse of bone_head_pos

            inverse_bone_head_pos_z = -0.01 * bone_head_pos_y
            # inverse_bone_head_pos_z = 0

            # inverse_bone_head_pos_x = -0.01 * bone_head_pos_z
            inverse_bone_head_pos_x = 0
            
            print("DEBUG: move_root_node_to_origin(): inverse_bone_head_pos_x=" + str(inverse_bone_head_pos_x) + ", inverse_bone_head_pos_z=" + str(inverse_bone_head_pos_z))
            bpy.ops.transform.translate(value=(inverse_bone_head_pos_x, 0, inverse_bone_head_pos_z))
            # apply transformation
            bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)

def add_decimate_modifier_old():
    # add decimate modifier
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.modifier_add(type='DECIMATE')
            bpy.context.object.modifiers["Decimate"].ratio = 0.2

# separate by materials, but also delete unwanted meshes and unwanted materials, and remerging some meshes back together
def separate_by_materials():
    # separate by materials
    bpy.ops.object.mode_set(mode="OBJECT")
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode="EDIT")
            bpy.ops.mesh.separate(type='MATERIAL')
            bpy.ops.object.mode_set(mode="OBJECT")

    # clean up unwanted materials
    bpy.ops.object.mode_set(mode="OBJECT")
    fingernail_obj = None
    arms_obj = None
    toenails_obj = None
    legs_obj = None
    eyes_list = []
    head_obj = None
    mouth_list = []
    for obj in bpy.data.objects:
        # query for material names of each obj
        if obj.type == 'MESH':
            obj_materials = obj.data.materials
            # if only one material, then rename object to material.name + "_Geo"
            if len(obj_materials) == 1:
                obj.name = obj_materials[0].name.replace(" ","") + "_Geo"
            for mat in obj_materials:
                # if "Tear" in mat.name or "moisture" in mat.name.lower() or "eyebrows" in mat.name.lower() or "eyelashes" in mat.name.lower() or "teeth" in mat.name.lower() or "mouth" in mat.name.lower():
                if "Tear" in mat.name or "moisture" in mat.name.lower() or "eyebrows" in mat.name.lower() or "eyelashes" in mat.name.lower():
                    # remove obj
                    print("DEBUG: Removing object " + obj.name + " with material: " + mat.name)
                    # delete heirarchy of object
                    descendents = obj.children
                    bpy.ops.object.select_all(action='DESELECT')
                    for ob in descendents:
                        ob.select_set(True)
                    bpy.ops.object.delete()
                    obj.select_set(True)
                    bpy.ops.object.delete()
                    break
                if "head" in mat.name.lower():
                    head_obj = obj
                    # get decimation modifier
                    decimate_modifier = None
                    for mod in obj.modifiers:
                        if mod.type == "DECIMATE":
                            decimate_modifier = mod
                            break
                    if decimate_modifier is not None:
                        # change decimate ratio to 0.36
                        decimate_modifier.ratio = 0.36
                        # apply decimate modifier
                        bpy.ops.object.select_all(action='DESELECT')
                        obj.select_set(True)
                        bpy.context.view_layer.objects.active = obj
                        bpy.ops.object.modifier_apply(modifier=decimate_modifier.name)

                if "eye" in mat.name.lower():
                    eyes_list.append(obj)
                    # get decimation modifier
                    decimate_modifier = None
                    for mod in obj.modifiers:
                        if mod.type == "DECIMATE":
                            decimate_modifier = mod
                            break
                    if decimate_modifier is not None:
                        # change decimate ratio to 0.09
                        decimate_modifier.ratio = 0.09
                        # apply decimate modifier
                        bpy.ops.object.select_all(action='DESELECT')
                        obj.select_set(True)
                        bpy.context.view_layer.objects.active = obj
                        bpy.ops.object.modifier_apply(modifier=decimate_modifier.name)
                if "teeth" in mat.name.lower():
                    mouth_list.append(obj)
                    # get decimation modifier
                    decimate_modifier = None
                    for mod in obj.modifiers:
                        if mod.type == "DECIMATE":
                            decimate_modifier = mod
                            break
                    if decimate_modifier is not None:
                        # change decimate ratio to 0.09
                        decimate_modifier.ratio = 0.09
                        # apply decimate modifier
                        bpy.ops.object.select_all(action='DESELECT')
                        obj.select_set(True)
                        bpy.context.view_layer.objects.active = obj
                        bpy.ops.object.modifier_apply(modifier=decimate_modifier.name)
                if "mouth cavity" in mat.name.lower():
                    mouth_list.append(obj)
                    # get decimation modifier
                    decimate_modifier = None
                    for mod in obj.modifiers:
                        if mod.type == "DECIMATE":
                            decimate_modifier = mod
                            break
                    if decimate_modifier is not None:
                        # change decimate ratio to 0.068
                        decimate_modifier.ratio = 0.068
                        # apply decimate modifier
                        bpy.ops.object.select_all(action='DESELECT')
                        obj.select_set(True)
                        bpy.context.view_layer.objects.active = obj
                        bpy.ops.object.modifier_apply(modifier=decimate_modifier.name)
                elif "mouth" in mat.name.lower():
                    mouth_list.append(obj)
                    # get decimation modifier
                    decimate_modifier = None
                    for mod in obj.modifiers:
                        if mod.type == "DECIMATE":
                            decimate_modifier = mod
                            break
                    if decimate_modifier is not None:
                        # change decimate ratio to 0.036
                        decimate_modifier.ratio = 0.036
                        # apply decimate modifier
                        bpy.ops.object.select_all(action='DESELECT')
                        obj.select_set(True)
                        bpy.context.view_layer.objects.active = obj
                        bpy.ops.object.modifier_apply(modifier=decimate_modifier.name)
                
                if "fingernails" in mat.name.lower():
                    fingernail_obj = obj
                if "arms" in mat.name.lower():
                    arms_obj = obj
                if "toenails" in mat.name.lower():
                    toenails_obj = obj
                if "legs" in mat.name.lower():
                    legs_obj = obj
    
    # merge objects
    print("DEBUG: merging objects...")
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = bpy.data.objects[0]
    bpy.ops.object.mode_set(mode="OBJECT")
    if fingernail_obj is not None and arms_obj is not None:
        # deselect all objects
        bpy.ops.object.select_all(action='DESELECT')
        arms_obj.select_set(True)
        fingernail_obj.select_set(True)
        bpy.context.view_layer.objects.active = arms_obj
        bpy.ops.object.join()
        if do_experimental_remove_materials:
            bpy.context.view_layer.objects.active = arms_obj
            # remove material named "Fingernails"
            material_name = "Fingernails"
            material_slot = next((slot for slot in arms_obj.material_slots if slot.name == material_name), None)
            # If the material slot exists, remove it
            if material_slot:
                bpy.context.object.active_material_index = arms_obj.material_slots.find(material_name)
                bpy.ops.object.material_slot_remove()

    if toenails_obj is not None and legs_obj is not None:
        # deselect all objects
        bpy.ops.object.select_all(action='DESELECT')
        legs_obj.select_set(True)
        toenails_obj.select_set(True)
        bpy.context.view_layer.objects.active = legs_obj
        bpy.ops.object.join()
        if do_experimental_remove_materials:
            bpy.context.view_layer.objects.active = legs_obj
            # remove material named "Toenails"
            material_name = "Toenails"
            material_slot = next((slot for slot in legs_obj.material_slots if slot.name == material_name), None)
            # If the material slot exists, remove it
            if material_slot:
                bpy.context.object.active_material_index = legs_obj.material_slots.find(material_name)
                bpy.ops.object.material_slot_remove()

    if len(eyes_list) > 0 and head_obj is not None:
        # merge eyes
        print("DEBUG: merging eyes...")
        bpy.ops.object.select_all(action='DESELECT')
        head_obj.select_set(True)
        for obj in eyes_list:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = head_obj
        bpy.ops.object.mode_set(mode="OBJECT")
        bpy.ops.object.join()
        if do_experimental_remove_materials:
            bpy.context.view_layer.objects.active = head_obj
            # remove material named "Eye Left" and "Eye Right"
            material_name = "Eye Left"
            material_slot = next((slot for slot in head_obj.material_slots if slot.name == material_name), None)
            # If the material slot exists, remove it
            if material_slot:
                bpy.context.object.active_material_index = head_obj.material_slots.find(material_name)
                bpy.ops.object.material_slot_remove()
            material_name = "Eye Right"
            material_slot = next((slot for slot in head_obj.material_slots if slot.name == material_name), None)
            # If the material slot exists, remove it
            if material_slot:
                bpy.context.object.active_material_index = head_obj.material_slots.find(material_name)
                bpy.ops.object.material_slot_remove()

    if len(mouth_list) > 0 and head_obj is not None:
        # merge mouth, mouth cavity, and teeth
        print("DEBUG: merging mouth, mouth cavity, and teeth...")
        bpy.ops.object.select_all(action='DESELECT')
        head_obj.select_set(True)
        for obj in mouth_list:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = head_obj
        bpy.ops.object.mode_set(mode="OBJECT")
        bpy.ops.object.join()
        if do_experimental_remove_materials:
            bpy.context.view_layer.objects.active = head_obj
            # remove material named "Mouth Cavity" and "Teeth"
            material_name = "Mouth Cavity"
            material_slot = next((slot for slot in head_obj.material_slots if slot.name == material_name), None)
            # If the material slot exists, remove it
            if material_slot:
                bpy.context.object.active_material_index = head_obj.material_slots.find(material_name)
                bpy.ops.object.material_slot_remove()
            material_name = "Teeth"
            material_slot = next((slot for slot in head_obj.material_slots if slot.name == material_name), None)
            # If the material slot exists, remove it
            if material_slot:
                bpy.context.object.active_material_index = head_obj.material_slots.find(material_name)
                bpy.ops.object.material_slot_remove()
            # remove material named "Mouth"
            material_name = "Mouth"
            material_slot = next((slot for slot in head_obj.material_slots if slot.name == material_name), None)
            # If the material slot exists, remove it
            if material_slot:
                bpy.context.object.active_material_index = head_obj.material_slots.find(material_name)
                bpy.ops.object.material_slot_remove()

    print("DEBUG: done separating by materials")

def separate_by_loose_parts():
    print("DEBUG: separate_by_loose_parts()")
    # separate by loose parts
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode="EDIT")
            bpy.ops.mesh.separate(type='LOOSE')
            bpy.ops.object.mode_set(mode="OBJECT")

    # clean up loose parts
    right_arm = []
    left_arm = []
    right_leg = []
    left_leg = []
    head_list = []
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            obj_materials = obj.data.materials
            for mat in obj_materials:
                if "Head" in mat.name:
                    head_list.append(obj)
                    # break to next obj
                    break
                if "Arms" in mat.name:
                    # check vertices of obj, if x position is less than 0 then collect into right_arm array to merge together
                    for v in obj.data.vertices:
                        if v.co.x > 0:
                            left_arm.append(obj)
                            break
                        else:
                            right_arm.append(obj)
                            break
                    # break to next obj
                    break
                if "Legs" in mat.name:
                    # check vertices of obj, if x position is less than 0 then collect into right_leg array to merge together
                    for v in obj.data.vertices:
                        if v.co.x > 0:
                            left_leg.append(obj)
                            break
                        else:
                            right_leg.append(obj)
                            break
                    # break to next obj
                    break

    # merge right_arm
    print("DEBUG: merging right_arm...")
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = bpy.data.objects[0]
    bpy.ops.object.mode_set(mode="OBJECT")
    if len(right_arm) > 0:
        bpy.ops.object.select_all(action='DESELECT')
        for obj in right_arm:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = right_arm[0]
        bpy.ops.object.join()
        right_arm[0].name = "RightArm_Geo"
    # merge left_arm
    print("DEBUG: merging left_arm...")
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = bpy.data.objects[0]
    bpy.ops.object.mode_set(mode="OBJECT")
    if len(left_arm) > 0:
        bpy.ops.object.select_all(action='DESELECT')
        for obj in left_arm:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = left_arm[0]
        bpy.ops.object.join()
        left_arm[0].name = "LeftArm_Geo"
    # merge right_leg
    print("DEBUG: merging right_leg...")
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = bpy.data.objects[0]
    bpy.ops.object.mode_set(mode="OBJECT")
    if len(right_leg) > 0:
        bpy.ops.object.select_all(action='DESELECT')
        for obj in right_leg:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = right_leg[0]
        bpy.ops.object.join()
        right_leg[0].name = "RightLeg_Geo"
    # merge left_leg
    print("DEBUG: merging left_leg...")
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = bpy.data.objects[0]
    bpy.ops.object.mode_set(mode="OBJECT")
    if len(left_leg) > 0:
        bpy.ops.object.select_all(action='DESELECT')
        for obj in left_leg:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = left_leg[0]
        bpy.ops.object.join()
        left_leg[0].name = "LeftLeg_Geo"

    # merge head
    print("DEBUG: merging head...")
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = bpy.data.objects[0]
    bpy.ops.object.mode_set(mode="OBJECT")
    if len(head_list) > 0:
        bpy.ops.object.select_all(action='DESELECT')
        for obj in head_list:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = head_list[0]
        bpy.ops.object.join()
        head_list[0].name = "Head_Geo"

    print("DEBUG: done separating by loose parts")


def separate_by_bone_influence():
    print("DEBUG: separate_by_bone_influence()")
    # separate by bone influence
    bpy.ops.object.mode_set(mode="OBJECT")
    bone_table = {
        "RightArm_Geo": ["RightHand", "RightLowerArm", "RightUpperArm"],
        "LeftArm_Geo": ["LeftHand", "LeftLowerArm", "LeftUpperArm"],
        "RightLeg_Geo": ["RightFoot", "RightLowerLeg", "RightUpperLeg"],
        "LeftLeg_Geo": ["LeftFoot", "LeftLowerLeg", "LeftUpperLeg"],
        "Body_Geo": ["UpperTorso", "LowerTorso"]
    }
    # deselect all
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and obj.name in bone_table:
            bone_list = bone_table[obj.name]
            bpy.context.view_layer.objects.active = obj
            for bone_name in bone_list:
                print("DEBUG: beginning vertex separation for bone_name=" + bone_name)
                bpy.ops.object.mode_set(mode="EDIT")
                # deselect all vertices
                bpy.ops.mesh.select_all(action='DESELECT')
                bpy.ops.object.mode_set(mode="OBJECT")
                # get list of all objects before separation operation
                before_list = set(bpy.context.scene.objects)
                # select vertices by bone group
                group = obj.vertex_groups.get(bone_name)
                for v in obj.data.vertices:
                    for g in v.groups:
                        if g.group == group.index:
                            v.select = True                
                bpy.ops.object.mode_set(mode="EDIT")
                # separate by selection
                bpy.ops.mesh.separate(type='SELECTED')
                # rename newly separated object
                # find the new object
                # get list of all objects after separation operation
                after_list = set(bpy.context.scene.objects)
                new_obj = None
                for obj_temp in after_list:
                    if obj_temp not in before_list:
                        new_obj = obj_temp
                        break
                # Switch to object mode
                bpy.ops.object.mode_set(mode='OBJECT')
                # The newly created object is the active object
                if new_obj == obj:
                    print("ERROR: new_obj.name=" + new_obj.name + " is the same as " + obj.name)
                else:
                    # Select the new object
                    print("DEBUG: new_obj.name=" + new_obj.name + " renaming to " + bone_name + "_Geo ...")
                    new_obj.name = bone_name + "_Geo"
                    # if "Hand" or "Foot" in bone_name, then set decimate modifier to 0.1
                    if "Hand" in bone_name or "Foot" in bone_name:
                        # get decimation modifier
                        decimate_modifier = None
                        for mod in new_obj.modifiers:
                            if mod.type == "DECIMATE":
                                print("DEBUG: setting decimate ratio to 0.1 for " + new_obj.name)
                                # change decimate ratio to 0.1
                                mod.ratio = 0.1
                                break
                    # deselect all objects
                    bpy.ops.object.select_all(action='DESELECT')
                    bpy.context.view_layer.objects.active = obj

    # clean up empty objects without vertices
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and len(obj.data.vertices) == 0:
            print("DEBUG: Removing empty object: " + obj.name)
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.ops.object.delete()


def load_and_merge_cage_meshes_from_template_file(template_filepath_blend):
    # load and merge cage meshes from template file
    bpy.ops.wm.append(filename="CageMeshes", directory=template_filepath_blend + "/Object/")
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and "CageMesh" in obj.name:
            obj.select_set(True)
    bpy.ops.object.join()

def load_and_merge_attachments_from_template_file(template_filepath_blend):
    # load and merge attachments from template file
    bpy.ops.wm.append(filename="Attachments", directory=template_filepath_blend + "/Object/")
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and "Attachment" in obj.name:
            obj.select_set(True)
    bpy.ops.object.join()

def transfer_weights(source_mesh_name, target_mesh_name):
    # Ensure objects exist
    if source_mesh_name not in bpy.data.objects or target_mesh_name not in bpy.data.objects:
        raise ValueError(f"One or both of the specified meshes do not exist in the scene: {source_mesh_name}, {target_mesh_name}")

    # Get source and target objects
    source_obj = bpy.data.objects[source_mesh_name]
    target_obj = bpy.data.objects[target_mesh_name]

    # Switch to Object Mode
    bpy.ops.object.mode_set(mode='OBJECT')

    # Deselect all objects
    bpy.ops.object.select_all(action='DESELECT')

    # Select and activate the target mesh
    target_obj.select_set(True)
    bpy.context.view_layer.objects.active = target_obj

    # Delete existing vertex groups on the target mesh
    if target_obj.vertex_groups:
        bpy.ops.object.vertex_group_remove(all=True)

    # Create corresponding vertex groups in the target mesh
    for src_vg in source_obj.vertex_groups:
        target_obj.vertex_groups.new(name=src_vg.name)

    # Ensure both objects are selected and source is active
    source_obj.select_set(True)
    target_obj.select_set(True)
    bpy.context.view_layer.objects.active = source_obj

    # Use data_transfer operator to transfer vertex group weights
    bpy.ops.object.data_transfer(
        data_type='VGROUP_WEIGHTS',
        use_create=True,
        vert_mapping='POLYINTERP_NEAREST',
        layers_select_src='ALL',
        layers_select_dst='NAME',
        mix_mode='REPLACE',
        mix_factor=1.0
    )

    # Switch back to Object Mode
    bpy.ops.object.mode_set(mode='OBJECT')

    # get armature from source_obj and parent target_obj to same armature
    armature_name = None
    for mod in source_obj.modifiers:
        if mod.type == "ARMATURE":
            armature_name = mod.object.name
            break
    if armature_name is not None:
        # deselect all
        bpy.ops.object.select_all(action='DESELECT')
        # select target_obj
        target_obj.select_set(True)
        bpy.data.objects[armature_name].select_set(True)
        bpy.context.view_layer.objects.active = bpy.data.objects[armature_name]
        bpy.ops.object.parent_set(type='ARMATURE')
        for mod in target_obj.modifiers:
            if mod.type == "ARMATURE":
                mod.name = armature_name
        

    print(f"Weights transferred successfully from {source_mesh_name} to {target_mesh_name}!")


# Execute main()
if __name__=='__main__':
    print("Starting script...")
    _add_to_log("Starting script... DEBUG: sys.argv=" + str(sys.argv))
    _main(sys.argv[4:])
    print("script completed.")
    exit(0)
