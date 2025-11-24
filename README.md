# 🖼️ Projeto: TAD imageRGB  
### AED — Algoritmos e Estruturas de Dados (2025/2026)  
### Universidade de Aveiro

## 👨‍💻 Autores  
- **David Caride Cálix** — NMec 125043  
- **Diogo André Ruivo** — NMec XXXXX  

---

## 📌 Descrição  

Este projeto consiste na implementação e otimização do TAD **imageRGB**, responsável por manipular imagens RGB através de:

- Uma matriz 2D de índices (labels)
- Uma LUT (Look-Up Table) com os respetivos valores RGB

Foram desenvolvidas todas as funcionalidades obrigatórias definidas no enunciado oficial do trabalho, incluindo cópia profunda, comparações, transformações geométricas, algoritmos de Region Growing e segmentação total por cores.

O **relatório obrigatório** encontra-se no ficheiro **Relatorio.pdf**.

---

## 📁 Estrutura do Projeto

imageRGB.c → Implementação do TAD (entrega obrigatória)
imageRGB.h → Interface fornecida (não alterado)
PixelCoords.* → TAD auxiliar fornecido
PixelCoordsStack.* → TAD auxiliar fornecido
PixelCoordsQueue.* → TAD auxiliar fornecido
instrumentation.* → Contadores e análise experimental
error.c / error.h → Erros fornecidos
testOptimized.c → Ficheiro de testes desenvolvido por nós
Relatorio.pdf → Relatório final
README.md → Este ficheiro

---

## 🔨 Compilação

Compilar todos os módulos com:

```bash
gcc -Wall -Wextra -O2 -g -o testOptimized testOptimized.c \
    imageRGB.c instrumentation.c \
    PixelCoords.c PixelCoordsQueue.c PixelCoordsStack.c error.c

👉 Executar todos os testes:
./testOptimized

👉 Executar testes com estatísticas de performance:
./testOptimized --perf

Após correr os testes, serão gerados ficheiros .ppm no diretório atual para inspeção visual.

---

✔️ Funcionalidades Implementadas

Todas as funções pedidas no enunciado foram implementadas e testadas:

🔹 Manipulação de imagens

ImageCopy(img)

ImageIsEqual(img1, img2)

ImageIsDifferent(img1, img2)

🔹 Transformações geométricas

ImageRotate90CW(img)

ImageRotate180CW(img)

🔹 Region Growing (4-vizinhos)

ImageRegionFillingRecursive(img, u, v, label)

ImageRegionFillingWithSTACK(img, u, v, label)

ImageRegionFillingWithQUEUE(img, u, v, label)

🔹 Segmentação completa

ImageSegmentation(img, fillFunct)

🔹 Função auxiliar adicionada

ImageSetPixel(img, u, v, label)

🧪 Testes

O ficheiro testOptimized.c inclui:

Testes unitários de todas as 8 funções pedidas

Teste de consistência entre Recursive / Stack / Queue

Testes de rotação e cópia

Segmentação de imagens fornecidas e geradas programaticamente

Testes de performance usando o módulo instrumentation

Geração automática de ficheiros .ppm para validação visual

📊 Relatório (documento separado)

Conforme o enunciado do trabalho, o relatório inclui:

Tabela com resultados experimentais

Análise formal da complexidade

Comparação entre custos formais e experimentais

Comparação entre as 3 versões de Region Growing (Recursive, Stack, Queue)

Discussão sobre otimizações realizadas

O relatório está incluído no ficheiro Relatorio.pdf.

🧹 Gestão de Memória

Todas as estruturas alocadas são libertadas corretamente

Verificado com: