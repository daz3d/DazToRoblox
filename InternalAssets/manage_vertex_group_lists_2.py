"""
game_readiness_roblox_data.py

This module contains data for use with the game_readiness_tools.py module.

Requirements:
    - Python 3.7+
    - Blender 3.6+

"""

import bpy

# get script dir
import os
try:
    script_dir = os.path.dirname(os.path.realpath(__file__))
    if (".blend" in script_dir.lower()):
        script_dir = ""
    print("script_dir found: " + script_dir)
except:
    script_dir = ""
    print("script_dir not found.")

if script_dir == "" or os.path.exists(script_dir) == False:
    script_dir = "c:/GitHub/DazToRoblox-daz3d/InternalAssets"

vertex_group_names = ["Lips_Loop",
                   "Eyelids_Loop",
                #    "Skullcap_Patch",
                #    "Face_Patch",
                #    "NonFace_Patch",
                   "Neck_Loop",
                   "LowerTorso_Loop",
                #    "UpperArms_Loop",
                   "RightUpperArm_Loop",
                   "LeftUpperArm_Loop",
                #    "LowerArms_Loop",
                   "RightLowerArm_Loop",
                   "LeftLowerArm_Loop",
                #    "Wrists_Loop",
                   "RightWrist_Loop",
                   "LeftWrist_Loop",
                #    "UpperLegs_Loop",
                   "RightUpperLeg_Loop",
                   "LeftUpperLeg_Loop",
                #    "LowerLegs_Loop",
                   "RightLowerLeg_Loop",
                   "LeftLowerLeg_Loop",
                #    "Ankles_Loop",
                   "RightAnkle_Loop",
                   "LeftAnkle_Loop",                   
                   ]

geo_group_names = ["UpperTorso_GeoGroup",
                   "LowerTorso_GeoGroup",
                   "RightUpperArm_GeoGroup",
                    "LeftUpperArm_GeoGroup",
                    "RightLowerArm_GeoGroup",
                    "LeftLowerArm_GeoGroup",
                    "RightHand_GeoGroup",
                    "LeftHand_GeoGroup",
                    "RightUpperLeg_GeoGroup",
                    "LeftUpperLeg_GeoGroup",
                    "RightLowerLeg_GeoGroup",
                    "LeftLowerLeg_GeoGroup",
                    "RightFoot_GeoGroup",
                    "LeftFoot_GeoGroup",
                    "Head_GeoGroup",
                   ]

decimation_group_names = [
                    "Skullcap_DecimationGroup",
                    "NonFace_DecimationGroup",
                    "Face_DecimationGroup",
                    "UpperTorso_DecimationGroup",
                    "LowerTorso_DecimationGroup",
                    "RightUpperArm_DecimationGroup",
                    "LeftUpperArm_DecimationGroup",
                    "RightLowerArm_DecimationGroup",
                    "LeftLowerArm_DecimationGroup",
                    "RightHand_DecimationGroup",
                    "LeftHand_DecimationGroup",
                    "RightUpperLeg_DecimationGroup",
                    "LeftUpperLeg_DecimationGroup",
                    "RightLowerLeg_DecimationGroup",
                    "LeftLowerLeg_DecimationGroup",
                    "RightFoot_DecimationGroup",
                    "LeftFoot_DecimationGroup",
                   ]


#############################################################################


def create_vertex_group(obj, group_name, vertex_indices):
    print("DEBUG: obj=" + obj.name + " creating vertex group: " + group_name + ", index count: " + str(len(vertex_indices)))
    # Create a new vertex group
    new_group = obj.vertex_groups.new(name=group_name)
    # Assign the vertex indices to the group
    new_group.add(vertex_indices, 1.0, 'REPLACE')

# set up vertex index python data file from a prepared model
def generate_vertex_index_python_data(obj, group_names):
    # print("DEBUG: generate_vertex_index_python_data():...")
    filename = f"{script_dir}/python_vertex_indices.py"
    for group_name in group_names:
        vertex_indices = get_vertexgroup_indices(group_name, obj)
        # print(vertex_indices)
        print("DEBUG: writing to file: " + filename)
        with open(filename, "a") as file:
            if vertex_indices is None:
                file.write(f"# Vertex group '{group_name}' not found.\n\n")
            else:
                index_string_buffer = []
                for index in vertex_indices:
                    index_string_buffer += f"{index},"
                # file.write(f"{group_name} = [{index_string_buffer[:-1]}]\n\n")
                file.write(f"{group_name} = {vertex_indices}\n\n")

# set up vertex index files from a prepared model
def generate_vertex_index_files(obj, group_names):
    # print("DEBUG: generate_vertex_index_files():...")
    for group_name in group_names:
        vertex_indices = get_vertexgroup_indices(group_name, obj)
        # print(vertex_indices)
        filename = f"{script_dir}/vertex_indices/{group_name}_vertex_indices.txt"
        print("DEBUG: writing to file: " + filename)
        with open(filename, "w") as file:
            if vertex_indices is None:
                file.write("Vertex group not found.")
            else:
                for index in vertex_indices:
                    file.write(f"{index}\n")

def get_vertexgroup_indices(group_name, obj=None):
    # object mode
    bpy.ops.object.mode_set(mode='OBJECT')

    # Ensure your object is selected and active in Blender
    if obj is None:
        obj = bpy.context.active_object
    else:
        bpy.context.view_layer.objects.active = obj

    # List to hold the indices of vertices in the group
    vertex_indices = []

    # Ensure the object is a mesh
    if obj.type == 'MESH':       
        # Find the vertex group by name
        vertex_group = obj.vertex_groups.get(group_name)
        if vertex_group is not None:
            # Iterate through each vertex in the mesh
            for vertex in obj.data.vertices:
                for group in vertex.groups:
                    # Check if the vertex is in the vertex group
                    if group.group == vertex_group.index:
                        # Add the vertex index to the list
                        vertex_indices.append(vertex.index)
                        break  # Stop checking this vertex's groups
            
        else:
            print(f"Vertex group '{group_name}' not found.")
            return None
    else:
        print("Active object is not a mesh.")
        return None
    
    return vertex_indices


def main():

    fig = bpy.context.scene.objects["Genesis9.Shape"]
    # write vertex groups to disk
    # generate_vertex_index_files(fig, vertex_group_names + geo_group_names + decimation_group_names)
    # generate_vertex_index_python_data(fig, vertex_group_names + geo_group_names + decimation_group_names)

    return

if __name__ == "__main__":
    main()
    print("script complete.")
