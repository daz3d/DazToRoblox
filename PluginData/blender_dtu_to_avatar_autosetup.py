"""Blender Convert DTU to Roblox Blend (S1)

This is a command-line script to import a dtu/fbx intermediate file pair into
Blender and convert it to a format compatible with Roblox Avatar Autosetup, aka
S1.

- Developed and tested with Blender 3.6.1 (Python 3.10.12)
- Uses modified blender_tools.py module
- Requires Blender 3.6 or later

USAGE: blender.exe --background --python blender_dtu_to_roblox_S1.py <fbx file>

EXAMPLE:

    C:/Blender3.6/blender.exe --background --python blender_dtu_to_roblox_S1.py C:/Users/dbui/Documents/DazToRoblox/Amelia9YoungAdult/Amelia9YoungAdult.fbx

"""

logFilename = "blender_dtu_to_roblox_S1.log"

## Do not modify below
def _print_usage():
    # print("Python version: " + str(sys.version))
    print("\nUSAGE: blender.exe --background --python blender_dtu_to_roblox_S1.py <fbx file>\n")

from pathlib import Path
script_dir = str(Path( __file__ ).parent.absolute())

import sys
import os
import json
import re
import shutil
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
                    "NonFace_DecimationGroup": 0.4619,
                    "Face_DecimationGroup": 0.6,
                    "UpperTorso_DecimationGroup": 0.1382,
                    "LowerTorso_DecimationGroup": 0.14,
                    "RightUpperArm_DecimationGroup": 0.145,
                    "LeftUpperArm_DecimationGroup": 0.145,
                    "RightLowerArm_DecimationGroup": 0.145,
                    "LeftLowerArm_DecimationGroup": 0.145,
                    "RightHand_DecimationGroup": 0.09,
                    "LeftHand_DecimationGroup": 0.09,
                    "RightUpperLeg_DecimationGroup": 0.14,
                    "LeftUpperLeg_DecimationGroup": 0.14,
                    "RightLowerLeg_DecimationGroup": 0.11,
                    "LeftLowerLeg_DecimationGroup": 0.11,
                    "RightFoot_DecimationGroup": 0.1,
                    "LeftFoot_DecimationGroup": 0.1,
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

    # load FBX
    _add_to_log("DEBUG: main(): loading fbx file: " + str(fbxPath))
    blender_tools.import_fbx(fbxPath)
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
        # apply_i_pose()
        pass


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

    # remove moisture materials, extra meshes, extra materials
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            game_readiness_tools.remove_moisture_materials(obj)
    game_readiness_tools.remove_extra_meshes(["genesis9.shape", "genesis9mouth.shape", "genesis9eyes.shape"])
    game_readiness_tools.remove_extra_materials(["body"])

    # read from python data file (import vertex_indices_for_daztoroblox.py)
    for group_name in geo_group_names + decimation_group_names:
        print("DEBUG: creating vertex group: " + group_name)
        # evaluate group_name to python variable name
        vertex_index_buffer = globals()[group_name]
        game_readiness_tools.create_vertex_group(main_obj, group_name, vertex_index_buffer)

    # # add decimation modifier
    # bpy.context.view_layer.objects.active = main_obj
    # for decimation_group_name in decimation_group_names:
    #     print("DEBUG: main(): adding decimation modifier for group: " + decimation_group_name + " to object: " + main_obj.name)
    #     new_modifier = main_obj.modifiers.new(name=decimation_group_name, type='DECIMATE')
    #     new_modifier.name = decimation_group_name
    #     new_modifier.decimate_type = 'COLLAPSE'
    #     try:
    #         new_modifier.ratio = decimation_lookup[decimation_group_name]
    #     except:
    #         new_modifier.ratio = 0.91
    #     new_modifier.use_collapse_triangulate = True
    #     new_modifier.use_symmetry = True
    #     new_modifier.vertex_group = decimation_group_name

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

    # apply all decimation modifiers
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            print("DEBUG: main(): applying decimation modifiers to object: " + obj.name)
            for modifier in obj.modifiers:
                if modifier.type == 'DECIMATE':
                    bpy.context.view_layer.objects.active = obj
                    bpy.ops.object.modifier_apply(modifier=modifier.name)

    # rejoin all remaining meshes
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            obj.select_set(True)
    # head_geo to active
    bpy.context.view_layer.objects.active = bpy.data.objects.get("Head_Geo")
    bpy.ops.object.join()

    # merge all vertices by distance
    bpy.ops.object.select_all(action='DESELECT')
    obj = bpy.data.objects.get("Head_Geo")
    if obj is not None:
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.mesh.remove_doubles(threshold=0.00005)
    bpy.ops.object.mode_set(mode="OBJECT")
    if obj is not None:
        obj.name = "Genesis9_Geo"

    # unparent mesh from armature
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            obj.select_set(True)
    bpy.context.view_layer.objects.active = bpy.data.objects[0]
    bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')

    # remove armature modifiers
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            for modifier in obj.modifiers:
                if modifier.type == 'ARMATURE':
                    obj.modifiers.remove(modifier)

    # remove armature
    for obj in bpy.data.objects:
        if obj.type == 'ARMATURE':
            bpy.data.objects.remove(obj)

    # prepare destination folder path
    blenderFilePath = fbxPath.replace(".fbx", ".blend")
    intermediate_folder_path = os.path.dirname(fbxPath)

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

    # select all
    bpy.ops.object.select_all(action="SELECT")
    # scale to 0.0333
    bpy.ops.transform.resize(value=(0.0333, 0.0333, 0.0333))
    # apply using "All Transforms to Deltas"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)


    # export to fbx
    roblox_asset_name = dtu_dict["Asset Name"]
    roblox_output_path = dtu_dict["Output Folder"]
    destinationPath = roblox_output_path.replace("\\","/")
    if (not os.path.exists(destinationPath)):
        os.makedirs(destinationPath)
    fbx_base_name = os.path.basename(fbxPath)
    fbx_output_name = fbx_base_name.replace(".fbx", "_S1_ready_for_avatar_autosetup.fbx")
    fbx_output_file_path = os.path.join(destinationPath, fbx_output_name).replace("\\","/")
    _add_to_log("DEBUG: saving Roblox FBX file to destination: " + fbx_output_file_path)
    try:
        bpy.ops.export_scene.fbx(filepath=fbx_output_file_path, 
                                #  global_scale = 0.0333,
                                 add_leaf_bones = False,
                                 path_mode = "COPY",
                                 embed_textures = True,
                                 use_visible = True,
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
            inverse_bone_head_pos_x = -0.01 * bone_head_pos_z
            print("DEBUG: move_root_node_to_origin(): inverse_bone_head_pos_x=" + str(inverse_bone_head_pos_x) + ", inverse_bone_head_pos_z=" + str(inverse_bone_head_pos_z))
            bpy.ops.transform.translate(value=(inverse_bone_head_pos_x, 0, inverse_bone_head_pos_z))
            # apply transformation
            bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)


# Execute main()
if __name__=='__main__':
    print("Starting script...")
    _add_to_log("Starting script... DEBUG: sys.argv=" + str(sys.argv))
    _main(sys.argv[4:])
    print("script completed.")
    exit(0)
