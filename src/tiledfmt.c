#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "level.h"
#include "tiles.h"
#include "json.h"
#include "dynarr.h"

struct tileset_info {
	int width, height;
	int tile_width, tile_height;
	int xoffs, yoffs;
	int firstgid;
	const char *imgfile;

	struct tileset *tiles;
};

#define MAX_TILESETS	16
static struct tileset_info tsinfo[MAX_TILESETS];
static int num_tsets;

static int load_tsinfo(struct tileset_info *tsi, const char *fname);
static struct tileimg *get_global_tile(int id);

int load_level(struct level *lvl, const char *fname)
{
	static const char *layer_name[] = {"floor", "walls", "void", 0};
	FILE *fp;
	long len;
	char *buf;
	const char *str;
	struct json_obj json, *jobj;
	struct json_item *jitem;
	struct json_arr *jarr, *jdata;
	int i, j, width, height, layer_idx, res = -1;
	struct level_cell *cell;
	char *path = 0, *searchpath, *last_slash;
	int pathbuf_size = 0;

	num_tsets = 0;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "load_level: failed to open: %s\n", fname);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);

	if(!(buf = malloc(len + 1))) {
		fprintf(stderr, "load_level: failed to allocate text buffer\n");
		fclose(fp);
		return -1;
	}
	fread(buf, 1, len, fp);
	buf[len] = 0;

	searchpath = alloca(strlen(fname) + 1);
	strcpy(searchpath, fname);
	if((last_slash = strrchr(searchpath, '/'))) {
		last_slash[1] = 0;
	} else {
		searchpath[0] = 0;
	}

	json_init_obj(&json);
	if(json_parse(&json, buf) == -1) {
		fprintf(stderr, "load_level: failed to parse: %s\n", fname);
		fclose(fp);
		free(buf);
		return -1;
	}
	free(buf);
	fclose(fp);

	width = json_lookup_int(&json, "width", -1);
	height = json_lookup_int(&json, "height", -1);
	if(width <= 0 || width != height) {
		fprintf(stderr, "load_level: invalid level dimensions: %dx%d\n", width, height);
		goto end;
	}

	if(create_level(lvl, width >> 1, MAX_LAYERS) == -1) {
		fprintf(stderr, "load_level: failed to create level\n");
		goto end;
	}

	/* load tileset information */
	if(!(jarr = json_lookup_arr(&json, "tilesets", 0))) {
		fprintf(stderr, "load_level: missing tilesets array\n");
		goto end;
	}
	for(i=0; i<jarr->size; i++) {
		if(jarr->val[i].type != JSON_OBJ) continue;

		jobj = &jarr->val[i].obj;
		if(!(str = json_lookup_str(jobj, "source", 0))) {
			fprintf(stderr, "load_level: ignoring tileset block without source item\n");
			continue;
		}

		len = strlen(str) + strlen(searchpath);
		if(pathbuf_size < len + 1) {
			pathbuf_size = len + 1;
			free(path);
			path = malloc_nf(pathbuf_size);
		}
		strcpy(path, searchpath);
		strcat(path, str);

		if(load_tsinfo(tsinfo + num_tsets, path) == -1) {
			goto end;
		}
		tsinfo[num_tsets++].firstgid = json_lookup_int(jobj, "firstgid", 1);
	}

	/* make sure all the tilesets use the same image */
	for(i=1; i<num_tsets; i++) {
		if(strcmp(tsinfo[i].imgfile, tsinfo[0].imgfile) != 0) {
			fprintf(stderr, "load_level: multiple tilesets using different images are not supported\n");
			goto end;
		}
	}
	/* if the current tilemap uses a different image, load the correct one */
	if(!tileset.name || strcmp(tileset.name, tsinfo[0].imgfile) != 0) {
		if(tileset.name) tiles_destroy(&tileset);
		if(tiles_load(&tileset, tsinfo[0].imgfile) == -1) {
			goto end;
		}
	}

	/* populate tilemaps */
	if(!(jitem = json_find_item(&json, "layers")) || jitem->val.type != JSON_ARR) {
		fprintf(stderr, "load_level: failed to find layers\n");
		goto end;
	}
	jarr = &jitem->val.arr;

	for(i=0; i<jarr->size; i++) {
		if(jarr->val[i].type != JSON_OBJ) continue;
		jobj = &jarr->val[i].obj;
		if(!(str = json_lookup_str(jobj, "name", 0))) continue;
		layer_idx = -1;
		for(j=0; layer_name[j]; j++) {
			if(strcmp(str, layer_name[j]) == 0) {
				layer_idx = j;
			}
		}
		if(layer_idx == -1) continue;
		if(!(jdata = json_lookup_arr(jobj, "data", 0))) continue;

		/* found one of the layers */
		for(j=0; j<jdata->size; j++) {
			lvl->tmap[layer_idx][j] = get_global_tile(jdata->val[j].inum);
		}
	}

	/* populate cells */
	cell = lvl->cells;
	for(i=0; i<lvl->size; i++) {
		for(j=0; j<lvl->size; j++) {
			cell->cx = j;
			cell->cy = i;

			if(get_cell_tile(lvl, cell, 0, 0)) {
				cell->flags |= CELL_WALK;
			}

			calc_cell_height(lvl, cell);

			/* TODO determine exit directions */
			cell++;
		}
	}

	res = 0;
end:
	free(path);
	json_destroy_obj(&json);
	return res;
}

static int load_tsinfo(struct tileset_info *tsi, const char *fname)
{
	FILE *fp;
	long len;
	struct json_obj json;
	char *buf;
	const char *str;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open tileset info file: %s\n", fname);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);

	if(!(buf = malloc(len + 1))) {
		fprintf(stderr, "load_tsinfo(%s): failed to allocate buffer\n", fname);
		fclose(fp);
		return -1;
	}
	fread(buf, 1, len, fp);
	buf[len] = 0;
	fclose(fp);

	json_init_obj(&json);
	if(json_parse(&json, buf) == -1) {
		fprintf(stderr, "load_tsinfo(&s): failed to parse tileset file: %s\n", fname);
		free(buf);
		return -1;
	}
	free(buf);

	tsi->width = json_lookup_int(&json, "imagewidth", 0);
	tsi->height = json_lookup_int(&json, "imageheight", 0);
	tsi->tile_width = json_lookup_int(&json, "tilewidth", 0);
	tsi->tile_height = json_lookup_int(&json, "tileheight", 0);
	tsi->xoffs = json_lookup_int(&json, "tileoffset.x", 0);
	tsi->yoffs = json_lookup_int(&json, "tileoffset.y", 0);
	tsi->tiles = 0;

	if(!(str = json_lookup_str(&json, "image", 0))) {
		fprintf(stderr, "load_tsinfo(%s): tileset without image\n", fname);
		return -1;
	}
	tsi->imgfile = strdup_nf(str);
	tsi->tiles = 0;

	json_destroy_obj(&json);
	return 0;
}

static struct tileimg *get_global_tile(int id)
{
	int i, idx, x, y, tile_x, tile_y, row_tiles, num_tiles;
	struct tileset_info *best = tsinfo;
	struct tileimg *tile;

	if(id <= 0) return 0;

	for(i=1; i<num_tsets; i++) {
		if(tsinfo[i].firstgid <= id && tsinfo[i].firstgid > best->firstgid) {
			best = tsinfo + i;
		}
	}

	idx = id - best->firstgid;
	row_tiles = best->width / best->tile_width;
	tile_y = idx / row_tiles;
	tile_x = idx % row_tiles;
	x = tile_x * best->tile_width;
	y = tile_y * best->tile_height;

	num_tiles = dynarr_size(tileset.tiles);
	for(i=0; i<num_tiles; i++) {
		if(tileset.tiles[i]->x == x && tileset.tiles[i]->y == y) {
			/* TODO insert into rbtree by id */
			return tileset.tiles[i];
		}
	}

	/* not found, define it */
	tile = tiles_define(&tileset, x, y, best->tile_width, best->tile_height);
	tile->xorg = -best->xoffs;
	tile->yorg = -best->yoffs;
	return tile;
}
