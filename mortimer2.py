
# TODO: support 0 = transparent paletted images

ALWAYS_CONVERT_INDEXED_IMAGES = True
MAX_TILES = 256

import PIL.Image
import operator, struct

class Tile:
    idx = None
    data = None
    def __init__(self, data):
        self.data = data

    def __hash__(self): return hash(self.data)
    def __eq__(self, other): return self.data == other.data

def mortimer(input_image_name, map_out_name, atlas_out_name, tile_size = (16,16)):
    slab = PIL.Image.open(input_image_name)
    if ALWAYS_CONVERT_INDEXED_IMAGES: slab = slab.convert()
    (width_in_tiles,height_in_tiles) = map(operator.div, slab.size, tile_size)
    print "w/h in tiles: %d, %d; max tiles: %d" % (width_in_tiles,height_in_tiles, width_in_tiles*height_in_tiles)
    if not all(map(lambda x,y:x%y==0, slab.size, tile_size)): raise Exception("Slab doesn't match tile sizes")
    pels = slab.tostring()
    bpp = {'P':1, 'RGB':3, 'RGBA':4}[slab.mode]
    (map_of_ptrs, atlas) = extract_tiles(pels, bpp, (width_in_tiles,height_in_tiles), tile_size)
    print "%d tiles" % len(atlas.keys())
    with open(map_out_name, 'w') as f:
        write_map(f, map_of_ptrs, atlas, (width_in_tiles,height_in_tiles))
    atlas_img = create_atlas_image(atlas, tile_size, slab.mode, bpp)
    atlas_img.save(atlas_out_name)
    return True

def write_map(f, map_of_ptrs, atlas, size):
    n_tiles = 0
    f.write("MAP")
    f.write(struct.pack("!BHH", 1, size[0], size[1]))
    for t in map_of_ptrs:
        if t.idx is None:
            assert n_tiles < MAX_TILES
            t.idx = n_tiles
            n_tiles += 1
        f.write(chr(t.idx))

def create_atlas_image(atlas, tile_size, mode, bpp):
    ordered_atlas = sorted(atlas.values(), key=lambda x:x.idx)
    assert ordered_atlas[0].idx == 0
    assert ordered_atlas[-1].idx == len(ordered_atlas)-1
    p = reduce(lambda p,x: p+x.data, ordered_atlas, '')
    assert len(p) == bpp*tile_size[0]*tile_size[1]*len(ordered_atlas), "p was %d, oa was %d" % (len(p), len(ordered_atlas))
    return PIL.Image.fromstring(mode=mode, size=(tile_size[0], tile_size[1]*len(ordered_atlas)), data=p)

def extract_row_of_tiles(pels, w, ch, th):
    tiles = [''] * w
    for y in range(0, th):
        for x in range(0, w):
            tiles[x] += pels[:ch]
            pels = pels[ch:]
    return (tiles, pels)

def extract_tiles(pels, bpp, size, tile_size):
    assert len(pels) == reduce(operator.mul, size+tile_size)*bpp
    (map_of_ptrs, atlas, ch) = ([], {}, bpp*tile_size[0])
    for ty in range(0, size[1]):
        (tiles,pels) = extract_row_of_tiles(pels, size[0], bpp*tile_size[0], tile_size[1])

        # add row of tiles to map
        for x in range(0, size[0]):
            assert len(tiles[x]) == tile_size[0]*tile_size[1]*bpp, \
                '%d != %d' % (len(tiles[x]), tile_size[0]*tile_size[1]*bpp)
            t = Tile(tiles[x])
            if not atlas.has_key(t):
                atlas[t] = t
            map_of_ptrs.append(atlas[t])
    return (map_of_ptrs, atlas)

import sys, getopt
if __name__ == "__main__":
    (opts, args) = getopt.getopt(sys.argv[1:], "w:h:", ["width=","height="])
    if len(args) != 3: raise Exception("Too few arguments")
    tile_size = (16,16)
    for (option,value) in opts:
        if option == '-w': tile_size = (int(value), tile_size[1])
        if option == '-h': tile_size = (tile_size[0], int(value))
    status = mortimer(*args, tile_size=tile_size)
    print "success" if status else "failure"
    exit(0 if status else 1)

### TESTS
# verify that an empty image produces a map of all zeros
# reject images that don't match
