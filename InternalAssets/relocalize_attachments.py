import bpy
from mathutils import Matrix, Quaternion, Vector

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
    new_rotation = parent_bone_rotation
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

def main():
    # iterate through all objects
    for obj in bpy.data.objects:
        # if object is a mesh and ends with _Att
        if obj.type == 'MESH' and obj.name.endswith('_Att'):
            relocalize_attachment(obj)
    

main()
print("done.")