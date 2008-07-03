/*
	VertexColoration
	Andrew Coleman
	Licensed under the GPL version 2 or later.
	Full license details are available at http://gnu.org/
	CSC-2400

	This program is designed to create or read in a graph
	from a user and find a way to identify each vertex
	without touching another vertex of the same indentifier
	(color) and using as few colors as possible.
*/

#define USE_TI92PLUS
#define OPTIMIZE_ROM_CALLS
#define MIN_AMS 208
#define SAVE_SCREEN

#include <tigcclib.h>

#define MAX_VERTEX 100
#define SLIDER_WIDTH 125
#define START_COL 2
#define START_ROW 10

int (*neighbor)[MAX_VERTEX];
FILE *output;
void RandomizeGraph ( int );
void UserDefineGraph ( int );
void DrawGraph ( int );
void Greedy ( int );
void BackTrack ( int );
/* algorithmstr, numvertexes, numsteps, numcolors, colorarray */
void ColorationResults ( const char *, int, unsigned long, int, int * );

void _main(void)
{
	randomize();
	int numvert = 0;
    randomize();
	
	output = NULL;
	
	char *params = calloc ( 11, sizeof( char ) );
	short *opt = calloc ( 2, sizeof( short ) );
	opt[1] = 1;
	/* TIOS specific windowing */
	HANDLE sizewin = DialogNewSimple ( 120, 70 );
	HANDLE sourcepopup = PopupNew ( NULL, 0 );
	PopupAddText ( sourcepopup, -1, "Random", 0 );
	PopupAddText ( sourcepopup, -1, "User", 0 );
	DialogAddTitle ( sizewin, "Graph Source", BT_OK, BT_NONE );
	DialogAddRequest ( sizewin, 2, 16, "Size: ", 0, 3, 4 );
	DialogAddPulldown ( sizewin, 2, 29, "Source: ", sourcepopup, 1 );
	DialogAddRequest ( sizewin, 2, 42, "Output: ", 3, 8, 9 );
	
	do	{
		if ( DialogDo ( sizewin, CENTER, CENTER, params, opt ) == KEY_ESC )
		{
			HeapFree ( sizewin );
			HeapFree ( sourcepopup );
			free ( params );
			free ( opt );
			return;
		}
		/* convert the number of strings and filename to something i can use */
		char *tmp = calloc ( 8, sizeof( char ) );
		memcpy ( tmp, params, 3 * sizeof( char ) );
		numvert = atoi ( tmp );
		memcpy ( tmp, params + 3, 8 * sizeof( char ) );
		if ( strcmp ( tmp, "" ) != 0 )
			output = fopen ( tmp, "w" );	
		free ( tmp );
	} while ( numvert <= 0 && numvert >= MAX_VERTEX );
	HeapFree ( sizewin );
	HeapFree ( sourcepopup );
	free ( params );
	
	/* initialize the adjacency matrix */
	neighbor = calloc ( numvert, sizeof( *neighbor ) );

	if ( opt[1] == 1 )
		RandomizeGraph ( numvert );
	else
		UserDefineGraph ( numvert );
	free ( opt );
	
	/* display the neighborlist, size permitting */
	DrawGraph ( numvert );
	/* perform a greedy coloration */
	Greedy ( numvert );
	/* perform a backtrack coloration */
	BackTrack ( numvert );

	if ( output != NULL )
		fclose ( output );
	free ( neighbor );
}

/*
		This is the random graph algorithm handed out in class.
*/
void RandomizeGraph ( int numvert )
{
	ClrScr();
	int row = 0, col = 0;
	DrawStr ( 1, 1, "Creating neighbors...", A_NORMAL );
	/* For the slider */
	DrawLine ( START_COL, START_ROW, START_COL + SLIDER_WIDTH, START_ROW, A_NORMAL );
	DrawLine ( START_COL, START_ROW + 2, START_COL + SLIDER_WIDTH, START_ROW + 2, A_NORMAL );
	DrawPix ( START_COL, START_ROW + 1, A_NORMAL );
	DrawPix ( START_COL + SLIDER_WIDTH, START_ROW + 1, A_NORMAL );
	int lastsize = 1;
	for ( row = 0; row < numvert - 2; row++ )
	{
		/* again for the slider */
		for ( col = lastsize; col < ceil ( row * SLIDER_WIDTH / numvert ); col++ )
			DrawPix ( col + START_COL, START_ROW + 1, A_NORMAL );
		lastsize = ceil ( row * SLIDER_WIDTH / numvert );
		
		/* randomly pick edges for each vertex after this one to eliminate duplicates */
		for ( col = row + 1; col < numvert; col++ )
		{
			int tmp = random ( numvert ) + 1;
			if ( tmp > numvert / 2 )
			{
				neighbor[row][col] = 1;
				neighbor[col][row] = 1;
			}
		}
	}
	
	/* finish drawing the slider */
	for ( col = lastsize; col < SLIDER_WIDTH; col++ )
		DrawPix ( col + START_COL, START_ROW + 1, A_NORMAL );
}

/*
	This function will query the user for each vertex's neighbors
*/
void UserDefineGraph ( int numvert )
{
	char *letters = calloc ( MAX_VERTEX * 2, sizeof( int ) );
	HANDLE neighbors = DialogNewSimple ( 145, 70 );
	DialogAddTitle ( neighbors, "Neighbors", BT_OK, BT_NONE );
	
	int row = 0;
	char title[23];
	for ( row = 0; row < numvert; row++ )
	{
		/* repeat the same dialog pattern for each vertex */
		memset ( title, 0, 23 * sizeof( char ) );
		memset ( letters, 0, MAX_VERTEX * 2 * sizeof( char ) );
		sprintf ( title, "Enter neighbors for %d", row + 1 );
		DialogAddText ( neighbors, 2, 16, title );
		DialogAddText ( neighbors, 2, 29, "separated by commas" );
		DialogAddRequest ( neighbors, 2, 42, "", 0, MAX_VERTEX * 2, 20 );
		int error = 0;
		if ( DialogDo ( neighbors, CENTER, CENTER, letters, NULL ) == KEY_ESC )
				error = 1;
		if ( !error )
		{
			/* convert the string to neighbors */
			char *num = strtok ( letters, "," );
			while ( num != NULL )
			{
				int neighbornum = atoi ( num );
				/* if neighbornum is out of this range, there is nothing we can do with it */
				if ( neighbornum > 0 && neighbornum <= MAX_VERTEX && neighbornum != row )
				{
					/* otherwise, put in a neighbor */
					neighbor[row][neighbornum - 1] = 1;
					neighbor[neighbornum - 1][row] = 1;
				}
				num = strtok ( NULL, "," );
			}
		}
		/* the user pressed esc, so they want to leave */
		else
			break;
	}
	free ( letters );
	HeapFree ( neighbors );
}

/*
	This function will display the neighbor list so long as there are < 15 vertecies (screen is too small...)
*/
void DrawGraph ( int numvert )
{
	ClrScr();

	int row = 0, col = 0;
	char *tmp = calloc ( 5, sizeof ( char ) );
	if ( numvert < 15 )
		DrawStr ( 1, 1, "Neighbor listing...", A_NORMAL );
	if ( output != NULL )
		fputs ( "Neighbor listing...\n", output );
	
	/* go over each vertex, yes, we can go over this even if we have > 15 vertecies and we aren't printing to a file */
	for ( row = 0; row < numvert; row++ )
	{
		sprintf ( tmp, "%d: ", row + 1 );
		if ( numvert < 15 )
			DrawStr ( 1, (row + 1) * 8 + 1, tmp, A_NORMAL );
		if ( output != NULL )
			fputs ( tmp, output );
			
		/* find all neighbors of current vertex */
		for ( col = 0; col < numvert; col++ )
		{
			/* if we have a neighbor, and the random/user graph functions make sure we don't have (row == col) being a neighbor */
			if ( neighbor[row][col] == 1 )
			{
				sprintf ( tmp, " %d", col + 1 );
				if ( numvert < 15 )
					DrawStr ( col * 14 + 20, (row + 1) * 8 + 2, tmp, A_NORMAL );
				if ( output != NULL )
					fputs ( tmp, output );
			}
		}
		if ( output != NULL )
			fputs ( "\n", output );
	}
	if ( output != NULL )
		fputc ( '\n', output );
	free ( tmp );

	/* if we displayed something, wait for the user to get done before continuing */
	if ( numvert < 15 )
	{
		GKeyFlush();
		ngetchx();
	}
}

/*
	This function will color the graph in a greedy fashion
*/
void Greedy ( int numvert )
{
	ClrScr();
	int maxcolor = 1, row = 0, col = 0, curcolor = 0;
	unsigned long steps = 0;
	/* array of colors for each vertex */
	int *colors = calloc ( numvert, sizeof( int ) );
	/* display what we are doing and the box for the slider bar */
	DrawStr ( 1, 1, "Coloring by greed...", A_NORMAL );
	DrawLine ( START_COL, START_ROW, START_COL + SLIDER_WIDTH, START_ROW, A_NORMAL );
	DrawLine ( START_COL, START_ROW + 2, START_COL + SLIDER_WIDTH, START_ROW + 2, A_NORMAL );
	DrawPix ( START_COL, START_ROW + 1, A_NORMAL );
	DrawPix ( START_COL + SLIDER_WIDTH, START_ROW + 1, A_NORMAL );
	
	/* used to keep the last position for the slider */
	int lastsize = 1;
	/* we want to color each vertex */
	for ( row = 0; row < numvert; row++ )
	{
		/* flag to know if a neighbor of the same current color was found */
		int found = 0;
        
		/* update the slider bar for each vertex */
		for ( col = lastsize; col < ceil ( row * SLIDER_WIDTH / numvert ); col++ )
			DrawPix ( col + START_COL, START_ROW + 1, A_NORMAL );
        lastsize = ceil ( row * SLIDER_WIDTH / numvert );
        
        /* we want to search all colors from 1 to maxcolor + 1 */
        for ( curcolor = 1; curcolor <= maxcolor + 1; curcolor++ )
        {
            /* we already have a color selected, move on */
            if ( colors[row] != 0 )
                continue;
            found = 0;
            /* now we search every other vertex for a neighbor */
            for ( col = 1; col <= numvert; col++ )
            {
                /* don't keep looking if we have already found a color */
                if ( found )
                    break;
                /* do we have a neighbor  of the same color */
                if ( neighbor[row][col - 1] == 1 && colors[col - 1] == curcolor )
                    found = 1;
                /* that's another step towards the solution */
                steps++;
            }
            
            /* if no neighbor of this color was found and no color has been assigned, use curcolor */
            if ( !found && colors[row] == 0 )
                colors[row] = curcolor;
            
            /* if we have gone over our maximum color limit, increase it */
            if ( colors[row] > maxcolor )
                maxcolor = colors[row];
        }
	}

	/* finish drawing the slider */
    for ( col = lastsize; col < SLIDER_WIDTH; col++ )
		DrawPix ( col + START_COL, START_ROW + 1, A_NORMAL );
	
	/* display the results */
	ColorationResults ( "Greed Colors", numvert, steps, maxcolor, colors );
	free ( colors );
}

void BackTrack ( int numvert )
{
	ClrScr();
	/* if we output partial listings, we can break the 64KB file limit on the calculator, plus file writes are *slow* */
	int partiallistings = 0;
	if ( output != NULL && DlgMessage ( "Question", "Output partial listings of BackTrack?", BT_YES, BT_NO ) == KEY_ENTER )
		partiallistings = 1;
	int maxcolor = 1, curcolor = 1, row = 0, col = 0;
	unsigned long steps = 0;
	int *colors = calloc ( numvert, sizeof( int ) );
	char *tmp = calloc ( 20, sizeof( char ) );
	/* let the user know what's going on */
	DrawStr ( 1, 1, "BackTrack Color", A_NORMAL );
	ST_helpMsg ( "Hit [ESC] to quit..." );
	sprintf ( tmp, "MaxColor: %d", maxcolor );
	DrawStr ( 1, 9, tmp, A_NORMAL );
	
	/* clear the keyboard queue to make sure any stray kepresses don't bother the results */
	GKeyFlush();
	
	/* flag to find a neighbor of same color, if we are backtracking in the for loop, and if we have an error */
	int found = 0, back = 0, error = 0;
	if ( output != NULL && partiallistings )
		fputs ( "\nBacktrack Partial Colorings:\n", output );
	for ( row = 0; row < numvert; row++ )
	{
		/* if we are backtracking, start coloring above that vertex's current color */
		if ( back )
		{
			curcolor = colors[row] + 1;
			back = 0;
		}
		/* if we are not backtracking, and we do want to output partial listings, print */
		else if ( output != NULL && partiallistings )
		{
			int i = 0, zero = 0;
			for ( i = 0; i < numvert; i++ )
			{
				char outputltr[4];
				if ( colors[i] != 0 )
				{
					zero = 1;
					sprintf ( outputltr, "%d ", colors[i] );
					fputs ( outputltr, output );
				}
			}
			if ( zero )
				fputs ( "\n", output );
		}
		
		found = 0;
		
		/* if current color is bigger than maxcolor, we need to stop and back up */
		if ( curcolor > maxcolor )
		{
			/* if we are at the first vertex, then we can increase the current maximum color limit */
			if ( row == 0 )
			{
				/* increase maximum colors */
				maxcolor++;
				sprintf ( tmp, "MaxColor: %d   ", maxcolor );
				DrawStr ( 1, 9, tmp, A_REPLACE );
				/* start coloring again */
				curcolor = 1;
				/* if maxcolor is greater than the number of vertecies, then we cannot use more than 1 color per vertex */
				if ( maxcolor > numvert )
				{
					ClrScr();
					DrawStr ( 1, 1, "Cannot be solved by\n tracking backwards.", A_NORMAL );
					if ( output != NULL )
						fputs ( "Cannot be solved by tracking backwards.\n", output );
					error = 1;
					break;
				}
			}
			/* since we aren't at the first vertex, reset the color, since it was wrong anyways, and backtrack */
			else
			{
				colors[row] = 0;
				row = row - 2;
				back = 1;
				continue;
			}
		}
		
		/* since we aren't backtracking, and we don't need to backtrack, do some work */
		for ( col = 0; col < numvert; col++ )
		{
			/* don't keep looking if we already have a neighbor of same color */
			if ( found )
				continue;
			/* if we have a neighbor of same color */
			if ( neighbor[row][col] == 1 && colors[col] == curcolor )
				found = 1;
			steps++;
		}
		
		/* if a color is not found, increase color and try the same vertex */
		if ( found )
		{
			curcolor++;
			row--;
			continue;
		}
		/* found a color */
		else
		{
			colors[row] = curcolor;
		}
		
		/* backtrack takes a long time, so if the user wants to quit prematurely, just hit esc */
		if ( kbhit() )
		{
			if ( ngetchx() == KEY_ESC )
			{
				ClrScr();
				error = 1;
				break;
			}
		}
	}
	if ( output != NULL && partiallistings )
		fputs ( "\n", output );
	
	/* if the user didn't press esc, display the results */
	if ( !error )
	{
		ColorationResults ( "BackTrack Colors", numvert, steps, maxcolor, colors );
	}
	
	free ( colors );
	free ( tmp );
}

/*
	this function will display the results for each of the algorithms
*/
void ColorationResults ( const char *algorithm, int numvert, unsigned long steps, int maxcolor, int *colors )
{
	/* the number of vertecies to put on one row */
	#define ROW_WIDTH 6
	ClrScr();
	char *tmp = calloc ( 20, sizeof( char ) );
	int row = 0, col = 0;
	
	/* clear the keyboard queue to make sure we don't get a stray keypress */
	GKeyFlush();
	
	DrawStr ( 1, 1, algorithm, A_NORMAL );
	/* print to a file if we need to */
	if ( output != NULL )
	{
		fputs ( algorithm, output );
		fputs ( "\n", output );
	}
	/* print each vertex */
	for ( row = 0; row <= numvert / ROW_WIDTH; row++ )
	{
		/* find each vertex's neighbors */
		for ( col = 1; col <= ROW_WIDTH; col++ )
		{
			/* make sure we don't try to display a vertex that doesn't exist */
			if ( row * ROW_WIDTH + col <= numvert )
			{
				int vert = row * ROW_WIDTH + col;
				sprintf ( tmp, "%d: %d", vert, colors[vert - 1] );
				DrawStr ( ( col - 1 ) * 40, ( row + 2 ) * 8, tmp, A_REPLACE );
				if ( output != NULL )
				{
					fputs ( tmp, output );
					fputs ( "\n", output );
				}
			}
		}
	}
	/* display and print out colors used */
	sprintf ( tmp, "Colors used: %d", maxcolor );
	DrawStr ( 1, ceil ( numvert / ROW_WIDTH + 4 ) * 8, tmp, A_NORMAL );
	if ( output != NULL )
	{
		fputs ( tmp, output );
		fputs ( "\n", output );
	}
	/* display and print out steps taken */
	sprintf ( tmp, "Steps taken: %lu", steps );
	DrawStr ( 1, ceil ( numvert / ROW_WIDTH + 5 ) * 8, tmp, A_NORMAL );
	if ( output != NULL )
	{
		fputs ( tmp, output );
		fputs ( "\n", output );
		fputs ( "\n", output );
	}
	/* let the user press a key when done */
	ngetchx();
	free ( tmp );
	#undef ROW_WIDTH
}
