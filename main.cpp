#include <ncurses.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <assert.h>

#define DEBUG 1

#define debug_print(fmt, ...)                                  \
    do                                                         \
    {                                                          \
        if (DEBUG)                                             \
            fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, \
                    __LINE__, __func__, __VA_ARGS__);          \
    } while (0)

int grid_begy, grid_begx;
int grid_w, grid_h;

int cell_width = 7;
int cell_height = 3;
int grid_num_cols = 6;
int grid_num_rows = 6;

int north = 1 << 0; // 1
int south = 1 << 1; // 2
int east = 1 << 2;  // 4
int west = 1 << 3;  // 8

char PATH = '.';
char WALL = 'W';
char USER = '@';

int cur_y = 0;
int cur_x = 0;

struct MazeCell
{
    int y;
    int x;
    char value = '.';
    WINDOW *win = NULL;
};

std::vector<MazeCell> grid;

void setup()
{
    grid = std::vector<MazeCell>(grid_num_rows * grid_num_cols);

    grid_h = grid_num_rows * cell_height;
    grid_w = grid_num_cols * cell_width;

    assert(grid_w <= COLS);
    assert(grid_h <= LINES);

    grid_begx = (COLS - grid_w) / 2;
    grid_begy = (LINES - grid_h) / 2;

    for (int r = 0; r < grid_num_rows; r++)
    {
        for (int c = 0; c < grid_num_cols; c++)
        {
            MazeCell &cur = grid[r * grid_num_cols + c];
            cur.y = r;
            cur.x = c;
            cur.value = PATH;

            cur.win = newwin(cell_height, cell_width, grid_begy + cell_height * cur.y, grid_begx + cell_width * cur.x);
            mvwaddch(cur.win, 1, 3, cur.value);
        }
    }
}

void draw()
{
    for (MazeCell &cell : grid)
    {
        if (cell.y == cur_y && cell.x == cur_x)
        {
            cell.value = USER;
        }
        else
        {
            cell.value = PATH;
        }

        mvwaddch(cell.win, cell_height / 2, cell_width / 2, cell.value);
        wnoutrefresh(cell.win);
    }

    doupdate();
}

int main()
{
    initscr();
    noecho();
    cbreak();
    refresh();
    curs_set(0);
    keypad(stdscr, true);

    int logfile = open("app.log", O_CREAT | O_TRUNC | O_WRONLY, 0640);
    dup2(logfile, 2);
    close(logfile);

    debug_print("init screen with %d ln x %d col", LINES, COLS);
    debug_print("grid rows:%d col:%d", grid_num_rows, grid_num_cols);
    debug_print("cell height:%d width:%d", cell_height, cell_width);

    setup();

    while (1)
    {
        draw();

        int ch = getch();
        if (ch == 'q')
        {
            break;
        }
        else if (ch == KEY_RIGHT || ch == 'l')
        {
            if (cur_x + 1 < grid_num_cols)
            {
                cur_x++;
            }
        }
        else if (ch == KEY_LEFT || ch == 'h')
        {
            if (cur_x > 0)
            {
                cur_x--;
            }
        }
        else if (ch == KEY_UP || ch == 'k')
        {
            if (cur_y > 0)
            {
                cur_y--;
            }
        }
        else if (ch == KEY_DOWN || ch == 'j')
        {
            if (cur_y + 1 < grid_num_rows)
            {
                cur_y++;
            }
        }
    }

    endwin();
}