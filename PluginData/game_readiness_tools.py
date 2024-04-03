""" Game Readiness Tools for Blender
game_readiness_tools.py

This module contains functions for preparing 3D models for use in games and other real-time applications.  Intended for use with Daz Bridges and related tools.

Requirements:
    - Python 3.7+
    - Blender 3.6+

"""

from pathlib import Path
script_dir = str(Path( __file__ ).parent.absolute())

import os
import bpy


# do a mesh separation based on a vertex group, and keep the normals of the original mesh, but by "keep_normals", I mean to copy normal vectors to two arrays for the new object and the remaining object based on the vertex group or inverse of the vertex group, then for each vertex in the new mesh and remaining mesh, assign the normal vector from the corresponding array
# def separate_by_vertexgroup_keep_normals(obj, vertex_group_name):
def separate_by_vertexgroup(obj, vertex_group_name):
    # make list of all original objects
    original_objects = bpy.data.objects[:]

    # 1. check if obj is a mesh
    if obj.type == 'MESH':
        # 2. then check if the vertex group exists
        vertex_group = obj.vertex_groups.get(vertex_group_name)
        if vertex_group:
            # 3. then prepare two arrays for normal vectors, one for the new object, and one for the remaining object
            normals_new = []
            normals_remaining = []
            vertex_group_index = vertex_group.index
            # 4. copy normal vectors to the arrays based on the vertex group
            for vertex in obj.data.vertices:
                if any(group.group == vertex_group_index for group in vertex.groups):
                    normals_new.append(vertex.normal)
                else:
                    normals_remaining.append(vertex.normal)

            # 5. then perform the separation based on the vertex group by selecting the vertices in the vertex group and then separate by selection
            print("DEBUG: Separating mesh based on vertex group: ", vertex_group_name)
            bpy.ops.object.mode_set(mode='OBJECT')
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_all(action='DESELECT')
            bpy.ops.object.vertex_group_set_active(group=vertex_group_name)
            bpy.ops.object.vertex_group_select()
            bpy.ops.mesh.separate(type='SELECTED')
            bpy.ops.object.mode_set(mode='OBJECT')

            # look for the new object by checking which object is not in the original objects
            new_obj = None
            for new_obj in bpy.data.objects:
                if new_obj not in original_objects:
                    break
            if new_obj is None:
                print("ERROR: New object not found after separation")
                return
            new_obj.name = f"{vertex_group_name.replace("_GeoGroup", "_Geo")}"

            # 6. then assign the normal vectors from the arrays to the new mesh and the remaining mesh
            # print("DEBUG: Applying custom normals for vertex group: ", vertex_group_name)
            # apply_custom_normals(new_obj, normals_new)
            # print("DEBUG: Applying custom normals for remaining vertices")
            # apply_custom_normals(obj, normals_remaining)

        else:
            print(f"ERROR: Vertex group '{vertex_group_name}' not found.")
            return
    else:
        print("ERROR: Active object is not a mesh.")
        return
    print(f"Separation based on vertex group '{vertex_group_name}' completed.")

def apply_custom_normals(obj, custom_normals):
    # Ensure the object is in object mode
    bpy.ops.object.mode_set(mode='OBJECT')
    obj.data.use_auto_smooth = True
    
    # Ensure that custom_normals is structured correctly:
    # It should be a list where each item is a tuple or list of three floats.
    structured_normals = [(n.x, n.y, n.z) for n in custom_normals]

    # sanity checks:
    # 1. check if number of elements in custom_normals matches number of normals in the mesh
    if len(structured_normals) != len(obj.data.vertices):
        print("ERROR: Number of custom normals does not match number of vertices in mesh")
        return
    # 2. check if the number of elements in each normal is 3
    if any(len(n) != 3 for n in structured_normals):
        print("ERROR: Each normal must be a tuple or list of three floats")
        return
    else:
        print("INFO: Number of custom normals and vertices match, len=3 for each normal")
    # 3. check if all elements in each normal are floats
    if any(not all(isinstance(v, float) for v in n) for n in structured_normals):
        print("ERROR: Each normal must be a tuple or list of three floats")
        return
    else:
        print("INFO: All elements in each normal are floats")
    # 4. check if custom normals are nearly equal to the original normals
    epsilon = 1e-6
    zero_vectors = 0
    for i, vertex in enumerate(obj.data.vertices):
        if all(abs(a - b) < epsilon for a, b in zip(vertex.normal, structured_normals[i])):
            print(f"INFO: Custom normal for vertex {i} is nearly equal to original normal")
            # set custom normal to zero vector
            structured_normals[i] = (0.0, 0.0, 0.0)
            zero_vectors += 1
    if zero_vectors > 0:
        print(f"INFO: {zero_vectors} custom normals set to zero vector")
    else:
        print("INFO: No custom normals set to zero vector")

    # Set custom normals
    obj.data.normals_split_custom_set_from_vertices(structured_normals)
    print("DEBUG: Custom normals applied to mesh: ", obj.name)



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


# create vertex groups from vertex index files
def create_vertex_groups_from_files(obj, group_names):
    # print("DEBUG: create_vertex_groups_from_files():...")
    for group_name in group_names:
        filename = f"{script_dir}/vertex_indices/{group_name}_vertex_indices.txt"
        print("DEBUG: filename: " + filename)
        if not os.path.exists(filename):
            print(f"File '{filename}' not found.")
            continue
        with open(filename, "r") as file:
            try:
                vertex_indices = [int(line) for line in file]
            except ValueError:
                print(f"Error reading file '{filename}'.")
                continue
            # Create a new vertex group
            new_group = obj.vertex_groups.new(name=group_name)
            # Assign the vertex indices to the group
            new_group.add(vertex_indices, 1.0, 'REPLACE')


def create_vertex_group(obj, group_name, vertex_indices):
    # Create a new vertex group
    new_group = obj.vertex_groups.new(name=group_name)
    # Assign the vertex indices to the group
    new_group.add(vertex_indices, 1.0, 'REPLACE')

