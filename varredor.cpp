#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <cstdint>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "collatz.h"

using namespace std;

struct DadosResultado {
    double tempo_execucao;
    uint64_t total_passos;
};

string para_notacao_cientifica(double valor) {
    if (valor < 0) return "-1";
    ostringstream ss;
    ss << scientific << setprecision(2) << valor;
    return ss.str();
}

void processar_bloco(uint64_t inicio, uint64_t fim, DadosResultado& res) {
    auto t_inicio = chrono::high_resolution_clock::now();
    uint64_t soma_passos = 0;

    for (uint64_t n = inicio; n <= fim; ++n) {
        soma_passos += conta_passos_collatz(n);
    }

    auto t_fim = chrono::high_resolution_clock::now();
    res.tempo_execucao = chrono::duration<double>(t_fim - t_inicio).count();
    res.total_passos = soma_passos;
}

void processar_ciclico(uint64_t A, uint64_t B, int id_worker, int num_workers, DadosResultado& res) {
    auto t_inicio = chrono::high_resolution_clock::now();
    
    uint64_t resto_A = A % num_workers;
    uint64_t primeiro = A + ((uint64_t)id_worker >= resto_A ? 
                            ((uint64_t)id_worker - resto_A) : 
                            (num_workers + (uint64_t)id_worker - resto_A));

    uint64_t soma_passos = 0;
    for (uint64_t n = primeiro; n <= B; n += num_workers) {
        soma_passos += conta_passos_collatz(n);
    }

    auto t_fim = chrono::high_resolution_clock::now();
    res.tempo_execucao = chrono::duration<double>(t_fim - t_inicio).count();
    res.total_passos = soma_passos;
}

int main(int argc, char* argv[]) {
    if (argc < 7) {
        cerr << "Uso correto: " << argv[0] << " <A> <B> <W> <modo> <particao> <arquivo_saida>\n";
        return 1;
    }

    preencher_cache_collatz();

    uint64_t A = stoull(argv[1]);
    uint64_t B = stoull(argv[2]);
    int W = stoi(argv[3]);
    string modo = argv[4];
    string particao = argv[5];
    string arquivo_saida = argv[6];

    uint64_t L = B - A + 1;

    double tempo_max_filho = -1.0;
    double tempo_min_filho = -1.0;
    double tempo_agregacao = -1.0;
    volatile uint64_t total_passos_global = 0;

    auto t_inicio_pai = chrono::high_resolution_clock::now();

    if (W == 1) {
        uint64_t soma = 0;
        for (uint64_t n = A; n <= B; ++n) {
            soma += conta_passos_collatz(n);
        }
        total_passos_global = soma;
    } 
    else {
        if (modo == "thread") {
            vector<DadosResultado> resultados(W);
            vector<thread> threads;

            for (int w = 0; w < W; ++w) {
                if (particao == "bloco") {
                    uint64_t tamanho_base = L / W;
                    uint64_t resto = L % W;
                    uint64_t inicio = A + w * tamanho_base + min((uint64_t)w, resto);
                    uint64_t tamanho = tamanho_base + ((uint64_t)w < resto ? 1 : 0);
                    uint64_t fim = inicio + tamanho - 1;
                    
                    threads.emplace_back(processar_bloco, inicio, fim, ref(resultados[w]));
                } else {
                    threads.emplace_back(processar_ciclico, A, B, w, W, ref(resultados[w]));
                }
            }

            for (auto& th : threads) {
                th.join();
            }

            auto t_fim_filhos = chrono::high_resolution_clock::now();

            tempo_max_filho = resultados[0].tempo_execucao;
            tempo_min_filho = resultados[0].tempo_execucao;
            uint64_t soma = resultados[0].total_passos;

            for (int w = 1; w < W; ++w) {
                if (resultados[w].tempo_execucao > tempo_max_filho) tempo_max_filho = resultados[w].tempo_execucao;
                if (resultados[w].tempo_execucao < tempo_min_filho) tempo_min_filho = resultados[w].tempo_execucao;
                soma += resultados[w].total_passos;
            }
            total_passos_global = soma;

            auto t_fim_agregacao = chrono::high_resolution_clock::now();
            tempo_agregacao = chrono::duration<double>(t_fim_agregacao - t_fim_filhos).count();

        } else if (modo == "processo") {
            for (int w = 0; w < W; ++w) {
                pid_t pid = fork();
                if (pid == 0) {
                    DadosResultado res_filho;
                    if (particao == "bloco") {
                        uint64_t tamanho_base = L / W;
                        uint64_t resto = L % W;
                        uint64_t inicio = A + w * tamanho_base + min((uint64_t)w, resto);
                        uint64_t tamanho = tamanho_base + ((uint64_t)w < resto ? 1 : 0);
                        uint64_t fim = inicio + tamanho - 1;
                        
                        processar_bloco(inicio, fim, res_filho);
                    } else {
                        processar_ciclico(A, B, w, W, res_filho);
                    }

                    ofstream arq_parcial("parcial_" + to_string(w) + ".txt");
                    arq_parcial << scientific << setprecision(10) << res_filho.tempo_execucao << " " << res_filho.total_passos << "\n";
                    arq_parcial.close();
                    _exit(0);
                }
            }

            for (int w = 0; w < W; ++w) {
                wait(NULL);
            }

            auto t_fim_filhos = chrono::high_resolution_clock::now();

            uint64_t soma = 0;
            for (int w = 0; w < W; ++w) {
                string nome_arq = "parcial_" + to_string(w) + ".txt";
                ifstream arq_parcial(nome_arq);
                double t_filho = 0.0;
                uint64_t p_filho = 0;
                if (arq_parcial >> t_filho >> p_filho) {
                    if (tempo_max_filho < 0 || t_filho > tempo_max_filho) tempo_max_filho = t_filho;
                    if (tempo_min_filho < 0 || t_filho < tempo_min_filho) tempo_min_filho = t_filho;
                    soma += p_filho;
                }
                arq_parcial.close();
                remove(nome_arq.c_str());
            }
            total_passos_global = soma;

            auto t_fim_agregacao = chrono::high_resolution_clock::now();
            tempo_agregacao = chrono::duration<double>(t_fim_agregacao - t_fim_filhos).count();
        }
    }

    auto t_fim_total = chrono::high_resolution_clock::now();
    double tempo_total = chrono::duration<double>(t_fim_total - t_inicio_pai).count();

    string linha_saida = modo + ", " +
                              particao + ", " +
                              to_string(W) + ", " +
                              to_string(L) + ", " +
                              para_notacao_cientifica(tempo_total) + ", " +
                              para_notacao_cientifica(tempo_max_filho) + ", " +
                              para_notacao_cientifica(tempo_min_filho) + ", " +
                              para_notacao_cientifica(tempo_agregacao);
    cout << linha_saida << endl;

    ifstream checa_arquivo(arquivo_saida);
    bool arquivo_existe = checa_arquivo.good();
    checa_arquivo.close();

    ofstream arq(arquivo_saida, ios::out | ios::app);
    if (arq.is_open()) {
        if (!arquivo_existe) {
            arq << "modo, particao, W, L, tempo_total, tempo_max_filho, tempo_min_filho, tempo_agregacao\n";
        }
        arq << linha_saida << "\n";
        arq.flush();
        arq.close();
    }

    cout <<"[CHECKPOINT - Soma dos passos de Collatz]: "<< total_passos_global << endl;

    return 0;
}
