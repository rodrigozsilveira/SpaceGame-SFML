#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <optional>
#include <cstdlib>
#include <SFML/Window/Event.hpp>
#include <SFML/Audio.hpp>
#include <map>

using namespace sf;

unsigned int width = 800;
unsigned int height = 800;

const float ASTEROID_SPAWN_TIME = 3.0F;
const float BULLET_COOLDONW = 0.5f;

const float ACCELERATION = 0.2f;
const float DRAG = 0.02f;
const float MAX_SPEED = 6.0f;

bool isGameStarted;

using namespace sf;

class Animation {
private:
    sf::Sprite sprite;
    sf::Clock clock;
    bool finished = false;
    int currentFrame = 0;
    float frameDuration;
    int frameCount;
    int frameWidth;
    int frameHeight;
    bool loopable;

public:

    Animation(sf::Texture& texture, sf::Vector2f position, int frameCount, int frameWidth, int frameHeight, float frameTime = 0.1f, bool loopable = false)
        : sprite(texture), frameCount(frameCount), frameWidth(frameWidth), frameHeight(frameHeight), frameDuration(frameTime), loopable(loopable) {
        sprite.setOrigin({ frameWidth / 2.0f, frameHeight / 2.0f });
        sprite.setScale({ 4.0f, 4.0f });
        sprite.setPosition(position);
        sprite.setTextureRect(IntRect({ 0, 0 }, { frameWidth, frameHeight }));
    }

    void update() {
        if (finished) return;

        if (clock.getElapsedTime().asSeconds() >= frameDuration) {
            clock.restart();
            currentFrame++;

            if (currentFrame >= frameCount) {
                if (loopable) {
                    currentFrame = 0;
                }
                else {
                    finished = true;
                    return;
                }
            }

            sprite.setTextureRect(IntRect({ currentFrame * frameWidth, 0 }, { frameWidth, frameHeight }));
        }
    }


    void draw(sf::RenderWindow& window) {
        if (!finished) {
            window.draw(sprite);
        }
    }

    bool isFinished() const {
        return finished;
    }
};

class Asteroid {
private:
    Sprite sprite;
    Vector2f velocity; 
    float rotationSpeed;
    float x_scale, y_scale;
    bool collided;
    bool large;

public:
    Asteroid(Texture& texture, float x_scale, float y_scale, bool islarge = false)
        : sprite(texture), x_scale(x_scale), y_scale(y_scale), collided(false), large(islarge) {

        sprite.setColor(Color(255, 255, 255, 255));
        sprite.setOrigin({ texture.getSize().x / 2.0f, texture.getSize().y / 2.0f });
        sprite.setScale({ x_scale, y_scale });
        rotationSpeed = 0.0f;
    }

    Sprite& getSprite() { return sprite; }
    Vector2f getVelocity() { return velocity; }
    float getScaleX() { return x_scale; }
    bool isLarge() { return large; }

    void resetCollisionFlag() { collided = false; }
    void setCollided() { collided = true; }
    bool hasCollided() const { return collided; }

    void setVelocity(Vector2f velocity) { this->velocity = velocity; }

    void appear(unsigned windowWidth, unsigned windowHeight) {
        int side = rand() % 4;
        float spawnX = 0, spawnY = 0;
        float centerX = windowWidth / 2.0f;
        float centerY = windowHeight / 2.0f;

        float offsetX = (rand() % 301 - 100);
        float offsetY = (rand() % 301 - 100);

        float targetX = centerX + offsetX;
        float targetY = centerY + offsetY;

        switch (side) {
        case 0: spawnX = 0; spawnY = rand() % static_cast<int>(windowHeight); break;
        case 1: spawnX = windowWidth; spawnY = rand() % static_cast<int>(windowHeight); break;
        case 2: spawnX = rand() % static_cast<int>(windowWidth); spawnY = 0; break;
        case 3: spawnX = rand() % static_cast<int>(windowWidth); spawnY = windowHeight; break;
        }
        sprite.setPosition({ spawnX, spawnY });

        float deltaX = targetX - spawnX;
        float deltaY = targetY - spawnY;
        float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);

        float speedMultiplier = 1.5f;  // Example multiplier to increase speed by 50%

        if (distance != 0) {
            float baseSpeed = (0.8f + (rand() % 100) / 100.0f * 0.5f);
            velocity.x = (deltaX / distance) * baseSpeed * speedMultiplier;
            velocity.y = (deltaY / distance) * baseSpeed * speedMultiplier;
        }

        rotationSpeed = (rand() % 5 + 1) * (rand() % 2 == 0 ? 1 : -1);
    }

    void update() {
        sprite.move(velocity);
        sprite.rotate(degrees(rotationSpeed));
    }

    void draw(sf::RenderWindow& window) {
        window.draw(sprite);
    }

    bool isOutOfScreen(float windowWidth, float windowHeight, float margin = 50.0f) {
        return sprite.getPosition().x < -margin || sprite.getPosition().x > windowWidth + margin ||
            sprite.getPosition().y < -margin || sprite.getPosition().y > windowHeight + margin;
    }
};

class Timer {
private:
    Text clockText;
    Clock clock;

public:
    Timer(const Font& font, unsigned int characterSize = 48, Vector2f position = { width / 2.0f, 55 }) : clockText(font) {
        clockText.setFont(font);
        clockText.setCharacterSize(characterSize);
        clockText.setFillColor(Color(200, 211, 253));
        clockText.setString("00:00");
        FloatRect textBounds = clockText.getLocalBounds();
        clockText.setOrigin(textBounds.getCenter());
        clockText.setPosition(position);
        clock.restart(); // Start the clock
    }

    // Returns time in seconds
    float getElapsedTime() const {
        return clock.getElapsedTime().asSeconds();
    }

    void update() {
        int elapsed = static_cast<int>(clock.getElapsedTime().asSeconds());
        int minutes = elapsed / 60;
        int seconds = elapsed % 60;

        std::ostringstream timeStream;
        timeStream << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << seconds;

        clockText.setString(timeStream.str());
    }

    void draw(RenderWindow& window) {
        window.draw(clockText);
    }
  
    void reset() {
        clock.restart(); // Restart the clock
        clockText.setString("00:00"); // Reset the displayed time
        update(); // Update to reflect the reset time
    }

};

class Bullet {
private:
    Sprite sprite;
    Vector2f velocity;
    Texture texture;
    float speed = 10.0f; // Speed of the bullet
    bool active = true;

public:
    Bullet(const Texture& texture, sf::Vector2f position, float angle) : sprite(texture){
        sprite.setTexture(texture);
        sprite.setOrigin({ texture.getSize().x / 2.0f, texture.getSize().y / 2.0f });
        sprite.setPosition(position);
        sprite.setRotation(degrees(angle));

        // Convert angle to radians
        float radian = (angle - 90) * 3.14159f / 180.0f; // SFML rotates clockwise, but we need a top-down angle

        velocity.x = std::cos(radian) * speed;
        velocity.y = std::sin(radian) * speed;
    }

    void update() {
        if (active) {
            sprite.move(velocity);
        }
    }

    void draw(sf::RenderWindow& window) {
        if (active) {
            window.draw(sprite);
        }
    }

    Sprite& getSprite() { return sprite; }

    bool isActive() const { return active; }
    void deactivate() { active = false; }
    sf::FloatRect getBounds() const { return sprite.getGlobalBounds(); }
};

class Spaceship {
protected:
    float orientation;
    Sprite sprite;
    Vector2f velocity;
    bool collide = true;
    Clock collisionTimer;
    unsigned int lives = 5;
    Texture lifeTexture;
    Texture damage_tex;

public:
    Sprite& getSprite() { return sprite; }

    Spaceship(const Texture& texture, const Texture& life_tex, const Texture& life_animation) : sprite(texture), lifeTexture(life_tex)  {
        sprite.setScale({ 4,6 });
        sprite.setTexture(texture);
        sprite.setOrigin({ texture.getSize().x / 2.0f, texture.getSize().y / 2.0f });
        sprite.setPosition({ width / 2.0f, height / 2.0f });
        orientation = 90.0f;
        sprite.setRotation(degrees(orientation));
    }

    void handleKBInput() {
        // Handle acceleration based on WASD keys
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W)) {
            velocity.y -= ACCELERATION;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S)) {
            velocity.y += ACCELERATION;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A)) {
            velocity.x -= ACCELERATION;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D)) {
            velocity.x += ACCELERATION;
        }

        // Apply drag
        if (velocity.x > 0) {
            velocity.x -= DRAG;
            if (velocity.x < 0) velocity.x = 0;
        }
        if (velocity.x < 0) {
            velocity.x += DRAG;
            if (velocity.x > 0) velocity.x = 0;
        }
        if (velocity.y > 0) {
            velocity.y -= DRAG;
            if (velocity.y < 0) velocity.y = 0;
        }
        if (velocity.y < 0) {
            velocity.y += DRAG;
            if (velocity.y > 0) velocity.y = 0;
        }

        // Limit speed
        if (std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y) > MAX_SPEED) {
            float angle = std::atan2(velocity.y, velocity.x);
            velocity.x = std::cos(angle) * MAX_SPEED;
            velocity.y = std::sin(angle) * MAX_SPEED;
        }
    }

    void reset() {
        // Reset position and rotation
        sprite.setPosition({ width / 2.0f, height / 2.0f });
        orientation = 90.0f;
        sprite.setRotation(degrees(orientation));

        // Reset movement
        velocity = { 0.f, 0.f };

        // Reset lives
        lives = 5;

        // Reset collision state
        collide = true;
        sprite.setColor(Color(255, 255, 255, 255)); // Full opacity
    }


    void displayLives(sf::RenderWindow& window) {
        for (int i = 0; i < lives; ++i) {
            Sprite lifeSprite(lifeTexture);
            lifeSprite.setOrigin({ 5, 5 });
            lifeSprite.setScale({ 3.0f, 3.0f }); // Scale smaller icons
            lifeSprite.setPosition({ 20.0f + (i * 40.0f), height - 50.0f }); // Offset each life icon
            window.draw(lifeSprite);

        }
    }

    void handleMouseInput(const Vector2i& mousePos) {
        // Get the position of the spaceship
        Vector2f spaceshipPos = sprite.getPosition();

        // Calculate the angle between the spaceship and the mouse cursor
        float deltaX = mousePos.x - spaceshipPos.x;
        float deltaY = mousePos.y - spaceshipPos.y;
        orientation = (std::atan2(deltaY, deltaX) * 180.0f / 3.14159f) + 90.f;
    }

    void update() {
        sprite.move(velocity);  // Apply movement (WASD)
        sprite.setRotation(degrees(orientation));  // Rotate the sprite towards the mouse

        // Check if spaceship goes out of bounds and teleport to the opposite side
        if (sprite.getPosition().x < 0) {
            sprite.setPosition({ static_cast<float>(width), sprite.getPosition().y });  // Teleport to the right side
        }
        else if (sprite.getPosition().x > static_cast<float>(width)) {
            sprite.setPosition({ 0, sprite.getPosition().y });  // Teleport to the left side
        }

        if (sprite.getPosition().y < 0) {
            sprite.setPosition({ sprite.getPosition().x, static_cast<float>(height) });  // Teleport to the bottom
        }
        else if (sprite.getPosition().y > static_cast<float>(height)) {
            sprite.setPosition({ sprite.getPosition().x, 0 });  // Teleport to the top
        }

        if (!collide && collisionTimer.getElapsedTime().asSeconds() >= 2.0f) {
            collide = true;
            sprite.setColor(sf::Color(255, 255, 255, 255)); // Full opacity
        }

    }

    float getOrientation() { return orientation;}

    void setLives(int x) {
        lives = x;
    }

    void Collision() {
        if (collide) {
            collide = false; // Disable collision
            collisionTimer.restart(); // Start cooldown
            sprite.setColor(Color(255, 255, 255, 120));
        }

        if (lives > 0) {
            lives--; // Reduce lives on collision
        }
    }

    unsigned int getLives() { return lives; }
    bool canCollide() { return collide; }

    void draw(sf::RenderWindow& window) {
        window.draw(sprite);  // Draw the sprite
        displayLives(window);
    }
};

enum class MenuType { Main, Dead, Options};

class MenuButton {
private:
    sf::Text m_text;
    std::string m_label;
    bool m_wasMousePressed = false;

public:
    MenuButton(const std::string& label, float x, float y, const sf::Font& font)
        : m_label(label), m_text(font)
    {
        m_text.setFont(font);
        m_text.setString(label);
        m_text.setCharacterSize(45);
        m_text.setFillColor(sf::Color::White);
        m_text.setPosition({ x, y });

        // Center origin
        sf::FloatRect bounds = m_text.getLocalBounds();
        m_text.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(m_text);
    }

    void updateScale(const sf::RenderWindow& window) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);

        float scale = m_text.getGlobalBounds().contains(worldPos) ? 1.2f : 1.0f;
        m_text.setScale({ scale, scale });
    }

    bool wasJustClicked() {
        bool isCurrentlyPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

        if (isCurrentlyPressed && !m_wasMousePressed) {
            m_wasMousePressed = true; // Just pressed
            return true;
        }

        if (!isCurrentlyPressed) {
            m_wasMousePressed = false; // Reset when released
        }

        return false;
    }

    bool isClicked(const sf::RenderWindow& window, bool justClicked) const {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);
        return justClicked && m_text.getGlobalBounds().contains(worldPos);
    }


    const std::string& getLabel() const {
        return m_label;
    }

    void setLabel(const std::string& newText) {
        m_label = newText;
        m_text.setString(newText);

        sf::FloatRect bounds = m_text.getLocalBounds();
        m_text.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
    }
};


class Menu {
private:
    RenderWindow& m_window;
    Font m_font;
    Music& m_music;
    std::map<MenuType, std::vector<MenuButton>> m_buttons;
    std::map<MenuType, std::vector<sf::Text>> m_staticTexts;
    MenuType m_currentType;
    MenuType m_previousType;

    bool m_wasMousePressed = false;
    bool m_isGameStarted = false;

public:
    Menu(RenderWindow& window, Font font, Music &music)
        : m_window(window), m_font(font), m_music(music)
    {
        if (!m_font.openFromFile("Minecraft.ttf")) {
            std::cerr << "ERROR: COULD NOT LOAD FONT\n";
        }

        // Setup menus
        setupMainMenu();
        setupOptionsMenu();
        setupDeadMenu();
        m_currentType = MenuType::Main;
    }

    sf::Text createStaticText(const std::string& str, float x, float y, unsigned int size, const sf::Font& font, bool centered = true) {
        sf::Text text(font);
        text.setString(str);
        text.setCharacterSize(size);
        text.setFillColor(sf::Color::White);
        text.setPosition({ x, y });

        if (centered) {
            auto bounds = text.getLocalBounds();
            text.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
        }

        return text;
    }

    bool wasJustClicked() {
        bool isCurrentlyPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

        if (isCurrentlyPressed && !m_wasMousePressed) {
            m_wasMousePressed = true; // Just pressed
            return true;
        }

        if (!isCurrentlyPressed) {
            m_wasMousePressed = false; // Reset when released
        }

        return false;
    }


    void setMenuType(MenuType type) {
        m_previousType = m_currentType;
        m_currentType = type;
    }

    void render() {
        m_window.clear(sf::Color::Black);
        for (auto& button : m_buttons[m_currentType]) {
            button.updateScale(m_window);
            button.draw(m_window);
        }
        for (const auto& text : m_staticTexts[m_currentType]) {
            m_window.draw(text);
        }

        m_window.display();
    }

    MenuButton* findButton(MenuType type, const std::string& label) {
        for (auto& button : m_buttons[type]) {
            if (button.getLabel() == label || button.getLabel() == "Off") {
                return &button;
            }
        }
        return nullptr;
    }

    void handleClick() {
        
        bool justClicked = wasJustClicked();

        for (auto& button : m_buttons[m_currentType]) {
            if (button.isClicked(m_window, justClicked)) {
                std::string label = button.getLabel();
                std::cout << "Clicked label: [" << label << "]\n";

                if (label == "Start Game" || label == "Play Again") m_isGameStarted = true;
                else if (label == "Options") setMenuType(MenuType::Options);
                else if (label == "Return") setMenuType(m_previousType);
                else if (label == "Return to Menu") setMenuType(MenuType::Main);
                else if (label == "Exit" || label == "Quit") m_window.close();
                else if (label == "On" || label == "Off") {
                    bool turnOff = label == "On";
                    m_music.setVolume(turnOff ? 0.f : 100.f);
                    button.setLabel(label == "On" ? "Off" : "On");
                }
            }
        }
    }

    bool isGameStarted() const {
        return m_isGameStarted;
    }

    void setGameStarted(bool started) {
        m_isGameStarted = started;
    }

    bool isMusicOn() const {
        return m_music.getVolume() > 0.f;
    }

private:
    void setupMainMenu() {
        m_staticTexts[MenuType::Main].clear();
        m_staticTexts[MenuType::Main].push_back(createStaticText("Space Ratao", width / 2, 100, 70, m_font));
        m_staticTexts[MenuType::Main].push_back(createStaticText("Version: Beta 1.0", 20, 720, 20, m_font, false));
        m_staticTexts[MenuType::Main].push_back(createStaticText("Made by Rodrigo Z Silveira", 20, 760, 20, m_font, false));

        m_buttons[MenuType::Main].emplace_back("Start Game", width / 2, 300, m_font);
        m_buttons[MenuType::Main].emplace_back("Options", width / 2, 400, m_font);
        m_buttons[MenuType::Main].emplace_back("Exit", width / 2, 500, m_font);

    }

    void setupOptionsMenu() {
        m_staticTexts[MenuType::Options].clear();
        m_staticTexts[MenuType::Options].push_back(createStaticText("Options", width / 2, 100, 70, m_font));
        m_staticTexts[MenuType::Options].push_back(createStaticText("Music:", 200, 300, 45, m_font));

        m_buttons[MenuType::Options].clear();
        m_buttons[MenuType::Options].emplace_back("On", 600, 300, m_font);
        m_buttons[MenuType::Options].emplace_back("Return", width / 2, 500, m_font);

    }

    void setupDeadMenu() {
        m_staticTexts[MenuType::Dead].clear();
        m_staticTexts[MenuType::Dead].push_back(createStaticText("You Died!", width / 2, 100, 70, m_font));

        m_buttons[MenuType::Dead].emplace_back("Play Again", width / 2, 300, m_font);
        m_buttons[MenuType::Dead].emplace_back("Options", width / 2, 400, m_font);
        m_buttons[MenuType::Dead].emplace_back("Return to Menu", width / 2, 500, m_font);

    }
};

bool CheckCollision(sf::Sprite& object_1, sf::Sprite& object_2, float scaleFactor = 0.65f) {
    // Get the global bounds of both objects
    sf::FloatRect bounds1 = object_1.getGlobalBounds();
    sf::FloatRect bounds2 = object_2.getGlobalBounds();

    // Scale the size of the collision box (width and height)
    bounds1.size *= scaleFactor;
    bounds2.size *= scaleFactor;

    // Check if the adjusted bounds intersect
    if (!bounds1.findIntersection(bounds2)) {
        return false;  // No intersection means no collision
    }
    return true;
}

int main() {
    RenderWindow window(VideoMode({ width, height }, 24), "Spaceship", Style::Default);
    window.setFramerateLimit(60);

    Clock clock;

    Music bg_music;
    if (!bg_music.openFromFile("Audio/bg.ogg")) {
        std::cerr << "ERROR: COULD NOT LOAD MUSIC: bg.ogg!\n";
        return -1;
    }

    // song randomizer
    std::vector<Time> songStartTimes = {
        seconds(0),     // Nebula Purple
        seconds(239),   // Martian Red
        seconds(461),   // Gamma Ray Yellow
        seconds(572),   // Plasma Blue
        seconds(811),   // Cosmic Dust Brown
        seconds(920),   // Pulsar Green
        seconds(1160),  // Quantum Indigo
        seconds(1390),  // Asteroid Grey
        seconds(1625),  // Quasar Gold
        seconds(1800),  // Nebula Purple II
        seconds(2039),  // Martian Red II
        seconds(2261),  // Gamma Ray Yellow II
        seconds(2372),  // Plasma Blue II
        seconds(2611),  // Cosmic Dust Brown II
        seconds(2720),  // Pulsar Green II
        seconds(2960),  // Quantum Indigo II
        seconds(3190),  // Asteroid Grey II
        seconds(3425)   // Quasar Gold II
    }; // individual song randomizer 
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    int index = std::rand() % songStartTimes.size();

    bg_music.setPlayingOffset(songStartTimes[index]);

    bg_music.setLooping(true);
    bg_music.play();


    Font font;
    if (!font.openFromFile("Minecraft.ttf")) {
        std::cerr << "ERROR: COULD NOT LOAD FONT: Minecraft.ttf!\n";
        return -1;
    }

    Timer timer(font);

    Texture explosion_tex("Sprites/Explosion.png");
    Texture spaceship_tex("Sprites/Spaceship3.png");
    Texture life_tex("Sprites/Life.png");
    Texture life_animation("Sprites/LifeAnimation.png");
    Texture frieren_tex("Sprites/frierensprite.png");
    Texture bullet_tex("Sprites/bullet1.png");

    if (!frieren_tex.loadFromFile("Sprites/frierensprite.png")) {
        std::cerr << "ERROR: COULD NOT LOAD SPRITE: Sprites/frierensprite.png" << std::endl;
    }
    if (!explosion_tex.loadFromFile("Sprites/Explosion.png")) {
        std::cerr << "ERROR: COULD NOT LOAD SPRITE: Sprites/Explosion.png" << std::endl;
    }
    if (!life_animation.loadFromFile("Sprites/LifeAnimation.png")) {
        std::cerr << "ERROR: COULD NOT LOAD SPRITE: Sprites/LifeAnimation.png" << std::endl;
    }
    if (!spaceship_tex.loadFromFile("Sprites/Spaceship3.png")) {
        std::cerr << "ERROR: COULD NOT LOAD SPRITE: Sprites/Spaceship3.png" << std::endl;
    }
    if (!life_tex.loadFromFile("Sprites/Life.png")) {
        std::cerr << "ERROR: COULD NOT LOAD SPRITE: Sprites/Life.png" << std::endl;
    }
    if (!bullet_tex.loadFromFile("Sprites/bullet1.png")) {
        std::cerr << "ERROR: COULD NOT LOAD SPRITE: Sprites/bullet1.png" << std::endl;
    }

    Texture* selectedTexture = nullptr;
    Texture asteroid_Stex("Sprites/AsteroidSmall.png");
    Texture asteroid_Mtex("Sprites/AsteroidMedium.png");
    Texture asteroid_Ltex("Sprites/AsteroidLarge.png");

    if (!asteroid_Stex.loadFromFile("Sprites/AsteroidSmall.png")) {
        std::cerr << "ERROR: COULD NOT LOAD SPRITE: Sprites/AsteroidSmall.png" << std::endl;
    }
    if (!asteroid_Mtex.loadFromFile("Sprites/AsteroidMedium.png")) {
        std::cerr << "ERROR: COULD NOT LOAD SPRITE: Sprites/AsteroidMedium.png" << std::endl;
    }
    if (!asteroid_Ltex.loadFromFile("Sprites/AsteroidLarge.png")) {
        std::cerr << "ERROR: COULD NOT LOAD SPRITE: Sprites/AsteroidLarge.png" << std::endl;
    }

    Spaceship spaceship(spaceship_tex, life_tex, life_animation);  // Create main Spaceship

    int spawnCounter = 0;
    float asteroid_spawn_time = ASTEROID_SPAWN_TIME;
    float bullet_cooldown = 0.5f;

    std::vector<Asteroid> asteroids;
    std::vector<Asteroid> menuAsteroids;
    std::vector<Animation> explosions;
    std::vector<Animation> hexplosions;
    std::vector<Bullet> bullets;

    Menu menu(window, font, bg_music);
    MenuType type = MenuType::Main;


    while (window.isOpen()) {
        Time deltaTime = clock.restart();
        while (const std::optional event = window.pollEvent()) {
            if (event->is<Event::Closed>())
                window.close();
        }

        if (!menu.isGameStarted()) {
            menu.handleClick();
            menu.render();
            timer.reset();
        }

        // Game started
        if (menu.isGameStarted()) {
            if(menu.isMusicOn()) bg_music.setVolume(70);
            Vector2i mousePos = Mouse::getPosition(window); // Spaceship update
            spaceship.handleMouseInput(mousePos);
            spaceship.handleKBInput();
            spaceship.update();
            timer.update();
            asteroid_spawn_time -= 0.1f;
            bullet_cooldown -= 0.1f;

            // Handle Asteroid - Spaceship collision
            for (auto it = asteroids.begin(); it != asteroids.end(); ) {
                if (spaceship.canCollide() && CheckCollision(spaceship.getSprite(), it->getSprite())) {
                    spaceship.Collision();

                    // Create explosion at the asteroid's position
                    explosions.emplace_back(explosion_tex, it->getSprite().getPosition(), 6, 25, 25);

                    // Create Heart loosing animation
                    if (spaceship.getLives() > 0) {
                        float heartPosX = 20.0f + ((spaceship.getLives()) * 40.0f);  // Position of last heart
                        float heartPosY = height - 50.0f;  // Consistent Y position
                        hexplosions.emplace_back(life_animation, sf::Vector2f(heartPosX, heartPosY), 5, 10, 10);
                    }

                    // Remove asteroid from the vector
                    it = asteroids.erase(it);
                }
                else {
                    it->update();
                    ++it;
                }
            }

            // Handle Asteroid - Bullets collision
            for (auto bulletIt = bullets.begin(); bulletIt != bullets.end(); ) {
                bool bulletDestroyed = false; // Track if the bullet is destroyed

                for (auto asteroidIt = asteroids.begin(); asteroidIt != asteroids.end(); ) {
                    if (CheckCollision(bulletIt->getSprite(), asteroidIt->getSprite())) {
                        // Create explosion at asteroid's position
                        explosions.emplace_back(explosion_tex, asteroidIt->getSprite().getPosition(), 6, 25, 25);

                        // Erase asteroid from vector
                        asteroidIt = asteroids.erase(asteroidIt);

                        // Erase bullet and break out of asteroid loop
                        bulletIt = bullets.erase(bulletIt);
                        bulletDestroyed = true;
                        break;
                    }
                    else {
                        ++asteroidIt;
                    }
                }

                // If the bullet was destroyed, we already erased it, so continue the loop
                if (!bulletDestroyed) {
                    ++bulletIt;
                }
            }

            // Update bullets
            for (auto& bullet : bullets) {
                bullet.update();
            }

            // Removes finished explosions
            for (auto it = explosions.begin(); it != explosions.end(); ) {
                it->update();
                if (it->isFinished()) {
                    it = explosions.erase(it);
                }
                else {
                    ++it;
                }
            }

            // Removes finished heart animations
            for (auto it = hexplosions.begin(); it != hexplosions.end(); ) {
                it->update();
                if (it->isFinished()) {
                    it = hexplosions.erase(it);
                }
                else {
                    ++it;
                }
            }

            // Spawn new asteroids
            if (asteroid_spawn_time <= 0.0f) {
                spawnCounter++;
                bool large = false;
                if (spawnCounter % 3 == 0) {
                    selectedTexture = &asteroid_Stex;
                }
                else if (spawnCounter % 3 == 1) {
                    selectedTexture = &asteroid_Mtex;
                }
                else {
                    selectedTexture = &asteroid_Ltex;
                    large = true;
                }

                Asteroid newAsteroid(*selectedTexture, 3.5f, 4.5f, large);
                newAsteroid.appear(width, height);
                asteroids.push_back(newAsteroid);
                asteroid_spawn_time = ASTEROID_SPAWN_TIME;
            }

            // Removes out-of-screen asteroids
            for (auto it = asteroids.begin(); it != asteroids.end(); ) {
                it->update();
                if (it->isOutOfScreen(width, height)) {
                    it = asteroids.erase(it);
                }
                else {
                    ++it;
                }
            }

            // Checks for bullets shooted
            if (Mouse::isButtonPressed(Mouse::Button::Left) && bullet_cooldown <= 0.0f) {
                float spaceshipAngle = spaceship.getSprite().getRotation().asDegrees(); // Get rotation in degrees

                // Get spaceship tip position
                sf::Vector2f spaceshipPos = spaceship.getSprite().getPosition();
                float spaceshipLength = spaceship.getSprite().getGlobalBounds().size.y / 2.0f; // Half of the spaceship height (assuming it's vertical)

                // Convert angle to radians for offset calculation
                float radian = (spaceshipAngle - 90) * 3.14159f / 180.0f;

                // Calculate tip position
                sf::Vector2f bulletSpawnPos = spaceshipPos + sf::Vector2f(std::cos(radian) * spaceshipLength, std::sin(radian) * spaceshipLength);

                // Spawn bullet at the tip
                bullets.emplace_back(bullet_tex, bulletSpawnPos, spaceshipAngle);
                bullet_cooldown = 0.5f;
            }

            if (spaceship.getLives() <= 0) {
                menu.setMenuType(MenuType::Dead);
                menu.setGameStarted(false);
                spaceship.reset();
                asteroids.clear();
            }

            window.clear();
            spaceship.draw(window);
            timer.draw(window);

            // Draw asteroids
            for (auto& asteroid : asteroids) {
                asteroid.draw(window);
            }

            for (auto& bullets : bullets) {
                bullets.draw(window);
            }

            // Draw explosions
            for (auto& explosion : explosions) {
                explosion.draw(window);
            }

            for (auto& hexplosion : hexplosions) {
                hexplosion.draw(window);
            }

            window.display();
        }
    }

    return 0;
}