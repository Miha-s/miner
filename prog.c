#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

enum {
    MAX_Y = 50,
    MAX_X = 50,
    col_amount = 10,
    zero_pair = 10,
    bomb_pair = 11,
    flag_pair = 12,
    fon_pair  = 13,
    cur_pair  = 14
};

struct game_data {
    char field[MAX_Y][MAX_X];
    char mask[MAX_Y][MAX_X];
    int sizey, sizex;
    int cury, curx; /* cursor */
    int open_area;  /* remaining amounts of closed cells */
    int bombs;
    int posy, posx;
    char fon;   /* fon symbol */
    char flag;
    char zero;  /* zero symbol */
    char bomb;
    char cursor;
};

struct colors {
    int backgr;
    int numbers[col_amount];
    int zero;
    int flag;
    int bomb;
    int fon;
    int cur;
};

enum direction {
    up,
    down,
    right,
    left
};

int count = 0;

void init_game(struct game_data *gm);
void init_colors(struct colors *col);
void draw_game(struct game_data *gm);
int make_shot(struct game_data *gm);
int set_flag(struct game_data *gm);
void move_curs(struct game_data *gm, enum direction dir);

void handle_lose(struct game_data *gm, int *play);
void handle_win(struct game_data *gm, int *play);

void write_message(char *mes);
void clear_message();

/*******************************************************/
int main(int argc, char **argv)
{
    int play = 1;
    srand(time(NULL));

    struct game_data game;
        game.sizey = 10;
        game.sizex = 15;
        game.bombs = 15;
        game.posy  = 6;
        game.posx  = 15;
        game.fon   = '#';
        game.zero  = '.';
        game.bomb  = '*';
        game.cursor = '|';
        game.flag  = 'F';

    struct colors col;
        col.backgr = COLOR_WHITE;
        col.numbers[1] = COLOR_BLUE;
        col.numbers[2] = COLOR_GREEN;
        col.numbers[3] = COLOR_YELLOW;
        col.numbers[4] = COLOR_BLACK;
        col.numbers[5] = COLOR_RED;
        col.flag = COLOR_RED;
        col.bomb = COLOR_RED;
        col.zero = COLOR_MAGENTA;
        col.fon  = COLOR_BLACK;
        col.cur  = COLOR_RED;
    
    init_game(&game);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    curs_set(0);
    start_color();
    init_colors(&col);
    
    int ch;
    write_message("To hit cell press Enter");
    write_message("To put flag press 'F'");
    write_message("To exit game press 'q'");
    while(play) {
        draw_game(&game);
        ch = getch();
        switch(ch) {
            case 'F':
            case 'f':
                set_flag(&game);
                break;
            case ' ':
            case '\n':
                if(make_shot(&game)) {
                    draw_game(&game);
                    handle_lose(&game, &play);
                }
                else if(game.open_area <= 0) {
                    draw_game(&game);
                    handle_win(&game, &play);
                }
                break;
            case KEY_UP:
                move_curs(&game, up);
                break;
            case KEY_DOWN:
                move_curs(&game, down);
                break;
            case KEY_LEFT:
                move_curs(&game, left);
                break;
            case KEY_RIGHT:
                move_curs(&game, right);
                break;
            case 'Q':
            case 'q':
                play = 0;
        }
        refresh();
    }


    endwin();

    return 0;
}
/*****************************************************/

void fill_area(char (*ar)[MAX_X], int sizey, int sizex, char symb)
{
    for(int i = 0; i < sizey; i++)
        for(int a = 0; a < sizex; a++)
            ar[i][a] = symb;
}

int within_border(int y, int x, struct game_data *gm)
{
    if(y >= 0 && y < gm->sizey && x >= 0 && x < gm->sizex)
        return 1;
    return 0;
}

void init_game(struct game_data *gm)
{
    fill_area(gm->field, gm->sizey, gm->sizex, 0);
    fill_area(gm->mask, gm->sizey, gm->sizex, gm->fon);
    
    gm->open_area = gm->sizey*gm->sizex - gm->bombs;
    
    /* first cell */
    gm->cury = 0;
    gm->curx = 0;

    int y, x;
    for(int i = 0; i < gm->bombs;) {
        y = rand()%gm->sizey;
        x = rand()%gm->sizex;
        if(gm->field[y][x] != -1) {
            gm->field[y][x] = -1;
            i++;
        } else
            continue;

    // making correct number around bomb
        for(int j = y-1; j < y+2; j++)
            for(int k = x-1; k < x+2; k++)
                if(within_border(j, k, gm) && gm->field[j][k] != -1)
                    gm->field[j][k]++;
    }
}

void init_colors(struct colors *col)
{
    for(int i = 1; i < col_amount; i++)
        init_pair(i, col->numbers[i], col->backgr);
    init_pair(zero_pair, col->zero, col->backgr);
    init_pair(bomb_pair, col->bomb, col->backgr);
    init_pair(flag_pair, col->flag, col->backgr);
    init_pair(fon_pair, col->fon, col->backgr);
    init_pair(cur_pair, col->cur, col->backgr);
}

/* return 0 if ok, return 1 if bomb */
int make_shot(struct game_data *gm)
{
    int y = gm->cury;
    int x = gm->curx;
    int num = gm->field[y][x];
    if(gm->mask[y][x] == gm->flag)
        return 0;
    if(num == -1)
        return 1;
    if(num != 0) {
        gm->mask[y][x] = '0' + num;
        gm->open_area--;
        return 0;
    }
    gm->mask[y][x] = gm->zero;

    /* reccurent openning of nearest 8 cells */
    for(int j = y-1; j < y+2; j++)
        for(int k = x-1; k < x+2; k++)
            if(within_border(j, k, gm) && gm->mask[j][k] == gm->fon) {
                gm->cury = j;
                gm->curx = k;
                make_shot(gm);
            }
    gm->cury = y;
    gm->curx = x;
    gm->open_area--;
    return 0;
}

/* retrun 0 if flag set/unset, else return 1 */
int set_flag(struct game_data *gm)
{
    if(gm->mask[gm->cury][gm->curx] == gm->fon) {
        gm->mask[gm->cury][gm->curx] = gm->flag;
        return 0;
    } 
    else if(gm->mask[gm->cury][gm->curx] == gm->flag) {
        gm->mask[gm->cury][gm->curx] = gm->fon;
        return 0;
    }

    return 1;
}

void draw_curs(struct game_data *gm)
{
    int ry = gm->posy + 1 + gm->cury;
    int rx = gm->posx + 1 + gm->curx*2;
    move(ry, rx-1);
    addch(gm->cursor | COLOR_PAIR(cur_pair));
    move(ry, rx+1);
    addch(gm->cursor | COLOR_PAIR(cur_pair));
}

void draw_game(struct game_data *gm)
{
    int y, x;
    y = gm->posy;
    x = gm->posx;
    for(int i = 0; i < gm->sizex*2+1; i++) {
        move(y, x+i);
        addch(' ' | COLOR_PAIR(1));
    }
    y++;

    for(int i = 0; i < gm->sizey; i++) {
        move(y, x);
        addch(' ' | COLOR_PAIR(1));
        for(int j = 0; j < gm->sizex; j++) {
            move(y, x+(j+1)*2);
            addch(' ' | COLOR_PAIR(1));
            move(y, x+1+j*2);

            char symb = gm->mask[i][j];
            char pr;
            if(symb == gm->fon)
                pr = fon_pair;
            else if(symb == gm->flag)
                pr = flag_pair;
            else if(symb == gm->zero)
                pr = zero_pair;
            else if(symb == gm->bomb)
                pr = bomb_pair;
            else 
                pr = gm->field[i][j];
            addch(symb | COLOR_PAIR(pr) | A_BOLD);
        }
        y++;
    }

    for(int i = 0; i < gm->sizex*2+1; i++) {
        move(y, x+i);
        addch(' ' | COLOR_PAIR(1));
    }
    
    draw_curs(gm);

    refresh();
}

void move_curs(struct game_data *gm, enum direction dir)
{
    switch(dir) {
        case right:
            gm->curx = (gm->curx + 1)%gm->sizex;
            break;
        case down:
            gm->cury = (gm->cury + 1)%gm->sizey;
            break;
        case left:
            if(gm->curx == 0)
                gm->curx = gm->sizex;
            gm->curx -= 1;
            break;
        case up:
            if(gm->cury == 0)
                gm->cury = gm->sizey;
            gm->cury -= 1;
            break;
    }

}

void reset(struct game_data *gm, int *play)
{
    write_message("Do you wanna play again?[Y/n]");
    int ch;
    while(1) {
        ch = getch();
        if(ch == 'n' || ch == 'N' || ch == 'q' || ch == 'Q') {
            *play = 0;
            return;
        }
        if(ch == 'Y' || ch == 'y' || ch == KEY_ENTER)
            break;
    }

    clear_message();

    init_game(gm);
}

void reveal_bombs(struct game_data *gm)
{
    for(int i = 0; i < gm->sizey; i++)
        for(int j = 0; j < gm->sizex; j++)
            if(gm->field[i][j] == -1)
                gm->mask[i][j] = gm->bomb;
    draw_game(gm);
}

void handle_lose(struct game_data *gm, int *play)
{
    reveal_bombs(gm);
    write_message("You've lose");
    reset(gm, play);
    clear_message();
}
void handle_win(struct game_data *gm, int *play)
{
    write_message("Congratulations! You've won");
    reset(gm, play);
    clear_message();
}

void write_message(char *mes)
{
    move(count, 0);
    printw("%s", mes);
    count++;
}

void clear_message()
{
    if(count == 0)
        return;
    count--;
    move(count, 0);
    printw("                                        ");
}
