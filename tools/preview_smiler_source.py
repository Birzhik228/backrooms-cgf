import bpy,os,math
from mathutils import Vector
base=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
qa=os.path.join(base,'tests/artifacts/v12/model')
os.makedirs(qa,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=os.path.join(base,'assets/models/smiler/source/Zombie Scream.fbx'),use_anim=True)
scene=bpy.context.scene
arm=next(o for o in scene.objects if o.type=='ARMATURE')
mesh=next(o for o in scene.objects if o.type=='MESH')
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO'
scene.display.shading.studiolight_rotate_z=.5
scene.display.shading.color_type='MATERIAL'
scene.display.shading.show_shadows=True
scene.display.shading.show_cavity=True
scene.display.shading.background_type='WORLD'
scene.world=bpy.data.worlds.new('Preview world')
scene.world.color=(.14,.16,.19)
scene.render.resolution_x=600
scene.render.resolution_y=680
scene.render.resolution_percentage=100
bpy.ops.object.camera_add()
camera=bpy.context.object
camera.data.type='ORTHO'
scene.camera=camera
for label,frame in [('1',1),('20',20),('40',40),('65',65),('85',85),('rest',None)]:
    if frame is None:arm.data.pose_position='REST'
    else:scene.frame_set(frame)
    bpy.context.view_layer.update()
    deps=bpy.context.evaluated_depsgraph_get()
    evaluated=mesh.evaluated_get(deps)
    points=[evaluated.matrix_world@v.co for v in evaluated.data.vertices]
    lo=Vector(tuple(min(p[k] for p in points) for k in range(3)))
    hi=Vector(tuple(max(p[k] for p in points) for k in range(3)))
    center=(lo+hi)*.5
    camera.location=center+Vector((2,-30,1))
    camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.ortho_scale=max(hi.z-lo.z,(hi.x-lo.x)*1.2)*1.1
    scene.render.filepath=os.path.join(qa,'source-'+label+'.png')
    bpy.ops.render.render(write_still=True)
    print(label,tuple(lo),tuple(hi))
