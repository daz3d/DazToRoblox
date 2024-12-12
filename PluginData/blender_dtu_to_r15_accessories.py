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


logFilename = "blender_dtu_to_roblox_accessories.log"

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
        _add_to_log(f"DEBUG: token_id={token_id}")
    except:
        _add_to_log(f"ERROR: unable to parse token_id from '{line}'")
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
    for obj in bpy.data.objects:
        # if cage, transfer weights and parent to armature
        if obj.type == 'MESH' and (
            "_OuterCage" in obj.name or
            "_InnerCage" in obj.name
        ):
            game_readiness_tools.transfer_weights("Genesis9.Shape", obj.name)

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
    if "Hidden Surface Removal" in dtu_dict:
        hidden_surface_removal = dtu_dict["Hidden Surface Removal"]
    else:
        hidden_surface_removal = False
    if "Remove Scalp Material" in dtu_dict:
        remove_scalp_material = dtu_dict["Remove Scalp Material"]
    else:
        remove_scalp_material = False

    if "Has Animation" in dtu_dict:
        bHasAnimation = dtu_dict["Has Animation"]
        # FUTURE TODO: import and process facial animation
    else:
        bHasAnimation = False

    # clear all animation data
    # Iterate over all objects
    _add_to_log("DEBUG: main(): clearing animation data")
    for obj in bpy.data.objects:
        # Check if the object has animation data
        if obj.animation_data:
            # Clear all animation data
            obj.animation_data_clear()        

    # move root node to origin
    _add_to_log("DEBUG: main(): moving root node to origin")
    move_root_node_to_origin()

    daz_generation = dtu_dict["Asset Id"]

    main_obj = None
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            if obj.name == "Genesis9.Shape":
                _add_to_log("DEBUG: main(): obj.name=" + obj.name)
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
    if "layered" in roblox_asset_type or "ALL" in roblox_asset_type:
        # roblox_tools.duplicate_cage("Template_InnerCage")
        cage_template = bpy.data.objects.get("Template_InnerCage")
        if cage_template is None:
            _add_to_log("DEBUG: main(): making inner cage template")
            cage_template = roblox_tools.make_inner_cage("Genesis9.Shape")
            if cage_template is not None:
                cage_template.name = "Template_InnerCage"
        else:
            # shrinkwrap cage to main_obj
            # game_readiness_tools.autofit_mesh(cage_template, main_obj, 0.90, 10, 50, 3)
            # game_readiness_tools.autofit_mesh(cage_template, main_obj, 0.2, 10, 200, 0, False)
            game_readiness_tools.autofit_mesh(cage_template, main_obj, 0.9, 10, 25, 0, False)
            # game_readiness_tools.scale_by_face_normals(cage_template, 0.90)

    figure_list = ["genesis9.shape", "genesis9mouth.shape", "genesis9eyes.shape"]
    for obj in bpy.data.objects:
        # if obj.type == 'MESH' and (
        #     "eyebrow" in obj.name.lower() or
        #     "eyelash" in obj.name.lower() or
        #     "tear" in obj.name.lower()
        # ) :
        if obj.type == 'MESH' and "StudioPresentationType" in obj:
            asset_type = obj["StudioPresentationType"]
            # if "eyebrow" in asset_type.lower() or "eyelash" in asset_type.lower() or "tear" in asset_type.lower():
            if "tear" in asset_type.lower():
                figure_list.append(obj.name.lower())
    cage_list = []
    cage_obj_list = []
    accessories_list = []
    delete_list = []
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            if obj.name.lower() in figure_list:
                delete_list.append(obj)
            elif "_OuterCage" in obj.name or "_InnerCage" in obj.name:
                _add_to_log("DEBUG: cage obj.name=" + obj.name)
                cage_list.append(obj.name.lower())
                cage_obj_list.append(obj)
                obj.hide_render = True
            elif "_Att" in obj.name:
                delete_list.append(obj)
            else:
                accessories_list.append(obj)

    # move cages to new collection
    for obj in cage_obj_list:
        # Unlink the object from its current collection(s)
        for col in obj.users_collection:
            col.objects.unlink(obj)
        # Link the object to the new collection
        cage_collection.objects.link(obj)

    DEBUG_INNERCAGE_ON = 0
    if DEBUG_INNERCAGE_ON:
        # create debug filter collection
        debug_filter_collection = bpy.data.collections.new(name="Debug Filter")
        bpy.context.scene.collection.children.link(debug_filter_collection)
        # hide junk
        layer_collection = find_layer_collection(debug_filter_collection)
        if layer_collection is not None:
            layer_collection.exclude = True
        else:
            debug_filter_collection.hide_viewport = True
        for obj in bpy.data.objects:
            if obj.type == 'MESH':
                # if obj.name == "Genesis9.Shape" or obj.name == "Template_InnerCage" or obj.name == "Template_InnerCage_Copy":
                if obj.name == "Genesis9.Shape" or obj.name == "Template_InnerCage":
                    pass
                else:
                    # add to junk
                    for col in obj.users_collection:
                        col.objects.unlink(obj)
                    debug_filter_collection.objects.link(obj)
        bpy.ops.wm.save_as_mainfile(filepath=blenderFilePath.replace(".blend", "_debug.blend"))
        exit()

    # delete objects
    for obj in delete_list:
        _add_to_log("DEBUG: deleting obj.name=" + obj.name)
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.ops.object.delete()

    if len(accessories_list) == 0:
        _add_to_log("DEBUG: main(): no accessories found.")
        exit()

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

    if remove_scalp_material:
        hair_obj_list = []
        for obj in bpy.data.objects:
            if obj.type == 'MESH' and "StudioPresentationType" in obj:
                if "hair" in obj["StudioPresentationType"].lower():
                    hair_obj_list.append(obj)
        for obj in hair_obj_list:
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_all(action='DESELECT')
            bpy.ops.object.mode_set(mode='OBJECT')
            for face in obj.data.polygons:
                material_name = obj.material_slots[face.material_index].material.name
                # fuzzy match scalp and skullcap
                # exact match for "Cap"
                if ("scalp" in material_name.lower() or 
                    "skullcap" in material_name.lower() or 
                    "Cap" == material_name
                    ):
                    _add_to_log("DEBUG: SCALP REMOVER: material_name=" + material_name)
                    for vert_index in face.vertices:
                        obj.data.vertices[vert_index].select = True
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.delete(type='VERT')
            bpy.ops.object.mode_set(mode='OBJECT')

    # convert to texture atlas
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
            _add_to_log("DEBUG: running texture atlas for obj: " + obj.name + ", num materials: " + str(num_materials) + ", images: " + str(image_list))
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
        _add_to_log("DEBUG: safe_material_names_list=" + str(safe_material_names_list))
        game_readiness_tools.remove_extra_materials(safe_material_names_list + ["cage_material", "attachment_material"])

    if "layered" in roblox_asset_type or "ALL" in roblox_asset_type:
        # for each obj, make list of vertex group names
        for obj in bpy.data.objects:
            if obj.type == 'MESH'and (
                "_outercage" not in obj.name.lower() and
                "_innercage" not in obj.name.lower() and
                "_att" not in obj.name.lower()
                ):

                DEBUG_BOOTS_ON = 0
                if DEBUG_BOOTS_ON:
                    if "boots" not in obj.name.lower():
                        # delete object
                        bpy.ops.object.select_all(action='DESELECT')
                        obj.select_set(True)
                        bpy.ops.object.delete()
                        continue

                inner_cage = roblox_tools.duplicate_cage("Template_InnerCage")
                if inner_cage is not None:
                    inner_cage.name = obj.name + "_InnerCage"
                    # game_readiness_tools.autofit_mesh(inner_cage, obj, 0.1)
                else:
                    raise Exception("ERROR: main(): unable to make inner cage.")

                outer_cage = roblox_tools.duplicate_cage("Template_InnerCage")
                if outer_cage is not None:
                    outer_cage.name = obj.name + "_OuterCage"
                    # game_readiness_tools.autofit_mesh(outer_cage, obj, 1.5, 10)
                    game_readiness_tools.autofit_mesh(outer_cage, obj, 1.05, 2, 25, 3, False)
                else:
                    raise Exception("ERROR: main(): unable to make outer cage.")

                if hidden_surface_removal:
                    # remove hidden faces
                    thresholds = [t * 28 for t in [0.005, 0.010, 0.015]]
                    # game_readiness_tools.remove_obscured_faces(obj, 0.0001, [1000000])
                    game_readiness_tools.remove_obscured_faces(obj)

                # decimate
                tolerance = 0.005
                target_triangles = 4000 * (1-tolerance)
                game_readiness_tools.adjust_decimation_to_target(obj, target_triangles, tolerance)


    # remove missing or unused images
    _add_to_log("DEBUG: deleting missing or unused images...")
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
    _add_to_log("DEBUG: main(): cleaning up unused data blocks...")
    bpy.ops.outliner.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)

    # pack all images
    _add_to_log("DEBUG: main(): packing all images...")
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
 
     # Apply all modifiers
    for obj in bpy.data.objects:
        if obj.type == 'MESH'and (
            "_outercage" not in obj.name.lower() and
            "_innercage" not in obj.name.lower() and
            "_att" not in obj.name.lower()
            ):
            try:
                blender_tools.apply_mesh_modifiers(obj)
            except Exception as e:
                _add_to_log("ERROR: unable to apply mesh modifiers: " + str(e))

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


def move_root_node_to_origin():
    _add_to_log("DEBUG: move_root_node_to_origin(): bpy.data.objects=" + str(bpy.data.objects))
    # move root node to origin
    for obj in bpy.data.objects:
        # _add_to_log("DEBUG: move_root_node_to_origin(): obj.name=" + obj.name + ", obj.type=" + obj.type)
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
            # _add_to_log("DEBUG: move_root_node_to_origin(): bone_head_pos_y=" + str(bone_head_pos_y) + ", bone_head_pos_z=" + str(bone_head_pos_z))
            bpy.ops.object.mode_set(mode="OBJECT")
            # select all objects in object mode
            bpy.ops.object.select_all(action='SELECT')
            # move all objects by the inverse of bone_head_pos

            inverse_bone_head_pos_z = -0.01 * bone_head_pos_y
            # inverse_bone_head_pos_z = 0

            # inverse_bone_head_pos_x = -0.01 * bone_head_pos_z
            inverse_bone_head_pos_x = 0
            
            # _add_to_log("DEBUG: move_root_node_to_origin(): inverse_bone_head_pos_x=" + str(inverse_bone_head_pos_x) + ", inverse_bone_head_pos_z=" + str(inverse_bone_head_pos_z))
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
    _add_to_log("Starting script (accessories)...\nDEBUG: sys.argv=" + str(sys.argv))
    _main(sys.argv[4:])
    _add_to_log("Script completed.\n")
    exit(0)
