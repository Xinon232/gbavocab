"""Synthetic-only native executable verification of v4 and legacy upgrades."""
import struct
import zlib
from pathlib import Path
import dictionary_builder as pack

def accent_upgrade(directory, build, open_review):
    pairs=[('zoo','last'),('mañana','CAFÉ'),('manana','cafe'),('ÉLÈVE','ǘber')]
    spec=dict(name='Accent self-test',front='es',back='en',entries=pairs)
    directory=Path(directory)
    path=directory/'Accents.dict'
    build(spec,path)
    data=path.read_bytes(); decoded=pack.read_dict(data)
    assert data[:8]==b'GVDIDX04' and decoded['entries']==pairs
    assert decoded['indexes']==[[3,1,2,0],[1,2,0,3]]
    assert pack.key('MAÑANA')==pack.key('man\u0303ana')==b'manana'
    assert pack.key('CAFÉ')==pack.key('cafe\u0301')==b'cafe'
    assert pack.key('ß æ œ ø ł عَرَبِيّ 茶')=='ß æ œ ø ł عَرَبِيّ 茶'.encode()
    def slot(kind,target,text=''):
        out=bytearray(208);out[:4]=kind;struct.pack_into('<I',out,4,target)
        payload=text.encode();out[8:8+len(payload)]=payload
        struct.pack_into('<I',out,200,zlib.crc32(out[:200]));out[204:]=b'OK01';return out
    legacy=pack.build_v3_dict(spec)
    for version in (1,2):
        old=bytearray(legacy);old[7]=ord(str(version))
        for side,at in enumerate(struct.unpack_from('<II',old,124)):
            for rank,row in enumerate(sorted(range(len(pairs)),key=lambda i:(pack.legacy_key(pairs[i][side]),i))):
                cell=struct.pack('<I',row);old[at+rank*8:at+rank*8+8]=cell+struct.pack('<I',zlib.crc32(cell))
        struct.pack_into('<I',old,136,zlib.crc32(old[160:]));struct.pack_into('<I',old,156,zlib.crc32(old[:156]))
        old+=slot(b'ADD1',0,'café\tadded')
        expected=pairs+[('café','added')]
        if version==2:
            old+=slot(b'REP2',1,'MAÑANA\tchanged')+slot(b'DEL2',0)+slot(b'REP2',0x80000000,'ÉLÈVE\tnew')
            expected=[('MAÑANA','changed'),pairs[2],pairs[3],('ÉLÈVE','new')]
        source=directory/f'Legacy{version}.dict';source.write_bytes(old)
        review=open_review(source); output=directory/f'Upgraded{version}.dict'
        build(dict(review.defaults,entries=review.included_pairs()),output)
        assert pack.read_dict(output.read_bytes())['entries']==expected
        assert output.read_bytes()[:8]==b'GVDIDX04' and source.read_bytes()==old
    return dict(accent_indexes_verified=True,legacy_upgrade_verified=True)
