import bpy
import bmesh


def select_faces_by_index(obj, indexes):
    # Ensure we're in Edit Mode
    if obj.mode != 'EDIT':
        bpy.ops.object.mode_set(mode='EDIT')
    
    # Get the BMesh
    bm = bmesh.from_edit_mesh(obj.data)
    bm.faces.index_update()
    bm.faces.ensure_lookup_table()
    
    # Deselect all vertices first
    for f in bm.faces:
        f.select = False
    
    # Select vertices by index
    for index in indexes:
        if index < len(bm.faces):
            bm.faces[index].select = True
    
    # Update the mesh
    bmesh.update_edit_mesh(obj.data)
    
    # Force the viewport to update
    bpy.context.view_layer.update()


obj = bpy.context.active_object
face_indexes = []

select_faces_by_index(obj, face_indexes)
