// Fényes Balázs (fenyesb.github.io)
// f.balazs96@gmail.com

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <wchar.h>

//példaállományok forrása:
//https://github.com/noolsjs/sudoku-example/blob/master/src/data.js
//https://puzzling.stackexchange.com/q/305
char* fname;

#define max(a,b) ((a)>(b)?(a):(b))

#define EMPTY_CHAR '_'
#define PRINT_FREQUENCY 10000

//https://stackoverflow.com/a/3586005
#define KNRM  L"\x1B[0m"
#define KRED  L"\x1B[31m"
#define KGRN  L"\x1B[32m"
#define KYEL  L"\x1B[33m"
#define KBLU  L"\x1B[34m"
#define KMAG  L"\x1B[35m"
#define KCYN  L"\x1B[36m"
#define KWHT  L"\x1B[37m"

//https://stackoverflow.com/a/26423946
#define gotoxy(x,y) wprintf(L"\033[%d;%dH", (x), (y))
#define clear() wprintf(L"\033[H\033[J")

/* Ha elõre tudható a döntések száma */
#define MAX_deepness (9*9)

typedef enum {
	EMPTY, GIVEN, GUESS
} CELL_STATE;

typedef struct{
    int copy_of_total_choice_count;
    int copy_of_current_choice_id;
    int table[9][9];
    CELL_STATE cell_state[9][9];
} model;

model m;
model* models;

typedef enum {
	END, CHECK, BACKTRACK, CHOOSE
} PROGRAM_STATE;

PROGRAM_STATE state; /* Vezérlo változó */

int previous_choice_count;	/* Korábbi döntések száma */
int total_choice_count;	/* Választási lehetoségek száma */
int current_choice_id;	/* Aktuális lehetoség sorszáma */

int iter = 0; //próbálkozások száma

void empty_list(int* arr)
{
	for(int i = 0; i < 9; i++)
		arr[i] = 0; 
}

int list_sum(int* arr)
{
	int s = 0;
	for(int i = 0; i < 9; i++)
		s += arr[i];
	return s;
}

int list_max(int* arr)
{
	int m = 0;
	for(int i = 0; i < 9; i++)
		m = max(m, arr[i]);
	return m;
}

void table_print()
{	
	gotoxy(0,0);
	if(iter == 0)
	{	
    	clear();
    	wprintf(L" press any key to begin...\n" KRED " %s" KNRM "\n", fname);
	}
	else
	{
		wprintf(L"\n" KRED " %s %d" KNRM "\n", fname, iter);
	}
	wprintf(L" ┏━━━━━━━━━┯━━━━━━━━━┯━━━━━━━━━┓\n");
	for(int y = 0; y < 9; y++)
	{
		if(y == 3 || y == 6)
		{
			wprintf(L" ┃─────────┼─────────┼─────────┃\n");
		}
		wprintf(L" ┃");
		for(int x = 0; x < 9; x++)
		{
			if(x == 3 || x == 6)
			{
				wprintf(L"│");
			}
			switch(m.cell_state[x][y])
			{
				case EMPTY:
					wprintf(L" ░ ");
					break;
				case GIVEN:
					wprintf(KBLU " %d " KNRM, m.table[x][y]);
					break;
				case GUESS:
					wprintf(KGRN " %d " KNRM, m.table[x][y]);
					break;
			}
		}
		wprintf(L"┃\n");
	}
	wprintf(L" ┗━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┛\n");
}

bool is_goal_reached(){/* A célt elértük? */
	
	// ki van töltve az egész táblázat?
	for(int x = 0; x < 9; x++)
	{
		for(int y = 0; y < 9; y++)
		{
			if(m.cell_state[x][y] == EMPTY)
			{
				return false;
			}
		}
	}
	
	// az oszlopok megfelelőek?
	for(int x = 0; x < 9; x++)
	{
		int l[9];
		empty_list(l);
		for(int y = 0; y < 9; y++)
		{
			l[m.table[x][y] - 1] = 1;
		}
		if(list_sum(l) != 9)
		{
			return false;
		}
	}
	
	// a sorok megfelelőek?
	for(int y = 0; y < 9; y++)
	{
		int l[9];
		empty_list(l);
		for(int x = 0; x < 9; x++)
		{
			l[m.table[x][y] - 1] = 1;
		}
		if(list_sum(l) != 9)
		{	
			return false;
		}
	}
	
	//a 3×3-as négyzetek megfelelőek?
	for(int gx = 0; gx < 3; gx++)
	{
		for(int gy = 0; gy < 3; gy++)
		{
			int l[9];
			empty_list(l);
			for(int x = gx * 3; x < gx * 3 + 3; x++)
			{
				for(int y = gy * 3; y < gy * 3 + 3; y++)
				{
					l[m.table[x][y] - 1]++;
				}
			}
			if(list_sum(l) != 9)
			{
				return false;
			}
		}
	}
	
	return true;
}

int is_goal_reachable(){/* Egyáltalán lehetséges még a célt elérni? */
	
	// egy oszlopban minden szám legfeljebb egyszer szerepelhet
	for(int x = 0; x < 9; x++)
	{
		int l[9];
		empty_list(l);
		for(int y = 0; y < 9; y++)
		{
			if(m.cell_state[x][y] != EMPTY)
			{
				l[m.table[x][y] - 1]++;
			}
		}
		if(list_max(l) > 1)
		{	
			return false;
		}
	}
	
	// egy sorban minden szám legfeljebb egyszer szerepelhet
	for(int y = 0; y < 9; y++)
	{
		int l[9];
		empty_list(l);
		for(int x = 0; x < 9; x++)
		{
			if(m.cell_state[x][y] != EMPTY)
			{
				l[m.table[x][y] - 1]++;
			}
		}
		if(list_max(l) > 1)
		{	
			return false;
		}
	}
	
	// egy 3×3-as négyzetben minden szám legfeljebb egyszer szerepelhet
	for(int gx = 0; gx < 3; gx++)
	{
		for(int gy = 0; gy < 3; gy++)
		{
			int l[9];
			empty_list(l);
			for(int x = gx * 3; x < gx * 3 + 3; x++)
			{
				for(int y = gy * 3; y < gy * 3 + 3; y++)
				{
					if(m.cell_state[x][y] != EMPTY)
					{
						l[m.table[x][y] - 1]++;
					}
				}
			}
			if(list_max(l) > 1)
			{
				return false;
			}
		}
	}
	
	return true;
}

void prepare(FILE* f){/* Előkészítés */
    models=calloc(sizeof(model), MAX_deepness);
	for(int y = 0; y < 9; y++)
	{
		for(int x = 0; x < 9; x++)
		{
			char c;
			int retval = fscanf(f, "%c ", &c);
			if(retval != 1)
			{
				printf("file format error");
				return;
			}
			m.cell_state[x][y] = c == EMPTY_CHAR ? EMPTY : GIVEN;
			m.table[x][y] = c - '0';
		}
	}
	table_print();
	clear();
}

int get_choice_count()
{/* Döntési lehetoségek számba vétele */
	return 9;
}

void order_choices()
{/* Döntési lehetoségek rangsorolása */
}

void stack_pop()
{/*Visszaállítás */
    m = models[previous_choice_count];
    total_choice_count = m.copy_of_total_choice_count;
    current_choice_id = m.copy_of_current_choice_id;
}

void stack_push()
{/*Mentés */
    m.copy_of_total_choice_count = total_choice_count;
    m.copy_of_current_choice_id = current_choice_id;
    models[previous_choice_count] = m;
}

void apply_choice()
{/*Érvényre juttatás */
	iter++;
	int counter = current_choice_id;
	for(int x = 0; x < 9; x++)
	{
		for(int y = 0; y < 9; y++)
		{
			if(m.cell_state[x][y] == EMPTY)
			{
				m.table[x][y] = counter + 1;
				m.cell_state[x][y] = GUESS;
				goto finished;
			}
		}
	}
finished:
	if((iter%PRINT_FREQUENCY)==0)
	{
		table_print();
	}
}

void print_solution()
{/* Megoldás kijelzése */
	table_print();
}

void hide_cursor()
{
	wprintf(L"\x9B\x3F\x32\x35\x6C");
}

void show_cursor()
{
	wprintf(L"\x9B\x3F\x32\x35\x68");
}

int main(int argc, char** argv)
{
	if(argc <= 1)
	{
		printf("filename required\n");
		return 0;
	}
	fname = argv[1];
	FILE* f = fopen(fname, "r");
	if(!f)
	{
		printf("fopen error\n");
		return 0;
	}
    setlocale(LC_ALL, "");
    fwide(stdout, 1);
	prepare(f);
    table_print();
    getchar();
    hide_cursor();
	state = CHECK;
	previous_choice_count = 0;
	int iter = 0;
	while(state != END)
	{
		switch(state)
		{
			case CHECK:
				if (is_goal_reached())
				{
					print_solution();
					state = END;
				}
				else if (is_goal_reachable())
				{
					total_choice_count = get_choice_count();
					if (total_choice_count > 0)
					{
						order_choices();
						current_choice_id = 0;
						state = CHOOSE;
					}
					else
					{
						state = BACKTRACK;
					}
				}
				else
				{
					state = BACKTRACK;
				}
				break;
			case BACKTRACK:
				if (previous_choice_count > 0)
				{
					previous_choice_count--;
					stack_pop();
					if (current_choice_id + 1 < total_choice_count)
					{
						current_choice_id++;
						state = CHOOSE;
					}
					else
					{
						state = BACKTRACK;
					}
				}
				else
				{
					wprintf(KRED "Sorry, this problem has no solution!\n" KNRM);
					state = END;
				}
				break;
			case CHOOSE:
				stack_push();
				apply_choice();
				previous_choice_count++;
				state = CHECK;
				break;
		}
	}
	fclose(f);
	show_cursor();
}
