#!/usr/bin/env python3
"""Import only reviewed public functional replies, never auth/key or arbitrary register bodies."""
import argparse,hashlib,json,re
from pathlib import Path
import yaml
COMMIT='8680ba4c8d96a3ca412c6321c3d431622b9b247a'
NAMES=['probe/'+n+'.yaml' for n in ['somfy_rs100_probe_subscribe_event_battery_sx1262','somfy_rs100_probe_subscribe_event_daylight_sx1262','atlantic_adelis_probe_mode_reads','atlantic_adelis_probe_boost_remaining','atlantic_adelis_probe_register_01_sweep','atlantic_adelis_probe_register_01_sweep_drying','atlantic_adelis_probe_status_mp_fp','atlantic_calypso_probe_status_mp_fp']]+['exchange/atlantic_calypso_exchange_register_read.yaml']
REGS={0:{0,4,6,7,8,0x17,0x21},1:{0,3,0x15,0x19,0x26,0x31,0x32,0x33,0x34,0x36,0x38,0x3A,0x3E}}
def safe(w):
 if len(w)<9 or len(w)>32 or w[0]&0x20 or (w[0]&31)+1!=len(w):return False
 d=w[9:]
 if w[8]==0x71:return (len(d)==10 and d[2:7]==bytes.fromhex('2005050600')) or (len(d)==6 and d[2:5]==bytes.fromhex('200101') and d[5]<=100)
 if w[8]==4:return len(d)>=6
 if w[8]==0x21:return len(d)>=4 and d[:2]==bytes.fromhex('0c60') and d[2] in REGS and d[3] in REGS[d[2]]
 return False

def build(root):
 sources=[];frames=[]
 for i,name in enumerate(NAMES):
  path=root/'tests/corpus/captures'/name;raw=path.read_bytes();doc=yaml.safe_load(raw)
  sources.append(dict(path='tests/corpus/captures/'+name,source_url=f'https://github.com/laberning/home_io_control/blob/{COMMIT}/tests/corpus/captures/{name}',source_sha256=hashlib.sha256(raw).hexdigest(),upstream_commit=COMMIT,source_issue=doc.get('source',{}).get('issue'),capture_date=str(doc.get('source',{}).get('date')),omitted_frames=0))
  for number,frame in enumerate(doc['frames'],1):
   w=bytes.fromhex(frame['hex'])
   if not safe(w):sources[-1]['omitted_frames']+=1;continue
   frames.append(dict(source_index=i,source_frame=number,direction=frame['dir'],reported_frequency_hz=frame.get('freq'),hex=w.hex(' ').upper(),capture_class='A',confidence='published_complete_frame_authentication_unverifiable',crc=frame.get('crc','unknown')))
 return dict(schema_version=1,upstream_commit=COMMIT,sources=sources,frames=frames)
def header(doc):
 out=['#pragma once','#include <cstdint>','namespace AlignmentCaptures {','struct Frame {uint8_t sourceIndex,length;uint32_t frequency;uint8_t wire[32];};','inline constexpr Frame frames[]={']
 for r in doc['frames']:
  w=bytes.fromhex(r['hex'])
  if not safe(w) or r['capture_class']!='A':raise ValueError('unreviewed public frame')
  out.append('{%d,%d,%d,{%s}},'%(r['source_index'],len(w),r['reported_frequency_hz'] or 0,','.join('0x%02X'%b for b in w)))
 return '\n'.join(out+['};','}'])+'\n'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('input',type=Path);p.add_argument('--output',type=Path);p.add_argument('--check',action='store_true');a=p.parse_args()
 if a.check:
  doc=json.loads(a.input.read_text());assert all(re.fullmatch('[0-9a-f]{64}',s['source_sha256']) for s in doc['sources']);assert a.input.with_suffix('.h').read_text()==header(doc)
 else:
  doc=build(a.input);assert a.output;a.output.write_text(json.dumps(doc,indent=2)+'\n');a.output.with_suffix('.h').write_text(header(doc))
if __name__=='__main__':main()
