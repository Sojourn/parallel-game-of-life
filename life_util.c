#include "life_util.h"

int grid_init(grid_t *grid, size_t size)
{
	size_t x;
	size_t y;
	life_state *cells;
	grid->size = size;

	grid->cells = (life_state **) malloc(sizeof(life_state *) * size);
	if(grid->cells == NULL)
	{
		fprintf(stderr, "Insufficient memory\n");
		return -1;
	}

	cells = (life_state *) malloc(sizeof(life_state) * size * size);
	if(cells == NULL)
	{
		fprintf(stderr, "Insufficient memory\n");
		return -1;
	}

	for(y = 0; y < size; y++)
	{
		grid->cells[y] = &cells[y * size];

		for(x = 0; x < size; x++)
		{
			grid->cells[y][x] = DEAD_STATE;
		}
	}

	return 0;
}

void grid_deinit(grid_t *grid)
{
	free(grid->cells[0]);
	free(grid->cells);
	
	grid->cells = NULL;
	grid->size = 0;
}

/* Returns 1 if the cell is alive, and 0 if dead.
 * Cells outside of the grid are treated as dead.
*/
size_t isAlive(grid_t *grid, size_t x, size_t y)
{
	if(x >= grid->size || x < 0) return 0;
	if(y >= grid->size || y < 0) return 0;
	
	return (grid->cells[y][x] == ALIVE_STATE);
}

void grid_eval(grid_t *dst, grid_t *src, size_t x, size_t y)
{
	size_t alive = 0;
	
	/* Sum the neighbors which are alive  */
	alive += isAlive(src, x    , y - 1); /* N  */
	alive += isAlive(src, x + 1, y - 1); /* NE  */
	alive += isAlive(src, x + 1, y    ); /* E  */
	alive += isAlive(src, x + 1, y + 1); /* SE  */
	alive += isAlive(src, x    , y + 1); /* S  */
	alive += isAlive(src, x - 1, y + 1); /* SW  */
	alive += isAlive(src, x - 1, y    ); /* W  */
	alive += isAlive(src, x - 1, y - 1); /* NW  */

	/* Cell is dead */
	if(src->cells[y][x] == DEAD_STATE)
	{
		/* Spawned from neighbors  */
		if(alive == 3)
		{
			dst->cells[y][x] = ALIVE_STATE;
		}

		/* No change  */
		else
		{
			dst->cells[y][x] = DEAD_STATE;
		}
	}

	/* Cell is alive */
	else
	{
		/* Over-crowding death */
		if(alive >= 4)
		{
			dst->cells[y][x] = DEAD_STATE;
		}
		
		/* Loneliness death */
		else if(alive <= 1)
		{
			dst->cells[y][x] = DEAD_STATE;
		}

		/* No change  */
		else
		{
			dst->cells[y][x] = ALIVE_STATE;
		}
	}
}

int grid_read(const char *file, grid_t *grid)
{
	size_t x;
	size_t y;
	FILE *in;
	char c;
	char error = 0;

	/* Open file for reading  */
	if(file != NULL)
	{
		in = fopen(file, "r");
		if(in == NULL)
		{
			fprintf(stderr, "Unable to open %s for reading\n", file);
			return -1;
		}
	}
	else
	{
		in = stdin;
	}

	/* Parse the grid from the file  */
	for(y = 0; (y < grid->size) && !error; y++)
	{
		for(x = 0; x < grid->size; x++)
		{
			
			/* Parse the cell state */
			c = fgetc(in);
			if((c != '0') && (c != '1'))
			{
				error = 1;
				fprintf(stderr, "Unexpected character: %c  at (%d, %d)\n", c, 2 * x, y);
				break;
			}
			else
			{
				grid->cells[y][x] = c - '0';
			}

			/* Parse the whitespace or newline */
			c = fgetc(in);
			if((c != ' ') && (c != '\n') && (c != '\r'))
			{
				error = 1;
				fprintf(stderr, "Unexpected delimiter: %c at (%d, %d)\n", c, 2 * x + 1, y);
				break;
			}
		}
	}

	if(file != NULL) 
	{
		fclose(in);
	}
	
	if(error)
	{
		return -1;
	}
	 
	return 0;
}

int grid_write(const char *file, grid_t *grid)
{
	size_t x;
	size_t y;
	size_t result;
	FILE *out = NULL;

	/* Open the file for writing  */
	if(file != NULL)
	{
		out = fopen(file, "w");
		if(out == NULL)
		{
			fprintf(stderr, "Unable to open %s for writing\n", file);
			return -1;
		}
	}
	else
	{
		out = stdout;
	}

	/* Write the grid to the file  */
	for(y = 0; y < grid->size; y++)
	{
		for(x = 0; x < grid->size; x++)
		{

			/* We print out a space if it is not the last cell in a row  */
			if(x != grid->size - 1) 
				result = fprintf(out, "%d ", grid->cells[y][x]);
			else 
				result = fprintf(out, "%d", grid->cells[y][x]);
			if(result <= 0)
			{
				fprintf(stderr, "Write operation failed\n");
				return -1;
			}
		}
		
		result = fprintf(out, "\n");
		if(result <= 0)
		{
			fprintf(stderr, "Write operation failed\n");
			return -1;
		}
	}

	if(file != NULL)
	{
		fclose(out);
	}

	return 0;
}

