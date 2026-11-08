"""Check the real reserved-bit migration and execute the production model getter."""
from pathlib import Path
import subprocess,os,re,tempfile,xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1];ns={'k':'http://knx.org/xml/project/20'}
new=ET.fromstring((root/'src/IoHomeHeating.templ.xml').read_text())
old=ET.fromstring(subprocess.check_output(['git','show','4f78936:src/IoHomeHeating.templ.xml'],cwd=root,text=True))
def parameters(tree):return {e.attrib['Id']:e.attrib for e in tree.findall('.//k:Parameters//k:Parameter',ns)}
a,b=parameters(old),parameters(new)
for key,value in a.items():assert b[key]==value,(key,value,b[key])
assert len(b)==len(a)+1
model=next(v for v in b.values() if v['Name']=='c%C%Model');assert model['Offset']=='0' and model['BitOffset']=='3' and model['Value']=='0'
assert new.find('.//k:Union',ns).attrib['SizeInBit']==old.find('.//k:Union',ns).attrib['SizeInBit']=='24'
assert [e.attrib for e in new.findall('.//k:ComObject',ns)]==[e.attrib for e in old.findall('.//k:ComObject',ns)]
source=(root/'src/IoHomecontrolChannel.h').read_text();start=source.index('  IoHomeAdelis::HeatingModel selectedHeatingModel()const');end=source.index('\n  bool privateHeatingEnabled()',start);getter=source[start:end]
with tempfile.TemporaryDirectory() as folder:
 p=Path(folder);common='#include <cassert>\n#include "protocol/IoHomeAdelis.h"\nunsigned selected=0;bool enabled=true;\n'
 for defines in ['', '#define ParamHTG_cModel selected\n#define ParamHTG_cEnabled enabled\n']:
  code=common+defines+'struct Channel{'+getter+'};\nint main(){Channel ch;'
  if defines:code+='for(unsigned i=0;i<4;++i){selected=i;assert(unsigned(ch.selectedHeatingModel())==(i<3?i:0));}selected=2;enabled=false;'
  code+='assert(ch.selectedHeatingModel()==IoHomeAdelis::HeatingModel::Unknown);}'
  (p/'model.cpp').write_text(code);subprocess.run([os.environ.get('CXX','clang++'),'-std=c++17','-I'+str(root/'src'),str(p/'model.cpp'),'-o',str(p/'model')],check=True);subprocess.run([str(p/'model')],check=True)
print('Heating model: reserved bits only, unchanged existing parameters/KOs, fail-closed defaults and production getter passed')
