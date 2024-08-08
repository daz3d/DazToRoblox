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
        if obj.type == 'MESH' and (
            "_OuterCage" in obj.name or
            "_InnerCage" in obj.name
        ):
            cage_obj_list.append(obj)
            game_readiness_tools.transfer_weights("Genesis9.Shape", obj.name)
        # if attachment, delete
        if obj.type == 'MESH' and "_Att" in obj.name:
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.ops.object.delete()

    blender_tools.center_all_viewports()
    jsonPath = fbxPath.replace(".fbx", ".dtu")
    _add_to_log("DEBUG: main(): loading json file: " + str(jsonPath))
    dtu_dict = blender_tools.process_dtu(jsonPath)

    # global variables and settings from DTU
    roblox_asset_name = dtu_dict["Asset Name"]
    roblox_output_path = dtu_dict["Output Folder"]
    roblox_asset_type = dtu_dict["Asset Type"]
    if "Texture Bake Quality" in dtu_dict:
        texture_bake_quality = dtu_dict["Texture Bake Quality"]
    else:
        texture_bake_quality = 1
    if "Texture Size" in dtu_dict:
        roblox_texture_size = dtu_dict["Texture Size"]
    else:
        roblox_texture_size = 1024
    if "Bake Single Outfit" in dtu_dict:
        bake_single_outfit = dtu_dict["Bake Single Outfit"]
    else:
        bake_single_outfit = False

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

    top_collection = bpy.context.scene.collection
    # create new collection for cages
    cage_collection = bpy.data.collections.new(name="Unused Cages")
    # Link the new collection to the scene
    bpy.context.scene.collection.children.link(cage_collection)

    # make inner cage
    cage_template = bpy.data.objects.get("Template_InnerCage")
    if cage_template is None:
        print("DEBUG: main(): making inner cage template")
        cage_template = roblox_tools.make_inner_cage("Genesis9.Shape")
        if cage_template is not None:
            cage_template.name = "Template_InnerCage"
    else:
        # shrinkwrap cage to main_obj
        game_readiness_tools.autofit_mesh(cage_template, main_obj, 0.99)
    if cage_template is not None:
        for col in cage_template.users_collection:
            col.objects.unlink(cage_template)        
        cage_collection.objects.link(cage_template)

    figure_list = ["genesis9.shape", "genesis9mouth.shape", "genesis9eyes.shape"]
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and (
            "eyebrow" in obj.name.lower() or
            "eyelash" in obj.name.lower() or
            "tear" in obj.name.lower()
        ) :
            figure_list.append(obj.name.lower())
    cage_list = []
    att_list = []
    cage_obj_list = []
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            if obj.name.lower() in figure_list:
                print("DEBUG: deleting figure_list obj.name=" + obj.name)
                bpy.ops.object.select_all(action='DESELECT')
                obj.select_set(True)
                bpy.ops.object.delete()
            elif "_OuterCage" in obj.name:
                print("DEBUG: cage obj.name=" + obj.name)
                cage_list.append(obj.name.lower())
                cage_obj_list.append(obj)
                obj.hide_render = True
            elif "_Att" in obj.name:
                print("DEBUG: attachment obj.name=" + obj.name)
                att_list.append(obj.name.lower())
                obj.hide_render = True

    # move cages to new collection
    for obj in cage_obj_list:
        # Unlink the object from its current collection(s)
        for col in obj.users_collection:
            col.objects.unlink(obj)
        # Link the object to the new collection
        cage_collection.objects.link(obj)

    if bake_single_outfit:
        # join all mesh objects together
        main_item = None
        join_list = []
        for obj in bpy.data.objects:
            if obj.type == 'MESH'and (
                "_outercage" not in obj.name.lower() and
                "_innercage" not in obj.name.lower() and
                "_att" not in obj.name.lower()
                ):
                # check if obj is rigged to armature
                if obj.vertex_groups:
                    main_item = obj
                    join_list.append(obj)
        if main_item is not None:
            bpy.ops.object.select_all(action='DESELECT')
            bpy.context.view_layer.objects.active = main_item
            for obj in join_list:
                obj.select_set(True)
            if len(join_list) > 1:
                bpy.ops.object.join()
            if "ALL" in roblox_asset_type:
                main_item.name = roblox_asset_name + "_Outfit"
            else:
                main_item.name = roblox_asset_name

    safe_material_names_list = []
    for obj in bpy.data.objects:
        if obj.type == 'MESH'and (
            "_outercage" not in obj.name.lower() and
            "_innercage" not in obj.name.lower() and
            "_att" not in obj.name.lower()
            ):
            # check if obj has multiple images in multiple materials, if so, then convert to atlas
            # 1. check for multiple materials
            num_materials = len(obj.material_slots)
            if num_materials <= 1:
                continue
            image_list = []
            for mat_slot in obj.material_slots:
                if mat_slot.material and mat_slot.material.use_nodes:
                    for node in mat_slot.material.node_tree.nodes:
                        if node.type == 'TEX_IMAGE' and node.image is not None:
                            # if node's 'Color' output is linked to 'Base Color' input of Principled BSDF node
                            if node.outputs["Color"].is_linked:
                                linked_node = node.outputs["Color"].links[0].to_node
                                if linked_node.type == 'BSDF_PRINCIPLED':
                                    bsdf_node = linked_node
                                    if bsdf_node.inputs["Base Color"].is_linked:
                                        base_color_node = bsdf_node.inputs["Base Color"].links[0].from_node
                                        if node == base_color_node:
                                            if node.image not in image_list:
                                                image_list.append(node.image)
                                                continue
            if len(image_list) <= 1:
                continue
            print("DEBUG: running texture atlas for obj: " + obj.name + ", num materials: " + str(num_materials) + ", images: " + str(image_list))
            atlas, atlas_material, _ = game_readiness_tools.convert_to_atlas(obj, intermediate_folder_path, roblox_texture_size, texture_bake_quality)
            safe_material_names_list.append(atlas_material.name.lower())

    # Remove multilpe materials
    for obj in bpy.data.objects:
        if obj.type != 'MESH'or (
            "_outercage" in obj.name.lower() or
            "_innercage" in obj.name.lower() or
            "_att" in obj.name.lower()
            ):
            continue
        obj_materials = obj.data.materials
        best_mat = None
        best_match_strength = 0
        for mat in obj_materials:
            mat_match_strength = 0
            nodes = mat.node_tree.nodes
            # find and interpret BSDF node data
            for node in nodes:
                if node.type == 'BSDF_PRINCIPLED':
                    if "Normal" in node.inputs and node.inputs["Normal"].is_linked:
                        mat_match_strength += 1
                    if "Metallic" in node.inputs and node.inputs["Metallic"].is_linked:
                        mat_match_strength += 1
                    if "Roughness" in node.inputs and node.inputs["Roughness"].is_linked:
                        mat_match_strength += 1
                    if "Base Color" in node.inputs and node.inputs["Base Color"].is_linked:
                        mat_match_strength += 1
                    break
            # find the best match
            if best_mat is None or mat_match_strength > best_match_strength:
                best_mat = mat
                best_match_strength = mat_match_strength
            # keep going until done or perfect match found
            if best_match_strength == 4:
                break
        if best_mat is not None:
            mat_name = obj.name + "_material"
            best_mat.name = mat_name
            safe_material_names_list.append(best_mat.name.lower())
    if len(safe_material_names_list) > 0:
        print("DEBUG: safe_material_names_list=" + str(safe_material_names_list))
        game_readiness_tools.remove_extra_materials(safe_material_names_list + ["cage_material", "attachment_material"])

    if "layered" in roblox_asset_type or "ALL" in roblox_asset_type:
        # for each obj, make list of vertex group names
        for obj in bpy.data.objects:
            if obj.type == 'MESH'and (
                "_outercage" not in obj.name.lower() and
                "_innercage" not in obj.name.lower() and
                "_att" not in obj.name.lower()
                ):

                inner_cage = roblox_tools.duplicate_cage("Template_InnerCage")
                if inner_cage is not None:
                    inner_cage.name = obj.name + "_InnerCage"
                    game_readiness_tools.autofit_mesh(inner_cage, obj, 0.99)
                else:
                    raise Exception("ERROR: main(): unable to make inner cage.")

                outer_cage = roblox_tools.duplicate_cage("Template_InnerCage")
                # create dummy target
                if outer_cage is not None:
                    game_readiness_tools.autofit_mesh(outer_cage, obj, 1.01)
                    outer_cage.name = obj.name + "_OuterCage"
                    # target_list = []
                    # target_list.append(inner_cage)
                    # target_list.append(obj)
                    # dummy_target = roblox_tools.make_dummy_target(target_list)
                    # if dummy_target is not None:
                    #     obj_bounding_box = roblox_tools.get_bounding_box_from_obj(obj)
                    #     shrinkable_vertex_group = roblox_tools.make_vertex_group_from_bounding_box(outer_cage, obj_bounding_box, "shrinkable_zone")
                    #     # add shrinkwrap modifier to outer_cage_obj
                    #     bpy.ops.object.select_all(action='DESELECT')
                    #     outer_cage.select_set(True)
                    #     bpy.context.view_layer.objects.active = outer_cage
                    #     bpy.ops.object.modifier_add(type='SHRINKWRAP')
                    #     bpy.context.object.modifiers["Shrinkwrap"].target = dummy_target
                    #     bpy.context.object.modifiers["Shrinkwrap"].vertex_group = shrinkable_vertex_group.name
                    #     bpy.context.object.modifiers["Shrinkwrap"].wrap_method = 'NEAREST_SURFACEPOINT'
                    #     bpy.context.object.modifiers["Shrinkwrap"].wrap_mode = 'OUTSIDE'
                    #     ## Blender Bug workaround: 0.0005 is actually 0.014 before changing scene scale to 1/28
                    #     bpy.context.object.modifiers["Shrinkwrap"].offset = 0.06 * 28
                    #     # apply
                    #     bpy.ops.object.modifier_apply(modifier="Shrinkwrap")
                    #     outer_cage.name = obj.name + "_OuterCage"
                    #     # delete dummy_target
                    #     bpy.ops.object.select_all(action='DESELECT')
                    #     dummy_target.select_set(True)
                    #     bpy.ops.object.delete()
                    # else:
                    #     raise Exception("ERROR: main(): unable to make dummy target for outer cage.")
                else:
                    raise Exception("ERROR: main(): unable to make outer cage.")


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

    # roblox UGC Validation Fixes
    roblox_tools.ugc_validation_fixes()

    if cage_collection is not None:
        layer_collection = find_layer_collection(cage_collection)
        if layer_collection is not None:
            layer_collection.exclude = True
        else:
            cage_collection.hide_viewport = True

    # prepare destination folder paths
    destinationPath = roblox_output_path.replace("\\","/")
    if (not os.path.exists(destinationPath)):
        os.makedirs(destinationPath)
    fbx_base_name = os.path.basename(fbxPath)
    if bake_single_outfit:
        fbx_output_name = fbx_base_name.replace(".fbx", "_outfit.fbx")
    elif "layered" in roblox_asset_type or "ALL" in roblox_asset_type:
        fbx_output_name = fbx_base_name.replace(".fbx", "_layered_accessories.fbx")
    else:
        fbx_output_name = fbx_base_name.replace(".fbx", "_rigid_accessories.fbx")
    fbx_output_file_path = os.path.join(destinationPath, fbx_output_name).replace("\\","/")
    _add_to_log("DEBUG: saving Roblox FBX file to destination: " + fbx_output_file_path)
    # export to fbx
    try:
        bpy.ops.export_scene.fbx(filepath=fbx_output_file_path, 
                                 add_leaf_bones = False,
                                 path_mode = "COPY",
                                 embed_textures = True,
                                 use_visible = True,
                                 use_custom_props = True,
                                 bake_anim=False,
                                 bake_anim_use_nla_strips = False,
                                 bake_anim_use_all_actions = False
                                 )
        _add_to_log("DEBUG: save completed.")
    except Exception as e:
        _add_to_log("ERROR: unable to save Roblox FBX file: " + fbx_output_file_path)
        _add_to_log("EXCEPTION: " + str(e))

    # save blender file to destination
    blender_output_file_path = fbx_output_file_path.replace(".fbx", ".blend")
    bpy.ops.wm.save_as_mainfile(filepath=blender_output_file_path)

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
        bpy.context.object.pose.bones["LeftUpperArm"].rotation_euler[2] = -0.6
        bpy.context.object.pose.bones["RightUpperArm"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["RightUpperArm"].rotation_euler[2] = 0.6
        # elbows
        bpy.context.object.pose.bones["LeftLowerArm"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["LeftLowerArm"].rotation_euler[0] = 0.115
        bpy.context.object.pose.bones["LeftLowerArm"].rotation_euler[1] = 0.079
        bpy.context.object.pose.bones["RightLowerArm"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["RightLowerArm"].rotation_euler[0] = 0.115
        bpy.context.object.pose.bones["RightLowerArm"].rotation_euler[1] = -0.079
        # wrists
        bpy.context.object.pose.bones["LeftHand"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["LeftHand"].rotation_euler[0] = -0.122
        bpy.context.object.pose.bones["LeftHand"].rotation_euler[1] = -0.084
        bpy.context.object.pose.bones["RightHand"].rotation_mode= "XYZ"
        bpy.context.object.pose.bones["RightHand"].rotation_euler[0] = -0.122
        bpy.context.object.pose.bones["RightHand"].rotation_euler[1] = 0.084
        # L and R hips to 5 degrees
        bpy.context.object.pose.bones["LeftUpperLeg"].rotation_mode= "XYZ"
        # bpy.context.object.pose.bones["LeftUpperLeg"].rotation_euler[0] = -0.17
        bpy.context.object.pose.bones["LeftUpperLeg"].rotation_euler[2] = -0.026
        bpy.context.object.pose.bones["RightUpperLeg"].rotation_mode= "XYZ"
        # bpy.context.object.pose.bones["RightUpperLeg"].rotation_euler[0] = -0.17
        bpy.context.object.pose.bones["RightUpperLeg"].rotation_euler[2] = 0.026


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

def find_layer_collection(collection, layer_collection = None):
    """Recursively search for the layer collection that corresponds to the given collection."""
    if layer_collection is None:
        layer_collection = bpy.context.view_layer.layer_collection
    if layer_collection.collection == collection:
        return layer_collection
    for child in layer_collection.children:
        result = find_layer_collection(collection, child)
        if result:
            return result
    return None

# Execute main()
if __name__=='__main__':
    print("Starting script...")
    _add_to_log("Starting script... DEBUG: sys.argv=" + str(sys.argv))
    _main(sys.argv[4:])
    print("script completed.")
    exit(0)
