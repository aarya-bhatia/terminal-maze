#include <ncurses.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <unordered_set>
#include <assert.h>
#include <algorithm>
#include <random>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEBUG 1

#define log_printf(fmt, ...)                                   \
    do                                                         \
    {                                                          \
        if (DEBUG)                                             \
            fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, \
                    __LINE__, __func__, __VA_ARGS__);          \
    } while (0)

#define log_println(str) log_printf("%s", str)

int grid_begy, grid_begx;
int grid_w, grid_h;

int cell_width = 7;
int cell_height = 3;
int num_cols = 6;
int num_rows = 6;

char PATH = '.';
char WALL = 'W';
char USER = '@';

int cur_y = 0;
int cur_x = 0;

int NORTH = 0;
int SOUTH = 1;
int EAST = 2;
int WEST = 3;

struct MazeCell
{
    int y;
    int x;
    char value;
    WINDOW *win = NULL;

    MazeCell *union_set = NULL;
};

std::vector<MazeCell> grid;

MazeCell *get_union_set(MazeCell *cell)
{
    if (!cell->union_set)
    {
        return cell;
    }

    return (cell->union_set = get_union_set(cell->union_set));
}

bool is_reachable(MazeCell *cell1, MazeCell *cell2)
{
    return get_union_set(cell1) == get_union_set(cell2);
}

void add_union_set(MazeCell *cell1, MazeCell *cell2)
{
    MazeCell *root1 = get_union_set(cell1);
    MazeCell *root2 = get_union_set(cell2);

    if (root1 == root2)
    {
        return;
    }

    root2->union_set = root1;

    log_printf("Adding path between %d,%d and %d,%d", cell1->y, cell1->x, cell2->y, cell2->x);
}

void get_neighbors(MazeCell *cur, MazeCell *neighbors[4])
{
    assert(cur);

    for (int i = 0; i < 4; i++)
        neighbors[i] = NULL;

    if (cur->y > 0)
    {
        neighbors[NORTH] = &grid[(cur->y - 1) * num_cols + (cur->x)];
    }

    if (cur->y + 1 < num_rows)
    {
        neighbors[SOUTH] = &grid[(cur->y + 1) * num_cols + (cur->x)];
    }

    if (cur->x > 0)
    {
        neighbors[WEST] = &grid[(cur->y) * num_cols + (cur->x - 1)];
    }

    if (cur->x + 1 < num_cols)
    {
        neighbors[EAST] = &grid[(cur->y) * num_cols + (cur->x + 1)];
    }
}

void generate_maze()
{
    std::vector<MazeCell *> walls;

    MazeCell *entry_cell = &grid[0];
    MazeCell *exit_cell = &grid[num_rows * num_cols - 1];

    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_cols; c++)
        {
            MazeCell *cell = &grid[r * num_cols + c];
            cell->value = WALL;
            cell->union_set = NULL;
            walls.push_back(cell);
        }
    }

    entry_cell->value = PATH;
    exit_cell->value = PATH;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(walls.begin(), walls.end(), g);

    while (!walls.empty() && !is_reachable(entry_cell, exit_cell))
    {
        MazeCell *cur = walls.back();
        walls.pop_back();

        if (cur->value == PATH)
        {
            continue;
        }

        cur->value = PATH;

        MazeCell *neighbors[4];
        get_neighbors(cur, neighbors);
        int count_neighbors = 0;
        int reachable_neighbors = 0;

        for (int i = 0; i < 4; i++)
        {
            if (neighbors[i] == NULL)
            {
                continue;
            }
            count_neighbors++;

            if (neighbors[i]->value == PATH)
            {
                add_union_set(neighbors[i], cur);
                reachable_neighbors++;
            }
        }

        // if (count_neighbors > 0 && reachable_neighbors == 0)
        // {
        //     int random_neighbor_index = rand() % count_neighbors;
        //     for (int i = 0; i < 4; i++)
        //     {
        //         if (neighbors[i] != NULL)
        //         {
        //             random_neighbor_index--;

        //             if (random_neighbor_index == 0)
        //             {
        //                 neighbors[i]->value = PATH;
        //                 add_union_set(neighbors[i], cur);
        //                 break;
        //             }
        //         }
        //     }
        // }
    }
}

void setup()
{
    grid = std::vector<MazeCell>(num_rows * num_cols);

    grid_h = num_rows * cell_height;
    grid_w = num_cols * cell_width;

    assert(grid_w <= COLS);
    assert(grid_h <= LINES);

    grid_begx = (COLS - grid_w) / 2;
    grid_begy = (LINES - grid_h) / 2;

    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_cols; c++)
        {
            MazeCell &cur = grid[r * num_cols + c];
            cur.y = r;
            cur.x = c;
            cur.win = newwin(cell_height, cell_width, grid_begy + cell_height * cur.y, grid_begx + cell_width * cur.x);
        }
    }
}

void draw()
{
    char value;

    for (MazeCell &cell : grid)
    {
        value = cell.value;

        if (cell.y == cur_y && cell.x == cur_x)
        {
            value = USER;
        }

        mvwaddch(cell.win, cell_height / 2, cell_width / 2, value);
        wnoutrefresh(cell.win);
    }

    doupdate();
}

int main(int argc, char *argv[])
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
    {
        perror("ioctl");
        return 1;
    }

    int max_rows = w.ws_row / cell_height;
    int max_cols = w.ws_col / cell_width;

    int player_rows = 0, player_cols = 0;

    if (argc == 3)
    {
        player_rows = atoi(argv[1]);
        player_cols = atoi(argv[2]);
    }

    if (player_rows == 0 && player_cols == 0)
    {
        printf("Enter number of rows (min: 1, max: %d): ", max_rows);
        scanf("%d", &player_rows);
        printf("Enter number of cols (min: 1, max: %d): ", max_cols);
        scanf("%d", &player_cols);
    }

    if (player_rows > max_rows || player_cols > max_cols || player_rows < 1 || player_cols < 1)
    {
        printf("illegal size");
        exit(1);
    }

    num_rows = player_rows;
    num_cols = player_cols;

    initscr();
    cbreak();
    refresh();
    keypad(stdscr, true);

    int logfile = open("app.log", O_CREAT | O_TRUNC | O_WRONLY, 0640);
    dup2(logfile, 2);
    close(logfile);

    log_printf("init screen with %d ln x %d col", LINES, COLS);
    log_printf("grid rows:%d col:%d", num_rows, num_cols);
    log_printf("cell height:%d width:%d", cell_height, cell_width);

    noecho();
    curs_set(0);
    setup();

    log_println("generating maze...");
    generate_maze();

    log_println("starting game...");

    int total_moves = 0;

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
            if (cur_x + 1 < num_cols)
            {
                if (grid[(cur_y)*num_cols + (cur_x + 1)].value == PATH)
                {
                    cur_x++;
                    total_moves++;
                }
            }
        }
        else if (ch == KEY_LEFT || ch == 'h')
        {
            if (cur_x > 0)
            {
                if (grid[(cur_y)*num_cols + (cur_x - 1)].value == PATH)
                {
                    cur_x--;
                    total_moves++;
                }
            }
        }
        else if (ch == KEY_UP || ch == 'k')
        {
            if (cur_y > 0)
            {
                if (grid[(cur_y - 1) * num_cols + (cur_x)].value == PATH)
                {
                    cur_y--;
                    total_moves++;
                }
            }
        }
        else if (ch == KEY_DOWN || ch == 'j')
        {
            if (cur_y + 1 < num_rows)
            {
                if (grid[(cur_y + 1) * num_cols + (cur_x)].value == PATH)
                {
                    cur_y++;
                    total_moves++;
                }
            }
        }

        if (cur_y == num_rows - 1 && cur_x == num_cols - 1)
        {
            break;
        }
    }

    clear();
    printw("Congrats, you cleared the maze in %d moves!", total_moves);
    refresh();
    getch();

    endwin();
}