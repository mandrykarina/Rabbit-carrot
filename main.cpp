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

template<int n, typename T>
class Matrix {
private:
    T data[n];
public:
    Matrix() {
        for (int i = 0; i < n; ++i) data[i] = T();
    }

    Matrix(std::initializer_list<T> values) {
        int i = 0;
        for (T v : values) {
            if (i < n) data[i++] = v;
        }
        for (; i < n; ++i) data[i] = T();
    }

    T& operator[](int index) { return data[index]; }
    const T& operator[](int index) const { return data[index]; }

    Matrix<n, T> operator+(const Matrix<n, T>& other) const {
        Matrix<n, T> result;
        for (int i = 0; i < n; ++i) result[i] = data[i] + other[i];
        return result;
    }

    Matrix<n, T> operator-(const Matrix<n, T>& other) const {
        Matrix<n, T> result;
        for (int i = 0; i < n; ++i) result[i] = data[i] - other[i];
        return result;
    }

    Matrix<n, T> operator*(T scalar) const {
        Matrix<n, T> result;
        for (int i = 0; i < n; ++i) result[i] = data[i] * scalar;
        return result;
    }

    bool operator==(const Matrix<n, T>& other) const {
        for (int i = 0; i < n; ++i)
            if (data[i] != other[i]) return false;
        return true;
    }

    T x() const { return data[0]; }
    T y() const { return data[1]; }
};

// === Константы ===
const int WIDTH = 140;
const int HEIGHT = 40;
const int BASE_SPEED = 180;
const int MAX_ITEMS = 6;
const int SPEED_STEP = 20;
const int SPEED_INTERVAL = 20;

#define RABBIT    L"🐇"
#define CARROT    L"🥕"
#define CABBAGE   L"🥬"
#define BONE      L"☠"
#define HEART     L"❤"
#define TIGER     L"🐯"

enum ItemType { CARROT_TYPE, CABBAGE_TYPE, BONE_TYPE };

struct Item {
    Matrix<2, int> pos;
    ItemType type;
    int fall_timer;
};

class Tiger {
public:
    Matrix<2, int> pos;
    bool active;
    time_t appear_time;
    int duration; // сколько секунд тигр активен

    Tiger() : active(false), pos({-1, -1}), appear_time(0), duration(7) {}

    void spawn() {
        // Появляется в случайной точке экрана
        pos[0] = rand() % WIDTH;
        pos[1] = rand() % HEIGHT;
        active = true;
        appear_time = time(nullptr);
    }

    void deactivate() {
        active = false;
        pos = {-1, -1};
    }

    bool is_active() const {
        if (!active) return false;
        // Проверяем длительность активности
        time_t now = time(nullptr);
        if (now - appear_time >= duration) {
            return false;
        }
        return true;
    }

    void move_towards(const Matrix<2, int>& target) {
        if (!active) return;

        // Замедленная скорость тигра: двигается через кадр (через рандом с задержкой)
        static int frame_count = 0;
        frame_count++;
        if (frame_count % 2 != 0) return;  // двигается только через кадр

        int dx = target[0] - pos[0];
        int dy = target[1] - pos[1];

        // Движемся по оси X
        if (dx != 0) pos[0] += (dx > 0) ? 1 : -1;

        // Движемся по оси Y
        if (dy != 0) pos[1] += (dy > 0) ? 1 : -1;

        // Границы экрана
        if (pos[0] < 0) pos[0] = 0;
        if (pos[0] >= WIDTH) pos[0] = WIDTH - 1;
        if (pos[1] < 0) pos[1] = 0;
        if (pos[1] >= HEIGHT) pos[1] = HEIGHT - 1;
    }
};

class Game {
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

    Tiger tiger;
    time_t last_tiger_spawn;

public:
    Game(int diff) : difficulty(diff), score(0), lives(3), game_over(false), last_tiger_spawn(0) {
        player_pos = {WIDTH / 2, HEIGHT / 2};
        game_speed = BASE_SPEED + (3 - difficulty) * 100;
        init_curses();
        start_time = time(nullptr);
        last_speedup = start_time;
    }

    void init_curses() {
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
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    }

    void spawn_item() {
        int bone_count = std::count_if(items.begin(), items.end(), [](Item& i) { return i.type == BONE_TYPE; });

        if ((int)items.size() < MAX_ITEMS + difficulty + 6) { // увеличено максимальное число предметов для большего кол-ва моркови
            int rand_type = rand() % 100;

            ItemType itype;
            // Сделаем моркови чаще — например 40%, капусты 25%, кости 35%
            if (rand_type < 40)
                itype = CARROT_TYPE;
            else if (rand_type < 65)
                itype = CABBAGE_TYPE;
            else
                itype = BONE_TYPE;

            int group_size = (itype == BONE_TYPE && bone_count < 12) ? (1 + rand() % 3) : 1;

            for (int i = 0; i < group_size; ++i) {
                int x = rand() % (WIDTH - 1);
                int y = 0;
                items.push_back({Matrix<2, int>{x, y}, itype, 0});
            }
        }
    }

    void draw_ui() {
        attron(COLOR_PAIR(1));
        mvprintw(0, 0, "Lives: ");
        attron(COLOR_PAIR(2));
        for (int i = 0; i < lives; i++) addwstr(HEART);
        attroff(COLOR_PAIR(2));

        attron(COLOR_PAIR(1));
        mvprintw(1, 0, "Score: %d", score);
        mvprintw(2, 0, "Difficulty: %d", difficulty);
    }

    void draw_border() {
        for (int i = 0; i < WIDTH; i++) {
            mvaddwstr(HEIGHT + 1, i, L"─");
        }
    }

    void update_speed() {
        time_t now = time(nullptr);
        if (now - last_speedup >= SPEED_INTERVAL) {
            if (game_speed > 60) game_speed -= SPEED_STEP;
            last_speedup = now;
        }
    }

    void update() {
        int ch = getch();
        if (ch == KEY_LEFT || ch == 'a' || ch == 'A') if (player_pos[0] > 0) player_pos[0]--;
        if (ch == KEY_RIGHT || ch == 'd' || ch == 'D') if (player_pos[0] < WIDTH - 1) player_pos[0]++;
        if (ch == KEY_UP || ch == 'w' || ch == 'W') if (player_pos[1] > 0) player_pos[1]--;
        if (ch == KEY_DOWN || ch == 's' || ch == 'S') if (player_pos[1] < HEIGHT - 1) player_pos[1]++;
        if (ch == 27) { game_over = true; return; }

        // Обновляем предметы
        for (auto &item : items) {
            item.fall_timer--;
            if (item.fall_timer <= 0) {
                item.pos[1]++;
                item.fall_timer = (item.type == BONE_TYPE) ? 1 : 2;
            }

            if (std::abs(player_pos[0] - item.pos[0]) <= 1 && std::abs(player_pos[1] - item.pos[1]) <= 1) {
                switch (item.type) {
                    case CARROT_TYPE: score += 20 * difficulty; break;
                    case CABBAGE_TYPE: score += 10 * difficulty; break;
                    case BONE_TYPE:
                        lives--;
                        if (lives <= 0) game_over = true;
                        break;
                }
                item.pos = {-1, -1};
            }

            if (item.pos[1] >= HEIGHT) {
                item.pos = {-1, -1};
            }
        }

        items.erase(std::remove_if(items.begin(), items.end(),
                    [](Item &it) { return it.pos == Matrix<2, int>{-1, -1}; }),
                    items.end());

        spawn_item();
        update_speed();

        // Логика появления тигра
        time_t now = time(nullptr);
        if (!tiger.active && now - last_tiger_spawn > 10 + rand() % 20) {
            tiger.spawn();
            last_tiger_spawn = now;
        }

        // Если тигр активен - двигаем его и проверяем столкновение
        if (tiger.is_active()) {
            tiger.move_towards(player_pos);

            // Если тигр догнал игрока - игра окончена
            if (tiger.pos == player_pos) {
                lives = 0;
                game_over = true;
            }
        } else if (tiger.active) {
            // Время активности тигра вышло - деактивируем
            tiger.deactivate();
        }
    }

    void render() {
        clear();

        // Рисуем зайца
        attron(COLOR_PAIR(1));
        mvaddwstr(player_pos[1], player_pos[0], RABBIT);

        // Рисуем предметы
        for (auto &item : items) {
            switch (item.type) {
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

        // Рисуем тигра если активен
        if (tiger.is_active()) {
            attron(COLOR_PAIR(5));
            mvaddwstr(tiger.pos[1], tiger.pos[0], TIGER);
        }

        draw_border();
        draw_ui();
        refresh();
        timeout(game_speed);
    }

    bool is_game_over() const {
        return game_over;
    }

    int get_score() const {
        return score;
    }

    void run() {
        while (!game_over) {
            update();
            render();
        }

        // Вывести финальный счет и паузу перед выходом из игры
        clear();
        mvprintw(HEIGHT / 2, (WIDTH - 20) / 2, "Game Over! Your score: %d", score);
        mvprintw(HEIGHT / 2 + 1, (WIDTH - 35) / 2, "Press any key to return to menu...");
        refresh();
        nodelay(stdscr, FALSE);
        getch();
    }

    ~Game() {
        endwin();
    }
};

int main() {
    srand(time(nullptr));

    while (true) {
        setlocale(LC_ALL, "");
        initscr();
        noecho();
        curs_set(0);
        keypad(stdscr, TRUE);

        clear();
        mvprintw(10, 40, "Carrot Catcher");
        mvprintw(12, 40, "Select difficulty:");
        mvprintw(14, 40, "1 - Easy");
        mvprintw(15, 40, "2 - Medium");
        mvprintw(16, 40, "3 - Hard");
        mvprintw(17, 40, "4 - Exit");

        int choice = getch();

        if (choice == '4' || choice == 'q' || choice == 'Q') {
            endwin();
            break;
        }

        int diff = 1;
        if (choice == '1') diff = 1;
        else if (choice == '2') diff = 2;
        else if (choice == '3') diff = 3;
        else continue;

        endwin();

        Game game(diff);
        game.run();
    }

    return 0;
}
