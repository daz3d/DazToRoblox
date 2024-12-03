import bpy

def remove_scale_animation(obj):
    """Remove scale keyframes from the object's animation data."""
    if obj.animation_data and obj.animation_data.action:
        action = obj.animation_data.action
        fcurves_to_remove = []
        
        # Iterate through the action's FCurves
        for fcurve in action.fcurves:
            # Check if the FCurve is for a scale property
            if fcurve.data_path.endswith("scale"):
                fcurves_to_remove.append(fcurve)
            else:
                print("DEBUG: skipping: " + str(fcurve.data_path))
        
        # Remove the identified FCurves
        for fcurve in fcurves_to_remove:
            print("DEBUG: removing: " + str(fcurve.data_path))
            action.fcurves.remove(fcurve)

def main():
    """Main function to remove all scale animations in the scene."""
    for obj in bpy.data.objects:
        remove_scale_animation(obj)
        # Check if the object has shape key animations
        if obj.data and hasattr(obj.data, "shape_keys") and obj.data.shape_keys:
            remove_scale_animation(obj.data.shape_keys)

    # Also iterate through actions in case any are not linked to objects
    for action in bpy.data.actions:
        fcurves_to_remove = []
        for fcurve in action.fcurves:
            if fcurve.data_path.startswith("scale"):
                fcurves_to_remove.append(fcurve)
        for fcurve in fcurves_to_remove:
            action.fcurves.remove(fcurve)

if __name__ == "__main__":
    main()
    print("done")
