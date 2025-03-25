#include <SFML/Graphics.hpp>
#include <omp.h>
#include <iostream>
#include <ctime>

#define WIDTH 800
#define HEIGHT 800
#define MAX_ITER 1000

void compute_mandelbrot_serial(double real_min, double real_max, double imag_min, double imag_max, int width, int height, int max_iter, int *output) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double real = real_min + (real_max - real_min) * x / width;
            double imag = imag_min + (imag_max - imag_min) * y / height;
            double zr = real, zi = imag;
            int iter;
            for (iter = 0; iter < max_iter; iter++) {
                double zr2 = zr * zr, zi2 = zi * zi;
                if (zr2 + zi2 > 4.0) break;
                zi = 2.0 * zr * zi + imag;
                zr = zr2 - zi2 + real;
            }
            output[y * width + x] = iter;
        }
    }
}

void compute_mandelbrot_parallel(double real_min, double real_max, double imag_min, double imag_max, int width, int height, int max_iter, int *output) {
    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double real = real_min + (real_max - real_min) * x / width;
            double imag = imag_min + (imag_max - imag_min) * y / height;
            double zr = real, zi = imag;
            int iter;
            for (iter = 0; iter < max_iter; iter++) {
                double zr2 = zr * zr, zi2 = zi * zi;
                if (zr2 + zi2 > 4.0) break;
                zi = 2.0 * zr * zi + imag;
                zr = zr2 - zi2 + real;
            }
            output[y * width + x] = iter;
        }
    }
}

int main() {
    int *output_serial = (int *)malloc(WIDTH * HEIGHT * sizeof(int));
    int *output_parallel = (int *)malloc(WIDTH * HEIGHT * sizeof(int));

    clock_t start_serial, end_serial;
    double time_serial;
    start_serial = clock();
    compute_mandelbrot_serial(-2.0, 1.0, -1.5, 1.5, WIDTH, HEIGHT, MAX_ITER, output_serial);
    end_serial = clock();
    time_serial = ((double) (end_serial - start_serial)) / CLOCKS_PER_SEC;

    double start_parallel, end_parallel;
    double time_parallel;
    start_parallel = omp_get_wtime();
    compute_mandelbrot_parallel(-2.0, 1.0, -1.5, 1.5, WIDTH, HEIGHT, MAX_ITER, output_parallel);
    end_parallel = omp_get_wtime();
    time_parallel = end_parallel - start_parallel;

    double speedup = time_serial / time_parallel;
    std::cout << "Serial Time: " << time_serial << " seconds\n";
    std::cout << "Parallel Time: " << time_parallel << " seconds\n";
    std::cout << "Speedup: " << speedup << "\n";

    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Mandelbrot Set");
    sf::Image image;
    image.create(WIDTH, HEIGHT);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int iter = output_parallel[y * WIDTH + x];
            sf::Color color = (iter == MAX_ITER) ? sf::Color::Black : sf::Color(255 * iter / MAX_ITER, 255 * iter / MAX_ITER, 255 * iter / MAX_ITER);
            image.setPixel(x, y, color);
        }
    }
    sf::Texture texture;
    texture.loadFromImage(image);
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
    free(output_serial);
    free(output_parallel);
    return 0;
}