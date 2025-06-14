#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iostream>

const int WIDTH = 1024;
const int HEIGHT = 1024;
const int OBSTACLE_COUNT = 8;
const int OBSTACLE_MIN_Y = 300;
const float SAFE_RADIUS = 150.f;

struct Obstacle {
    sf::Sprite sprite;
};

bool checkCollision(const sf::Sprite& a, const sf::Sprite& b) {
    return a.getGlobalBounds().intersects(b.getGlobalBounds());
}

float distance(const sf::Vector2f& a, const sf::Vector2f& b) {
    return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
}

int main() {
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Crazy Car Survival Game");
    window.setFramerateLimit(60);

    sf::Texture carTexture, backgroundTexture, obstacleTexture;
    if (!carTexture.loadFromFile("car.png") ||
        !backgroundTexture.loadFromFile("background.png") ||
        !obstacleTexture.loadFromFile("ob.png")) {
        std::cerr << "Nie można załadować jednej z tekstur (car.png / background.png / ob.png)\n";
        return -1;
    }

    sf::SoundBuffer crashBuffer, driftBuffer;
    if (!crashBuffer.loadFromFile("crash.wav") ||
        !driftBuffer.loadFromFile("drift.wav")) {
        std::cerr << "Nie można załadować dźwięków (crash.wav / drift.wav)\n";
        return -1;
    }

    sf::Sound crashSound(crashBuffer);
    sf::Sound driftSound(driftBuffer);
    driftSound.setLoop(true);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Nie można załadować czcionki arial.ttf\n";
        return -1;
    }

    sf::Sprite background(backgroundTexture);
    sf::Sprite car(carTexture);
    car.setScale(0.096f, 0.096f);
    car.setOrigin(car.getLocalBounds().width / 2, car.getLocalBounds().height / 2);
    car.setPosition(WIDTH / 2, HEIGHT / 2);

    sf::Text timerText("", font, 28);
    timerText.setFillColor(sf::Color::Black);
    timerText.setPosition(10, 10);

    sf::Text gameOverLabel("GAME OVER", font, 80);
    gameOverLabel.setFillColor(sf::Color::White);
    gameOverLabel.setPosition(WIDTH / 2.f - 300, HEIGHT / 2.f - 180);

    sf::Text gameOverHighscoreText("", font, 50);
    gameOverHighscoreText.setFillColor(sf::Color::White);
    gameOverHighscoreText.setPosition(WIDTH / 2.f - 200, HEIGHT / 2.f - 80);

    sf::Text lastScoreText("", font, 40);
    lastScoreText.setFillColor(sf::Color::White);
    lastScoreText.setPosition(WIDTH / 2.f - 150, HEIGHT / 2.f - 20);

   
    sf::RectangleShape startBtn({400, 80});
    startBtn.setFillColor(sf::Color::Black);
    startBtn.setOutlineColor(sf::Color::White);
    startBtn.setOutlineThickness(2);
    startBtn.setPosition(WIDTH / 2.f - 200, HEIGHT / 2.f - 40);

    sf::Text startLabel("START", font, 40);
    startLabel.setFillColor(sf::Color::White);
    startLabel.setPosition(WIDTH / 2.f - 60, HEIGHT / 2.f - 25);

  
    sf::RectangleShape restartBtn({400, 80});
    restartBtn.setFillColor(sf::Color::Black);
    restartBtn.setOutlineColor(sf::Color::White);
    restartBtn.setOutlineThickness(2);
    restartBtn.setPosition(WIDTH / 2.f - 200, HEIGHT / 2.f + 60);

    sf::Text restartLabel("RESTART", font, 40);
    restartLabel.setFillColor(sf::Color::White);
    restartLabel.setPosition(WIDTH / 2.f - 90, HEIGHT / 2.f + 75);

    std::vector<Obstacle> obstacles;
    sf::Clock clock, timer;

    float angle = -90.f, speed = 0.5f, accel = 0.00005f;
    float angularVelocity = 0.f, driftVisualAngle = 0.f;
    const float turnSpeed = 2.0f;
    const float driftAngularAccel = 0.33f;
    sf::Vector2f velocity;

    bool inGame = false, crashed = false, drifting = false, wasDrifting = false;
    int highscore = 0;
    int lastScore = 0;

    auto startGame = [&]() {
        inGame = true;
        crashed = false;
        speed = 0.5f;
        accel = 0.00005f;
        angle = -90.f;
        angularVelocity = 0.f;
        driftVisualAngle = 0.f;
        car.setRotation(angle);
        car.setPosition(WIDTH / 2.f, HEIGHT / 2.f);
        timer.restart();
        obstacles.clear();
        for (int i = 0; i < OBSTACLE_COUNT; ++i) {
            Obstacle ob;
            ob.sprite.setTexture(obstacleTexture);
            ob.sprite.setScale(0.05f, 0.05f);
            sf::Vector2f pos;
            do {
                pos = {static_cast<float>(rand() % (WIDTH - 100) + 50),
                       static_cast<float>(rand() % (HEIGHT - OBSTACLE_MIN_Y) + OBSTACLE_MIN_Y)};
                ob.sprite.setPosition(pos);
            } while (
                distance(pos, car.getPosition()) < SAFE_RADIUS ||
                std::any_of(obstacles.begin(), obstacles.end(), [&](const Obstacle& o) {
                    return o.sprite.getGlobalBounds().intersects(ob.sprite.getGlobalBounds());
                })
                );
            obstacles.push_back(ob);
        }
    };

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (!inGame && !crashed && event.type == sf::Event::MouseButtonPressed) {
                if (startBtn.getGlobalBounds().contains(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))) {
                    startGame();
                }
            }

            if (crashed && event.type == sf::Event::MouseButtonPressed) {
                if (restartBtn.getGlobalBounds().contains(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))) {
                    startGame();
                }
            }
        }

        float dt = clock.restart().asSeconds();
        window.clear();

        if (!inGame && !crashed) {
            sf::RectangleShape black({(float)WIDTH, (float)HEIGHT});
            black.setFillColor(sf::Color::Black);
            window.draw(black);
            window.draw(startBtn);
            window.draw(startLabel);
        }

        if (!inGame && crashed) {
            if (driftSound.getStatus() == sf::Sound::Playing) driftSound.stop();
            sf::RectangleShape blackOverlay({(float)WIDTH, (float)HEIGHT});
            blackOverlay.setFillColor(sf::Color::Black);
            window.draw(blackOverlay);
            window.draw(gameOverLabel);
            gameOverHighscoreText.setString("Highscore: " + std::to_string(highscore) + " s");
            window.draw(gameOverHighscoreText);
            lastScoreText.setString("Your score: " + std::to_string(lastScore) + " s");
            window.draw(lastScoreText);
            window.draw(restartBtn);
            window.draw(restartLabel);
        }

        if (inGame) {
            window.draw(background);

            drifting = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
            if (drifting && !wasDrifting) {
                if (driftSound.getStatus() != sf::Sound::Playing) driftSound.play();
            }
            if (!drifting && wasDrifting) {
                driftSound.stop();
            }
            wasDrifting = drifting;

            accel += 0.00005f;
            speed += accel;

            if (!crashed) {
                if (drifting) {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) angularVelocity -= driftAngularAccel;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) angularVelocity += driftAngularAccel;
                    driftVisualAngle += angularVelocity * 0.6f;
                    driftVisualAngle *= 0.92f;
                } else {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) angle -= turnSpeed;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) angle += turnSpeed;
                    angularVelocity = 0.f;
                    driftVisualAngle *= 0.9f;
                }
            }

            if (drifting) {
                angle += angularVelocity;
                angularVelocity *= 0.92f;
            }

            float rad = angle * 3.14159f / 180.f;
            velocity = sf::Vector2f(std::cos(rad), std::sin(rad)) * speed;
            if (!crashed)
                car.move(velocity);

            car.setRotation(angle + driftVisualAngle);
            window.draw(car);

            for (auto& ob : obstacles) {
                window.draw(ob.sprite);
                if (!crashed && checkCollision(car, ob.sprite)) {
                    if (crashSound.getStatus() != sf::Sound::Playing) crashSound.play();
                    if (driftSound.getStatus() == sf::Sound::Playing) driftSound.stop();
                    crashed = true;
                    inGame = false;
                    lastScore = static_cast<int>(timer.getElapsedTime().asSeconds());
                    if (lastScore > highscore) highscore = lastScore;
                }
            }

            if (!crashed && (car.getPosition().y < OBSTACLE_MIN_Y ||
                             car.getPosition().x < 0 || car.getPosition().x > WIDTH ||
                             car.getPosition().y > HEIGHT)) {
                if (crashSound.getStatus() != sf::Sound::Playing) crashSound.play();
                if (driftSound.getStatus() == sf::Sound::Playing) driftSound.stop();
                crashed = true;
                inGame = false;
                lastScore = static_cast<int>(timer.getElapsedTime().asSeconds());
                if (lastScore > highscore) highscore = lastScore;
            }

            int t = static_cast<int>(timer.getElapsedTime().asSeconds());
            timerText.setString("Time: " + std::to_string(t) + " s");
            window.draw(timerText);
        }

        window.display();
    }

    return 0;
}
