// testOptimized.c - Testes específicos para as 8 funções otimizadas
//
// Compila com: gcc -Wall -Wextra -O2 -g -o testOptimized testOptimized.c imageRGB.c instrumentation.c error.c PixelCoords.c PixelCoordsQueue.c PixelCoordsStack.c
// Executa: ./testOptimized
// OU
// ./testOptimized --perf (para teste de performance)
//
// AED 2025 - DETI/UA

/*------------------------------------------------------------------
 * testOptimized.c
 *
 * Conjunto de testes desenvolvido para validar:
 *   - correção funcional das 8 funções otimizadas
 *   - consistência entre versões recursive / stack / queue
 *   - comportamento geométrico das rotações
 *   - segmentação correta em padrões artificiais e PBM reais
 *
 * Este ficheiro complementa o imageRGBTest.c do Professor e cobre cenários
 * que não estavam incluídos no original — especialmente:
 *   - integridade das deep copies
 *   - igualdade estrutural pós-transformações repetidas
 *   - teste exaustivo das 3 variantes de Flood Fill
 *   - segmentação rigorosa em imagens definidas por regras
 *
 * Inclui ainda um modo --perf para medir instruções contadas pelo
 * módulo de instrumentação, permitindo comparação direta entre
 * implementações otimizadas.
 *-----------------------------------------------------------------*/


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "imageRGB.h"
#include "instrumentation.h"

// Cores para facilitar testes
#define RED    0xff0000
#define GREEN  0x00ff00
#define BLUE   0x0000ff
#define YELLOW 0xffff00

// Contador de testes
int tests_passed = 0;
int tests_total = 0;

// Função auxiliar para validar testes
void test(const char* name, int condition) {
    tests_total++;
    if (condition) {
        tests_passed++;
        printf("  [x] %s\n", name);
    } else {
        printf("  [ ] FALHOU: %s\n", name);
    }
}

// ============================================================================
// TESTE 1: ImageCopy
// ============================================================================
void test_ImageCopy() {
    printf("\n=== TESTE 1: ImageCopy ===\n");
    
    Image original = ImageCreateChess(100, 80, 20, RED);
    Image copy = ImageCopy(original);
    
    test("Copy não é NULL", copy != NULL);
    test("Dimensões iguais (width)", ImageWidth(copy) == 100);
    test("Dimensões iguais (height)", ImageHeight(copy) == 80);
    test("Número de cores igual", ImageColors(copy) == ImageColors(original));
    test("Conteúdo idêntico", ImageIsEqual(original, copy));
    
    // Modificar cópia não deve afetar original
    ImageRegionFillingWithSTACK(copy, 50, 40, 2);
    test("Modificação não afeta original", ImageIsDifferent(original, copy));
    
    ImageDestroy(&original);
    ImageDestroy(&copy);
}

// ============================================================================
// TESTE 2: ImageIsEqual
// ============================================================================
void test_ImageIsEqual() {
    printf("\n=== TESTE 2: ImageIsEqual ===\n");
    
    Image img1 = ImageCreateChess(60, 60, 15, BLUE);
    Image img2 = ImageCreateChess(60, 60, 15, BLUE);
    Image img3 = ImageCreateChess(60, 60, 15, GREEN);
    Image img4 = ImageCreate(70, 60);
    
    test("Imagens idênticas são iguais", ImageIsEqual(img1, img2));
    test("Mesma instância é igual", ImageIsEqual(img1, img1));
    test("Cores diferentes → não iguais", !ImageIsEqual(img1, img3));
    test("Dimensões diferentes → não iguais", !ImageIsEqual(img1, img4));
    
    ImageDestroy(&img1);
    ImageDestroy(&img2);
    ImageDestroy(&img3);
    ImageDestroy(&img4);
}

// ============================================================================
// TESTE 3: ImageRotate90CW
// ============================================================================
void test_ImageRotate90CW() {
    printf("\n=== TESTE 3: ImageRotate90CW ===\n");
    
    Image original = ImageCreateChess(120, 80, 20, RED);
    Image rotated = ImageRotate90CW(original);
    
    test("Rotação não é NULL", rotated != NULL);
    test("Dimensões trocadas: width", ImageWidth(rotated) == 80);
    test("Dimensões trocadas: height", ImageHeight(rotated) == 120);
    test("Cores preservadas", ImageColors(rotated) == ImageColors(original));
    
    // 4 rotações = imagem original
    Image r1 = ImageRotate90CW(original);
    Image r2 = ImageRotate90CW(r1);
    Image r3 = ImageRotate90CW(r2);
    Image r4 = ImageRotate90CW(r3);
    
    test("4 rotações 90° = original", ImageIsEqual(original, r4));
    
    ImageSavePPM(rotated, "test_rotate90.ppm");
    printf("  → Ficheiro salvo: test_rotate90.ppm\n");
    
    ImageDestroy(&original);
    ImageDestroy(&rotated);
    ImageDestroy(&r1);
    ImageDestroy(&r2);
    ImageDestroy(&r3);
    ImageDestroy(&r4);
}

// ============================================================================
// TESTE 4: ImageRotate180CW
// ============================================================================
void test_ImageRotate180CW() {
    printf("\n=== TESTE 4: ImageRotate180CW ===\n");
    
    Image original = ImageCreateChess(100, 60, 20, YELLOW);
    Image rotated = ImageRotate180CW(original);
    
    test("Rotação não é NULL", rotated != NULL);
    test("Width preservado", ImageWidth(rotated) == 100);
    test("Height preservado", ImageHeight(rotated) == 60);
    
    // 2 rotações 180° = original
    Image r2 = ImageRotate180CW(rotated);
    test("2 rotações 180° = original", ImageIsEqual(original, r2));
    
    // 180° = 2x90°
    Image r90_1 = ImageRotate90CW(original);
    Image r90_2 = ImageRotate90CW(r90_1);
    test("Rotação 180° = 2x90°", ImageIsEqual(rotated, r90_2));
    
    ImageSavePPM(rotated, "test_rotate180.ppm");
    printf("  → Ficheiro salvo: test_rotate180.ppm\n");
    
    ImageDestroy(&original);
    ImageDestroy(&rotated);
    ImageDestroy(&r2);
    ImageDestroy(&r90_1);
    ImageDestroy(&r90_2);
}

// ============================================================================
// TESTE 5: ImageRegionFillingRecursive
// ============================================================================
void test_RegionFillingRecursive() {
    printf("\n=== TESTE 5: ImageRegionFillingRecursive ===\n");
    
    // Teste 5.1: Preencher imagem 30x30 toda branca
    Image img = ImageCreate(30, 30);
    int count = ImageRegionFillingRecursive(img, 15, 15, 2);
    
    test("Preencheu 900 pixels (30x30)", count == 900);
    
    // Teste 5.2: Preencher região já preenchida = 0
    int count2 = ImageRegionFillingRecursive(img, 15, 15, 2);
    test("Região já preenchida = 0 pixels", count2 == 0);
    
    ImageDestroy(&img);
    
    // Teste 5.3: Região parcial com bordas
    Image img2 = ImageCreate(40, 40);
    // Criar borda preta
    for (uint32 i = 0; i < 40; i++) {
    ImageSetPixel(img2, i, 0,   BLACK);  // linha de cima
    ImageSetPixel(img2, i, 39,  BLACK);  // linha de baixo
    ImageSetPixel(img2, 0,   i, BLACK);  // coluna esquerda
    ImageSetPixel(img2, 39,  i, BLACK);  // coluna direita
}

    
    int count3 = ImageRegionFillingRecursive(img2, 20, 20, 2);
    test("Região parcial 38x38 = 1444 pixels", count3 == 1444);
    
    ImageDestroy(&img2);
}

// ============================================================================
// TESTE 6: ImageRegionFillingWithSTACK
// ============================================================================
void test_RegionFillingWithSTACK() {
    printf("\n=== TESTE 6: ImageRegionFillingWithSTACK ===\n");
    
    Image img = ImageCreate(30, 30);
    int count = ImageRegionFillingWithSTACK(img, 15, 15, 2);
    
    test("Preencheu 900 pixels (30x30)", count == 900);
    
    // Comparar com Recursive
    Image img_rec = ImageCreate(30, 30);
    ImageRegionFillingRecursive(img_rec, 15, 15, 2);
    
    test("STACK = Recursive (resultado idêntico)", ImageIsEqual(img, img_rec));
    
    ImageDestroy(&img);
    ImageDestroy(&img_rec);
}

// ============================================================================
// TESTE 7: ImageRegionFillingWithQUEUE
// ============================================================================
void test_RegionFillingWithQUEUE() {
    printf("\n=== TESTE 7: ImageRegionFillingWithQUEUE ===\n");
    
    Image img = ImageCreate(30, 30);
    int count = ImageRegionFillingWithQUEUE(img, 15, 15, 2);
    
    test("Preencheu 900 pixels (30x30)", count == 900);
    
    // Comparar com STACK
    Image img_stack = ImageCreate(30, 30);
    ImageRegionFillingWithSTACK(img_stack, 15, 15, 2);
    
    test("QUEUE = STACK (resultado idêntico)", ImageIsEqual(img, img_stack));
    
    ImageDestroy(&img);
    ImageDestroy(&img_stack);
}

// ============================================================================
// TESTE 8: ImageSegmentation
// ============================================================================
void test_ImageSegmentation() {
    printf("\n=== TESTE 8: ImageSegmentation ===\n");
    
    // Teste 8.1: Segmentar padrão chess (4 regiões brancas)
    Image img = ImageCreateChess(60, 60, 30, BLACK);
    int regions = ImageSegmentation(img, ImageRegionFillingWithSTACK);
    
    test("Padrão chess tem 4 regiões", regions == 4);
    test("LUT tem 6 cores (2 orig + 4 novas)", ImageColors(img) == 6);
    
    ImageSavePPM(img, "test_segmentation_chess.ppm");
    printf("  → Ficheiro salvo: test_segmentation_chess.ppm\n");
    
    ImageDestroy(&img);
    
    // Teste 8.2: Comparar os 3 métodos
    Image img_rec = ImageLoadPBM("img/feep.pbm");
    Image img_stack = ImageLoadPBM("img/feep.pbm");
    Image img_queue = ImageLoadPBM("img/feep.pbm");
    
    int r_rec = ImageSegmentation(img_rec, ImageRegionFillingRecursive);
    int r_stack = ImageSegmentation(img_stack, ImageRegionFillingWithSTACK);
    int r_queue = ImageSegmentation(img_queue, ImageRegionFillingWithQUEUE);
    
    test("Mesmo nº regiões (Rec vs Stack)", r_rec == r_stack);
    test("Mesmo nº regiões (Stack vs Queue)", r_stack == r_queue);
    test("Mesmo nº cores geradas", ImageColors(img_rec) == ImageColors(img_stack));
    
    printf("  → Regiões encontradas: Rec=%d, Stack=%d, Queue=%d\n", 
           r_rec, r_stack, r_queue);
    
    ImageSavePPM(img_stack, "test_segmentation_feep.ppm");
    printf("  → Ficheiro salvo: test_segmentation_feep.ppm\n");
    
    ImageDestroy(&img_rec);
    ImageDestroy(&img_stack);
    ImageDestroy(&img_queue);
    
    // Teste 8.3: Imagem branca = 1 região
    Image white = ImageCreate(50, 50);
    int r_white = ImageSegmentation(white, ImageRegionFillingWithQUEUE);
    
    test("Imagem branca = 1 região", r_white == 1);
    
    ImageDestroy(&white);
}

// ============================================================================
// TESTE DE PERFORMANCE
// ============================================================================
void test_Performance() {
    printf("\n=== TESTE DE PERFORMANCE ===\n");
    printf("Comparando Region Filling 150150 (22500 pixels)\n\n");
    
    // Recursive
    Image img_rec = ImageCreate(150, 150);
    InstrReset();
    ImageRegionFillingRecursive(img_rec, 75, 75, 2);
    printf("Recursive:\n");
    InstrPrint();
    ImageDestroy(&img_rec);
    
    // STACK
    Image img_stack = ImageCreate(150, 150);
    InstrReset();
    ImageRegionFillingWithSTACK(img_stack, 75, 75, 2);
    printf("\nSTACK:\n");
    InstrPrint();
    ImageDestroy(&img_stack);
    
    // QUEUE
    Image img_queue = ImageCreate(150, 150);
    InstrReset();
    ImageRegionFillingWithQUEUE(img_queue, 75, 75, 2);
    printf("\nQUEUE:\n");
    InstrPrint();
    ImageDestroy(&img_queue);
    
    // Rotações
    printf("\n\nComparando Rotações 200x200\n\n");
    Image large = ImageCreateChess(200, 200, 40, RED);
    
    InstrReset();
    Image rot90 = ImageRotate90CW(large);
    printf("Rotate90CW:\n");
    InstrPrint();
    ImageDestroy(&rot90);
    
    InstrReset();
    Image rot180 = ImageRotate180CW(large);
    printf("\nRotate180CW:\n");
    InstrPrint();
    ImageDestroy(&rot180);
    ImageDestroy(&large);
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char* argv[]) {
    program_name = argv[0];
    
    printf("+----------------------------------------------------------+\n");
    printf("|     TESTES DAS 8 FUNÇÕES OTIMIZADAS - imageRGB.c         |\n");
    printf("+----------------------------------------------------------+\n");
    
    ImageInit();
    
    // Executar todos os testes
    test_ImageCopy();
    test_ImageIsEqual();
    test_ImageRotate90CW();
    test_ImageRotate180CW();
    test_RegionFillingRecursive();
    test_RegionFillingWithSTACK();
    test_RegionFillingWithQUEUE();
    test_ImageSegmentation();
    
    // Testes de performance (opcional)
    if (argc > 1 && strcmp(argv[1], "--perf") == 0) {
        test_Performance();
    }
    
    // Resumo final
    printf("\n+----------------------------------------------------------+\n");
    printf("|  RESUMO DOS TESTES                                       |\n");
    printf("|  Passaram: %2d / %2d                                       |\n", 
           tests_passed, tests_total);
    
    if (tests_passed == tests_total) {
        printf("|  Status: [x] TODOS OS TESTES PASSARAM!                     |\n");
    } else {
        printf("|  Status: [ ] ALGUNS TESTES FALHARAM                        |\n");
    }
    
    printf("+----------------------------------------------------------+\n");
    printf("|  Ficheiros gerados:                                      |\n");
    printf("|    • test_rotate90.ppm                                   |\n");
    printf("|    • test_rotate180.ppm                                  |\n");
    printf("|    • test_segmentation_chess.ppm                         |\n");
    printf("|    • test_segmentation_feep.ppm                          |\n");
    printf("+----------------------------------------------------------+\n");
    printf("|  Dica: Execute './testOptimized --perf' para ver         |\n");
    printf("|        comparações de performance detalhadas             |\n");
    printf("+----------------------------------------------------------+\n");
    
    return (tests_passed == tests_total) ? 0 : 1;
}