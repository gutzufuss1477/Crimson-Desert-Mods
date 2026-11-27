"""2.06 subset used for Alden category controls and their native reference widgets."""
import struct
from ui_schema_b26636972 import Reader as LegacyReader, Node, number, string, strings
import ui_schema_b26636972 as legacy

def encode(node):
    data=bytearray(strings(node.css))
    data.append(node.has_children or bool(node.children))
    if node.has_children or node.children:
        data+=number(len(node.children))
        for child in node.children:
            data.append(child is not None)
            if child is not None:data+=encode(child)
    data.append(node.has_attrs or bool(node.attrs))
    if node.has_attrs or node.attrs:
        data+=number(len(node.attrs))
        for k,v in node.attrs:data+=string(k)+string(v)
    for index,value in enumerate(node.optional):
        data.append(value is not None)
        if value is None:continue
        if index in (0,2):data+=strings(value)
        elif index==1:data+=value
        elif index==3:data+=string(value[0])+bytes(value[1:])
        else:raise ValueError('Unsupported optional field in authored category')
    data.append(0)  # Additional optional node data in game 2.06, absent on these controls.
    for s in node.names[:5]:data+=string(s)
    condition=node.names[5]
    data+=number(int(bool(condition)))
    if condition:data+=string(condition.removeprefix('!'))+bytes([condition.startswith('!')])
    return bytes(data+node.flags)

class Reader(LegacyReader):
    def node(self,depth=0):
        assert depth<100
        n=Node(start=self.pos);n.css=self.strings()
        n.has_children=self.flag()
        if n.has_children:
            count=self.u32();assert count<10000
            n.children=[self.node(depth+1) if self.flag() else None for _ in range(count)]
        n.has_attrs=self.flag()
        if n.has_attrs:
            count=self.u32();assert count<10000
            n.attrs=[(self.string(),self.string()) for _ in range(count)]
        for index in range(5):
            if not self.flag():continue
            if index in (0,2):n.optional[index]=self.strings()
            elif index==1:
                matches=[b for b in legacy.EVENT_BLOCKS if self.data.startswith(b,self.pos)]
                assert len(matches)==1,(hex(self.pos),'unknown event')
                n.optional[index]=self.raw(len(matches[0]))
            elif index==3:n.optional[index]=(self.string(),*self.raw(2))
            else:raise ValueError('Unsupported optional control data')
        assert not self.flag(),(hex(self.pos),'unsupported extra binding')
        n.names=[self.string() for _ in range(5)]
        count=self.u32();assert count<=1
        cond=''
        if count:
            value=self.string()
            cond=('!' if self.flag() else '')+value
        n.names.append(cond)
        n.flags=self.raw(3);n.end=self.pos
        return n
