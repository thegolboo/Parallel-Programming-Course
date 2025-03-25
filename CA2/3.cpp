#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <chrono>
#include <omp.h>
#include <random>
#include <vector>
#include <thread>

constexpr int DISPLAY_SIZE = 800;
constexpr int CIRCLE_RADIUS = 400;
constexpr int NUM_POINTS = 100000;

int performPiCalcParallel(sf::RenderWindow& display, const sf::CircleShape& roundShape, const sf::RectangleShape& squareShape) {
    std::vector<sf::Vertex> pointsInsideCircle;
    std::vector<sf::Vertex> pointsOutsideCircle;

    int pointsInCircle = 0;

    #pragma omp parallel num_threads(8)
    {
        thread_local std::mt19937 randomGen(std::random_device{}());
        std::uniform_real_distribution<float> uniformDist(0, DISPLAY_SIZE);

        std::vector<sf::Vertex> localPointsInside;
        std::vector<sf::Vertex> localPointsOutside;
        int localInCircle = 0;

        #pragma omp for
        for (int i = 0; i < NUM_POINTS; i++) {
            float posX = uniformDist(randomGen);
            float posY = uniformDist(randomGen);

            float dX = posX - CIRCLE_RADIUS;
            float dY = posY - CIRCLE_RADIUS;

            if (dX * dX + dY * dY <= CIRCLE_RADIUS * CIRCLE_RADIUS) {
                localInCircle++;
                localPointsInside.emplace_back(sf::Vector2f(posX, posY), sf::Color::Green);
            }
            else {
                localPointsOutside.emplace_back(sf::Vector2f(posX, posY), sf::Color::Red);
            }
        }

        #pragma omp critical
        {
            pointsInCircle += localInCircle;
            pointsInsideCircle.insert(pointsInsideCircle.end(), localPointsInside.begin(), localPointsInside.end());
            pointsOutsideCircle.insert(pointsOutsideCircle.end(), localPointsOutside.begin(), localPointsOutside.end());
        }
    }

    display.clear();
    display.draw(roundShape);
    display.draw(squareShape);
    for (const auto& point : pointsInsideCircle) display.draw(&point, 1, sf::Points);
    for (const auto& point : pointsOutsideCircle) display.draw(&point, 1, sf::Points);
    display.display();
    return 4.0f * pointsInCircle / NUM_POINTS;
}

int performPiCalcSerial(sf::RenderWindow& display, const sf::CircleShape& roundShape, const sf::RectangleShape& squareShape) {
    sf::VertexArray pointsInside(sf::Points);
    sf::VertexArray pointsOutside(sf::Points);

    int pointsInCircle = 0;
    for (int i = 0; i < NUM_POINTS; i++) {
        float posX = static_cast<float>(rand()) / RAND_MAX * DISPLAY_SIZE;
        float posY = static_cast<float>(rand()) / RAND_MAX * DISPLAY_SIZE;
        float dX = posX - CIRCLE_RADIUS;
        float dY = posY - CIRCLE_RADIUS;

        if (dX * dX + dY * dY <= CIRCLE_RADIUS * CIRCLE_RADIUS) {
            pointsInCircle++;
            pointsInside.append(sf::Vertex(sf::Vector2f(posX, posY), sf::Color::Green));
        }
        else {
            pointsOutside.append(sf::Vertex(sf::Vector2f(posX, posY), sf::Color::Red));
        }

        if (i % 10000 == 0 || i == NUM_POINTS - 1) {
            display.clear();
            display.draw(roundShape);
            display.draw(squareShape);
            display.draw(pointsInside);
            display.draw(pointsOutside);
            display.display();
        }
    }
    return 4.0f * pointsInCircle / NUM_POINTS;
}

double computeSpeedUp(long long parallelDuration, long long serialDuration) {
    return static_cast<double>(serialDuration) / parallelDuration;
}

int main() {
    sf::RenderWindow display(sf::VideoMode(DISPLAY_SIZE, DISPLAY_SIZE), "Monte Carlo Simulation");
    display.setFramerateLimit(60);

    sf::CircleShape roundShape(CIRCLE_RADIUS);
    roundShape.setFillColor(sf::Color::Transparent);
    roundShape.setOutlineColor(sf::Color::White);
    roundShape.setOutlineThickness(2);
    roundShape.setPosition(0, 0);

    sf::RectangleShape squareShape(sf::Vector2f(DISPLAY_SIZE, DISPLAY_SIZE));
    squareShape.setFillColor(sf::Color::Transparent);
    squareShape.setOutlineColor(sf::Color::White);
    squareShape.setOutlineThickness(2);

    auto serialStart = std::chrono::high_resolution_clock::now();
    int piSerialEstimate = performPiCalcSerial(display, roundShape, squareShape);
    auto serialEnd = std::chrono::high_resolution_clock::now();
    auto serialDuration = std::chrono::duration_cast<std::chrono::milliseconds>(serialEnd - serialStart).count();

    std::cout << "Estimated Pi (Serial): " << piSerialEstimate << std::endl;
    std::cout << "Serial Execution Time: " << serialDuration << " ms" << std::endl;

    auto parallelStart = std::chrono::high_resolution_clock::now();
    int piParallelEstimate = performPiCalcParallel(display, roundShape, squareShape);
    auto parallelEnd = std::chrono::high_resolution_clock::now();
    auto parallelDuration = std::chrono::duration_cast<std::chrono::milliseconds>(parallelEnd - parallelStart).count();

    std::cout << "Estimated Pi (Parallel): " << piParallelEstimate << std::endl;
    std::cout << "Parallel Execution Time: " << parallelDuration << " ms" << std::endl;

    double speedUp = computeSpeedUp(parallelDuration, serialDuration);
    std::cout << "Speed-Up: " << speedUp << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
