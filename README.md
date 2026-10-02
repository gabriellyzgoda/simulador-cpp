# Simulação Dinâmica de Partículas — Sequencial vs. Paralela (OpenMP)

**Integrante:** Gabrielly Azevedo Zgoda

Trabalho da disciplina de Programação Paralela: implementação de uma simulação gravitacional de N-corpos em duas versões (sequencial e paralela com OpenMP), com visualização e comparação de desempenho.

## Arquivos deste pacote

| Arquivo | O que é |
|---|---|
| `simulacao_gravitacional.cpp` | Versão sequencial da simulação |
| `simulacao_paralela.cpp` | Versão paralela (OpenMP) da simulação |
| `README.md` | Este arquivo |
| `relatorio.pdf` | Relatório do trabalho |
| `visualizador_simulacao.html` | Página que anima a simulação a partir do CSV gerado (ver seção própria abaixo) |

## Como compilar e rodar

Compilado e testado em ambiente Linux (WSL2/Ubuntu no Windows 11) com g++. 
O suporte a OpenMP (`-fopenmp`) é padrão no GCC, não requer instalação extra.

```bash
# Versão sequencial
g++ simulacao_gravitacional.cpp -o simulacao -O2
./simulacao
# (pede a quantidade de partículas ao rodar)

# Versão paralela
g++ -fopenmp simulacao_paralela.cpp -o simulacao_paralela -O2
./simulacao_paralela
# (pede a quantidade de partículas e a quantidade de threads ao rodar)
```

Ambos os programas imprimem o tempo de execução no terminal ao final, e geram um arquivo `.csv` com a posição de cada partícula ao longo da simulação (`simulacao.csv` e `simulacao_paralela.csv`, respectivamente).

## Como a simulação funciona

Cada partícula é representada por uma `struct` com massa, posição (x, y), velocidade (vx, vy) e aceleração (ax, ay). A cada "passo de tempo" (o loop principal roda 20.000 vezes), o programa faz três coisas para cada partícula:

1. **Calcula a aceleração** resultante da gravidade de todas as outras partículas, usando a Lei da Gravitação Universal de Newton: `força = G × massa1 × massa2 ÷ distância²`. Um pequeno fator de suavização (*softening*) é somado à distância ao quadrado para evitar divisão por zero quando duas partículas ficam muito próximas.
2. **Atualiza a velocidade** com base nessa aceleração.
3. **Atualiza a posição** com base na velocidade.

O integrador usado é o **Velocity Verlet**: a aceleração é recalculada *no meio* de cada passo (depois de já ter movido a posição), o que evita o acúmulo de erro de energia que aconteceria com o método de Euler simples — importante para simulações orbitais longas, onde Euler faria as órbitas "vazarem" energia e se desformarem com o tempo.

**Geração das partículas:** a primeira partícula é sempre uma "estrela" central fixa (massa muito maior que as demais). As demais são geradas com posição aleatória (a uma distância e ângulo sorteados) e velocidade calculada para gerar uma órbita circular estável: `velocidade = raiz(G × massa_central ÷ raio)` — a mesma fórmula usada para calcular a velocidade de satélites reais. Uma semente fixa (42) garante que a mesma sequência "aleatória" seja gerada toda vez, tornando as duas versões (sequencial e paralela) comparáveis com exatidão.

A cada 20 passos, a posição de cada partícula é gravada numa linha do CSV (`passo,id,x,y`) — isso evita gerar um arquivo gigante salvando todos os 20.000 passos.

## Diferença entre a versão sequencial e a paralela

Ambas as versões calculam a aceleração de cada partícula considerando a influência gravitacional de todas as demais, de forma independente — ou seja, nenhuma das duas aproveita a simetria da 3ª Lei de Newton (ação e reação) para economizar cálculos. Essa escolha foi feita para isolar o efeito real da paralelização: ao garantir que as duas versões realizam exatamente o mesmo volume de operações, qualquer diferença de tempo medida pode ser atribuída à distribuição do trabalho entre threads, e não a uma diferença na quantidade de cálculos realizados.

A diferença entre elas está apenas em **como** esse cálculo é executado:

- **Sequencial** (`simulacao_gravitacional.cpp`): percorre todas as partículas em um único processo, calculando a aceleração de cada uma em relação às demais, uma de cada vez.
- **Paralela** (`simulacao_paralela.cpp`, com OpenMP): distribui esse mesmo cálculo entre múltiplas threads, uma partícula por thread a cada instante. Isso também evita condições de corrida (duas threads escrevendo na mesma partícula ao mesmo tempo), já que cada thread só escreve nos dados da partícula que lhe foi atribuída. A quantidade de threads é escolhida pelo usuário na hora de rodar, via `omp_set_num_threads()`.

## O arquivo CSV gerado

Formato: `passo,id,x,y` — por exemplo, `20,1,188.3,928.8` significa "no passo 20, a partícula 1 estava na posição (188.3, 928.8)". Nenhum dos dois programas desenha nada — eles só calculam e gravam os números; quem transforma isso em imagem é o visualizador HTML descrito a seguir.

## Visualizador (`visualizador_simulacao.html`)

Esse arquivo é um diferencial deste trabalho que a aluna pensou em acrescentar para que fosse permitida a vsualização das partículas. É uma página autocontida (não precisa de internet nem de servidor, basta abrir no navegador com duploclique) que lê um CSV gerado por qualquer uma das duas versões e anima o movimento das partículas, funcionando como uma prova visual de que a simulação está calculando órbitas corretamente.

**Para usar:** rode a versão sequencial ou a paralela normalmente (isso gera `simulacao.csv` ou `simulacao_paralela.csv`), abra o `visualizador_simulacao.html` no navegador, clique em **"Escolherarquivo"** e selecione o CSV gerado. A página desenha a posição de cada partícula a cada "quadro" (passo salvo no CSV), com controles de
play/pause, velocidade de reprodução e uma barra para avançar/retroceder manualmente pela simulação.

Como as duas versões usam a mesma semente de geração de partículas e o mesmo cálculo físico, abrir o CSV de qualquer uma das duas no visualizador deve produzir a mesma animação — uma forma visual de confirmar que a paralelização não alterou o resultado físico da simulação, só o tempo de execução.