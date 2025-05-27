#include <curses.h>
#include <locale.h>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <initializer_list>
#include <iostream>

// === Шаблон Matrix<n, T> ===
template <int n, typename T>
class Matrix
{
private:
    T data[n];

public:
    Matrix()
    {
        for (int i = 0; i < n; ++i)
            data[i] = T();
    }

    Matrix(std::initializer_list<T> values)
    {
        int i = 0;
        for (T v : values)
        {
            if (i < n)
                data[i++] = v;
        }
        for (; i < n; ++i)
            data[i] = T();
    }

    T &operator[](int index) { return data[index]; }
    const T &operator[](int index) const { return data[index]; }

    Matrix<n, T> operator+(const Matrix<n, T> &other) const
    {
        Matrix<n, T> result;
        for (int i = 0; i < n; ++i)
            result[i] = data[i] + other[i];
        return result;
    }

    Matrix<n, T> operator-(const Matrix<n, T> &other) const
    {
        Matrix<n, T> result;
        for (int i = 0; i < n; ++i)
            result[i] = data[i] - other[i];
        return result;
    }

    Matrix<n, T> operator*(T scalar) const
    {
        Matrix<n, T> result;
        for (int i = 0; i < n; ++i)
            result[i] = data[i] * scalar;
        return result;
    }

    bool operator==(const Matrix<n, T> &other) const
    {
        for (int i = 0; i < n; ++i)
            if (data[i] != other[i])
                return false;
        return true;
    }

    T x() const { return data[0]; }
    T y() const { return data[1]; }
};

// === Константы ===
const int WIDTH = 160;
const int HEIGHT = 40;
const int BASE_SPEED = 180;
const int MAX_ITEMS = 6;
const int SPEED_STEP = 20;
const int SPEED_INTERVAL = 20;

#define RABBIT L"\U0001F407"  // 🐇 Заяц
#define CARROT L"\U0001F955"  // 🥕 Морковка
#define CABBAGE L"\U0001F96C" // 🥬 Капуста
#define BONE L"\u2620"        // ☠
#define HEART L"\u2764"       // ❤

enum ItemType
{
    CARROT_TYPE,
    CABBAGE_TYPE,
    BONE_TYPE
};

struct Item
{
    Matrix<2, int> pos;
    ItemType type;
    int fall_timer;
};

class Game
{
private:
    Matrix<2, int> player_pos;
    int score;
    int lives;
    int difficulty;
    int game_speed;
    bool game_over;
    std::vector<Item> items;
    time_t start_time;
    time_t last_speedup;

public:
    Game(int diff) : difficulty(diff), score(0), lives(3), game_over(false)
    {
        player_pos = {WIDTH / 2, HEIGHT};
        game_speed = BASE_SPEED + (3 - difficulty) * 100;
        init_curses();
        start_time = time(nullptr);
        last_speedup = start_time;
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
        use_default_colors();
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
    }

    void spawn_item()
    {
        if ((int)items.size() < MAX_ITEMS + difficulty)
        {
            int type = rand() % 10;
            ItemType itype = (type < 5) ? CARROT_TYPE : (type < 8 ? CABBAGE_TYPE : BONE_TYPE);
            items.push_back({Matrix<2, int>{rand() % WIDTH, 0}, itype, 0});
        }
    }

    void draw_ui()
    {
        attron(COLOR_PAIR(1));
        mvprintw(0, 0, "Lives: ");

        attron(COLOR_PAIR(2)); // красный цвет для сердечек
        for (int i = 0; i < lives; i++)
        {
            addwstr(HEART);
        }
        attroff(COLOR_PAIR(2)); // отключить красный цвет

        attron(COLOR_PAIR(1)); // вернуть основной цвет
        mvprintw(1, 0, "Score: %d", score);
        mvprintw(2, 0, "Difficulty: %d", difficulty);
    }

    void draw_border()
    {
        for (int i = 0; i < WIDTH; i++)
        {
            mvaddwstr(HEIGHT + 1, i, L"─");
        }
    }

    void update_speed()
    {
        time_t now = time(nullptr);
        if (now - last_speedup >= SPEED_INTERVAL)
        {
            if (game_speed > 60)
                game_speed -= SPEED_STEP;
            last_speedup = now;
        }
    }

    void update()
    {
        int ch = getch();
        if (ch == KEY_LEFT && player_pos[0] > 0)
            player_pos[0]--;
        if (ch == KEY_RIGHT && player_pos[0] < WIDTH - 1)
            player_pos[0]++;
        if (ch == 27)
        {
            game_over = true;
            return;
        }

        for (auto &item : items)
        {
            item.fall_timer--;
            if (item.fall_timer <= 0)
            {
                item.pos[1]++;
                item.fall_timer = (item.type == BONE_TYPE) ? 1 : 2;
            }

            if (item.pos[1] == HEIGHT)
            {
                if (std::abs(player_pos[0] - item.pos[0]) <= 1)
                {
                    switch (item.type)
                    {
                    case CARROT_TYPE:
                        score += 20 * difficulty;
                        break;
                    case CABBAGE_TYPE:
                        score += 10 * difficulty;
                        break;
                    case BONE_TYPE:
                        lives--;
                        if (lives <= 0)
                            game_over = true;
                        break;
                    }
                }
                item.pos = {-1, -1}; // удалить
            }
        }

        items.erase(std::remove_if(items.begin(), items.end(),
                                   [](Item &it)
                                   { return it.pos == Matrix<2, int>{-1, -1}; }),
                    items.end());

        spawn_item();
        update_speed();
    }

    void render()
    {
        clear();
        attron(COLOR_PAIR(1));
        mvaddwstr(player_pos[1], player_pos[0], RABBIT);

        for (auto &item : items)
        {
            switch (item.type)
            {
            case CARROT_TYPE:
                attron(COLOR_PAIR(2));
                mvaddwstr(item.pos[1], item.pos[0], CARROT);
                break;
            case CABBAGE_TYPE:
                attron(COLOR_PAIR(3));
                mvaddwstr(item.pos[1], item.pos[0], CABBAGE);
                break;
            case BONE_TYPE:
                attron(COLOR_PAIR(4));
                mvaddwstr(item.pos[1], item.pos[0], BONE);
                break;
            }
        }

        draw_border();
        draw_ui();
        refresh();
        timeout(game_speed);
    }

    bool is_game_over() const
    {
        return game_over;
    }

    int get_score() const
    {
        return score;
    }

    void run()
    {
        while (!game_over)
        {
            update();
            render();
        }
        endwin();
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
        mvprintw(5, 10, "Rabbit Catcher!");
        for (int i = 0; i < 4; i++)
        {
            mvprintw(10 + i, 10, "%s %s", (i == choice) ? ">" : " ", items[i]);
        }
        refresh();

        int ch = getch();
        switch (ch)
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
    setlocale(LC_ALL, "");
    srand(time(nullptr));

    while (true)
    {
        int choice = show_menu();
        if (choice == 4)
            break;
        Game game(choice);
        game.run();
    }

    return 0;
}
