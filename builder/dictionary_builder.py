"""Offline, deterministic gbavocab custom indexed .dict builder. No ROM template."""
from pathlib import Path
import re
import struct
import zlib
from accent_fold import key

FILE_MAGIC = b'GVDIDX04'
V3_MAGIC = b'GVDIDX03'
MUTATION_MAGIC = b'GVDIDX02'
LEGACY_MAGIC = b'GVDIDX01'
LEGACY_MAGICS = (V3_MAGIC, MUTATION_MAGIC, LEGACY_MAGIC)
HEADER = 160
SECTOR = 512
BLOCK = 4096
DIR_ENTRIES = 15
DIR_KEY = 24
SLOT = 208
ADDITION_LIMIT = 512
MAX_ENTRIES = (1 << 24) - 1

def _align(value, size):
    return (value + size - 1) // size * size

def _dir_sectors(blocks):
    return (blocks + DIR_ENTRIES - 1) // DIR_ENTRIES

def _layout(n, blocks):
    records = SECTOR
    dirs = _align(records + 4 * n, SECTOR)
    dir0 = dirs
    dir1 = dir0 + _dir_sectors(blocks[0]) * SECTOR
    blocks0 = _align(dir1 + _dir_sectors(blocks[1]) * SECTOR, BLOCK)
    blocks1 = blocks0 + blocks[0] * BLOCK
    return records, (dir0, dir1), (blocks0, blocks1), blocks1 + blocks[1] * BLOCK

def _pack_blocks(pairs, order):
    """Greedy 4 KiB blocks of (row, front, back) entries in one sorted order."""
    blocks, current, used = [], [], 8
    for row in order:
        a, b = (text.encode('utf-8') for text in pairs[row])
        entry = row.to_bytes(3, 'little') + a + b'\0' + b + b'\0'
        if current and used + 2 + len(entry) > BLOCK:
            blocks.append(current); current, used = [], 8
        current.append(entry); used += 2 + len(entry)
    if current: blocks.append(current)
    return blocks

def _block_bytes(entries):
    out = bytearray(BLOCK)
    struct.pack_into('<H', out, 4, len(entries))
    at = 8 + 2 * len(entries)
    for i, entry in enumerate(entries):
        struct.pack_into('<H', out, 8 + 2 * i, at)
        out[at:at + len(entry)] = entry
        at += len(entry)
    struct.pack_into('<I', out, 0, zlib.crc32(out[4:]))
    return out

def build_dict(spec):
    """Version 4: per-direction sorted 4 KiB blocks plus a sector directory."""
    validate_spec(spec)
    pairs = spec['entries']
    n = len(pairs)
    if n > MAX_ENTRIES: raise ValueError('Too many dictionary entries')
    orders, packed = [], []
    for side in range(2):
        keys = [key(pair[side]) for pair in pairs]
        order = sorted(range(n), key=lambda j: (keys[j], j))
        orders.append((order, keys)); packed.append(_pack_blocks(pairs, order))
    records, dirs, starts, end = _layout(n, [len(p) for p in packed])
    if end > LIMIT: raise ValueError('Dictionary base exceeds 64 MiB')
    out = bytearray(end)
    out[:8] = FILE_MAGIC
    struct.pack_into('<II', out, 8, end, n)
    pos = 16
    for field, size in (('name',32),('front',12),('back',12),('front_label',24),('back_label',24)):
        value = spec.get(field, spec.get(field.removesuffix('_label'), ''))
        out[pos:pos+size] = fixed(value,size)
        pos += size
    struct.pack_into('<IIIIIIII', out, 120, BLOCK, records, dirs[0], starts[0], len(packed[0]), dirs[1], starts[1], len(packed[1]))
    for rank, row in enumerate(orders[0][0]):
        struct.pack_into('<I', out, records + 4 * row, rank)
    for side in range(2):
        order, keys = orders[side]
        rank = 0
        directory = bytearray(_dir_sectors(len(packed[side])) * SECTOR)
        for b, entries in enumerate(packed[side]):
            out[starts[side] + b * BLOCK:starts[side] + (b + 1) * BLOCK] = _block_bytes(entries)
            first = keys[order[rank]]
            at = (b // DIR_ENTRIES) * SECTOR + 4 + (b % DIR_ENTRIES) * 32
            struct.pack_into('<IHBB', directory, at, rank, len(entries), min(len(first), DIR_KEY), int(len(first) > DIR_KEY))
            directory[at + 8:at + 8 + min(len(first), DIR_KEY)] = first[:DIR_KEY]
            rank += len(entries)
        for sector in range(0, len(directory), SECTOR):
            struct.pack_into('<I', directory, sector, zlib.crc32(directory[sector + 4:sector + SECTOR]))
        out[dirs[side]:dirs[side] + len(directory)] = directory
    struct.pack_into('<I', out, 152, zlib.crc32(out[SECTOR:]))
    struct.pack_into('<I', out, 156, zlib.crc32(out[:156]))
    return bytes(out)

def build_v3_dict(spec):
    """Legacy GVDIDX03 writer, kept only to produce upgrade/rejection fixtures."""
    validate_spec(spec)
    pairs = spec['entries']
    n = len(pairs)
    records, first, second, strings = HEADER, HEADER+n*12, HEADER+n*20, HEADER+n*28
    out = bytearray(strings)
    out[:8] = V3_MAGIC
    struct.pack_into('<I', out, 12, n)
    pos = 16
    for field, size in (('name',32),('front',12),('back',12),('front_label',24),('back_label',24)):
        value = spec.get(field, spec.get(field.removesuffix('_label'), ''))
        out[pos:pos+size] = fixed(value,size)
        pos += size
    struct.pack_into('<IIII',out,120,records,first,second,strings)
    for i,(a,b) in enumerate(pairs):
        a,b = a.encode('utf-8')+b'\0',b.encode('utf-8')+b'\0'
        struct.pack_into('<III',out,records+i*12,len(out),len(out)+len(a),zlib.crc32(a+b))
        out.extend(a+b)
    for side,at in enumerate((first,second)):
        for i,row in enumerate(sorted(range(n),key=lambda j:(key(pairs[j][side]),j))):
            encoded=struct.pack('<I',row)
            out[at+i*8:at+i*8+8]=encoded+struct.pack('<I',zlib.crc32(encoded))
    struct.pack_into('<I',out,8,len(out))
    struct.pack_into('<I',out,136,zlib.crc32(out[HEADER:]))
    struct.pack_into('<I',out,156,zlib.crc32(out[:156]))
    return bytes(out)

def _identity(data):
    result = {}; at = 16
    for field,size in (('name',32),('front',12),('back',12),('front_label',24),('back_label',24)):
        field_data=data[at:at+size];at+=size
        zero=field_data.find(b'\0')
        if zero<1 or any(field_data[zero:]):raise ValueError('Invalid identity')
        result[field]=field_data[:zero].decode('utf-8')
        fixed(result[field],size)
    return result

def _read_v4(data):
    end, n = struct.unpack_from('<II', data, 8)
    block, records, dir0, blocks0, count0, dir1, blocks1, count1 = struct.unpack_from('<IIIIIIII', data, 120)
    if any(data[HEADER:SECTOR]) or block != BLOCK or not n or n > MAX_ENTRIES or not count0 or not count1 or count0 > n or count1 > n:
        raise ValueError('Invalid base layout')
    if (records, (dir0, dir1), (blocks0, blocks1), end) != _layout(n, (count0, count1)) or end > LIMIT or end > len(data):
        raise ValueError('Invalid base layout')
    if zlib.crc32(data[SECTOR:end]) != struct.unpack_from('<I', data, 152)[0]:
        raise ValueError('Base checksum mismatch')
    rows_by_side, texts = [], {}
    for side, (directory, start, count) in enumerate(((dir0, blocks0, count0), (dir1, blocks1, count1))):
        for sector in range(_dir_sectors(count)):
            at = directory + sector * SECTOR
            if zlib.crc32(data[at + 4:at + SECTOR]) != struct.unpack_from('<I', data, at)[0]:
                raise ValueError('Directory checksum mismatch')
            used = min(DIR_ENTRIES, count - sector * DIR_ENTRIES)
            if any(data[at + 4 + used * 32:at + SECTOR]): raise ValueError('Invalid directory padding')
        rows, rank = [], 0
        for b in range(count):
            at = start + b * BLOCK
            raw = data[at:at + BLOCK]
            if zlib.crc32(raw[4:]) != struct.unpack_from('<I', raw, 0)[0]: raise ValueError('Block checksum mismatch')
            entries = struct.unpack_from('<H', raw, 4)[0]
            if not entries or raw[6:8] != b'\0\0': raise ValueError('Invalid block')
            d = directory + (b // DIR_ENTRIES) * SECTOR + 4 + (b % DIR_ENTRIES) * 32
            first, dcount, keylen, flags = struct.unpack_from('<IHBB', data, d)
            if first != rank or dcount != entries or not 1 <= keylen <= DIR_KEY or flags > 1 or any(data[d + 8 + keylen:d + 32]):
                raise ValueError('Invalid directory entry')
            pos = 8 + 2 * entries
            for i in range(entries):
                offset = struct.unpack_from('<H', raw, 8 + 2 * i)[0]
                if offset != pos: raise ValueError('Invalid block offsets')
                row = int.from_bytes(raw[pos:pos + 3], 'little')
                z1 = raw.find(b'\0', pos + 3); z2 = raw.find(b'\0', z1 + 1) if z1 >= 0 else -1
                if row >= n or z1 < 0 or z2 < 0: raise ValueError('Invalid block entry')
                pair = (raw[pos + 3:z1].decode('utf-8'), raw[z1 + 1:z2].decode('utf-8'))
                validate_pair(pair)
                if texts.setdefault(row, pair) != pair: raise ValueError('Inconsistent entry text')
                if i == 0:
                    k = key(pair[side])
                    if bytes(data[d + 8:d + 8 + keylen]) != k[:DIR_KEY] or flags != int(len(k) > DIR_KEY):
                        raise ValueError('Invalid directory key')
                rows.append(row); pos = z2 + 1
            if any(raw[pos:]): raise ValueError('Invalid block padding')
            rank += entries
        if rank != n or sorted(rows) != list(range(n)): raise ValueError('Invalid sorted index')
        rows_by_side.append(rows)
    pairs = [texts[row] for row in range(n)]
    for side, rows in enumerate(rows_by_side):
        if rows != sorted(range(n), key=lambda j: (key(pairs[j][side]), j)): raise ValueError('Invalid sorted index')
    for rank, row in enumerate(rows_by_side[0]):
        if struct.unpack_from('<I', data, records + 4 * row)[0] != rank: raise ValueError('Invalid record table')
    if any(data[records + 4 * n:dir0]) or any(data[dir1 + _dir_sectors(count1) * SECTOR:blocks0]):
        raise ValueError('Invalid base padding')
    return end, n, pairs, True

def _read_legacy(data):
    end,n=struct.unpack_from('<II',data,8)
    rec,i0,i1,strings=struct.unpack_from('<IIII',data,120)
    if any(data[140:156]) or end>LEGACY_LIMIT or not n or (rec,i0,i1,strings)!=(HEADER,HEADER+n*12,HEADER+n*20,HEADER+n*28) or not strings<end<=len(data):
        raise ValueError('Invalid base layout')
    if zlib.crc32(data[HEADER:end])!=struct.unpack_from('<I',data,136)[0]:
        raise ValueError('Base checksum mismatch')
    pairs=[]
    for i in range(n):
        a,b,crc=struct.unpack_from('<III',data,rec+i*12)
        if not strings<=a<b<end or b-a>191:raise ValueError('Invalid string offsets')
        z=data.find(b'\0',b,min(end,a+193))
        if z<0 or data[b-1]!=0 or b'\0' in data[a:b-1] or zlib.crc32(data[a:z+1])!=crc:raise ValueError('Invalid base record')
        pair=(data[a:b-1].decode('utf-8'),data[b:z].decode('utf-8'))
        validate_pair(pair)
        pairs.append(pair)
    for side,at in enumerate((i0,i1)):
        rows=[]
        for j in range(n):
            row,crc=struct.unpack_from('<II',data,at+j*8)
            if zlib.crc32(data[at+j*8:at+j*8+4])!=crc:raise ValueError('Invalid index checksum')
            rows.append(row)
        sort_key = key if data[:8] == V3_MAGIC else legacy_key
        if rows!=sorted(range(n),key=lambda j:(sort_key(pairs[j][side]),j)):raise ValueError('Invalid sorted index')
    return end, n, pairs, data[:8] in (V3_MAGIC, MUTATION_MAGIC)

def read_dict(data):
    """Read version 4, or a legacy v1/v2/v3 file for conversion by Save As."""
    if len(data)<HEADER or data[:8] not in (FILE_MAGIC,) + LEGACY_MAGICS or zlib.crc32(data[:156])!=struct.unpack_from('<I',data,156)[0]:
        raise ValueError('Invalid or unsupported .dict header')
    if data[:8] == FILE_MAGIC:
        if len(data) < SECTOR: raise ValueError('Invalid base layout')
        end, n, pairs, mutations = _read_v4(data)
    else:
        end, n, pairs, mutations = _read_legacy(data)
    result = _identity(data)
    pairs = list(pairs)
    added={}
    if len(data)-end>SLOT*ADDITION_LIMIT:raise ValueError('Addition capacity exceeded')
    for at in range(end,len(data),SLOT):
        slot=data[at:at+SLOT]
        if len(slot)!=SLOT or slot[204:]!=b'OK01':continue
        if zlib.crc32(slot[:200])!=struct.unpack_from('<I',slot,200)[0]:raise ValueError('Corrupt committed addition')
        target=struct.unpack_from('<I',slot,4)[0]
        editing=slot[:4] in (b'REP2', b'DEL2') and mutations
        deleting=slot[:4]==b'DEL2'
        table, identity = pairs, target
        if editing:
            if target & 0x80000000: table, identity = added, target & 0x7fffffff
            if (identity not in table if isinstance(table, dict) else identity>=n):
                raise ValueError('Invalid mutation target')
            if table[identity] is None and not deleting:raise ValueError('Deleted mutation target')
        elif slot[:4]!=b'ADD1' or target!=(at-end)//SLOT:
            raise ValueError('Corrupt committed addition')
        row=slot[8:200];zero=row.find(b'\0')
        if zero<0 or any(row[zero:]):raise ValueError('Invalid addition padding')
        if deleting:
            if zero:raise ValueError('Invalid deletion payload')
            pair=None
        else:
            pair=tuple(row[:zero].decode('utf-8').split('\t'))
            validate_pair(pair)
        if editing:table[identity]=pair
        else:added[target]=pair
    pairs=[pair for pair in pairs if pair is not None]
    indexes=[sorted(range(len(pairs)),key=lambda j:(key(pairs[j][side]),j)) for side in range(2)]
    result.update(entries=pairs,indexes=indexes,additions=[pair for pair in added.values() if pair is not None])
    # Empty live dictionaries are valid after deletion; the PC builder still
    # refuses to emit an empty immutable base.
    validate_spec(result, allow_empty=True)
    return result


LIMIT = 64 * 1024 * 1024
LEGACY_LIMIT = 32 * 1024 * 1024

def legacy_key(text):
    return text.encode('utf-8').translate(bytes.maketrans(b'ABCDEFGHIJKLMNOPQRSTUVWXYZ',b'abcdefghijklmnopqrstuvwxyz'))

def fixed(text,size):
    data=text.encode('utf-8')
    if not data or len(data)>=size or any(ord(c)<32 or ord(c)==127 for c in text):
        raise ValueError(f'Label must contain 1..{size-1} UTF-8 bytes without controls')
    return data+bytes(size-len(data))

def validate_pair(pair):
    if len(pair)!=2:raise ValueError('Expected exactly two fields')
    for field in pair:
        data=field.encode('utf-8')
        if not any(c>32 for c in data) or any(ord(c)<32 or ord(c)==127 for c in field):raise ValueError('Invalid dictionary field')
    if len(('\t'.join(pair)).encode('utf-8'))>191:raise ValueError('Pair exceeds 191 UTF-8 bytes')

def validate_spec(spec, *, allow_empty=False):
    for side in ('front','back'):
        if not re.fullmatch(r'[a-z][a-z0-9-]{0,10}',spec[side]):
            raise ValueError('Language codes: 1..11 lowercase letters/digits/hyphens, starting with a letter')
    if spec['front']==spec['back']:raise ValueError('Choose distinct languages')
    if not spec['entries'] and not allow_empty:raise ValueError('Empty dictionary')
    for pair in spec['entries']:validate_pair(pair)

def parse_export(text):
    result = []
    for number, line in enumerate(text.lstrip('\ufeff').split('\n'), 1):
        line = line.removesuffix('\r')
        if '\t' not in line and re.fullmatch(r'[\s=\[\]_*+-]*(?:box|field)\s*[1-5][\s=\[\]_*+-]*', line, flags=re.IGNORECASE):
            continue
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        if all(c.isspace() or c in '=+-_|─━│┌┐└┘┼╔╗╚╝═║' for c in line):
            continue
        fields = line.split('\t')
        if len(fields) < 2 or not fields[0].strip() or not fields[1].strip():
            raise ValueError(f'Line {number}: expected two nonempty TAB-separated fields')
        pair = tuple(fields[:2])
        if any(any(ord(c) < 32 or ord(c) == 127 for c in field) for field in pair):
            raise ValueError(f'Line {number}: control character in field')
        if len(('\t'.join(pair)).encode('utf-8')) > 191:
            raise ValueError(f'Line {number}: exceeds 191 UTF-8 bytes for an editable pair')
        result.append(pair)
    if not result:
        raise ValueError('Export contains no vocabulary entries')
    return result
