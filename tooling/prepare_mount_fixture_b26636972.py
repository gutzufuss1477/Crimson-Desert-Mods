"""Generate the private native continuation from an exact, locally owned EXE."""
from pathlib import Path
import argparse,hashlib,struct

EXPECTED='8be67178d9b9a149c361a731051665e0ba4d441e250ae1dfe645bd6e524345c8'
def at(data,rva,size):
    pe=struct.unpack_from('<I',data,0x3c)[0]
    count=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0]
    for i in range(count):
        _,va,length,raw=struct.unpack_from('<4I',data,pe+24+optional+40*i+8)
        if va<=rva and rva+size<=va+length:return data[raw+rva-va:raw+rva-va+size]
    raise ValueError('RVA outside raw section')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);args=p.parse_args()
    data=args.exe.read_bytes()
    if hashlib.sha256(data).hexdigest()!=EXPECTED:raise ValueError('Unsupported EXE; re-audit required')
    # Authored ABI shim: preserve R14/RDI, supply owner/old maximum/division constant.
    prefix=bytes.fromhex('4156574883ec484c8bf1498bc848bfcff753e3a59bc420')
    code=prefix+at(data,0x1050269,0x1050302-0x1050269)+bytes.fromhex('4883c4485f415ec3')
    target=Path(__file__).with_name('native_mount_code.generated.hpp')
    target.write_text('static const U8 code[]={'+','.join(f'0x{x:02x}' for x in code)+'};\n')
    print('Generated ignored local native fixture:',target.name)
if __name__=='__main__':main()
