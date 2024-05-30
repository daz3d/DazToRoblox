import bpy

template_filename = 'C:/GitHub/DazToRoblox-daz3d/InternalAssets/Genesis9Action.blend'

# Load action from template file
# Load action from template file
with bpy.data.libraries.load(template_filename, link=False) as (data_from, data_to):
    data_to.actions = [name for name in data_from.actions if name == 'Genesis9Action']

# Asign loaded action to existing armature
armature_object = bpy.data.objects['Genesis9']
action = bpy.data.actions['Genesis9Action']
armature_object.animation_data_create()  # Create animation data if not already present
armature_object.animation_data.action = action

# Deselect all objects
bpy.ops.object.select_all(action='DESELECT')
# make armature active object
armature_object.select_set(True)
bpy.context.view_layer.objects.active = armature_object

# switch to "Edit Mode"
bpy.ops.object.mode_set(mode='EDIT')

# Get edit bone "Head"
head_bone = armature_object.data.edit_bones.get('Head')

# Duplicate head bone
dynamichead_bone = armature_object.data.edit_bones.new(name="DynamicHead")
dynamichead_bone.head = head_bone.head
dynamichead_bone.tail = head_bone.tail
dynamichead_bone.roll = head_bone.roll
dynamichead_bone.use_connect = False

dynamichead_bone.parent = head_bone

# move all children from head to dynamichead
for child in head_bone.children:
    child.parent = dynamichead_bone

# Switch back to Object Mode
bpy.ops.object.mode_set(mode='OBJECT')

# set up custom properties for Head_Geo
head_geo_obj = bpy.data.objects['Head_Geo']
head_geo_obj["RootFaceJoint"] = "DynamicHead"
head_geo_obj["Frame0"] = "Neutral"
head_geo_obj["Frame1"] = "EyesLookDown"
head_geo_obj["Frame2"] = "EyesLookLeft"
head_geo_obj["Frame3"] = "EyesLookRight"
head_geo_obj["Frame4"] = "EyesLookUp"
head_geo_obj["Frame5"] = "JawDrop"
head_geo_obj["Frame6"] = "LeftEyeClosed"
head_geo_obj["Frame7"] = "LeftLipCornerPuller"
head_geo_obj["Frame8"] = "LeftLipStretcher"
head_geo_obj["Frame9"] = "LeftLowerLipDepressor"
head_geo_obj["Frame10"] = "LeftUpperLipRaiser"
head_geo_obj["Frame11"] = "LipsTogether"
head_geo_obj["Frame12"] = "Pucker"
head_geo_obj["Frame14"] = "RightEyeClosed"
head_geo_obj["Frame15"] = "RightLipCornerPuller"
head_geo_obj["Frame16"] = "RightLipStretcher"
head_geo_obj["Frame17"] = "RightLowerLipDepressor"
head_geo_obj["Frame18"] = "RightUpperLipRaiser"