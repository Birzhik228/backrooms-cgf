"""Bake the supplied unrigged GLB using an authored four-chain IK rig.

Blender 4.3 --background --disable-autoexec --python tools/bake_aranea.py
All source triangles, material assignments and UV seams are retained. Blender
is an offline tool; the game samples the bounded BRARAN3 animation cache.
"""
import bpy, json, math, struct, hashlib
import numpy as np
from pathlib import Path
from mathutils import Vector, Quaternion

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'assets/models/aranea'
RIG=json.loads((ROOT/'tools/aranea-rig.json').read_text())
SOURCE=OUT/'source/Scene.glb'
SCALE=.53
# The source is an assembled pose, not a neutral rig. Its body centerline and
# skull both point diagonally. Solve those two rest orientations separately;
# changing only the runtime yaw would leave the feet and face moving sideways.
source_hips=[Vector(l['hip']) for l in RIG['legs']]
source_center=sum(source_hips,Vector())*.25
source_body_forward=(source_hips[0]+source_hips[1]-source_hips[2]-source_hips[3])*.5
body_yaw=math.atan2(source_body_forward.x,source_body_forward.y)
body_basis=Quaternion(Vector((0,0,1)),body_yaw).to_matrix()
def point(v):
    q=body_basis@(Vector(v)-Vector((source_center.x,source_center.y,-3.40)))
    return Vector((q.x,q.z,-q.y))*SCALE
def direction(v):
    q=body_basis@Vector(v)
    return Vector((q.x,q.z,-q.y))
def smooth(t): return max(0,min(1,t))**2*(3-2*max(0,min(1,t)))
def arr(v): return np.array(v,dtype=np.float64)
def rotation(axis,angle): return np.array(Quaternion(Vector(axis),angle).to_matrix(),dtype=np.float64)
def rigid(p,n,pivot,rot,target):
    return (p-arr(pivot))@rot.T+arr(target),n@rot.T

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(SOURCE))
head=RIG['head']
def mesh_center(name):
    obj=bpy.data.objects[name]
    return sum((obj.matrix_world@v.co for v in obj.data.vertices),Vector())/len(obj.data.vertices)
# These paired orbital rim meshes are anatomical landmarks, independent of
# the arbitrary GLB parent axes. Level the eyes and make their normal -Z.
socket_left=mesh_center(head['rest_alignment']['left_socket_mesh'])
socket_right=mesh_center(head['rest_alignment']['right_socket_mesh'])
source_eye=(socket_left+socket_right)*.5
source_right=(socket_right-socket_left).normalized()
source_forward=Vector((0,0,1)).cross(source_right).normalized()
source_up=source_right.cross(source_forward).normalized()
eye=point(source_eye);eye.x=0
def head_direction(v):
    q=Vector(v)
    return Vector((q.dot(source_right),q.dot(source_up),-q.dot(source_forward)))
def head_point(v): return eye+head_direction(Vector(v)-source_eye)*SCALE
neck=head_point(head['neck_pivot']);neck.x=0
jaw=head_point(head['jaw_pivot']);jaw.x=0
legs=[];parts={};triangles=[];masks=[];uvs=[];offset=0
for l in RIG['legs']:
    legs.append({k:point(l[k]) for k in ['hip','knee','tip']})
    legs[-1].update(upper=l['upper_mesh'],lower=l['lower_mesh'],name=l['name'])
for obj in sorted((o for o in bpy.context.scene.objects if o.type=='MESH'),key=lambda o:o.name):
    mesh=obj.data;mesh.calc_loop_triangles()
    world=obj.matrix_world;normal_matrix=world.to_3x3().inverted().transposed()
    pp=[];nn=[];mapping={};obj_masks=[];obj_uv=[]
    leg_index=next((i for i,l in enumerate(legs) if obj.name in [l['upper'],l['lower']]),None)
    is_lower=leg_index is not None and obj.name==legs[leg_index]['lower']
    is_head=obj.parent.name in [head['skull_parent'],head['jaw_parent']]
    for tri in mesh.loop_triangles:
        ids=[]
        for loop_index in tri.loops:
            vi=mesh.loops[loop_index].vertex_index
            uv=mesh.uv_layers.active.data[loop_index].uv
            n=mesh.corner_normals[loop_index].vector
            key=(vi,round(uv.x,7),round(uv.y,7),*(round(c,6) for c in n))
            if key not in mapping:
                mapping[key]=offset+len(pp)
                p=(head_point if is_head else point)(world@mesh.vertices[vi].co)
                normal=(head_direction if is_head else direction)(normal_matrix@n).normalized()
                pp.append(list(p));nn.append(list(normal));obj_uv.append(tuple(uv))
                feet=[0]*4
                if is_lower:
                    # Only the final tip/cap is a rigid grounded foot probe.
                    l=legs[leg_index]
                    t=(p-l['knee']).dot(l['tip']-l['knee'])/(l['tip']-l['knee']).length_squared
                    if t>.94: feet[leg_index]=255
                obj_masks.append([255 if is_head else 0,*feet])
            ids.append(mapping[key])
        material={'Mist_Carpet':20,'White_Carpet':21,'Black_Carpet':22}[obj.data.materials[tri.material_index].name]
        triangles.append((*ids,material))
    parts[obj.name]=dict(p=np.array(pp),n=np.array(nn),offset=offset,leg=leg_index,lower=is_lower,
                         head=is_head,jaw=obj.parent.name==head['jaw_parent'])
    masks.extend(obj_masks);uvs.extend(obj_uv);offset+=len(pp)

for l in legs:
    l['a']=(l['knee']-l['hip']).length;l['b']=(l['tip']-l['knee']).length
    lower=parts[l['lower']]['p']
    l['ground']=l['tip'].y-float(lower[:,1].min())+.012
    l['base']=l['tip'].copy();l['base'].y=l['ground']
    # A forward-facing creature keeps a broad stance, but centers its legs
    # enough to articulate inside the rooms instead of rotating the whole mesh.

def foot_phase(phase,index,run):
    # Sequential swing = three supporting feet. Chase uses diagonal pairs.
    return (phase+([0,.5,.5,0] if run else [0,.5,.75,.25])[index])%1

def pose(phase,kind,duck=False):
    run=kind=='run';moving=kind in ('walk','run');scare=kind=='scare'
    angle=phase*2*math.pi
    support=.58 if run else .78;stride=2.0 if run else 1.30
    roll=(.025 if run else .018)*math.sin(angle) if moving else .008*math.sin(angle)
    bob=(.038 if run else .016)*math.cos(angle*2) if moving else .007*math.sin(angle)
    drop=.45 if duck else 0
    body_offset=Vector((.030*math.sin(angle) if moving else 0,bob-drop,0))
    body_rot=rotation((0,0,1),roll)@rotation((1,0,0),-.035 if run else 0)
    body_pivot=Vector((0,1.65,0))
    head_pitch=.025*math.sin(angle*2+.7) if moving else .035*math.sin(angle)
    head_yaw=0
    jaw_angle=-.035*(.5+.5*math.sin(angle*2))
    if scare:
        attack=smooth(phase/.42);body_offset=Vector((0,-.12*attack,-.16*attack))
        body_rot=rotation((1,0,0),-.13*attack)
        # Counter the body's forward lunge tilt so the face stays square to
        # the close camera instead of pitching its snout over the open jaw.
        head_pitch=.13*attack+.025*math.sin(phase*35)*attack
        head_yaw=.045*math.sin(phase*67)*attack*(1-.55*phase)
        jaw_angle=-.50*attack
    def body(v): return Vector(body_rot@(arr(v)-arr(body_pivot))+arr(body_pivot+body_offset))
    pivot=body(neck);jaw_world=body(jaw)
    head_rot=rotation((0,1,0),head_yaw)@rotation((1,0,0),head_pitch)
    legposes=[];flags=0
    for i,l in enumerate(legs):
        u=foot_phase(phase,i,run)
        forward=lift=0
        if moving:
            if u<support: forward=(u-support*.5)*stride
            else:
                t=(u-support)/(1-support)
                forward=support*.5*stride*(1-2*smooth(t))
                lift=(.12 if duck else .30 if run else .18)*math.sin(math.pi*t)**1.3
        if not moving or u<support: flags|=1<<i
        foot=l['base'].copy();foot.z+=forward;foot.y+=lift
        # Folding under low lintels also pulls the long feet inward.
        foot.x*=1-.58*int(duck)
        if scare and i<2:
            foot.y+=.75*smooth(phase/.4);foot.z-=.55*smooth(phase/.4)
            flags&=~(1<<i)
        hip=body(l['hip']);delta=foot-hip;dist=delta.length
        unit=delta.normalized();d=max(abs(l['a']-l['b'])+.001,min(l['a']+l['b']-.001,dist))
        # Rest knee supplies the bend plane; retain its outward spider posture.
        pole=body(l['knee'])-hip
        if duck:
            # Fold the knees along the body through a narrow doorway, rather
            # than squashing its skull or leaving rigid legs in the lintel.
            pole=Vector((-.10 if i in (0,2) else .10,-.25,-1 if i<2 else 1))
        pole-=unit*pole.dot(unit);pole.normalize()
        along=(l['a']**2-l['b']**2+d*d)/(2*d)
        knee=hip+unit*along+pole*math.sqrt(max(0,l['a']**2-along*along))
        assert abs(dist-d)<.015, ('IK reach',i,phase,kind,duck,dist,d)
        legposes.append((hip,knee,foot))
    posed=[];normals=[]
    for name,part in parts.items():
        p,n=part['p'],part['n'];i=part['leg']
        if i is not None:
            l=legs[i];hip,knee,foot=legposes[i]
            a,b=(l['knee'],l['tip']) if part['lower'] else (l['hip'],l['knee'])
            aa,bb=(knee,foot) if part['lower'] else (hip,knee)
            rot=np.array((b-a).rotation_difference(bb-aa).to_matrix())
            q,ns=rigid(p,n,a,rot,aa)
            if part['lower']:
                t=((p-arr(a))@arr(b-a))/(b-a).length_squared
                blend=np.clip((t-.78)/.16,0,1);blend=blend*blend*(3-2*blend)
                # Tips remain oriented to the floor throughout their support
                # phase, with a flexible ankle zone above the actual contact.
                target=p+arr(foot-l['tip'])
                q=q*(1-blend[:,None])+target*blend[:,None]
                ns=ns*(1-blend[:,None])+n*blend[:,None]
        else:
            q,ns=rigid(p,n,body_pivot,body_rot,body_pivot+body_offset)
            if part['jaw']:
                q,ns=rigid(q,ns,jaw_world,rotation((1,0,0),jaw_angle),jaw_world)
            if part['head']:
                q,ns=rigid(q,ns,pivot,head_rot,pivot)
        posed.append(q);normals.append(ns)
    p=np.concatenate(posed);n=np.concatenate(normals)
    n/=np.linalg.norm(n,axis=1)[:,None]
    if scare:
        eye_world=head_rot@(arr(body(eye))-arr(pivot))+arr(pivot)
        offset=np.array([0,1.75,0])-eye_world
        p+=offset;pivot=Vector(arr(pivot)+offset)
    return p,n,pivot,flags

clips=[('idle',16,3.2),('walk',40,1.2),('run',40,.65),('idle_duck',16,3.2),('walk_duck',40,1.2),('run_duck',40,.65),('scare',28,2.0)]
assert offset<=20000 and len(triangles)==15220,(offset,len(triangles))
bank=bytearray(struct.pack('<8sIIIIff',b'BRARAN3\0',3,offset,len(triangles),7,1/8192,1/32767))
for name,count,duration in clips: bank+=struct.pack('<16sIfI',name.encode(),count,duration,int(name!='scare'))
for t in triangles: bank+=struct.pack('<IIIB3x',*t)
for mask,uv in zip(masks,uvs): bank+=struct.pack('<BBBBBff',*mask,*uv)
manifest=dict(source=str(SOURCE.relative_to(ROOT)),sourceSha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
              vertices=offset,triangles=len(triangles),parts=len(parts),sourceRig=False,rig='tools/aranea-rig.json',
              coordinateSystem='metres, Y-up, -Z forward',walkStride=1.30,runStride=2.0,
              feet=[l['name'] for l in legs],clips=[],
              restAlignment=dict(bodyYawCorrectionDegrees=math.degrees(body_yaw),
                                 sourceBodyCenter=list(source_center),
                                 sourceBodyForward=list(source_body_forward.normalized()),
                                 sourceHeadRight=list(source_right),sourceHeadForward=list(source_forward),
                                 headYawCorrectionDegrees=-math.degrees(math.atan2(source_right.y,source_right.x)),
                                 sourceEyeCenter=list(source_eye),runtimeEyeCenter=list(eye),
                                 runtimeNeckPivot=list(neck),runtimeJawPivot=list(jaw),
                                 runtimeHeadForward=list(head_direction(source_forward)),
                                 socketCenters=[list(head_point(socket_left)),list(head_point(socket_right))],
                                 socketVertexRanges=[dict(side=side,first=parts[name]['offset'],
                                     count=len(parts[name]['p'])) for side,name in
                                     [('left',head['rest_alignment']['left_socket_mesh']),
                                      ('right',head['rest_alignment']['right_socket_mesh'])]]))
for name,count,duration in clips:
    bounds=[np.full(3,1e9),np.full(3,-1e9)]
    for frame in range(count):
        p,n,pivot,flags=pose(frame/(count-1 if name=='scare' else count),name.split('_')[0],name.endswith('_duck'))
        bounds[0]=np.minimum(bounds[0],p.min(axis=0));bounds[1]=np.maximum(bounds[1],p.max(axis=0))
        values=np.concatenate((p*8192,n*32767),axis=1).round()
        assert np.abs(values).max()<=32767,(name,frame,'quantization overflow',values.min(),values.max())
        bank+=struct.pack('<hhhBB',*[round(x*8192) for x in pivot],flags,0)
        bank+=values.astype('<i2').tobytes()
    manifest['clips'].append(dict(name=name,frames=count,duration=duration,bounds=[v.tolist() for v in bounds]))
    print(name,manifest['clips'][-1],flush=True)
(OUT/'aranea.brm').write_bytes(bank)
manifest.update(bytes=len(bank),sha256=hashlib.sha256(bank).hexdigest())
(OUT/'aranea-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Baked',len(bank),'bytes;',offset,'vertices;',len(triangles),'triangles.',flush=True)
