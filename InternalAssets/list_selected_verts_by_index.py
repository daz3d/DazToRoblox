import bpy
import bmesh


def get_vertex_indexes(obj):
    bm = bmesh.from_edit_mesh(obj.data)
    bm.verts.index_update()
    bm.verts.ensure_lookup_table()

    selected_vertexes = []    
    for i, v in enumerate(bm.verts):
        if v.select:
#            print(f"DEBUG: i={i}, v.index={v.index}")
            selected_vertexes.append(i)
    
    print(f"DEBUG: selected vertexes: {selected_vertexes}")
    return selected_vertexes


obj = bpy.context.active_object
get_vertex_indexes(obj)



