import bpy
import bmesh
from mathutils import Vector
import math

def remove_obscured_faces(obj):
    # Object Mode
    bpy.ops.object.mode_set(mode='OBJECT')

    # Create a bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    
    # Get the scene for ray_cast
    scene = bpy.context.scene
    faces_to_remove = []

    # Define a very small offset, about 1/1000th of the figure's depth
    figure_depth = 0.01
    offset = figure_depth * 0.0001

    # for threshold in [offset*500, offset*1000, offset*2000, offset*3000, offset*4000]:
    for threshold in [0.005, 0.010, 0.015]:
        bm.faces.ensure_lookup_table()
        print(f"DEBUG: threshold = {threshold:.4f}")

        for face in bm.faces:
            obscured_verts = []
            if face in faces_to_remove:
                continue
            for v in face.verts:
                obscured_face_normal = []
                for f in v.link_faces:
                    vert_normal = f.normal
                    ray_origin = v.co + vert_normal * offset
                    ray_direction = vert_normal                

                    result, location, normal, index, hit_obj, _ = scene.ray_cast(bpy.context.view_layer.depsgraph, ray_origin, ray_direction)
                    # If ray hit something and it's not the current face, mark for removal
                    if result and (hit_obj != obj or (hit_obj == obj and index != face.index)):
                        # if location is far away, it's not an occluder
                        # print(f"DEBUG: [{v}] length = {(location - ray_origin).length:.2f} vs threshold={threshold:.2f}")
                        if (location - ray_origin).length > threshold:
                            continue
                        obscured_face_normal.append(f)
                # If all linked face normal directions are obscured, mark vertex for removal
                # print(f"DEBUG: [{v}] obscured_face_normal = {len(obscured_face_normal)} vs link_normals={len(v.link_faces)}")
                if len(obscured_face_normal) == len(v.link_faces):
                    obscured_verts.append(v)
            # If all verts are obscured, mark face for removal
            # print(f"DEBUG: obscured_verts = {len(obscured_verts)} vs face.verts={len(face.verts)}")
            if len(obscured_verts) == len(face.verts):
                faces_to_remove.append(face)               
    
    # Remove marked faces
    bmesh.ops.delete(bm, geom=faces_to_remove, context='FACES')
    
    # Update mesh
    bm.to_mesh(obj.data)
    obj.data.update()
    
    # Free bmesh
    bm.free()
    
    print(f"Removed {len(faces_to_remove)} obscured faces")


def main():
    obj = bpy.context.active_object
    if obj.type != 'MESH':
        print("Selected object is not a mesh")
        return

    # Call the function
    remove_obscured_faces(obj)

if __name__ == "__main__":
    main()