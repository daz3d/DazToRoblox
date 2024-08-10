import bpy
import bmesh

def get_triangle_count(obj):
    # Create a derived mesh to evaluate the modifier without applying it
    depsgraph = bpy.context.evaluated_depsgraph_get()
    obj_eval = obj.evaluated_get(depsgraph)
    mesh_eval = obj_eval.to_mesh()

    # Count the number of triangles in the evaluated mesh
    triangles_count = sum(len(poly.vertices) - 2 for poly in mesh_eval.polygons)

    # Clean up the evaluated mesh data to free up memory
    obj_eval.to_mesh_clear()
    
    return triangles_count

def adjust_decimation_to_target(obj, target_triangles, tolerance=0.01):
    # Ensure the object has a Decimate modifier
    decimate_mod = next((mod for mod in obj.modifiers if mod.type == 'DECIMATE'), None)
    if not decimate_mod:
        decimate_mod = obj.modifiers.new(name="Decimate", type='DECIMATE')
    
    decimate_mod.decimate_type = 'COLLAPSE'
    
    # Initial bounds for binary search
    low_ratio, high_ratio = 0.0, 1.0
    
    max_iterations = 50
    for iterations in range(max_iterations):
        # Set the current ratio
        current_ratio = (low_ratio + high_ratio) / 2
        decimate_mod.ratio = current_ratio

        # Update the mesh
        bpy.context.view_layer.update()
        
        # Get the current triangle count
        current_triangles = get_triangle_count(obj)

        print(f"DEBUG: Iteration {iterations}, Ratio: {current_ratio:.8f}, Triangles: {current_triangles}, Target: {target_triangles}")

        # Check if we're within tolerance
        if abs(current_triangles - target_triangles) <= tolerance * target_triangles:
            break
        
        # Adjust the bounds
        if current_triangles > target_triangles:
            high_ratio = current_ratio
        else:
            low_ratio = current_ratio

    print(f"Final ratio: {current_ratio:.4f}, Triangles: {current_triangles}")

def main():
    # Example usage
    obj = bpy.context.active_object
    target_triangles = 4000  # Set your desired triangle count here
    tolerance = 0.005
    triangles_within_tolerance = target_triangles * (1-tolerance)

    adjust_decimation_to_target(obj, triangles_within_tolerance, tolerance)

if __name__ == "__main__":
    main()