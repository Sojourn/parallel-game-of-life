#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "life_util.h"

int main(int argc, char **argv)
{
	grid_t grids[2];
	size_t size, x, y;

	if(argc < 4)
	{
		fprintf(stderr, "Too few arguments\n");
		fprintf(stderr, "Usage: %s <grid-size> <file-path-2> <file-path-1>\n", argv[0]);
		return -1;
	}

	size = atoi(argv[1]);
	if(size < 1)
	{
		fprintf(stderr, "<grid-size> must be larger than 0\n");
		return -1;
	}

	if(grid_init(&grids[0], size) == -1)
	{
		fprintf(stderr, "Insufficient memory\n");
		return -1;
	}
	if(grid_init(&grids[1], size) == -1)
	{
		fprintf(stderr, "Insufficient memory\n");
		grid_deinit(&grids[0]);
		return -1;
	}

	if(grid_read(argv[2], &grids[0]) == -1)
	{
		fprintf(stderr, "Unable to read <%s>\n", argv[2]);
		grid_deinit(&grids[0]);
		grid_deinit(&grids[1]);
		return -1;
	}
	if(grid_read(argv[3], &grids[1]) == -1)
	{
		fprintf(stderr, "Unable to read <%s>\n", argv[3]);
		grid_deinit(&grids[0]);
		grid_deinit(&grids[1]);
		return -1;
	}

	for(y = 0; y < size; y++)
	{
		for(x = 0; x < size; x++)
		{
			if(grids[0].cells[y][x] != grids[1].cells[y][x])
			{
				fprintf(stderr, "Difference detected at x=%d, y=%d\n", x, y);
				grid_deinit(&grids[0]);
				grid_deinit(&grids[1]);
				return -1;
			}
		}
	}

	grid_deinit(&grids[0]);
	grid_deinit(&grids[1]);

	return 0;
}
