import bpy
import bmesh

def select_verts_by_index(obj, indices):
    # Ensure we're in Edit Mode
    if obj.mode != 'EDIT':
        bpy.ops.object.mode_set(mode='EDIT')
    
    # Get the BMesh
    bm = bmesh.from_edit_mesh(obj.data)
    bm.verts.ensure_lookup_table()
    
    # Deselect all vertices first
    for v in bm.verts:
        v.select = False
    
    # Select vertices by index
    for index in indices:
        if index < len(bm.verts):
            bm.verts[index].select = True
    
    # Update the mesh
    bmesh.update_edit_mesh(obj.data)
    
    # Force the viewport to update
    bpy.context.view_layer.update()

obj = bpy.context.active_object
vertex_indexes = [0]

select_verts_by_index(obj, vertex_indexes)
