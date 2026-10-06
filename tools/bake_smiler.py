"""Offline asset conversion. Run with Blender --background --disable-autoexec.

Preserves the supplied FBX triangles, weights and two materials. Locomotion is
authored on its armature; the supplied scream action becomes the capture clip.
No FBX parsing, Blender, skinning library or shader changes are needed at runtime.
"""
import bpy, bmesh, os, math, struct, json, hashlib
from mathutils import Vector, Quaternion
from array import array

BASE=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT=os.path.join(BASE,'assets/models/smiler')
QA=os.path.join(BASE,'tests/artifacts/v12/model')
os.makedirs(QA,exist_ok=True)
SOURCE=os.path.join(OUT,'source/Zombie Scream.fbx')
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SOURCE,use_anim=True)
scene=bpy.context.scene
arm=next(o for o in scene.objects if o.type=='ARMATURE')
obj=next(o for o in scene.objects if o.type=='MESH')
# Lock polygon diagonals before posing so every clip uses identical indices.
bm=bmesh.new()
bm.from_mesh(obj.data)
bmesh.ops.triangulate(bm,faces=list(bm.faces))
bm.to_mesh(obj.data)
bm.free()
action=arm.animation_data.action
scene.frame_set(1)
bpy.context.view_layer.update()
base={p.name:(p.location.copy(),p.rotation_quaternion.copy(),p.scale.copy()) for p in arm.pose.bones}
axes={p.name:p.matrix.to_quaternion().inverted()@Vector((1,0,0)) for p in arm.pose.bones}
side_axes={p.name:p.matrix.to_quaternion().inverted()@Vector((0,0,1)) for p in arm.pose.bones}
arm.animation_data.action=None
def reset():
    for p in arm.pose.bones:
        p.rotation_mode='QUATERNION'
        p.location,p.rotation_quaternion,p.scale=[v.copy() for v in base[p.name]]
def rotate(name,angle,side=False):
    p=arm.pose.bones['mixamorig:'+name]
    p.rotation_quaternion=p.rotation_quaternion@Quaternion((side_axes if side else axes)[p.name],angle)
def pose(phase,kind,duck=False):
    reset()
    running=kind=='run'
    moving=kind!='idle'
    swing=math.sin(phase)
    c=1 if duck else 0
    if moving:
        amplitude=.65 if running else .38
        for side,sign in [('Left',1),('Right',-1)]:
            s=swing*sign
            rotate(side+'UpLeg',amplitude*s-.58*c)
            rotate(side+'Leg',max(0,-s)*(.95 if running else .45)+1.12*c)
            rotate(side+'Foot',-.18*s-.34*c)
            rotate(side+'Arm',-s*(.38 if running else .20))
            rotate(side+'ForeArm',max(0,s)*(.4 if running else .13))
        rotate('Spine',(.16 if running else .04)+.28*c)
        rotate('Spine1',math.sin(phase)*.018)
        rotate('Head',-.10 if running else -.035)
    else:
        for side in ['Left','Right']:
            rotate(side+'UpLeg',-.58*c)
            rotate(side+'Leg',1.12*c)
            rotate(side+'Foot',-.34*c)
        rotate('Spine',math.sin(phase)*.018+.28*c)
        rotate('Head',math.sin(phase+.6)*.018)
    if duck:
        rotate('Spine1',.27)
        rotate('Neck',-.25)
        for side in ['Left','Right']:
            rotate(side+'Arm',-.80)
            rotate(side+'ForeArm',-.60)
    bpy.context.view_layer.update()

SCALE=.306
def sample():
    deps=bpy.context.evaluated_depsgraph_get()
    evaluated=obj.evaluated_get(deps)
    mesh=evaluated.to_mesh()
    mesh.calc_loop_triangles()
    matrix=evaluated.matrix_world
    normal_matrix=matrix.to_3x3().inverted().transposed()
    points=[matrix@v.co for v in mesh.vertices]
    hip=arm.matrix_world@arm.pose.bones['mixamorig:Hips'].head
    floor=min(v.z for v in points)
    # Blender front=-Y and up=+Z -> game front=-Z and up=+Y.
    # Negating X as well retains a proper rotation and CCW winding.
    positions=[Vector((-(v.x-hip.x)*SCALE,(v.z-floor)*SCALE+.012,(v.y-hip.y)*SCALE)) for v in points]
    normals=[]
    for v in mesh.vertices:
        n=(normal_matrix@v.normal).normalized()
        normals.append(Vector((-n.x,n.z,n.y)))
    topology=[(t.vertices[0],t.vertices[1],t.vertices[2],20 if t.material_index==0 else 21) for t in mesh.loop_triangles]
    evaluated.to_mesh_clear()
    return positions,normals,topology

clips=[]
topology=None
for kind,frames,duration in [('idle',12,3.2),('walk',24,1.05),('run',24,.58),('idle_duck',12,3.2),('walk_duck',24,1.05),('run_duck',24,.58),('scare',28,1.05)]:
    samples=[]
    for frame in range(frames):
        if kind=='scare':
            arm.animation_data.action=action
            # The opening reach and scream crouch have the strongest silhouette.
            source_frame=1+frame/(frames-1)*55
            scene.frame_set(int(source_frame),subframe=source_frame%1)
            bpy.context.view_layer.update()
        else:
            arm.animation_data.action=None
            pose(frame/frames*2*math.pi,kind.split('_')[0],kind.endswith('_duck'))
        positions,normals,triangles=sample()
        if topology is None:topology=triangles
        assert topology==triangles,'Asset topology changed during rig evaluation'
        samples.append((positions,normals))
    clips.append((kind,duration,samples))

# Stabilize only the cinematic clip around the supplied model's eye geometry.
# Its source crouch otherwise moves the eyes down nearly a metre at close range.
white_indices={i for a,b,c,m in topology if m==21 for i in (a,b,c)}
neutral_positions=clips[0][2][0][0]
white_top=max(neutral_positions[i].y for i in white_indices)
eye_indices=[i for i in white_indices if neutral_positions[i].y>white_top-.095]
target_eye=Vector((0,2.60,0))
for positions,normals in clips[-1][2]:
    anchor=sum((positions[i] for i in eye_indices),Vector())/len(eye_indices)
    delta=target_eye-anchor
    for p in positions:p+=delta

# BRSMIL1: LE fixed header; 7 fixed-size clip descriptors; indexed topology;
# then per-frame vertices with signed-16 position/normal quantization.
# 1 position unit=1/8192 metre, 1 normal unit=1/32767.
vertex_count=len(clips[0][2][0][0])
asset=os.path.join(OUT,'smiler.brm')
with open(asset,'wb') as f:
    f.write(b'BRSMIL1\0')
    f.write(struct.pack('<IIIIff',1,vertex_count,len(topology),len(clips),1/8192,1/32767))
    for name,duration,samples in clips:
        f.write(struct.pack('<16sIfI',name.encode(),len(samples),duration,int(name!='scare')))
    for a,b,c,material in topology:f.write(struct.pack('<IIIB3x',a,b,c,material))
    for name,duration,samples in clips:
        for positions,normals in samples:
            data=array('h')
            for p,n in zip(positions,normals):
                q=[round(x*8192) for x in p]+[round(max(-1,min(1,x))*32767) for x in n]
                assert all(-32767<=x<=32767 for x in q),f'Quantization overflow {name}: {q}'
                data.extend(q)
            f.write(data.tobytes())

report={'source':'source/Zombie Scream.fbx','sha256':hashlib.sha256(open(SOURCE,'rb').read()).hexdigest(),'vertices':vertex_count,'triangles':len(topology),'bones':len(arm.data.bones),'asset_bytes':os.path.getsize(asset),'scale':SCALE,'capture_eye_anchor':list(target_eye),'capture_eye_vertex_count':len(eye_indices),'materials':{'20':'FBX Material.003: black body','21':'FBX Material.004: white eyes and teeth'},'source_action':action.name,'source_frame_range':list(action.frame_range),'clips':[]}
for name,duration,samples in clips:
    points=[p for ps,ns in samples for p in ps]
    report['clips'].append({'name':name,'frames':len(samples),'duration':duration,'min':[min(p[k] for p in points) for k in range(3)],'max':[max(p[k] for p in points) for k in range(3)]})
with open(os.path.join(OUT,'smiler-manifest.json'),'w') as f:json.dump(report,f,indent=2)
print(json.dumps(report,indent=2))

# Preview the actual exported poses, rather than a separately altered model.
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO'
scene.display.shading.color_type='MATERIAL'
scene.display.shading.show_shadows=True
scene.display.shading.show_cavity=True
scene.display.shading.background_type='WORLD'
scene.world=bpy.data.worlds.new('Preview world')
scene.world.color=(.12,.14,.17)
scene.render.resolution_x=600
scene.render.resolution_y=680
scene.render.resolution_percentage=100
bpy.ops.object.camera_add()
camera=bpy.context.object
camera.data.type='ORTHO'
scene.camera=camera
obj.hide_render=True
preview_mesh=bpy.data.meshes.new('Runtime clip preview')
preview=bpy.data.objects.new('Runtime Smiler',preview_mesh)
scene.collection.objects.link(preview)
for material in obj.data.materials:preview_mesh.materials.append(material)
for name,clip_index,frame in [('idle',0,0),('walk',1,6),('run',2,6),('duck',5,6),('scare',6,17)]:
    ps,ns=clips[clip_index][2][frame]
    preview_mesh.clear_geometry()
    # Convert game coordinates back to Blender for a consistent front preview.
    preview_mesh.from_pydata([(p.x,-p.z,p.y) for p in ps],[],[(a,b,c) for a,b,c,m in topology])
    for poly,tri in zip(preview_mesh.polygons,topology):
        poly.material_index=tri[3]-20
        poly.use_smooth=True
    preview_mesh.update()
    center=Vector((0,0,1.5))
    camera.location=Vector((.3,7,1.8))
    camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.ortho_scale=3.7 if name!='scare' else 4.4
    scene.render.filepath=os.path.join(OUT,'preview.png') if name=='idle' else os.path.join(QA,'preview-'+name+'.png')
    bpy.ops.render.render(write_still=True)
