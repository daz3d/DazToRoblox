import bpy

# Set the name of your source mesh
source_mesh_name = "Genesis9.Shape"

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
        vert_mapping='POLY_NEAREST',
        layers_select_src='ALL',
        layers_select_dst='NAME',
        mix_mode='REPLACE',
        mix_factor=1.0
    )

    # Switch back to Object Mode
    bpy.ops.object.mode_set(mode='OBJECT')

    print(f"Weights transferred successfully from {source_mesh_name} to {target_mesh_name}!")

# Loop through all objects in the scene
for obj in bpy.data.objects:
    print(f"Processing object: {obj.name}")
    if obj.type == 'MESH' and "_OuterCage" in obj.name:
        transfer_weights(source_mesh_name, obj.name)
