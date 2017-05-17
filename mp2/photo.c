/*									tab:8
 *
 * photo.c - photo display functions
 *
 * "Copyright (c) 2011 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    3
 * Creation Date:   Fri Sep  9 21:44:10 2011
 * Filename:	    photo.c
 * History:
 *	SL	1	Fri Sep  9 21:44:10 2011
 *		First written (based on mazegame code).
 *	SL	2	Sun Sep 11 14:57:59 2011
 *		Completed initial implementation of functions.
 *	SL	3	Wed Sep 14 21:49:44 2011
 *		Cleaned up code for distribution.
 */


#include <string.h>
#include "assert.h"
#include "modex.h"
#include "photo.h"
#include "photo_headers.h"
#include "world.h"

#define level_4_size 4096
#define level_4_node 128
#define level_2_size 64
#define level_2_node 64
#define red_bitmask 0x1F
#define green_bitmask 0x3F
#define blue_bitmask 0x1F
#define bitmask 0x3
#define bitmask_6 0x3F
#define palette_size 192
#define palette_dim 3

octree level_4[level_4_size];
octree level_2[level_2_size];
/* types local to this file (declared in types.h) */

/* 
 * A room photo.  Note that you must write the code that selects the
 * optimized palette colors and fills in the pixel data using them as 
 * well as the code that sets up the VGA to make use of these colors.
 * Pixel data are stored as one-byte values starting from the upper
 * left and traversing the top row before returning to the left of
 * the second row, and so forth.  No padding should be used.
 */
struct photo_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t        palette[192][3];     /* optimized palette colors */
    uint8_t*       img;                 /* pixel data               */
};

/* 
 * An object image.  The code for managing these images has been given
 * to you.  The data are simply loaded from a file, where they have 
 * been stored as 2:2:2-bit RGB values (one byte each), including 
 * transparent pixels (value OBJ_CLR_TRANSP).  As with the room photos, 
 * pixel data are stored as one-byte values starting from the upper 
 * left and traversing the top row before returning to the left of the 
 * second row, and so forth.  No padding is used.
 */
struct image_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t*       img;                 /* pixel data               */
};


/* file-scope variables */

/* 
 * The room currently shown on the screen.  This value is not known to 
 * the mode X code, but is needed when filling buffers in callbacks from 
 * that code (fill_horiz_buffer/fill_vert_buffer).  The value is set 
 * by calling prep_room.
 */
static const room_t* cur_room = NULL; 


/* 
 * fill_horiz_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the leftmost 
 *                pixel of a line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- leftmost pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_horiz_buffer (int x, int y, unsigned char buf[SCROLL_X_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgx;  /* loop index over pixels in object image      */ 
    int            yoff;  /* y offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_X_DIM; idx++) {
        buf[idx] = (0 <= x + idx && view->hdr.width > x + idx ?
		    view->img[view->hdr.width * y + x + idx] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (y < obj_y || y >= obj_y + img->hdr.height ||
	    x + SCROLL_X_DIM <= obj_x || x >= obj_x + img->hdr.width) {
	    continue;
	}

	/* The y offset of drawing is fixed. */
	yoff = (y - obj_y) * img->hdr.width;

	/* 
	 * The x offsets depend on whether the object starts to the left
	 * or to the right of the starting point for the line being drawn.
	 */
	if (x <= obj_x) {
	    idx = obj_x - x;
	    imgx = 0;
	} else {
	    idx = 0;
	    imgx = x - obj_x;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_X_DIM > idx && img->hdr.width > imgx; idx++, imgx++) {
	    pixel = img->img[yoff + imgx];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * fill_vert_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the top pixel of 
 *                a vertical line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- top pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_vert_buffer (int x, int y, unsigned char buf[SCROLL_Y_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgy;  /* loop index over pixels in object image      */ 
    int            xoff;  /* x offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_Y_DIM; idx++) {
        buf[idx] = (0 <= y + idx && view->hdr.height > y + idx ?
		    view->img[view->hdr.width * (y + idx) + x] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (x < obj_x || x >= obj_x + img->hdr.width ||
	    y + SCROLL_Y_DIM <= obj_y || y >= obj_y + img->hdr.height) {
	    continue;
	}

	/* The x offset of drawing is fixed. */
	xoff = x - obj_x;

	/* 
	 * The y offsets depend on whether the object starts below or 
	 * above the starting point for the line being drawn.
	 */
	if (y <= obj_y) {
	    idx = obj_y - y;
	    imgy = 0;
	} else {
	    idx = 0;
	    imgy = y - obj_y;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_Y_DIM > idx && img->hdr.height > imgy; idx++, imgy++) {
	    pixel = img->img[xoff + img->hdr.width * imgy];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * image_height
 *   DESCRIPTION: Get height of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_height (const image_t* im)
{
    return im->hdr.height;
}


/* 
 * image_width
 *   DESCRIPTION: Get width of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_width (const image_t* im)
{
    return im->hdr.width;
}

/* 
 * photo_height
 *   DESCRIPTION: Get height of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_height (const photo_t* p)
{
    return p->hdr.height;
}


/* 
 * photo_width
 *   DESCRIPTION: Get width of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_width (const photo_t* p)
{
    return p->hdr.width;
}


/* 
 * prep_room
 *   DESCRIPTION: Prepare a new room for display.  You might want to set
 *                up the VGA palette registers according to the color
 *                palette that you chose for this room.
 *   INPUTS: r -- pointer to the new room
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes recorded cur_room for this file
 */
void
prep_room (const room_t* r)
{
    /* Record the current room. */
    cur_room = r;
	photo_t* p = room_photo(r);
    
    int i;
    int offset_p;
    unsigned char red_6;
    unsigned char green_6;
    unsigned char blue_6; 
    for (i=0; i<192; i++) {
        offset_p = 64+i;
        red_6 = (p->palette[i][0] <<1) & bitmask_6;
        green_6 = p->palette[i][1] & bitmask_6;
        blue_6 = (p->palette[i][2] <<1) & bitmask_6;
        
        set_palette(offset_p, red_6, green_6, blue_6);
    }		
}

/* 
 * read_obj_image
 *   DESCRIPTION: Read size and pixel data in 2:2:2 RGB format from a
 *                photo file and create an image structure from it.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the image
 */
image_t*
read_obj_image (const char* fname)
{
    FILE*    in;		/* input file               */
    image_t* img = NULL;	/* image structure          */
    uint16_t x;			/* index over image columns */
    uint16_t y;			/* index over image rows    */
    uint8_t  pixel;		/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the image pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (img = malloc (sizeof (*img))) ||
	NULL != (img->img = NULL) || /* false clause for initialization */
	1 != fread (&img->hdr, sizeof (img->hdr), 1, in) ||
	MAX_OBJECT_WIDTH < img->hdr.width ||
	MAX_OBJECT_HEIGHT < img->hdr.height ||
	NULL == (img->img = malloc 
		 (img->hdr.width * img->hdr.height * sizeof (img->img[0])))) {
	if (NULL != img) {
	    if (NULL != img->img) {
	        free (img->img);
	    }
	    free (img);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = img->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; img->hdr.width > x; x++) {

	    /* 
	     * Try to read one 8-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (img->img);
		free (img);
	        (void)fclose (in);
		return NULL;
	    }

	    /* Store the pixel in the image data. */
	    img->img[img->hdr.width * y + x] = pixel;
	}
    }

    /* All done.  Return success. */
    (void)fclose (in);
    return img;
}


/* 
 * read_photo
 *   DESCRIPTION: Read size and pixel data in 5:6:5 RGB format from a
 *                photo file and create a photo structure from it.
 *                Code provided simply maps to 2:2:2 RGB.  You must
 *                replace this code with palette color selection, and
 *                must map the image pixels into the palette colors that
 *                you have defined.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the photo
 */
photo_t*
read_photo (const char* fname)
{
    FILE*    in;	/* input file               */
    photo_t* p = NULL;	/* photo structure          */
    uint16_t x;		/* index over image columns */
    uint16_t y;		/* index over image rows    */
    uint16_t pixel;	/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the photo pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (p = malloc (sizeof (*p))) ||
	NULL != (p->img = NULL) || /* false clause for initialization */
	1 != fread (&p->hdr, sizeof (p->hdr), 1, in) ||
	MAX_PHOTO_WIDTH < p->hdr.width ||
	MAX_PHOTO_HEIGHT < p->hdr.height ||
	NULL == (p->img = malloc 
		 (p->hdr.width * p->hdr.height * sizeof (p->img[0])))) {
	if (NULL != p) {
	    if (NULL != p->img) {
	        free (p->img);
	    }
	    free (p);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    ////////////////////////////////////////////////////////
    /*initialize level_2 and level_4 array*/
    int i;
    int j;
    for (i = 0; i < level_4_size; i++) {
    	level_4[i].red = 0;
    	level_4[i].green = 0;
    	level_4[i].blue = 0;
    	level_4[i].count = 0;
    	level_4[i].node_num = i;
    }
    for (j = 0; j < level_2_size; j++) {
    	level_2[j].red = 0;
    	level_2[j].green = 0;
    	level_2[j].blue = 0;
    	level_2[j].count = 0;
    	level_2[j].node_num = j;
    }
    ////////////////////////////////////////////////////
    /*initialize the color palette array. set all to zero*/
    /*
    for (i = 0; i < palette_size; i++) {
    	for (j = 0; j < palette_dim; j++) {
    		p -> palette[i][j] = 0;
    	}
    }
    */

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = p->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; p->hdr.width > x; x++) {

	    /* 
	     * Try to read one 16-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (p->img);
		free (p);
	        (void)fclose (in);
		return NULL;

	    }
	    /* 
	     * 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
	     * and 6 bits blue).  We change to 2:2:2, which we've set for the
	     * game objects.  You need to use the other 192 palette colors
	     * to specialize the appearance of each photo.
	     *
	     * In this code, you need to calculate the p->palette values,
	     * which encode 6-bit RGB as arrays of three uint8_t's.  When
	     * the game puts up a photo, you should then change the palette 
	     * to match the colors needed for that photo.
	     */
	    /*
	    p->img[p->hdr.width * y + x] = (((pixel >> 14) << 4) |
					    (((pixel >> 9) & 0x3) << 2) |
					    ((pixel >> 3) & 0x3));
		*/
		//////////////////////////////////////////////////////////////////
		/* mapping pixels into the 4th level and 3nd level of the octree arrays*/
		map_to_octree(pixel);

	}
    }
    /*initialize the color palette array. set all to zero*/
    
    for (i = 0; i < palette_size; i++) {
    	for (j = 0; j < palette_dim; j++) {
    		p -> palette[i][j] = 0;
    	}
    }
    
    ///////////////////////////////////////////
    fseek(in, 0, SEEK_SET);

    //////////////////////////////////////////
	/*map the average color into the palette array*/
	qsort(level_4, level_4_size,sizeof(octree), qsort_comp);
    /*
    	 64--128--64
    	 reserve 64 for the given color. 128 level 4th first,
    	 find the same color owned by both fourth and second.
	*/
    
   	for (i = 0; i < level_4_node; i++) {
   		if (level_4[i].count != 0) {	

            p->palette[i+64][0] = level_4[i].red / level_4[i].count; // ave level 4 red
            p->palette[i+64][1] = level_4[i].green / level_4[i].count; // ave level 4 green
            p->palette[i+64][2] = level_4[i].blue / level_4[i].count;	// ave level 4 blue
   		// use to check for second level
   		    int red_same = ((level_4[i].node_num >> 10) & bitmask) << 4; //find the same value in level 4 and 2
   		    int green_same = ((level_4[i].node_num >> 6) & bitmask) << 2; 
   		    int blue_same = (level_4[i].node_num >> 2) & bitmask;

   		    int index = 0;
  		    index = red_same | green_same | blue_same; // ccalculate the index

  		    level_2[index].red -= level_4[i].red; // delete the same color as level 4 from level 2
  		    level_2[index].green -= level_4[i].green; 
  		    level_2[index].blue -= level_4[i].blue;
  		    level_2[index].count -= level_4[i].count;
   		}
   	}

   	for (j = 0; j < level_2_node; j++) { // ave level 2 color
   		if (level_2[j].count != 0) {
   			p->palette[j][0] = level_2[j].red / level_2[j].count;
   			p->palette[j][1] = level_2[j].green / level_2[j].count;
   			p->palette[j][2] = level_2[j].blue / level_2[j].count;
   		}
   	}

   	for (y = p->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; p->hdr.width > x; x++) {

	    /* 
	     * Try to read one 16-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (p->img);
		free (p);
	        (void)fclose (in);
		return NULL;


	    }

	    if (p->hdr.width*y+x-2 < 0) continue; //////////////////////////////////////////
	    p->img[p->hdr.width*y+x-2] = vga_conventer(pixel);  // convert to vga

	}
	}

//////////////////////////////////////////////////////////////
    /* All done.  Return success. */
    (void)fclose (in);
    return p;
}


/* 
 * mapo_to_ctree
 *   DESCRIPTION: map the pixel into the fourth and second level array 
 *   INPUTS: pixel
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: map the pixel into the second and fourth array
 */

void map_to_octree(unsigned short pixel)
{
	int index_4 = 0;
	int index_2 = 0;

	int red_bit = (pixel>>11) & red_bitmask; // get the 5 bits for red
	int green_bit = (pixel>>5) & green_bitmask; // 6 bits for green
	int blue_bit = pixel & blue_bitmask; // 5 bits for blue

	// 4th level. each color 4 bits, index = 12 bits
	int red_4 = (red_bit>>1) << 8; // use bitmask to get 4 bits for colors
	int green_4 = (green_bit>>2) << 4;
	int blue_4 = blue_bit >> 1;
	//RRRRGGGGBBBB
	index_4 = red_4 | green_4 | blue_4;

	level_4[index_4].red += red_bit; //add all the same rrrrggggbbbb value color into the array
	level_4[index_4].green += green_bit;
	level_4[index_4].blue += blue_bit;
	level_4[index_4].count++; // increment frequncy

	// 2nd level. each color 2 bits, index = 12 bits
	int red_2 = (red_bit>>3) << 4; // use bitmask to get 2 bits for colors
	int green_2 = (green_bit>>4) << 2;
	int blue_2 = blue_bit >> 3;
	//RRGGBB
	index_2 = red_2 | green_2 | blue_2;

	level_2[index_2].red += red_bit; // add all same rrggbb value color into the array
	level_2[index_2].green += green_bit;
	level_2[index_2].blue += blue_bit;
	level_2[index_2].count++; //imcrement frequency
}

int qsort_comp(const void *a, const void *b) 
{
    // helper function for qsort, comparsion between two value
    octree *ia = (octree *)a;
	octree *ib = (octree *)b;
    if ((ia->count) > (ib->count)){
		return -1;
	}
    else if((ia->count) < (ib->count)){
		return 1;
	}
	
	return 0;
    //return (ia->count < ib->count);
}
/* 
 * vga_converter
 *   DESCRIPTION: search the pixels on fourth and second level array and return the corresponding VGA index
 *   INPUTS: pinxel
 *   OUTPUTS: none
 *   RETURN VALUE: vga index
 *   SIDE EFFECTS: 
 */
uint8_t vga_conventer(unsigned short pixel)
{
	int vga_offset_4 = 0;
	int vga_offset_2 = 0;

	int red_bit = (pixel>>11) & red_bitmask; // get 5 bits for red
	int green_bit = (pixel>>5) & green_bitmask; // 6 bits for green
	int blue_bit = pixel & blue_bitmask; // 5 bits for blue

    //bitmask and shifting to get rrrrggggbbbb
	int red_4 = (red_bit>>1) << 8; 
	int green_4 = (green_bit>>2) << 4;
	int blue_4 = blue_bit >> 1;
    //add all together
	vga_offset_4 = red_4 | green_4 | blue_4;
    //bitmask and shifting to get rrggbb
	int red_2 = (red_bit>>3) << 4;
	int green_2 = (green_bit>>4) << 2;
	int blue_2 = blue_bit >> 3;
    //add all together
	vga_offset_2 = red_2 | green_2 | blue_2;

    int i;
    uint8_t vga_offset;
    // for fourth level, convert to vga
	for (i = 0; i < 128; i++) {
		if (level_4[i].node_num == vga_offset_4) {
            vga_offset = i + level_4_node;
			return vga_offset;
		}
	}
    // for the second level, conver to vga
    vga_offset = vga_offset_2 + level_2_node;
	return vga_offset;
 	
}


