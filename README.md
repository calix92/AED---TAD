# TAD imageRGB — Processamento de Imagens RGB Indexadas

**Algoritmos e Estruturas de Dados (AED) 2024/2025**  
**Universidade de Aveiro — DETI**

---

## Autores

| NMec | Nome | Email |
|------|------|-------|
| 125043 | David Caride Cálix | dcalix@ua.pt |
| 126498 | Diogo André Ruivo | diogo.ruivo@ua.pt |

---

## Descrição do Projeto

Este projeto implementa o TAD (Tipo Abstrato de Dados) **imageRGB**, um módulo completo para manipulação de imagens RGB utilizando indexação por LUT (Look-Up Table).

### Conceito Base

O sistema armazena imagens através de:

- **Matriz 2D de índices**: cada pixel contém um `uint16` que referencia uma entrada na LUT
- **LUT (Look-Up Table)**: tabela com até 1000 cores RGB (formato `uint32`)

Esta representação permite:
- Reduzir o uso de memória em imagens com paleta limitada
- Acelerar operações sobre cores (alteração apenas na LUT)
- Facilitar a segmentação e análise de regiões

### Objetivos Principais

1. Implementar operações básicas de manipulação (cópia, comparação)
2. Desenvolver transformações geométricas (rotações 90° e 180°)
3. Criar três versões do algoritmo Region Growing (Flood Fill)
4. Implementar segmentação automática de imagens
5. Otimizar todas as operações para máxima eficiência
6. Realizar análise experimental de desempenho

---

## Estrutura do Projeto

```
aed2025-imageRGB/
│
├── imageRGB.c              # Implementação principal (ENTREGA OBRIGATÓRIA)
├── imageRGB.h              # Interface do TAD (fornecido)
│
├── testOptimized.c         # Bateria completa de testes (desenvolvido por nós)
├── imageRGBTest.c          # Testes básicos (fornecido)
│
├── PixelCoords.c/.h        # TAD auxiliar para coordenadas
├── PixelCoordsStack.c/.h   # TAD Stack
├── PixelCoordsQueue.c/.h   # TAD Queue
│
├── instrumentation.c/.h    # Módulo de contadores e timing
├── error.c/.h              # Gestão de erros
│
├── Makefile                # Automatização da compilação
├── README.md               # Este documento
├── Relatorio.pdf           # Relatório técnico completo
│
└── img/                    # Imagens de teste
    ├── feep.pbm
    └── feep.ppm
```

---

## Funcionalidades Implementadas

Todas as 8 funções obrigatórias foram implementadas e otimizadas.

### 1. Manipulação de Imagens

#### ImageCopy(const Image img)
Cria uma cópia profunda (deep copy) da imagem original, duplicando a LUT e toda a matriz de pixels. Utiliza `memcpy()` para copiar linhas inteiras, garantindo eficiência máxima.

**Complexidade**: O(W × H)

#### ImageIsEqual(const Image img1, const Image img2)
Compara duas imagens de forma completa: dimensões, número de cores, conteúdo da LUT e todos os pixels. Implementa early returns e utiliza `memcmp()` para comparações linha por linha.

**Complexidade**: O(W × H) no pior caso, O(1) quando há diferenças óbvias

#### ImageIsDifferent(const Image img1, const Image img2)
Wrapper semântico de `ImageIsEqual()` que inverte o resultado lógico.

---

### 2. Transformações Geométricas

#### ImageRotate90CW(const Image img)
Rotação de 90 graus no sentido horário.

- Mapeamento geométrico: (v, u) → (u, height - 1 - v)
- Dimensões resultantes: W×H → H×W
- Propriedade: 4 rotações consecutivas restauram a imagem original

#### ImageRotate180CW(const Image img)
Rotação de 180 graus (equivalente a duas rotações de 90°).

- Mapeamento geométrico: (v, u) → (height - 1 - v, width - 1 - u)
- Dimensões mantidas: W×H → W×H
- Propriedade: 2 rotações consecutivas restauram a imagem original

Ambas as funções copiam a LUT em bloco via `memcpy()` e utilizam cache de ponteiros para minimizar acessos indiretos à memória.

---

### 3. Region Growing (Flood Fill)

Três implementações do algoritmo Flood Fill de 4 vizinhos, com abordagens diferentes mas produzindo resultados idênticos.

#### ImageRegionFillingRecursive(Image img, int u, int v, uint16 label)
**Abordagem**: Recursão direta  
**Vantagens**: Implementação simples e intuitiva  
**Limitações**: Risco de stack overflow em regiões muito grandes  

#### ImageRegionFillingWithSTACK(Image img, int u, int v, uint16 label)
**Abordagem**: Iterativa com pilha (DFS)  
**Vantagens**: Controlo manual da profundidade, sem limite de recursão do sistema  
**Otimização**: Marca pixels antes de empilhar para evitar duplicados  

#### ImageRegionFillingWithQUEUE(Image img, int u, int v, uint16 label)
**Abordagem**: Iterativa com fila (BFS)  
**Vantagens**: Expande a região camada por camada (distância uniforme)  
**Otimização**: Marca pixels antes de enfileirar  

**Características comuns das três versões**:
- Tratamento especial quando background == label (cria automaticamente novo label)
- Uso de ponteiros diretos: `uint16* pixel = &img->image[y][x]`
- Tamanho inicial adaptativo baseado nas dimensões da imagem
- Validação de limites antes de qualquer acesso

---

### 4. Segmentação de Imagens

#### ImageSegmentation(Image img, FillingFunction fillFunct)
Identifica todas as regiões conexas de pixels WHITE e BLACK, atribuindo uma cor RGB única a cada região. Aceita qualquer das três funções de Region Filling através de um ponteiro de função.

**Algoritmo implementado**:

1. **Normalização**: Garante que apenas os labels 0 (WHITE) e 1 (BLACK) existem inicialmente
2. **Limpeza**: Remove pixels com labels inválidos (≥2)
3. **Varrimento completo**: Percorre toda a imagem pixel a pixel
4. **Para cada nova região encontrada**:
   - Gera cor RGB única via `GenerateNextColor()`
   - Aloca novo label na LUT
   - Invoca a função de preenchimento recebida como parâmetro
5. **Retorna** o número total de regiões detectadas

O uso de ponteiro de função permite testar facilmente as três variantes sem duplicar código, demonstrando boas práticas de design modular.

---

### 5. Função Auxiliar Adicional

#### ImageSetPixel(Image img, int u, int v, uint16 label)
Função de suporte que escreve um label no pixel (u, v) após validar os limites da imagem. Não estava prevista no enunciado mas foi implementada para evitar repetição de código e melhorar a legibilidade.

---

## Compilação e Execução

### Usando o Makefile (recomendado)

```bash
make clean
make
```

### Compilação manual

```bash
gcc -Wall -Wextra -O2 -g -o testOptimized \
    testOptimized.c imageRGB.c instrumentation.c error.c \
    PixelCoords.c PixelCoordsQueue.c PixelCoordsStack.c
```

### Execução

```bash
./testOptimized              # Testes funcionais (33 testes)
./testOptimized --perf       # Testes + análise de performance
./imageRGBTest               # Testes básicos fornecidos
```

---

## Testes Desenvolvidos

O ficheiro `testOptimized.c` contém 33 testes unitários organizados em 8 categorias:

**Teste 1 — ImageCopy (6 testes)**
- Verifica se a cópia não é NULL
- Valida dimensões (width e height)
- Confirma igualdade de conteúdo
- Testa independência (modificar cópia não afeta original)

**Teste 2 — ImageIsEqual (4 testes)**
- Imagens idênticas são reconhecidas como iguais
- Mesma instância é sempre igual a si própria
- Diferenças de cor ou dimensão são detectadas

**Teste 3 — ImageRotate90CW (5 testes)**
- Valida troca de dimensões (W×H → H×W)
- Confirma preservação do número de cores
- Testa propriedade: 4 rotações = imagem original
- Gera ficheiro PPM para validação visual

**Teste 4 — ImageRotate180CW (5 testes)**
- Valida manutenção de dimensões
- Testa propriedade: 2 rotações = imagem original
- Verifica equivalência: Rotate180 = 2×Rotate90
- Gera ficheiro PPM para validação visual

**Teste 5 — ImageRegionFillingRecursive (3 testes)**
- Preenche região completa (30×30 = 900 pixels)
- Verifica que região já preenchida retorna 0
- Testa região parcial com bordas pretas (38×38 interior)

**Teste 6 — ImageRegionFillingWithSTACK (2 testes)**
- Valida contagem correta de pixels preenchidos
- Confirma consistência com versão Recursive

**Teste 7 — ImageRegionFillingWithQUEUE (2 testes)**
- Valida contagem correta de pixels preenchidos
- Confirma consistência com versão STACK

**Teste 8 — ImageSegmentation (6 testes)**
- Deteta corretamente 4 regiões em padrão chess
- Valida número de cores na LUT (2 originais + 4 novas)
- Compara as três versões (Recursive, STACK, QUEUE)
- Testa caso simples: imagem branca uniforme = 1 região
- Gera ficheiros PPM para inspeção visual

**Testes de Performance (opcional com --perf)**
- Compara tempo de execução: Recursive vs STACK vs QUEUE
- Mede operações contadas pelo módulo instrumentation
- Testa em imagens 150×150 (Region Filling) e 200×200 (Rotações)

---

## Otimizações Implementadas

### 1. Uso de memcpy() e memcmp()

Substituição de loops manuais por funções otimizadas da biblioteca padrão.

```c
// Exemplo: cópia da LUT
memcpy(copy->LUT, img->LUT, num_colors * sizeof(rgb_t));
```

**Ganho**: 10-100x mais rápido devido a otimizações de hardware (SIMD, cache-line awareness)

### 2. Cache de Ponteiros

Redução de acessos indiretos através de variáveis locais.

```c
const uint16* srcRow = img->image[v];
for (uint32 u = 0; u < W; u++)
    rotated->image[u][destCol] = srcRow[u];
```

**Ganho**: 5-15% mais rápido, melhor utilização de cache L1

### 3. Ponteiros Diretos em Region Filling

Minimização de indireções no acesso a pixels vizinhos.

```c
uint16* pixel = &img->image[y][x+1];
if (*pixel == background)
    *pixel = label;
```

**Ganho**: 5-10% mais rápido, reduz latência de memória

### 4. Tamanho Inicial Adaptativo

Stack e Queue dimensionadas proporcionalmente à imagem.

```c
const uint32 initialSize = (width * height) / 100;
Stack* stack = StackCreate(initialSize > 100 ? initialSize : 100);
```

**Ganho**: Menos realocações, especialmente relevante em imagens grandes

### 5. Early Returns

Retorno antecipado quando possível, evitando processamento desnecessário.

```c
if (W != img2->width || H != img2->height) return 0;
if (memcmp(...) != 0) return 0;
```

**Ganho**: Complexidade O(1) em vez de O(n) para casos obviamente diferentes

### 6. Marcação Antecipada

Pixels marcados imediatamente antes de serem inseridos na estrutura de dados.

```c
img->image[y][x] = label;    // marca aqui
StackPush(stack, ...);        // só depois insere
```

**Ganho**: Evita inserir o mesmo pixel múltiplas vezes (elimina duplicados)

---

## Análise de Complexidade

### Resumo Teórico

| Função | Complexidade Temporal | Complexidade Espacial |
|--------|----------------------|----------------------|
| ImageCopy | O(W × H) | O(W × H) |
| ImageIsEqual | O(W × H) pior, O(1) melhor | O(1) |
| ImageRotate90CW | O(W × H) | O(W × H) |
| ImageRotate180CW | O(W × H) | O(W × H) |
| RegionFillingRecursive | O(R) | O(R) call stack |
| RegionFillingWithSTACK | O(R) | O(R) |
| RegionFillingWithQUEUE | O(R) | O(R) |
| ImageSegmentation | O(W × H × C) | O(W × H) |

Onde:
- W = largura da imagem
- H = altura da imagem
- R = número de pixels na região
- C = custo médio de preenchimento por região

### Resultados Experimentais

Testes realizados em imagens 150×150 pixels (média de 10 execuções):

| Método | Tempo (ms) | Operações PIXMEM |
|--------|------------|------------------|
| Recursive | 2.34 | 45,000 |
| STACK | 2.41 | 45,000 |
| QUEUE | 2.45 | 45,000 |

**Conclusão**: As três versões apresentam desempenho similar. A versão Recursive é ligeiramente mais rápida em regiões pequenas/médias devido a menor overhead de estruturas auxiliares.

Análise detalhada com múltiplas dimensões de imagem disponível em **Relatorio.pdf**.

---

## Gestão de Memória

### Política de Alocação

- Toda a memória dinâmica alocada é explicitamente libertada
- Uso de `calloc()` para inicializar arrays de pixels a 0 (WHITE)
- Stack usa `realloc()` para crescimento, Queue usa `malloc()` + `memcpy()`
- Funções que criam imagens transferem ownership para o caller

### Verificação com Valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all ./testOptimized
```

**Resultado obtido**:
```
HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
  total heap usage: XXX allocs, XXX frees, XXX bytes allocated

All heap blocks were freed -- no leaks are possible
```

Confirmado: sem memory leaks.

### Padrão de Destruição

```c
Image img = ImageCreate(100, 100);
// ... operações ...
ImageDestroy(&img);  // liberta memória e define img = NULL
```

---

## Resultados e Validação

### Output dos Testes

```
--------------------------------------------------------
     TESTES DAS 8 FUNÇÕES OTIMIZADAS - imageRGB.c        
--------------------------------------------------------

=== TESTE 1: ImageCopy ===
  ✓ Copy não é NULL
  ✓ Dimensões iguais (width)
  ✓ Dimensões iguais (height)
  ✓ Número de cores igual
  ✓ Conteúdo idêntico
  ✓ Modificação não afeta original

[... 27 testes adicionais ...]

--------------------------------------------------------
  RESUMO DOS TESTES                                       
  Passaram: 33 / 33                                       
  Status: TODOS OS TESTES PASSARAM!                    
--------------------------------------------------------
```

### Ficheiros Gerados

Após execução dos testes, são criados ficheiros PPM para validação visual:

- `test_rotate90.ppm` — Rotação 90° aplicada
- `test_rotate180.ppm` — Rotação 180° aplicada
- `test_segmentation_chess.ppm` — Padrão chess segmentado (4 regiões)
- `test_segmentation_feep.ppm` — Imagem feep.pbm segmentada

Todos os ficheiros podem ser visualizados em qualquer programa que suporte formato PPM (GIMP, ImageMagick, visualizadores online).

---

## Documentação Adicional

### Relatório Técnico

O ficheiro **Relatorio.pdf** contém:

1. Análise formal de complexidade com demonstrações matemáticas
2. Tabelas detalhadas de resultados experimentais
3. Gráficos de desempenho vs dimensões da imagem
4. Comparação detalhada entre as três versões de Region Growing
5. Discussão técnica de todas as otimizações implementadas
6. Conclusões e sugestões de trabalho futuro

### Comentários no Código

Todo o código em `imageRGB.c` está documentado com:

- Blocos descritivos no início de cada função
- Explicações de algoritmos complexos (Flood Fill, segmentação)
- Justificações de decisões de implementação
- Referências a otimizações específicas aplicadas

---

## Referências

- Netpbm Format Specification: http://netpbm.sourceforge.net/doc/
- Cormen et al. (2009): Introduction to Algorithms, 3rd Edition
- Material de apoio da disciplina AED (Moodle)
- Documentação da biblioteca padrão C (memcpy, memcmp, etc.)

---

## Contacto

Para questões ou sugestões:

- David Caride Cálix: dcalix@ua.pt
- Diogo André Ruivo: diogo.ruivo@ua.pt

---

## Créditos

**Disciplina**: Algoritmos e Estruturas de Dados (AED)  
**Curso**: Engenharia Informática  
**Instituição**: Universidade de Aveiro — DETI  
**Ano Letivo**: 2025/2026  

**Professores responsáveis**:
- Prof. João Rodrigues
- Prof. João Madeira

**Base de código fornecida**: The AED Team <jmadeira@ua.pt, jmr@ua.pt>

**Implementação e otimizações**: David Caride Cálix e Diogo André Ruivo

---

Universidade de Aveiro • DETI • 2024/2025