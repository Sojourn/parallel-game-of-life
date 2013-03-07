#ifndef LIFE_UTIL_H_
#define LIFE_UTIL_H_
#include <stdlib.h>
#include <stdio.h>

/* An enum which represents the state of a cell. */
/*typedef enum
{
	dead_state = 0,
	alive_state = 1
} life_state_e; */

typedef char life_state;
#define ALIVE_STATE ((life_state) 1)
#define DEAD_STATE ((life_state) 0)

/* A 2-dimensional array of cells. */
typedef struct
{
	size_t size;          /* The length of one dimension of cells.  */
	life_state **cells; /* A square, 2-dimensional array of size (size * size)  */
} grid_t;

/* Initialize a grid of the given size. Returns -1 on failure. */
int grid_init(grid_t *grid, size_t size);

/* Deinitialize a grid.  */
void grid_deinit(grid_t *grid);

/* Evaluate the next game of life state of src[y][x].
 * The result is recorded in dst[y][x]. 
*/
void grid_eval(grid_t *dst, grid_t *src, size_t x, size_t y);

/* Read a grid from the file. If file is NULL, read it from stdin.  
 * Return 0 on success, or -1 if errors were encountered. The error
 * message will be printed to stderr.
*/
int grid_read(const char *file, grid_t *grid);

/* Write the grid to the file. If file is NULL, it is written to stdout.
 * Return 0 on success, or -1 if errors were encountered. The error
 * message will be printed to stderr.
*/
int grid_write(const char *file, grid_t *grid);

#endif /* LIFE_UTIL_H_ */
