#include <iostream>
#include <vector>
#include <deque>
#include <raylib.h>
#include <raymath.h>
#include <cstdio>
#include <string>
#include <ctime>
#include <cmath>

using namespace std;

// --- Global Variables and Helper Functions ---
static bool allowMove = false;
Color green = {173, 204, 96, 255};
Color darkGreen = {43, 51, 24, 255};
Color explosiveFoodColor = {255, 0, 0, 255};
int cellSize = 30;
int cellCount = 25;
int offset = 75;
double lastUpdateTime = 0;
double gameSpeed = 0.2;
bool isHardMode = false;

bool ElementInDeque(Vector2 element, deque<Vector2> deque)
{
    for (unsigned int i = 0; i < deque.size(); i++)
    {
        if (Vector2Equals(deque[i], element))
        {
            return true;
        }
    }
    return false;
}

bool EventTriggered(double interval)
{
    double currentTime = GetTime();
    if (currentTime - lastUpdateTime >= interval)
    {
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

// --- Wall Class ---
class Wall {
private:
    Rectangle rect;

public:
    Wall(float x, float y, float width, float height) {
        rect = { x, y, width, height };
    }

    void Draw() const {
        DrawRectangleRec(rect, SKYBLUE);
    }

    bool CheckCollision(const Rectangle& target) const {
        return CheckCollisionRecs(rect, target);
    }
};

// --- MapBase Class ---
class MapBase {
protected:
    std::vector<Wall> walls;
    int blockSize;

public:
    MapBase(int blockSize = 30) : blockSize(blockSize) {}

    virtual void LoadWalls() = 0;
    virtual void Draw() const {
        for (const auto& wall : walls) {
            wall.Draw();
        }
    }

    virtual bool CheckCollision(Vector2 point) const {
        Rectangle testRect = { point.x, point.y, (float)blockSize, (float)blockSize };
        for (const auto& wall : walls) {
            if (wall.CheckCollision(testRect)) return true;
        }
        return false;
    }

    virtual bool CheckCollisionWithRect(Rectangle rect) const {
        for (const auto& wall : walls) {
            if (wall.CheckCollision(rect)) return true;
        }
        return false;
    }

    virtual ~MapBase() = default;
};

// --- HardModeMap Class ---
class HardModeMap : public MapBase {
public:
    HardModeMap(int blockSize = 30) : MapBase(blockSize) {
        LoadWalls();
    }
    void LoadWalls() override {
        walls.clear();
        walls.emplace_back(offset + 3 * blockSize, offset + 1 * blockSize, blockSize, 10 * blockSize); // Tường dọc trái dài hơn
        walls.emplace_back(offset + 3 * blockSize, offset + 12 * blockSize, 8 * blockSize, blockSize); // Tường ngang trên dài hơn
        walls.emplace_back(offset + 19 * blockSize, offset + 1 * blockSize, blockSize, 10 * blockSize); // Tường dọc phải dài hơn
        walls.emplace_back(offset + 11 * blockSize, offset + 18 * blockSize, 8 * blockSize, blockSize); // Tường ngang dưới dài hơn
    }
};

// --- Food Class ---
class Food
{
public:
    Vector2 position;
    Texture2D texture;
    MapBase* map;

    Food(deque<Vector2> snakeBody, MapBase* map = nullptr) : map(map)
    {
        Image image = LoadImage("Graphics/food.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
        position = GenerateRandomPos(snakeBody);
    }

    ~Food()
    {
        UnloadTexture(texture);
    }

    void Draw()
    {
        DrawTexture(texture, offset + position.x * cellSize, offset + position.y * cellSize, WHITE);
    }

public:
    Vector2 GenerateRandomPos(deque<Vector2> snakeBody)
    {
        Vector2 position = GenerateRandomCell();
        while (ElementInDeque(position, snakeBody) || (map && map->CheckCollision(Vector2{offset + position.x * cellSize, offset + position.y * cellSize})))
        {
            position = GenerateRandomCell();
        }
        return position;
    }

    static Vector2 GenerateRandomPosStatic(deque<Vector2> snakeBody, MapBase* map = nullptr)
    {
        Vector2 position = GenerateRandomCellStatic();
        while (ElementInDeque(position, snakeBody) || (map && map->CheckCollision(Vector2{offset + position.x * cellSize, offset + position.y * cellSize})))
        {
            position = GenerateRandomCellStatic();
        }
        return position;
    }

private:
    Vector2 GenerateRandomCell()
    {
        float x = GetRandomValue(0, cellCount - 1);
        float y = GetRandomValue(0, cellCount - 1);
        return Vector2{x, y};
    }

    static Vector2 GenerateRandomCellStatic()
    {
        float x = GetRandomValue(0, cellCount - 1);
        float y = GetRandomValue(0, cellCount - 1);
        return Vector2{x, y};
    }
};

// --- ExplosiveFood Class ---
class ExplosiveFood {
public:
    MapBase* map;
private:
    Vector2 position;
    int points;
    time_t spawnTime;
    const int duration = 7;
    const int basePoints = 100;
    bool isActive;

public:
    ExplosiveFood(MapBase* map = nullptr) : map(map) {
        isActive = false;
        points = basePoints;
        spawnTime = 0;
        position = {0, 0};
    }

    bool shouldSpawn(int foodEatenCount) {
        return foodEatenCount % 5 == 0 && !isActive;
    }

    void spawn(deque<Vector2> snakeBody) {
        position = Food::GenerateRandomPosStatic(snakeBody, map);
        points = basePoints;
        spawnTime = time(nullptr);
        isActive = true;
    }

    void update() {
        if (!isActive) return;

        double elapsedTime = difftime(time(nullptr), spawnTime);
        points = basePoints * pow(0.9, elapsedTime);
        if (elapsedTime >= duration) {
            isActive = false;
        }
    }

    void Draw() {
        if (isActive) {
            Rectangle rect = {offset + position.x * cellSize, offset + position.y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(rect, 0.5, 6, explosiveFoodColor);
            string pointsText = to_string(points);
            int textWidth = MeasureText(pointsText.c_str(), 20);
            DrawText(pointsText.c_str(), offset + position.x * cellSize + cellSize / 2 - textWidth / 2,
                     offset + position.y * cellSize - 20, 20, WHITE);
        }
    }

    bool isFoodActive() const {
        return isActive;
    }

    Vector2 getPosition() const {
        return position;
    }

    int getPoints() const {
        return points;
    }

    void eat() {
        isActive = false;
    }
};

// --- Snake Class ---
class Snake
{
public:
    deque<Vector2> body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
    Vector2 direction = {1, 0};
    bool addSegment = false;

    void Draw()
    {
        for (unsigned int i = 0; i < body.size(); i++)
        {
            float x = body[i].x;
            float y = body[i].y;
            Rectangle segment = Rectangle{offset + x * cellSize, offset + y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(segment, 0.5, 6, darkGreen);
        }
    }

    void Update()
    {
        body.push_front(Vector2Add(body[0], direction));
        if (addSegment == true)
        {
            addSegment = false;
        }
        else
        {
            body.pop_back();
        }
    }

    void Reset()
    {
        body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
        direction = {1, 0};
    }
};

// --- Game Class ---
class Game
{
public:
    Snake snake = Snake();
    Food food;
    ExplosiveFood explosiveFood;
    HardModeMap* hardMap = nullptr;
    bool running = false;
    int score = 0;
    int highestscore = 0;
    int foodEatenCount = 0;
    Sound eatSound;
    Sound wallSound;
    Sound selectSound;
    Sound menuMoveSound;
    Sound gameStartSound;
    Sound gameOverSound;
    Sound menuenterSound;
    Sound menuEnterEzSound;
    Sound menuEnterHardSound;
    Sound explosiveEatSound;
    bool gameovermenu = false;

    Game() : food(snake.body, nullptr), explosiveFood(nullptr)
    {
        InitAudioDevice();
        eatSound = LoadSound("Sounds/eat.mp3");
        wallSound = LoadSound("Sounds/wall.mp3");
        selectSound = LoadSound("Sounds/select.mp3");
        menuMoveSound = LoadSound("Sounds/menumove.mp3");
        gameStartSound = LoadSound("Sounds/gamestart.mp3");
        gameOverSound = LoadSound("Sounds/gameover.mp3");
        menuenterSound = LoadSound("Sounds/menuenter.mp3");
        menuEnterEzSound = LoadSound("Sounds/menuenterEz.mp3");
        menuEnterHardSound = LoadSound("Sounds/menuenterHard.mp3");
        explosiveEatSound = LoadSound("Sounds/explosive_eat.mp3");
        highestscore = loadhighestscore();
    }

    ~Game()
    {
        UnloadSound(eatSound);
        UnloadSound(wallSound);
        UnloadSound(selectSound);
        UnloadSound(menuMoveSound);
        UnloadSound(gameStartSound);
        UnloadSound(gameOverSound);
        UnloadSound(menuenterSound);
        UnloadSound(menuEnterEzSound);
        UnloadSound(menuEnterHardSound); 
        UnloadSound(explosiveEatSound);
        if (hardMap) delete hardMap;
        CloseAudioDevice();
    }

    void InitializeHardMode()
    {
        if (!hardMap) {
            hardMap = new HardModeMap(cellSize);
        }
        food.map = hardMap;
        explosiveFood.map = hardMap;
        food.position = food.GenerateRandomPos(snake.body);
        explosiveFood.eat();
    }

    void DisableHardMode()
    {
        if (hardMap) {
            delete hardMap;
            hardMap = nullptr;
        }
        food.map = nullptr;
        explosiveFood.map = nullptr;
        food.position = food.GenerateRandomPos(snake.body);
        explosiveFood.eat();
    }

    void Draw()
    {
        if (hardMap) hardMap->Draw();
        food.Draw();
        explosiveFood.Draw();
        snake.Draw();
    }

    int loadhighestscore()
    {
        FILE* file = fopen("highestscore.txt", "r");
        if (file != NULL)
        {
            fscanf(file, "%d", &highestscore);
            fclose(file);
        }
        else
        {
            file = fopen("highestscore.txt", "w");
            if (file != NULL)
            {
                fprintf(file, "%d", 0);
                fclose(file);
            }
            highestscore = 0;
        }
        return highestscore;
    }

    void savehighestscore()
    {
        FILE* file = fopen("highestscore.txt", "w");
        if (file != NULL)
        {
            fprintf(file, "%d", highestscore);
            fclose(file);
        }
        else
        {
            printf("Error: Could not open highestscore.txt for writing\n");
        }
    }

    void resetScores()
    {
        score = 0;
        highestscore = 0;
        foodEatenCount = 0;
        savehighestscore();
    }

    void resetCurrentScore()
    {
        score = 0;
        foodEatenCount = 0;
    }

    void Update()
    {
        if (running)
        {
            snake.Update();
            explosiveFood.update();
            CheckCollisionWithFood();
            CheckCollisionWithExplosiveFood();
            CheckCollisionWithEdges();
            CheckCollisionWithTail();
        }
    }

    void CheckCollisionWithFood()
    {
        if (Vector2Equals(snake.body[0], food.position))
        {
            food.position = food.GenerateRandomPos(snake.body);
            snake.addSegment = true;
            score++;
            foodEatenCount++;
            PlaySound(eatSound);
            if (explosiveFood.shouldSpawn(foodEatenCount))
            {
                explosiveFood.spawn(snake.body);
            }
        }
    }

    void CheckCollisionWithExplosiveFood()
    {
        if (explosiveFood.isFoodActive() && Vector2Equals(snake.body[0], explosiveFood.getPosition()))
        {
            score += explosiveFood.getPoints();
            explosiveFood.eat();
            snake.addSegment = true;
            PlaySound(explosiveEatSound);
        }
    }

    void CheckCollisionWithEdges()
    {
        if (!isHardMode || !hardMap)
        {
            if (snake.body[0].x >= cellCount || snake.body[0].x < 0 ||
                snake.body[0].y >= cellCount || snake.body[0].y < 0)
            {
                PlaySound(wallSound);
                GameOver();
            }
        }
        if (hardMap)
        {
            Rectangle snakeHead = {offset + snake.body[0].x * cellSize, offset + snake.body[0].y * cellSize, (float)cellSize, (float)cellSize};
            if (hardMap->CheckCollisionWithRect(snakeHead)||snake.body[0].x >= cellCount || snake.body[0].x < 0 ||
                snake.body[0].y >= cellCount || snake.body[0].y < 0)
            {
                PlaySound(wallSound);
                GameOver();
            }
        }
    }

    void GameOver()
    {
        if (score > highestscore)
        {
            highestscore = score;
            savehighestscore();
        }
        snake.Reset();
        food.position = food.GenerateRandomPos(snake.body);
        explosiveFood.eat();
        running = false;
        gameovermenu = true;
        PlaySound(gameOverSound);
    }

    void CheckCollisionWithTail()
    {
        deque<Vector2> headlessBody = snake.body;
        headlessBody.pop_front();
        if (ElementInDeque(snake.body[0], headlessBody))
        {
            GameOver();
        }
    }
};

// --- GameScreen Class ---
class GameScreen {
public:
    enum ScreenType {
        MENU,
        DIFFICULTY_SELECTION,
        GAME,
        PAUSED,
        GAME_OVER
    };

private:
    ScreenType currentScreen;

public:
    GameScreen(ScreenType screen = MENU) : currentScreen(screen) {}

    ScreenType GetScreen() const { return currentScreen; }
    void SetScreen(ScreenType screen) { currentScreen = screen; }

    bool operator==(ScreenType screen) const { return currentScreen == screen; }
    bool operator!=(ScreenType screen) const { return currentScreen != screen; }

    string ToString() const {
        switch (currentScreen) {
            case MENU: return "MENU";
            case DIFFICULTY_SELECTION: return "DIFFICULTY_SELECTION";
            case GAME: return "GAME";
            case PAUSED: return "PAUSED";
            case GAME_OVER: return "GAME_OVER";
            default: return "MENU";
        }
    }
};

// --- Button Class ---
class Button {
private:
    Rectangle rect;
    string text;
    Color baseColor;
    Color hoverColor;
    Color textColor;
    bool isSelected;
    bool isHovered;

public:
    Button(Rectangle r, string t, Color base, Color hover, Color text, bool selected = false, bool hovered = false)
        : rect(r), text(t), baseColor(base), hoverColor(hover), textColor(text), 
          isSelected(selected), isHovered(hovered) {}

    void Draw() {
        DrawRectangleRounded(rect, 0.5, 6, (isHovered || isSelected) ? hoverColor : baseColor);
        int textWidth = MeasureText(text.c_str(), 30);
        DrawText(text.c_str(), rect.x + rect.width / 2 - textWidth / 2, rect.y + rect.height / 2 - 15, 30, textColor);
    }

    bool IsClicked() {
        if (CheckCollisionPointRec(GetMousePosition(), rect)) {
            isHovered = true;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                return true;
            }
        } else {
            isHovered = false;
        }
        return false;
    }

    Rectangle& GetRect() { return rect; }
    bool IsSelected() const { return isSelected; }
    void SetSelected(bool selected) { isSelected = selected; }
    bool IsHovered() const { return isHovered; }
};

// --- Main Function ---
int main()
{
    int screenWidth = 2 * offset + cellSize * cellCount;
    int screenHeight = 2 * offset + cellSize * cellCount;

    InitWindow(screenWidth, screenHeight, "Retro Snake");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    Game game;
    GameScreen currentScreen(GameScreen::MENU);
    bool initialMenuEntry = true;

    // --- Initialize Main Menu Buttons ---
    int mainButtonWidth = 250;
    int mainButtonHeight = 60;
    int mainButtonSpacing = 30;
    int mainStartY = screenHeight / 2 - (mainButtonHeight + mainButtonSpacing) / 2;

    Button playButton(
        {(float)screenWidth / 2 - mainButtonWidth / 2, (float)mainStartY, (float)mainButtonWidth, (float)mainButtonHeight},
        "PLAY",
        (Color){145, 221, 60, 255},
        (Color){175, 251, 90, 255},
        darkGreen,
        true
    );
    Button exitButton(
        {(float)screenWidth / 2 - mainButtonWidth / 2, (float)mainStartY + (mainButtonHeight + mainButtonSpacing), (float)mainButtonWidth, (float)mainButtonHeight},
        "EXIT",
        (Color){145, 221, 60, 255},
        (Color){175, 251, 90, 255},
        darkGreen
    );
    vector<Button*> menuButtons = {&playButton, &exitButton};
    int selectedMenuButtonIndex = 0;

    // --- Initialize Game Over Buttons ---
    int goButtonWidth = 250;
    int goButtonHeight = 50;
    int goButtonSpacing = 20;
    int goStartY = screenHeight / 2 + 50;

    Button retryButton(
        {(float)screenWidth / 2 - goButtonWidth / 2, (float)goStartY, (float)goButtonWidth, (float)goButtonHeight},
        "RETRY",
        (Color){145, 221, 60, 255},
        (Color){175, 251, 90, 255},
        darkGreen,
        true
    );
    Button goMenuButton(
        {(float)screenWidth / 2 - goButtonWidth / 2, (float)goStartY + (goButtonHeight + goButtonSpacing), (float)goButtonWidth, (float)goButtonHeight},
        "MAIN MENU",
        (Color){145, 221, 60, 255},
        (Color){175, 251, 90, 255},
        darkGreen
    );
    vector<Button*> gameOverButtons = {&retryButton, &goMenuButton};
    int selectedGameOverButtonIndex = 0;

    // --- Initialize Difficulty Selection Buttons ---
    int difficultyButtonWidth = 250;
    int difficultyButtonHeight = 60;
    int difficultyButtonSpacing = 30;
    int difficultyStartY = screenHeight / 2 - (difficultyButtonHeight + difficultyButtonSpacing);

    Button easyButton(
        {(float)screenWidth / 2 - difficultyButtonWidth / 2, (float)difficultyStartY, (float)difficultyButtonWidth, (float)difficultyButtonHeight},
        "EASY",
        (Color){145, 221, 60, 255},
        (Color){175, 251, 90, 255},
        darkGreen,
        true
    );
    Button hardButton(
        {(float)screenWidth / 2 - difficultyButtonWidth / 2, (float)difficultyStartY + (difficultyButtonHeight + difficultyButtonSpacing), (float)difficultyButtonWidth, (float)difficultyButtonHeight},
        "HARD",
        (Color){145, 221, 60, 255},
        (Color){175, 251, 90, 255},
        darkGreen
    );
    Button backButton(
        {(float)screenWidth / 2 - difficultyButtonWidth / 2, (float)difficultyStartY + 2 * (difficultyButtonHeight + difficultyButtonSpacing), (float)difficultyButtonWidth, (float)difficultyButtonHeight},
        "BACK",
        (Color){145, 221, 60, 255},
        (Color){175, 251, 90, 255},
        darkGreen
    );
    vector<Button*> difficultyButtons = {&easyButton, &hardButton, &backButton};
    int selectedDifficultyButtonIndex = 0;

    // --- Initialize Pause Menu Buttons ---
    int pauseButtonWidth = 250;
    int pauseButtonHeight = 60;
    int pauseButtonSpacing = 30;
    int pauseStartY = screenHeight / 2 - (pauseButtonHeight + pauseButtonSpacing) / 2;

    Button resumeButton(
        {(float)screenWidth / 2 - pauseButtonWidth / 2, (float)pauseStartY, (float)pauseButtonWidth, (float)pauseButtonHeight},
        "RESUME",
        (Color){145, 221, 60, 255},
        (Color){175, 251, 90, 255},
        darkGreen,
        true
    );
    Button pauseMenuButton(
        {(float)screenWidth / 2 - pauseButtonWidth / 2, (float)pauseStartY + (pauseButtonHeight + pauseButtonSpacing), (float)pauseButtonWidth, (float)pauseButtonHeight},
        "MAIN MENU",
        (Color){145, 221, 60, 255},
        (Color){175, 251, 90, 255},
        darkGreen
    );
    vector<Button*> pauseButtons = {&resumeButton, &pauseMenuButton};
    int selectedPauseButtonIndex = 0;

    // --- Title Colors ---
    Color titleColors[] = {
        (Color){0, 121, 241, 255},
        (Color){0, 173, 239, 255},
        (Color){0, 204, 255, 255},
        (Color){102, 0, 204, 255},
        (Color){153, 51, 255, 255},
        (Color){204, 0, 102, 255},
        (Color){255, 0, 0, 255},
        (Color){255, 128, 0, 255},
        (Color){255, 255, 0, 255},
        (Color){102, 255, 0, 255},
        (Color){0, 204, 0, 255},
        (Color){0, 153, 0, 255}
    };
    int numTitleColors = sizeof(titleColors) / sizeof(titleColors[0]);

    bool shouldExit = false;

    while (!shouldExit && !WindowShouldClose())
    {
         if (WindowShouldClose())
        {
            cout << "WindowShouldClose triggered. ESC pressed: " << IsKeyPressed(KEY_ESCAPE) << endl;
        }

        BeginDrawing();
        ClearBackground(green);

        // --- MENU Screen ---
        if (currentScreen == GameScreen::MENU)
        {
            if (initialMenuEntry)
            {
                PlaySound(game.menuenterSound);
                initialMenuEntry = false;
            }

            string titleText = "RETRO SNAKE";
            int titleFontSize = 60;
            int totalTextWidth = MeasureText(titleText.c_str(), titleFontSize);
            int startX = screenWidth / 2 - totalTextWidth / 2;
            int startY = screenHeight / 4;

            int currentX = startX;
            for (size_t i = 0; i < titleText.length(); ++i)
            {
                char currentLetter[2] = {titleText[i], '\0'};
                Color color = titleColors[i % numTitleColors];
                DrawText(currentLetter, currentX, startY, titleFontSize, color);
                currentX += MeasureText(currentLetter, titleFontSize);
            }

            playButton.GetRect().y = mainStartY;
            exitButton.GetRect().y = mainStartY + (mainButtonHeight + mainButtonSpacing);

            for (Button* btn : menuButtons)
            {
                btn->Draw();
                if (btn->IsClicked())
                {
                    PlaySound(game.selectSound);
                    if (btn == &playButton)
                    {
                        currentScreen.SetScreen(GameScreen::DIFFICULTY_SELECTION);
                        difficultyButtons[0]->SetSelected(true);
                        difficultyButtons[1]->SetSelected(false);
                        difficultyButtons[2]->SetSelected(false);
                        selectedDifficultyButtonIndex = 0;
                    }
                    else if (btn == &exitButton)
                    {
                        game.resetScores();
                        shouldExit = true;
                    }
                }
            }

            const char* snakeAscii[] = {
                "     ____ ",
                " >-( __o )  23CVD",
                "     / /      Nhóm 9        ~",
                "   / //\\/\\/\\/\\",
                "  (___/\\/\\/\\/\\"
            };

            int snakeFontSize = 30;
            int snakeStartY = mainStartY + (mainButtonHeight + mainButtonSpacing) + mainButtonHeight + 20;
            for (int i = 0; i < 5; i++)
            {
                int snakeX = offset;
                int snakeY = snakeStartY + i * (snakeFontSize + 2);
                DrawText(snakeAscii[i], snakeX, snakeY, snakeFontSize, darkGreen);
            }

            if (IsKeyPressed(KEY_DOWN))
            {
                menuButtons[selectedMenuButtonIndex]->SetSelected(false);
                selectedMenuButtonIndex = (selectedMenuButtonIndex + 1) % menuButtons.size();
                menuButtons[selectedMenuButtonIndex]->SetSelected(true);
                PlaySound(game.menuMoveSound);
            }
            if (IsKeyPressed(KEY_UP))
            {
                menuButtons[selectedMenuButtonIndex]->SetSelected(false);
                selectedMenuButtonIndex = (selectedMenuButtonIndex - 1 + menuButtons.size()) % menuButtons.size();
                menuButtons[selectedMenuButtonIndex]->SetSelected(true);
                PlaySound(game.menuMoveSound);
            }
            if (IsKeyPressed(KEY_ENTER))
            {
                PlaySound(game.selectSound);
                if (selectedMenuButtonIndex == 0)
                {
                    currentScreen.SetScreen(GameScreen::DIFFICULTY_SELECTION);
                    difficultyButtons[0]->SetSelected(true);
                    difficultyButtons[1]->SetSelected(false);
                    difficultyButtons[2]->SetSelected(false);
                    selectedDifficultyButtonIndex = 0;
                }
                else if (selectedMenuButtonIndex == 1)
                {
                    game.resetScores();
                    shouldExit = true;
                }
            }
        }
        // --- DIFFICULTY SELECTION Screen ---
        else if (currentScreen == GameScreen::DIFFICULTY_SELECTION)
        {
            int titleFontSize = 50;
            int textWidth = MeasureText("CHOOSE DIFFICULTY", titleFontSize);
            DrawText("CHOOSE DIFFICULTY", screenWidth / 2 - textWidth / 2, screenHeight / 4, titleFontSize, darkGreen);

            if (IsKeyPressed(KEY_DOWN))
            {
                difficultyButtons[selectedDifficultyButtonIndex]->SetSelected(false);
                selectedDifficultyButtonIndex = (selectedDifficultyButtonIndex + 1) % difficultyButtons.size();
                difficultyButtons[selectedDifficultyButtonIndex]->SetSelected(true);
                PlaySound(game.menuMoveSound);
            }
            if (IsKeyPressed(KEY_UP))
            {
                difficultyButtons[selectedDifficultyButtonIndex]->SetSelected(false);
                selectedDifficultyButtonIndex = (selectedDifficultyButtonIndex - 1 + difficultyButtons.size()) % difficultyButtons.size();
                difficultyButtons[selectedDifficultyButtonIndex]->SetSelected(true);
                PlaySound(game.menuMoveSound);
            }
            if (IsKeyPressed(KEY_ENTER))
            {
                PlaySound(game.selectSound);
                if (selectedDifficultyButtonIndex == 0) // EASY
                {
                    isHardMode = false;
                    gameSpeed = 0.2;
                    game.DisableHardMode();
                    game.snake.Reset();
                    game.food.position = game.food.GenerateRandomPos(game.snake.body);
                    game.explosiveFood.eat();
                    game.resetCurrentScore();
                    game.running = true;
                    game.gameovermenu = false;
                    allowMove = true;
                    StopSound(game.menuenterSound);
                    
                    currentScreen.SetScreen(GameScreen::GAME);
                    PlaySound(game.gameStartSound);
                     PlaySound(game.menuEnterEzSound);
                    
                }
                else if (selectedDifficultyButtonIndex == 1) // HARD
                {
                    isHardMode = true;
                    gameSpeed = 0.1;
                    game.InitializeHardMode();
                    game.snake.Reset();
                    game.food.position = game.food.GenerateRandomPos(game.snake.body);
                    game.explosiveFood.eat();
                    game.resetCurrentScore();
                    game.running = true;
                    game.gameovermenu = false;
                    allowMove = true;
                    StopSound(game.menuenterSound);
                    currentScreen.SetScreen(GameScreen::GAME);
                    PlaySound(game.gameStartSound);
                    PlaySound(game.menuEnterHardSound);
                }
                else if (selectedDifficultyButtonIndex == 2) // BACK
                {
                    game.resetScores();
                    game.DisableHardMode();
                    currentScreen.SetScreen(GameScreen::MENU);
                    menuButtons[0]->SetSelected(true);
                    menuButtons[1]->SetSelected(false);
                    selectedMenuButtonIndex = 0;
                    PlaySound(game.menuenterSound);
                }
            }

            for (Button* btn : difficultyButtons)
            {
                btn->Draw();
                if (btn->IsClicked())
                {
                    PlaySound(game.selectSound);
                    if (btn == &easyButton)
                    {
                        isHardMode = false;
                        gameSpeed = 0.2;
                        game.DisableHardMode();
                        game.snake.Reset();
                        game.food.position = game.food.GenerateRandomPos(game.snake.body);
                        game.explosiveFood.eat();
                        game.resetCurrentScore();
                        game.running = true;
                        game.gameovermenu = false;
                        allowMove = true;
                        StopSound(game.menuenterSound);
                        
                        currentScreen.SetScreen(GameScreen::GAME);
                        
                        PlaySound(game.gameStartSound);
                        PlaySound(game.menuEnterEzSound);
                        
                        
                    }
                    else if (btn == &hardButton)
                    {
                        isHardMode = true;
                        gameSpeed = 0.1;
                        game.InitializeHardMode();
                        game.snake.Reset();
                        game.food.position = game.food.GenerateRandomPos(game.snake.body);
                        game.explosiveFood.eat();
                        game.resetCurrentScore();
                        game.running = true;
                        game.gameovermenu = false;
                        allowMove = true;
                        StopSound(game.menuenterSound);
                        currentScreen.SetScreen(GameScreen::GAME);
                        PlaySound(game.gameStartSound);
                        PlaySound(game.menuEnterHardSound);
                        

                    }
                    else if (btn == &backButton)
                    {
                        game.resetScores();
                        game.DisableHardMode();
                        currentScreen.SetScreen(GameScreen::MENU);
                        menuButtons[0]->SetSelected(true);
                        menuButtons[1]->SetSelected(false);
                        selectedMenuButtonIndex = 0;
                        PlaySound(game.menuenterSound);
                    }
                }
            }

            
        }
        // --- GAME Screen ---
        else if (currentScreen == GameScreen::GAME)
        {
            if (EventTriggered(gameSpeed))
            {
                allowMove = true;
                game.Update();
            }

            if (IsKeyPressed(KEY_UP) && game.snake.direction.y != 1 && allowMove)
            {
                game.snake.direction = {0, -1};
                allowMove = false;
            }
            if (IsKeyPressed(KEY_DOWN) && game.snake.direction.y != -1 && allowMove)
            {
                game.snake.direction = {0, 1};
                allowMove = false;
            }
            if (IsKeyPressed(KEY_LEFT) && game.snake.direction.x != 1 && allowMove)
            {
                game.snake.direction = {-1, 0};
                allowMove = false;
            }
            if (IsKeyPressed(KEY_RIGHT) && game.snake.direction.x != -1 && allowMove)
            {
                game.snake.direction = {1, 0};
                allowMove = false;
            }

            if (IsKeyPressed(KEY_ESCAPE))
            {
                cout << "ESC pressed in GAME, switching to PAUSED" << endl;
                PlaySound(game.selectSound);
                currentScreen.SetScreen(GameScreen::PAUSED);
                game.running = false;
                pauseButtons[0]->SetSelected(true);
                pauseButtons[1]->SetSelected(false);
                selectedPauseButtonIndex = 0;
            }


            DrawRectangleLinesEx(Rectangle{(float)offset - 5, (float)offset - 5, (float)cellSize * cellCount + 10, (float)cellSize * cellCount + 10}, 5, darkGreen);
            string gameTitleText = "RETRO SNAKE";
            int gameTitleFontSize = 40;
            int gameStartX = offset - 5;
            int gameStartY = 20;
            int currentGameX = gameStartX;
            for (size_t i = 0; i < gameTitleText.length(); ++i)
            {
                char currentLetter[2] = {gameTitleText[i], '\0'};
                Color color = titleColors[i % numTitleColors];
                DrawText(currentLetter, currentGameX, gameStartY, gameTitleFontSize, color);
                currentGameX += MeasureText(currentLetter, gameTitleFontSize);
            }
            DrawText(TextFormat("Score: %i", game.score), offset - 5, offset + cellSize * cellCount + 10, 40, darkGreen);
            int highestScoreTextWidth = MeasureText(TextFormat("Highest Score: %i", game.highestscore), 40);
            int xPos = (2 * offset + cellSize * cellCount) - highestScoreTextWidth - 10;
            DrawText(TextFormat("Highest Score: %i", game.highestscore), xPos, offset + cellSize * cellCount + 30, 40, darkGreen);
            game.Draw();

            if (game.gameovermenu)
            {
                currentScreen.SetScreen(GameScreen::GAME_OVER);
                gameOverButtons[0]->SetSelected(true);
                gameOverButtons[1]->SetSelected(false);
                selectedGameOverButtonIndex = 0;
                StopSound(game.menuEnterEzSound);
                StopSound(game.menuEnterHardSound);
            }
        }
        // --- PAUSED Screen ---
        else if (currentScreen == GameScreen::PAUSED)
        {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));

            int panelWidth = 500;
            int panelHeight = 400;
            int panelX = screenWidth / 2 - panelWidth / 2;
            int panelY = screenHeight / 2 - panelHeight / 2;
            DrawRectangleRounded(Rectangle{(float)panelX, (float)panelY, (float)panelWidth, (float)panelHeight}, 0.2, 10, darkGreen);
            DrawRectangleLinesEx(Rectangle{(float)panelX, (float)panelY, (float)panelWidth, (float)panelHeight}, 4, WHITE);
            DrawText("PAUSED", panelX + panelWidth / 2 - MeasureText("PAUSED", 50) / 2, panelY + 40, 50, (Color){145, 221, 60, 255});

            resumeButton.GetRect().x = panelX + panelWidth / 2 - resumeButton.GetRect().width / 2;
            resumeButton.GetRect().y = panelY + 150;
            pauseMenuButton.GetRect().x = panelX + panelWidth / 2 - pauseMenuButton.GetRect().width / 2;
            pauseMenuButton.GetRect().y = resumeButton.GetRect().y + resumeButton.GetRect().height + pauseButtonSpacing;

            if (IsKeyPressed(KEY_DOWN))
            {
                pauseButtons[selectedPauseButtonIndex]->SetSelected(false);
                selectedPauseButtonIndex = (selectedPauseButtonIndex + 1) % pauseButtons.size();
                pauseButtons[selectedPauseButtonIndex]->SetSelected(true);
                PlaySound(game.menuMoveSound);
            }
            if (IsKeyPressed(KEY_UP))
            {
                pauseButtons[selectedPauseButtonIndex]->SetSelected(false);
                selectedPauseButtonIndex = (selectedPauseButtonIndex - 1 + pauseButtons.size()) % pauseButtons.size();
                pauseButtons[selectedPauseButtonIndex]->SetSelected(true);
                PlaySound(game.menuMoveSound);
            }
            if (IsKeyPressed(KEY_ENTER))
            {
                PlaySound(game.selectSound);
                if (selectedPauseButtonIndex == 0)
                {
                    currentScreen.SetScreen(GameScreen::GAME);
                    game.running = true;
                }
                else if (selectedPauseButtonIndex == 1)
                {
                    game.resetScores();
                    game.DisableHardMode();
                    currentScreen.SetScreen(GameScreen::MENU);
                    game.running = false;
                    menuButtons[0]->SetSelected(true);
                    menuButtons[1]->SetSelected(false);
                    selectedMenuButtonIndex = 0;
                    PlaySound(game.menuenterSound);
                }
            }

            for (Button* btn : pauseButtons)
            {
                btn->Draw();
                if (btn->IsClicked())
                {
                    PlaySound(game.selectSound);
                    if (btn == &resumeButton)
                    {
                        currentScreen.SetScreen(GameScreen::GAME);
                        game.running = true;
                    }
                    else if (btn == &pauseMenuButton)
                    {
                        game.resetScores();
                        game.DisableHardMode();
                        currentScreen.SetScreen(GameScreen::MENU);
                        game.running = false;
                        menuButtons[0]->SetSelected(true);
                        menuButtons[1]->SetSelected(false);
                        selectedMenuButtonIndex = 0;
                        PlaySound(game.menuenterSound);
                    }
                }
            }

           
        }
        // --- GAME OVER Screen ---
        else if (currentScreen == GameScreen::GAME_OVER)
        {
            
            int panelWidth = 500;
            int panelHeight = 400;
            int panelX = screenWidth / 2 - panelWidth / 2;
            int panelY = screenHeight / 2 - panelHeight / 2;

            DrawRectangleRounded(Rectangle{(float)panelX, (float)panelY, (float)panelWidth, (float)panelHeight}, 0.2, 10, darkGreen);
            DrawRectangleLinesEx(Rectangle{(float)panelX, (float)panelY, (float)panelWidth, (float)panelHeight}, 4, WHITE);
            DrawText("GAME OVER", panelX + panelWidth / 2 - MeasureText("GAME OVER", 50) / 2, panelY + 40, 50, (Color){255, 0, 0, 255});

            int scoreFontSize = 30;
            int yourScoreY = panelY + 120;
            int highestScoreY = panelY + 160;

            string yourScoreStr = "Your Score: " + to_string(game.score);
            string highestScoreStr = "Highest Score: " + to_string(game.highestscore);

            DrawText(yourScoreStr.c_str(), panelX + panelWidth / 2 - MeasureText(yourScoreStr.c_str(), scoreFontSize) / 2, yourScoreY, scoreFontSize, WHITE);
            DrawText(highestScoreStr.c_str(), panelX + panelWidth / 2 - MeasureText(highestScoreStr.c_str(), scoreFontSize) / 2, highestScoreY, scoreFontSize, WHITE);

            retryButton.GetRect().x = panelX + panelWidth / 2 - retryButton.GetRect().width / 2;
            retryButton.GetRect().y = panelY + 230;
            goMenuButton.GetRect().x = panelX + panelWidth / 2 - goMenuButton.GetRect().width / 2;
            goMenuButton.GetRect().y = retryButton.GetRect().y + retryButton.GetRect().height + goButtonSpacing;

            if (IsKeyPressed(KEY_DOWN))
            {
                gameOverButtons[selectedGameOverButtonIndex]->SetSelected(false);
                selectedGameOverButtonIndex = (selectedGameOverButtonIndex + 1) % gameOverButtons.size();
                gameOverButtons[selectedGameOverButtonIndex]->SetSelected(true);
                PlaySound(game.menuMoveSound);
            }
            if (IsKeyPressed(KEY_UP))
            {
                gameOverButtons[selectedGameOverButtonIndex]->SetSelected(false);
                selectedGameOverButtonIndex = (selectedGameOverButtonIndex - 1 + gameOverButtons.size()) % gameOverButtons.size();
                gameOverButtons[selectedGameOverButtonIndex]->SetSelected(true);
                PlaySound(game.menuMoveSound);
                
            }
            if (IsKeyPressed(KEY_ENTER))
            {
                PlaySound(game.selectSound);
                if (selectedGameOverButtonIndex == 0) // RESTART
                {
                    if (isHardMode) game.InitializeHardMode();
                    else game.DisableHardMode();
                    game.snake.Reset();
                    game.food.position = game.food.GenerateRandomPos(game.snake.body);
                    game.explosiveFood.eat();
                    game.resetCurrentScore();
                    game.running = true;
                    game.gameovermenu = false;
                    allowMove = true;
                    
                    currentScreen.SetScreen(GameScreen::GAME);
                    PlaySound(game.gameStartSound);
                    
                }
                else if (selectedGameOverButtonIndex == 1) // MENU
                {
                    game.resetScores();
                    game.DisableHardMode();
                    currentScreen.SetScreen(GameScreen::MENU);
                    game.gameovermenu = false;
                    menuButtons[0]->SetSelected(true);
                    menuButtons[1]->SetSelected(false);
                    selectedMenuButtonIndex = 0;
                    PlaySound(game.menuenterSound);
                }
            }

            for (Button* btn : gameOverButtons)
            {
                    btn->Draw();
                    if (btn->IsClicked())
                    {
                        PlaySound(game.selectSound);
                        if (btn == &retryButton)
                        {
                            if (isHardMode){
                             PlaySound(game.menuEnterHardSound);
                             game.InitializeHardMode();
                            }
                            else{
                             PlaySound(game.menuEnterEzSound);
                             game.DisableHardMode();
                            }
                            game.snake.Reset();
                            game.food.position = game.food.GenerateRandomPos(game.snake.body);
                            game.explosiveFood.eat();
                            game.resetCurrentScore();
                            game.running = true;
                            game.gameovermenu = false;
                            allowMove = true;
                            currentScreen.SetScreen(GameScreen::GAME);
                            PlaySound(game.gameStartSound);
                        }
                        else if (btn == &goMenuButton)
                        {
                            game.resetScores();
                            game.DisableHardMode();
                            currentScreen.SetScreen(GameScreen::MENU);
                            game.gameovermenu = false;
                            menuButtons[0]->SetSelected(true);
                            menuButtons[1]->SetSelected(false);
                            selectedMenuButtonIndex = 0;
                            PlaySound(game.menuenterSound);
                        }
                    }
                }

            
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}