"""Offline extraction of isolated steps from the two user-supplied recordings."""
import argparse, array, json, math, pathlib, subprocess, wave, hashlib
p=argparse.ArgumentParser()
p.add_argument('--ffmpeg',required=True); p.add_argument('--player',required=True); p.add_argument('--monster',required=True)
a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]
for kind,source,count,length in [('player',a.player,6,.34),('monster',a.monster,4,.43)]:
    source=pathlib.Path(source); dest=root/'assets/audio'/('footsteps-'+kind); dest.mkdir(parents=True,exist_ok=True)
    (dest/source.name).write_bytes(source.read_bytes())
    raw=subprocess.check_output([a.ffmpeg,'-v','error','-i',str(source),'-ac','1','-ar','22050','-f','s16le','-'])
    samples=array.array('h',raw); hop=220
    envelope=[sum(float(v)*v for v in samples[i:i+hop])/hop for i in range(0,len(samples)-hop,hop)]
    candidates=sorted(range(2,len(envelope)-2),key=envelope.__getitem__,reverse=True)
    peaks=[]
    for i in candidates:
        if all(abs(i-j)*hop>int(.38*22050) for j in peaks): peaks.append(i)
        if len(peaks)==count: break
    entries=[]
    for number,peak in enumerate(sorted(peaks),1):
        start=max(0,peak*hop-int(.045*22050)); clip=samples[start:start+int(length*22050)]
        gain=25000/max(1,max(abs(v) for v in clip))
        cooked=array.array('h',(int(v*gain*min(1,i/110,(len(clip)-1-i)/660)) for i,v in enumerate(clip)))
        name=f'step-{number:02}.wav'
        with wave.open(str(dest/name),'wb') as out:
            out.setparams((1,2,22050,0,'NONE','not compressed')); out.writeframes(cooked.tobytes())
        entries.append({'file':name,'start_seconds':round(start/22050,4),'duration':len(clip)/22050})
    (dest/'manifest.json').write_text(json.dumps({'source':source.name,'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'sample_rate':22050,'clips':entries},indent=2)+'\n')
    print(kind,entries)
