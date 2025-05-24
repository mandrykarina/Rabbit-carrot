#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <vector>
#include <string>

// Конфигурация игры
const int WIDTH = 40;
const int HEIGHT = 20;
const int BASE_SPEED = 120;
const int MAX_CARROTS = 5;

// Unicode символы
#define PLAYER L"\U0001F407" // 🐇
#define CARROT L"\U0001F955" // 🥕
#define HEART L"\U00002764"  // ❤

class Game
{
private:
    int player_x;
    int score;
    int lives;
    int difficulty;
    int game_speed;
    bool game_over;
    std::vector<std::pair<int, int>> carrots;

public:
    Game(int diff) : difficulty(diff), player_x(WIDTH / 2), score(0), lives(3), game_over(false)
    {
        game_speed = BASE_SPEED - diff * 20;
        init_curses();
        spawn_carrot();
    }

    void init_curses()
    {
        setlocale(LC_ALL, "");
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);
        curs_set(0);
        start_color();

        init_pair(1, COLOR_WHITE, COLOR_BLACK);  // Игрок
        init_pair(2, COLOR_YELLOW, COLOR_BLACK); // Морковь
        init_pair(3, COLOR_RED, COLOR_BLACK);    // Жизни
    }

    void spawn_carrot()
    {
        if (carrots.size() < MAX_CARROTS + difficulty)
        {
            carrots.emplace_back(rand() % WIDTH, 0);
        }
    }

    void draw_border()
    {
        attron(COLOR_PAIR(1));
        for (int i = 0; i < WIDTH; i++)
        {
            mvaddwstr(HEIGHT + 1, i, L"─");
        }
    }

    void draw_lives()
    {
        attron(COLOR_PAIR(3));
        mvprintw(0, 0, "Lives: ");
        for (int i = 0; i < lives; i++)
        {
            addwstr(HEART);
        }
    }

    void update()
    {
        int ch = getch();
        if (ch == KEY_LEFT && player_x > 0)
            player_x--;
        if (ch == KEY_RIGHT && player_x < WIDTH - 1)
            player_x++;
        if (ch == 'q')
            game_over = true;

        // Движение моркови
        for (auto &carrot : carrots)
        {
            carrot.second++;
            if (carrot.second > HEIGHT)
            {
                if (abs(player_x - carrot.first) > 2)
                {
                    if (--lives <= 0)
                        game_over = true;
                }
                carrot = {-1, -1}; // Удалить морковь
                score += 10 * difficulty;
            }
        }

        // Очистка и создание новых морковок
        carrots.erase(std::remove_if(carrots.begin(), carrots.end(),
                                     [](auto &c)
                                     { return c.first == -1; }),
                      carrots.end());
        spawn_carrot();
    }

    void render()
    {
        clear();

        // Отрисовка игрока
        attron(COLOR_PAIR(1));
        mvaddwstr(HEIGHT, player_x, PLAYER);

        // Отрисовка морковок
        attron(COLOR_PAIR(2));
        for (auto &carrot : carrots)
        {
            mvaddwstr(carrot.second, carrot.first, CARROT);
        }

        draw_border();
        draw_lives();
        mvprintw(1, 0, "Score: %d", score);
        mvprintw(2, 0, "Difficulty: %d", difficulty);

        refresh();
        timeout(game_speed);
    }

    void run()
    {
        while (!game_over)
        {
            update();
            render();
        }
        endwin();
        printf("Game Over! Final Score: %d\n", score);
    }
};

int show_menu()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int choice = 0;
    const char *items[] = {"Easy", "Medium", "Hard", "Exit"};

    while (true)
    {
        clear();
        mvprintw(5, 10, "Carrot Catcher!");
        for (int i = 0; i < 4; i++)
        {
            mvprintw(10 + i, 10, "%s %s",
                     (i == choice) ? ">" : " ", items[i]);
        }
        refresh();

        switch (getch())
        {
        case KEY_UP:
            if (choice > 0)
                choice--;
            break;
        case KEY_DOWN:
            if (choice < 3)
                choice++;
            break;
        case 10:
            endwin();
            return choice + 1;
        }
    }
}

int main()
{
    srand(time(NULL));
    int choice = show_menu();

    if (choice != 4)
    {
        Game game(choice).run();
    }
    return 0;
}
