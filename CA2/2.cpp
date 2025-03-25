#include <SFML/Graphics.hpp>
#include <complex>
#include <chrono>
#include <iostream>
#include <omp.h>

const int WIDTH = 800;
const int HEIGHT = 800;
const int MAX_ITER = 1000;

//const std::complex<double> C(-0.8, 0.156);
//const std::complex<double> C(-0.7, 0.27015);
//const std::complex<double> C(0.355, 0.355);
const std::complex<double> C(-0.5, 0.5);

sf::Color getColor(int iter) {
    return sf::Color(iter % 256, (iter * 5) % 256, (iter * 15) % 256);
}

int julia(double real, double imag) {
    std::complex<double> z(real, imag);
    int iter = 0;
    while (abs(z) <= 2 && iter < MAX_ITER) {
        z = z * z + C;
        ++iter;
    }
    return iter;
}

void computeSerial(sf::Image& image) {
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            double real = (x - WIDTH / 2.0) * 4.0 / WIDTH;
            double imag = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;
            int iter = julia(real, imag);
            sf::Color color = getColor(iter);
            image.setPixel(x, y, color);
        }
    }
}

void computeParallel(sf::Image& image) {
    std::vector<sf::Color> buffer(WIDTH * HEIGHT);
    #pragma omp parallel num_threads(8)
    {
        #pragma omp for schedule(static) 
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                double real = (x - WIDTH / 2.0) * 4.0 / WIDTH;
                double imag = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;
                int iter = julia(real, imag);
                sf::Color color = getColor(iter);
                buffer[y * WIDTH + x] = color;
            }
        }
        #pragma omp for schedule(static)
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                image.setPixel(x, y, buffer[y * WIDTH + x]);
            }
        }
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Julia Set - Parallel vs Serial");

    sf::Image serialImage, parallelImage;
    serialImage.create(WIDTH, HEIGHT, sf::Color::Black);
    parallelImage.create(WIDTH, HEIGHT, sf::Color::Black);

    //serial
    auto serialStart = std::chrono::high_resolution_clock::now();
    computeSerial(serialImage);
    auto serialEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> serialTime = serialEnd - serialStart;
    std::cout << "Serial execution time: " << serialTime.count() << " seconds\n";

    //parallel
    auto parallelStart = std::chrono::high_resolution_clock::now();
    computeParallel(parallelImage);
    auto parallelEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> parallelTime = parallelEnd - parallelStart;
    std::cout << "Parallel execution time: " << parallelTime.count() << " seconds\n";

    double speedup = serialTime.count() / parallelTime.count();
    std::cout << "Speedup: " << speedup << "x\n";

    sf::Texture texture;
    texture.loadFromImage(parallelImage);
    sf::Sprite sprite(texture);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }

    return 0;
}
