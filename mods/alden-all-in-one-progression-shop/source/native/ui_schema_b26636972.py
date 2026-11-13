"""Codec for Alden's inserted subtree, derived from native UI readers.

b26636972: UIPropertyConditionData adds hasCssAttribute after overrideType.
Two event blocks are opaque; validation requires their exact presence in the
fresh game's UI. Unsupported events fail closed. This is not a full UI codec.
"""
import struct
from pathlib import Path

from dataclasses import dataclass, field

def number(n):return struct.pack('<I',n)
def string(s):
    b=s.encode('utf-8');return number(len(b))+b
def strings(values):return number(len(values))+b''.join(string(s) for s in values)

EVENT_BLOCKS=()
EVENT_SPECS=[{'prefix': '0200000001170000004f6e53657454657874466f72456d6f6a69456e61626c65', 'size': 222, 'sha256': 'e69e392d206ff6ceaa55a07108a85a31b17cc2e57eb84afa9f88c093b3785093'}, {'prefix': '080000000111000000536574466f726365486964655374617465000000000000', 'size': 3172, 'sha256': '24a957c783ab6a36bbcbef192fe55247aecbb73f566b45cec1161e44fb8d223a'}]

@dataclass
class Node:
    css:list=field(default_factory=list)
    children:list=field(default_factory=list)
    attrs:list=field(default_factory=list)
    optional:list=field(default_factory=lambda:[None]*5)
    names:list=field(default_factory=lambda:['']*6)
    flags:bytes=b'\x01\x00\x06'
    start:int=0
    end:int=0
    has_children:bool=False
    has_attrs:bool=False
    @property
    def id(self):return self.names[0]
    def walk(self):
        yield self
        for child in self.children:
            if child:yield from child.walk()
    def encode(self):
        data=strings(self.css)
        data+=bytes([bool(self.children) or self.has_children])
        if self.children or self.has_children:
            data+=number(len(self.children))+b''.join(b'\1'+c.encode() if c else b'\0' for c in self.children)
        data+=bytes([bool(self.attrs) or self.has_attrs])
        if self.attrs or self.has_attrs:
            data+=number(len(self.attrs))+b''.join(string(k)+string(v) for k,v in self.attrs)
        for index,value in enumerate(self.optional):
            data+=bytes([value is not None])
            if value is not None:
                if index in (0,2):data+=strings(value)
                elif index==1:data+=value
                elif index==3:data+=string(value[0])+bytes(value[1:])
                elif index==4:data+=string(value[0])+string(value[1])
                else:raise ValueError('Unsupported complex binding')
        return data+b''.join(string(s) for s in self.names)+self.flags

class Reader:
    def __init__(self,data,text_bytes=2):self.data=data;self.pos=0;self.text_bytes=text_bytes
    def raw(self,n):
        assert 0<=n<=len(self.data)-self.pos,(hex(self.pos),n)
        out=self.data[self.pos:self.pos+n];self.pos+=n;return out
    def u32(self):return struct.unpack('<I',self.raw(4))[0]
    def flag(self):
        n=self.raw(1)[0];assert n in (0,1),(hex(self.pos-1),n);return bool(n)
    def string(self):return self.raw(self.u32()).decode('utf-8')
    def strings(self):
        n=self.u32();assert n<10000,(hex(self.pos-4),n)
        return [self.string() for _ in range(n)]
    def node(self,depth=0):
        assert depth<100
        n=Node(start=self.pos,css=self.strings())
        n.has_children=self.flag()
        if n.has_children:
            count=self.u32();assert count<10000
            n.children=[self.node(depth+1) if self.flag() else None for _ in range(count)]
        n.has_attrs=self.flag()
        if n.has_attrs:
            count=self.u32();assert count<10000
            n.attrs=[(self.string(),self.string()) for _ in range(count)]
        for i in range(5):
            if self.flag():
                if i in (0,2):n.optional[i]=self.strings()
                elif i==1:
                    matches=[b for b in EVENT_BLOCKS if self.data[self.pos:self.pos+len(b)]==b]
                    assert len(matches)==1, f"Unknown event binding at {self.pos:x}"
                    n.optional[i]=self.raw(len(matches[0]))
                elif i==3:n.optional[i]=(self.string(),*self.raw(self.text_bytes))
                elif i==4:n.optional[i]=(self.string(),self.string())
                else:raise ValueError(f'Unsupported complex binding at {self.pos:x}')
        n.names=[self.string() for _ in range(6)]
        n.flags=self.raw(3);n.end=self.pos
        assert n.encode()==self.data[n.start:n.end],hex(n.start)
        return n


def configure_events(vanilla):
    """Read opaque native events from the supplied game UI; verify exact hashes."""
    import hashlib
    global EVENT_BLOCKS
    found=[]
    for spec in EVENT_SPECS:
        prefix=bytes.fromhex(spec['prefix']);start=0;matches=[]
        while (at:=vanilla.find(prefix,start))>=0:
            block=vanilla[at:at+spec['size']]
            if hashlib.sha256(block).hexdigest()==spec['sha256']:matches.append(block)
            start=at+1
        if not matches:raise ValueError('Unsupported native event block')
        found.append(matches[0])
    EVENT_BLOCKS=tuple(found)
