#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <chrono>
#include <random>
#include <omp.h>

struct Particula {
    double massa;
    double x, y;
    double vx, vy;
    double ax, ay;
};

const double G = 6.674e-11;
const double AMORTECIMENTO = 1.0;

void calcularAceleracoesParalelo(std::vector<Particula>& particulas) {
    int n = particulas.size();

    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        double axSoma = 0.0, aySoma = 0.0;
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            double dx = particulas[j].x - particulas[i].x;
            double dy = particulas[j].y - particulas[i].y;
            double distSqr = dx * dx + dy * dy + AMORTECIMENTO * AMORTECIMENTO;
            double dist = std::sqrt(distSqr);
            double forca = G * particulas[i].massa * particulas[j].massa / distSqr;
            axSoma += (forca * dx / dist) / particulas[i].massa;
            aySoma += (forca * dy / dist) / particulas[i].massa;
        }
        particulas[i].ax = axSoma;
        particulas[i].ay = aySoma;
    }
}

std::vector<Particula> gerarParticulas(int n, unsigned int semente) {
    std::vector<Particula> particulas;
    particulas.reserve(n);
    double massaCentral = 5.0e15;
    particulas.push_back({massaCentral, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});

    std::mt19937 gerador(semente);
    std::uniform_real_distribution<double> distAngulo(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> distRaio(3000.0, 10000.0);
    std::uniform_real_distribution<double> distMassa(1.0e9, 1.0e10);
    std::uniform_real_distribution<double> distFatorVelocidade(0.9, 1.05);

    for (int i = 1; i < n; i++) {
        double angulo = distAngulo(gerador);
        double raio = distRaio(gerador);
        double x = raio * std::cos(angulo);
        double y = raio * std::sin(angulo);
        double velCircular = std::sqrt(G * massaCentral / raio);
        double vel = velCircular * distFatorVelocidade(gerador);
        double vx = -vel * std::sin(angulo);
        double vy =  vel * std::cos(angulo);
        double massa = distMassa(gerador);
        particulas.push_back({massa, x, y, vx, vy, 0.0, 0.0});
    }
    return particulas;
}

int main(int argc, char* argv[]) {
    int numParticulas;
    std::cout << "Quantas particulas deseja simular? ";
    std::cin >> numParticulas;

    if (numParticulas < 1) {
        std::cout << "Numero invalido de particulas.\n";
        return 1;
    }

    int numThreads;
    std::cout << "Quantas threads deseja usar? (maximo disponivel: " << omp_get_max_threads() << ") ";
    std::cin >> numThreads;

    if (numThreads < 1) {
        std::cout << "Numero invalido de threads.\n";
        return 1;
    }
    omp_set_num_threads(numThreads);

    unsigned int semente = 42;
    std::vector<Particula> particulas = gerarParticulas(numParticulas, semente);

    double dt = 1.0;
    int numPassos = 20000;
    int intervaloSalvar = 20;

    std::ofstream arquivo("simulacao_paralela.csv");
    arquivo << "passo,id,x,y\n";

    auto inicio = std::chrono::high_resolution_clock::now();

    calcularAceleracoesParalelo(particulas);

    for (int passo = 0; passo < numPassos; passo++) {
        for (auto& p : particulas) {
            p.vx += 0.5 * p.ax * dt;
            p.vy += 0.5 * p.ay * dt;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
        }

        calcularAceleracoesParalelo(particulas);

        for (auto& p : particulas) {
            p.vx += 0.5 * p.ax * dt;
            p.vy += 0.5 * p.ay * dt;
        }

        if (passo % intervaloSalvar == 0) {
            for (size_t i = 0; i < particulas.size(); i++) {
                arquivo << passo << "," << i << "," << particulas[i].x << "," << particulas[i].y << "\n";
            }
        }
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duracao = fim - inicio;

    arquivo.close();
    std::cout << "Simulação paralela com " << numParticulas << " partículas, usando "
              << omp_get_max_threads() << " threads, concluída em " << duracao.count() << " segundos.\n";
    std::cout << "Resultados salvos em simulacao_paralela.csv\n";

    return 0;
}