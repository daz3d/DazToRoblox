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

g_intermediate_folder_path = ""

def _add_to_log(sMessage):
    print(str(sMessage))
    with open(logFilename, "a") as file:
        file.write(sMessage + "\n")

def setup_world_lighting(color, strength=5.0):
    world = bpy.context.scene.world
    world.use_nodes = True
    nodes = world.node_tree.nodes
    nodes.clear()
    background = nodes.new(type='ShaderNodeBackground')
    background.inputs['Color'].default_value = color
    background.inputs['Strength'].default_value = strength
    world_output = nodes.new(type='ShaderNodeOutputWorld')
    links = world.node_tree.links
    links.new(background.outputs['Background'], world_output.inputs['Surface'])
    bpy.context.scene.world = world   
    return background

def add_basic_lighting(strength=5.0):
    bpy.ops.object.light_add(type='SUN', location=(0, 0, 10))
    sun = bpy.context.active_object
    sun.data.energy = strength  # Adjust as needed
    sun.data.use_shadow = False
    # sun.data.angle = 0.523599
    return sun

def setup_bake_nodes(material, atlas):
    nodes = material.node_tree.nodes
    for node in nodes:
        node.select = False
    # Create Image Texture node for baking target
    bake_node = nodes.new(type='ShaderNodeTexImage')
    bake_node.image = atlas
    bake_node.select = True
    nodes.active = bake_node   
    return bake_node

def cleanup_bake_nodes(obj, bake_nodes, unlink_emission=False):
    for mat_slot in obj.material_slots:
        if mat_slot.material and mat_slot.material.use_nodes:
            material = mat_slot.material
            for bake_node in bake_nodes:
                if bake_node in material.node_tree.nodes.values():
                    material.node_tree.nodes.remove(bake_node)
            if unlink_emission:
                nodes = material.node_tree.nodes
                if bpy.app.version >= (4, 0, 0):
                    material.node_tree.links.remove(nodes['Principled BSDF'].inputs['Emission Color'].links[0])
                else:
                    material.node_tree.links.remove(nodes['Principled BSDF'].inputs['Emission'].links[0])
    return

def find_roughness_node(material):
    nodes = material.node_tree.nodes
    for node in nodes:
        if node.type == 'BSDF_PRINCIPLED':
            if "Roughness" in node.inputs and node.inputs["Roughness"].is_linked:
                return node.inputs["Roughness"].links[0].from_node
    # create placeholder roughness node
    nodes = material.node_tree.nodes
    roughness_node = nodes.new(type='ShaderNodeRGB')
    if nodes.get('Principled BSDF') is not None:
        roughness_value = nodes['Principled BSDF'].inputs['Roughness'].default_value
        roughness_node.outputs['Color'].default_value = (roughness_value, roughness_value, roughness_value, 1.0)
    else:
        roughness_node.outputs['Color'].default_value = (0.5, 0.5, 0.5, 1.0)
    return roughness_node

def find_metallic_node(material):
    nodes = material.node_tree.nodes
    for node in nodes:
        if node.type == 'BSDF_PRINCIPLED':
            if "Metallic" in node.inputs and node.inputs["Metallic"].is_linked:
                return node.inputs["Metallic"].links[0].from_node
    # create placeholder metallic node
    nodes = material.node_tree.nodes
    metallic_node = nodes.new(type='ShaderNodeRGB')
    if nodes.get('Principled BSDF') is not None:
        metallic_value = nodes['Principled BSDF'].inputs['Metallic'].default_value
        metallic_node.outputs['Color'].default_value = (metallic_value, metallic_value, metallic_value, 1.0)
    else:
        metallic_node.outputs['Color'].default_value = (0.0, 0.0, 0.0, 1.0)
    return metallic_node

def find_diffuse_node(material):
    nodes = material.node_tree.nodes
    for node in nodes:
        if node.type == 'BSDF_PRINCIPLED':
            if "Base Color" in node.inputs and node.inputs["Base Color"].is_linked:
                return node.inputs["Base Color"].links[0].from_node
    # create placeholder diffuse node
    nodes = material.node_tree.nodes
    diffuse_node = nodes.new(type='ShaderNodeRGB')
    if nodes.get('Principled BSDF') is not None:
        diffuse_node.outputs['Color'].default_value = nodes['Principled BSDF'].inputs['Base Color'].default_value
    else:
        diffuse_node.outputs['Color'].default_value = (0.0, 0.0, 0.0, 1.0)
    return diffuse_node

def bake_roughness_to_atlas(obj, atlas):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.context.scene.render.engine = 'CYCLES'
    bpy.context.scene.cycles.samples = 4
    bpy.context.scene.cycles.time_limit = 2
    bpy.context.scene.cycles.bake_type = 'ROUGHNESS'
    bpy.context.scene.render.bake.margin = 16

    # Setup bake nodes for each material
    bake_nodes = []
    for mat_slot in obj.material_slots:
        if mat_slot.material and mat_slot.material.use_nodes:
            print(f"Setting up bake node for material: {mat_slot.material.name}")
            bake_node = setup_bake_nodes(mat_slot.material, atlas)
            bake_nodes.append(bake_node)
        else:
            print(f"Warning: Material slot has no material or doesn't use nodes: {mat_slot.name}")    
    if not bake_nodes:
        print("Error: No bake nodes were created. Check if the object has materials with nodes.")
        return

    # Bake
    print("Starting bake operation...")
    bpy.ops.object.bake(type='ROUGHNESS')
    print("Bake operation completed.")

    # Clean up bake nodes
    cleanup_bake_nodes(obj, bake_nodes)

    return


def bake_normal_to_atlas(obj, atlas):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.context.scene.render.engine = 'CYCLES'
    bpy.context.scene.cycles.samples = 4
    bpy.context.scene.cycles.time_limit = 2
    bpy.context.scene.cycles.bake_type = 'NORMAL'
    bpy.context.scene.render.bake.margin = 16

    # Setup bake nodes for each material
    bake_nodes = []
    for mat_slot in obj.material_slots:
        if mat_slot.material and mat_slot.material.use_nodes:
            print(f"Setting up bake node for material: {mat_slot.material.name}")
            bake_node = setup_bake_nodes(mat_slot.material, atlas)
            bake_nodes.append(bake_node)
        else:
            print(f"Warning: Material slot has no material or doesn't use nodes: {mat_slot.name}")    
    if not bake_nodes:
        print("Error: No bake nodes were created. Check if the object has materials with nodes.")
        return

    # Bake
    print("Starting bake operation...")
    bpy.ops.object.bake(type='NORMAL')
    print("Bake operation completed.")

    # Clean up bake nodes
    cleanup_bake_nodes(obj, bake_nodes)

    return

def bake_metallic_to_atlas(obj, atlas):
    # check metallic is linked to principled bsdf
    metallic_found = False
    for mat_slot in obj.material_slots:
        if mat_slot.material and mat_slot.material.use_nodes:
            for node in mat_slot.material.node_tree.nodes:
                if node.type == 'BSDF_PRINCIPLED':
                    if "Metallic" in node.inputs and node.inputs["Metallic"].is_linked:
                        metallic_found = True
                        break
            break    
    if not metallic_found:
        print("Warning: No metallic map found. Skipping metallic bake.")
        return
    
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.context.scene.render.engine = 'CYCLES'
    bpy.context.scene.cycles.samples = 4
    bpy.context.scene.cycles.time_limit = 2
    bpy.context.scene.cycles.bake_type = 'EMIT'
    bpy.context.scene.render.bake.use_pass_direct = False
    bpy.context.scene.render.bake.use_pass_indirect = False
    bpy.context.scene.render.bake.use_pass_color = False
    bpy.context.scene.render.bake.use_pass_glossy = False
    bpy.context.scene.render.bake.use_pass_diffuse = False
    bpy.context.scene.render.bake.use_pass_emit = True
    bpy.context.scene.render.bake.margin = 16

    # Setup bake nodes for each material
    bake_nodes = []
    for mat_slot in obj.material_slots:
        if mat_slot.material and mat_slot.material.use_nodes:
            print(f"Setting up bake node for material: {mat_slot.material.name}")
            bake_node = setup_bake_nodes(mat_slot.material, atlas)
            bake_nodes.append(bake_node)
            material = mat_slot.material
            nodes = material.node_tree.nodes
            metallic_node = find_metallic_node(material)
            # link image texture color to emission color of Principled BSDF node    
            if bpy.app.version >= (4, 0, 0):
                material.node_tree.links.new(metallic_node.outputs['Color'], nodes['Principled BSDF'].inputs['Emission Color'])
            else:
                material.node_tree.links.new(metallic_node.outputs['Color'], nodes['Principled BSDF'].inputs['Emission'])   
        else:
            print(f"Warning: Material slot has no material or doesn't use nodes: {mat_slot.name}")    
    if not bake_nodes:
        print("Error: No bake nodes were created. Check if the object has materials with nodes.")
        return

    # Bake
    print("Starting bake operation...")
    bpy.ops.object.bake(type='EMIT')
    print("Bake operation completed.")

    # Clean up bake nodes
    cleanup_bake_nodes(obj, bake_nodes, True)

    return

def bake_diffuse_to_atlas(obj, atlas):
    print(f"Starting bake process for object: {obj.name}")
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    
    bpy.context.scene.render.engine = 'CYCLES'
    bpy.context.scene.cycles.samples = 4
    bpy.context.scene.cycles.time_limit = 2
    bpy.context.scene.cycles.bake_type = 'COMBINED'
    bpy.context.scene.render.bake.use_pass_direct = False
    bpy.context.scene.render.bake.use_pass_indirect = False
    bpy.context.scene.render.bake.use_pass_color = True
    bpy.context.scene.render.bake.use_pass_glossy = False
    bpy.context.scene.render.bake.use_pass_diffuse = True
    bpy.context.scene.render.bake.use_pass_emit = True
    bpy.context.scene.render.bake.margin = 16

    # Setup bake nodes for each material
    bake_nodes = []
    for mat_slot in obj.material_slots:
        if mat_slot.material and mat_slot.material.use_nodes:
            print(f"Setting up bake node for material: {mat_slot.material.name}")
            bake_node = setup_bake_nodes(mat_slot.material, atlas)
            bake_nodes.append(bake_node)
            material = mat_slot.material
            nodes = material.node_tree.nodes
            diffuse_node = find_diffuse_node(material)
            # link image texture color to emission color of Principled BSDF node    
            if bpy.app.version >= (4, 0, 0):
                material.node_tree.links.new(diffuse_node.outputs['Color'], nodes['Principled BSDF'].inputs['Emission Color'])
            else:
                material.node_tree.links.new(diffuse_node.outputs['Color'], nodes['Principled BSDF'].inputs['Emission'])   
        else:
            print(f"Warning: Material slot has no material or doesn't use nodes: {mat_slot.name}")    
    if not bake_nodes:
        print("Error: No bake nodes were created. Check if the object has materials with nodes.")
        return

    # Bake
    print("Starting bake operation...")
    bpy.ops.object.bake(type='COMBINED')
    print("Bake operation completed.")
    
    # Clean up bake nodes
    cleanup_bake_nodes(obj, bake_nodes, True)

    return


def create_texture_atlas(obj, atlas_size=4096):
    atlas = bpy.data.images.new(name=f"{obj.name}_Atlas", width=atlas_size, height=atlas_size, alpha=True)
    atlas_material = bpy.data.materials.new(name=f"{obj.name}_Atlas_Material")
    atlas_material.use_nodes = True
    nodes = atlas_material.node_tree.nodes
    tex_node = nodes.new(type='ShaderNodeTexImage')
    tex_node.image = atlas
    atlas_material.node_tree.links.new(tex_node.outputs['Color'], nodes["Principled BSDF"].inputs['Base Color'])
    # connect alpha channel to alpha output
    atlas_material.node_tree.links.new(tex_node.outputs['Alpha'], nodes["Principled BSDF"].inputs['Alpha'])
    return atlas, atlas_material

def create_new_uv_layer(obj, name="AtlasUV"):
    mesh = obj.data
    new_uv = mesh.uv_layers.new(name=name)
    mesh.uv_layers.active = new_uv
    return new_uv

def repack_uv(obj):
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    # bpy.ops.uv.pack_islands(margin=0.001)
    bpy.ops.uv.pack_islands(udim_source='ACTIVE_UDIM', rotate=True, rotate_method='ANY', scale=True, 
                            merge_overlap=False, margin_method='SCALED', margin=0.001,
                            pin=False, pin_method='LOCKED', shape_method='CONCAVE')
    bpy.ops.object.mode_set(mode='OBJECT')

def unwrap_object(obj):
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.unwrap(method='ANGLE_BASED', fill_holes=True, correct_aspect=True, margin=0.001),
    bpy.ops.object.mode_set(mode='OBJECT')

def assign_atlas_to_object(obj, atlas_material):
    original_materials = [slot.material for slot in obj.material_slots]
    obj.data.materials.clear()
    obj.data.materials.append(atlas_material)
    return original_materials

def convert_to_atlas(obj):
    global g_intermediate_folder_path

    print(f"Starting atlas conversion for object: {obj.name}")
    
    # Add basic lighting if the scene has none
    background = None
    # if not any(obj.type == 'LIGHT' for obj in bpy.context.scene.objects):
    #     print("No lights found in the scene. Adding a basic sun light and hdri.")
    #     background = setup_world_lighting((1.0, 1.0, 1.0, 1.0), 1.0)
    
    diffuse_atlas, atlas_material = create_texture_atlas(obj)
    new_uv_name = "AtlasUV"
    new_uv = create_new_uv_layer(obj, new_uv_name)

    # unwrap_object(obj)
    repack_uv(obj)

    bake_diffuse_to_atlas(obj, diffuse_atlas)

    diffuse_atlas_path = g_intermediate_folder_path + "/" + f"{obj.name}_Atlas_D.png"
    print("DEBUG: saving: " + diffuse_atlas_path)
    # save atlas image to disk
    diffuse_atlas.filepath_raw = diffuse_atlas_path
    diffuse_atlas.file_format = 'PNG'
    diffuse_atlas.save()

    normal_atlas = bpy.data.images.new(name=f"{obj.name}_Atlas_N", width=4096, height=4096)
    bake_normal_to_atlas(obj, normal_atlas)

    normal_atlas_path = g_intermediate_folder_path + "/" + f"{obj.name}_Atlas_N.jpg"
    print("DEBUG: saving: " + normal_atlas_path)
    normal_atlas.filepath_raw = normal_atlas_path
    normal_atlas.file_format = 'JPEG'
    normal_atlas.save()

    metallic_atlas = bpy.data.images.new(name=f"{obj.name}_Atlas_M", width=4096, height=4096)
    bake_metallic_to_atlas(obj, metallic_atlas)

    metallic_atlas_path = g_intermediate_folder_path + "/" + f"{obj.name}_Atlas_M.jpg"
    print("DEBUG: saving: " + metallic_atlas_path)
    metallic_atlas.filepath_raw = metallic_atlas_path
    metallic_atlas.file_format = 'JPEG'
    metallic_atlas.save()

    roughness_atlas = bpy.data.images.new(name=f"{obj.name}_Atlas_R", width=4096, height=4096)
    bake_roughness_to_atlas(obj, roughness_atlas)

    roughness_atlas_path = g_intermediate_folder_path + "/" + f"{obj.name}_Atlas_R.jpg"
    print("DEBUG: saving: " + roughness_atlas_path)
    roughness_atlas.filepath_raw = roughness_atlas_path
    roughness_atlas.file_format = 'JPEG'
    roughness_atlas.save()

    nodes = atlas_material.node_tree.nodes
    # link normal atlas to atlas material
    normal_tex_node = nodes.new(type='ShaderNodeTexImage')
    normal_tex_node.image = normal_atlas
    normal_tex_node.image.colorspace_settings.name = 'Non-Color'
    normal_node = nodes.new(type='ShaderNodeNormalMap')
    atlas_material.node_tree.links.new(normal_tex_node.outputs['Color'], normal_node.inputs['Color'])
    atlas_material.node_tree.links.new(normal_node.outputs['Normal'], nodes["Principled BSDF"].inputs['Normal'])
    # link roughness atlas to atlas material
    roughness_node = nodes.new(type='ShaderNodeTexImage')
    roughness_node.image = roughness_atlas
#    roughness_node.image.colorspace_settings.name = 'Non-Color'
    atlas_material.node_tree.links.new(roughness_node.outputs['Color'], nodes["Principled BSDF"].inputs['Roughness'])
    # link metallic atlas to atlas material
    metallic_node = nodes.new(type='ShaderNodeTexImage')
    metallic_node.image = metallic_atlas
#    metallic_node.image.colorspace_settings.name = 'Non-Color'
    atlas_material.node_tree.links.new(metallic_node.outputs['Color'], nodes["Principled BSDF"].inputs['Metallic'])

    original_materials = assign_atlas_to_object(obj, atlas_material)

    # print("DEBUG: new_uv.name=" + str(new_uv.name))
    try:
        if str(new_uv.name) != "":
            obj.data.uv_layers[str(new_uv.name)].active_render = True
        else:
            obj.data.uv_layers[new_uv_name].active_render = True
    except:
        print("ERROR: retrying to set uv layer: " + new_uv_name)
        try:
            obj.data.uv_layers[new_uv_name].active_render = True
        except:
            print("ERROR: Unable to set active_render for uv layer: " + new_uv.name)
    
    # remove other UVs
    uv_layer_names_to_remove = []
    for uv_layer in obj.data.uv_layers:
        current_uv_name = str(uv_layer.name)
        if current_uv_name != new_uv_name:
            uv_layer_names_to_remove.append(current_uv_name)
    
    for uv_layer_name in uv_layer_names_to_remove:
        uv_layer = obj.data.uv_layers.get(uv_layer_name)
        if uv_layer is not None:
            print("DEBUG: removing uv_layer: " + uv_layer_name)
            obj.data.uv_layers.remove(uv_layer)

    # # remove world lighting
    # if background:
    #     background.inputs['Strength'].default_value = 0.0
    # # bpy.context.scene.world.use_nodes = False

    print("Atlas conversion completed.")

    return diffuse_atlas, atlas_material, original_materials


def _main(argv):
    global g_intermediate_folder_path
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
    g_intermediate_folder_path = intermediate_folder_path

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

    top_collection = bpy.context.scene.collection

    # create new collection for cages
    cage_collection = bpy.data.collections.new(name="Unused Cages")
    # Link the new collection to the scene
    bpy.context.scene.collection.children.link(cage_collection)
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
            atlas, atlas_material, _ = convert_to_atlas(obj)
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
                vertex_group_names = []
                vertex_group_vertices = {}
                inner_cage_obj_list = []
                outer_cage_obj_list = []
                # num_vert_threshold = len(obj.data.vertices) * 0.05
                num_vert_threshold = 4
                weight_threshold = 0.75
                # get list of names of all vertex groups that are not empty
                for vertex in obj.data.vertices:
                    for group in vertex.groups:
                        if group.weight > weight_threshold:
                            vg_name = obj.vertex_groups[group.group].name
                            if vg_name not in vertex_group_vertices.keys():
                                vertex_group_vertices[vg_name] = 1
                            else:
                                vertex_group_vertices[vg_name] += 1
                            if (vertex_group_vertices[vg_name] > num_vert_threshold):
                                if vg_name not in vertex_group_names:
                                    vertex_group_names.append(vg_name)
                    if len(vertex_group_names) == len(obj.vertex_groups):
                        break

                for name in vertex_group_names:
                    # get cage name
                    cage_name = name + "_OuterCage"
                    # get obj by name (cage_name)
                    cage_obj = bpy.data.objects.get(cage_name)
                    if cage_obj is not None:
                        # make 2 duplicates of cage_obj
                        bpy.ops.object.select_all(action='DESELECT')
                        cage_obj.select_set(True)
                        bpy.context.view_layer.objects.active = cage_obj
                        bpy.ops.object.duplicate()
                        if cage_obj != bpy.context.object and cage_obj.name in bpy.context.object.name:
                            cage_obj_copy1 = bpy.context.object
                            top_collection.objects.link(cage_obj_copy1)
                            cage_collection.objects.unlink(cage_obj_copy1)
                            inner_cage_obj_list.append(cage_obj_copy1)
                        else:
                            # throw exception
                            raise Exception("ERROR: main(): object duplication failed, obj name=" + cage_obj.name)
                        bpy.ops.object.select_all(action='DESELECT')
                        cage_obj.select_set(True)
                        bpy.context.view_layer.objects.active = cage_obj
                        bpy.ops.object.duplicate()
                        if cage_obj != bpy.context.object and cage_obj.name in bpy.context.object.name:
                            cage_obj_copy2 = bpy.context.object
                            top_collection.objects.link(cage_obj_copy2)
                            cage_collection.objects.unlink(cage_obj_copy2)
                            outer_cage_obj_list.append(cage_obj_copy2)
                        else:
                            # throw exception
                            raise Exception("ERROR: main(): object duplication failed, obj name=" + cage_obj.name)

                # join all cages in inner_cage_obj_list
                if len(inner_cage_obj_list) > 1:
                    bpy.ops.object.select_all(action='DESELECT')
                    for cage_obj in inner_cage_obj_list:
                        cage_obj.select_set(True)
                    bpy.context.view_layer.objects.active = inner_cage_obj_list[0]
                    bpy.ops.object.join()
                if len(inner_cage_obj_list) > 0:
                    inner_cage_obj = inner_cage_obj_list[0]
                    inner_cage_obj.name = obj.name + "_InnerCage"
                    # # add shrinkwrap modifier to inner_cage_obj
                    # bpy.context.view_layer.objects.active = inner_cage_obj
                    # bpy.ops.object.modifier_add(type='SHRINKWRAP')
                    # bpy.context.object.modifiers["Shrinkwrap"].target = obj
                    # bpy.context.object.modifiers["Shrinkwrap"].wrap_method = 'NEAREST_SURFACEPOINT'
                    # bpy.context.object.modifiers["Shrinkwrap"].wrap_mode = 'INSIDE'
                # join all cages in outer_cage_obj_list
                if len(outer_cage_obj_list) > 1:
                    bpy.ops.object.select_all(action='DESELECT')
                    for cage_obj in outer_cage_obj_list:
                        cage_obj.select_set(True)
                    bpy.context.view_layer.objects.active = outer_cage_obj_list[0]
                    bpy.ops.object.join()
                if len(outer_cage_obj_list) > 0:
                    outer_cage_obj = outer_cage_obj_list[0]
                    outer_cage_obj.name = obj.name + "_OuterCage"
                    # add shrinkwrap modifier to outer_cage_obj
                    bpy.ops.object.select_all(action='DESELECT')
                    outer_cage_obj.select_set(True)
                    bpy.context.view_layer.objects.active = outer_cage_obj
                    bpy.ops.object.modifier_add(type='SHRINKWRAP')
                    bpy.context.object.modifiers["Shrinkwrap"].target = obj
                    bpy.context.object.modifiers["Shrinkwrap"].wrap_method = 'NEAREST_SURFACEPOINT'
                    bpy.context.object.modifiers["Shrinkwrap"].wrap_mode = 'OUTSIDE'
                    #bpy.context.object.modifiers["Shrinkwrap"].offset = 0.0005
                    ## Blender Bug workaround: 0.0005 is actually 0.014 before changing scene scale to 1/28
                    bpy.context.object.modifiers["Shrinkwrap"].offset = 0.014

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
