/// imageRGB - A simple image module for handling RGB images,
///            pixel color values are represented using a look-up table (LUT)
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// The AED Team <jmadeira@ua.pt, jmr@ua.pt, ...>
/// 2025

// Student authors (fill in below):
// NMec: 125043
// Name: David Caride Cálix
// NMec:
// Name:Diogo André Ruivo
//
// Date:
//

#include "imageRGB.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PixelCoords.h"
#include "PixelCoordsQueue.h"
#include "PixelCoordsStack.h"
#include "instrumentation.h"

// The data structure
//
// A RGB image is stored in a structure containing 5 fields:
// Two integers store the image width and height.
// The next field is a pointer to an array that stores the pointers
// to the image rows.
//
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// FIXED SIZE of LUT for storing RGB triplets
#define FIXED_LUT_SIZE 1000

// Internal structure for storing RGB images
struct image {
  uint32 width;
  uint32 height;
  uint16** image;  // pointer to an array of pointers referencing the image rows
  uint16 num_colors;  // the number of colors (i.e., pixel labels) used
  rgb_t* LUT;         // table storing (R,G,B) triplets
};

// Design by Contract

// This module follows "design-by-contract" principles.
// `assert` is used to check function preconditions, postconditions
// and type invariants.
// This helps to find programmer errors.

/// Defensive Error Handling

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
//
// When one of these functions detects a memory or I/O error,
// it immediately prints an error message and aborts the program.
// This is a Fail-Fast strategy.
//
// You may use the `check` function to check a condition
// and exit the program with an error message if it is false.
// Note that it works similarly to `assert`, but cannot be disabled.
// It should be used to detect "external" uncontrolable errors,
// and not for "internal" programmer errors.
//
// See how it's used in ImageLoadPBM, for example.

// Check a condition and if false, print failmsg and exit.
static void check(int condition, const char* failmsg) {
  if (!condition) {
    perror(failmsg);
    exit(errno || 255);
  }
}

/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) {  ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!

/// Auxiliary (static) functions

static Image AllocateImageHeader(uint32 width, uint32 height) {
  // Create the header of an image data structure
  // Allocate the array of pointers to rows
  // And the look-up table

  Image newHeader = malloc(sizeof(struct image));
  // Error handling
  check(newHeader != NULL, "malloc");

  newHeader->width = width;
  newHeader->height = height;

  // Allocating the array of pointers to image rows
  newHeader->image = malloc(height * sizeof(uint16*));
  // Error handling
  check(newHeader->image != NULL, "Alloc failed ->image array");

  // Allocating the LUT
  newHeader->LUT = malloc(FIXED_LUT_SIZE * sizeof(rgb_t));
  // Error handling
  check(newHeader->LUT != NULL, "Alloc failed ->LUT array");

  // Initialize LUT with 2 fixed colors
  newHeader->num_colors = 2;
  newHeader->LUT[0] = 0xffffff;  // RGB WHITE
  newHeader->LUT[1] = 0x000000;  // RGB BLACK

  return newHeader;
}

// Allocate row of background (label=0) pixels
static uint16* AllocateRowArray(uint32 size) {
  uint16* newArray = calloc((size_t)size, sizeof(uint16));
  // Error handling
  check(newArray != NULL, "AllocateRowArray");

  return newArray;
}

/// Find color label for given RGB color in img LUT.
/// Return the label or -1 if not found.
static int LUTFindColor(Image img, rgb_t color) {
  for (uint16 index = 0; index < img->num_colors; index++) {
    if (img->LUT[index] == color) return index;
  }
  return -1;
}

/// Return color label for RGB color in img LUT.
/// Finds existing color or allocs new one!
static int LUTAllocColor(Image img, rgb_t color) {
  int index = LUTFindColor(img, color);
  if (index < 0) {
    check(img->num_colors < FIXED_LUT_SIZE, "LUT Overflow");
    index = img->num_colors++;
    img->LUT[index] = color;
  }
  return index;
}

/// Return a pseudo-random successor of the given color.
static rgb_t GenerateNextColor(rgb_t color) {
  return (color + 7639) & 0xffffff;
}

/// Image management functions

/// Create a new RGB image. All pixels with the background WHITE color.
///   width, height: the dimensions of the new image.
/// Requires: width and height must be non-negative.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageCreate(uint32 width, uint32 height) {
  assert(width > 0);
  assert(height > 0);

  // Just two possible pixel colors
  Image img = AllocateImageHeader(width, height);

  // Creating the image rows
  for (uint32 i = 0; i < height; i++) {
    img->image[i] = AllocateRowArray(width);  // Alloc all WHITE row
  }

  return img;
}

/// Create a new RGB image, with a color chess pattern.
/// The background is WHITE.
///   width, height: the dimensions of the new image.
///   edge: the width and height of a chess square.
///   color: the foreground color.
/// Requires: width, height and edge must be non-negative.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageCreateChess(uint32 width, uint32 height, uint32 edge, rgb_t color) {
  assert(width > 0);
  assert(height > 0);
  assert(edge > 0);

  Image img = ImageCreate(width, height);

  // Alloc color in LUT.
  uint8 label = LUTAllocColor(img, color);

  // Assigning the color to each image pixel

  // Pixel (0, 0) gets the chosen color label
  for (uint32 i = 0; i < height; i++) {
    uint32 I = i / edge;
    for (uint32 j = 0; j < width; j++) {
      uint32 J = j / edge;
      img->image[i][j] = (I + J) % 2 ? 0 : label;
    }
  }

  // Return the created chess image
  return img;
}

/// Create an image with a palete of generated colors.
Image ImageCreatePalete(uint32 width, uint32 height, uint32 edge) {
  assert(width > 0);
  assert(height > 0);
  assert(edge > 0);

  Image img = ImageCreate(width, height);

  // Fill LUT with generated colors
  rgb_t color = 0x000000;
  while (img->num_colors < FIXED_LUT_SIZE) {
    color = GenerateNextColor(color);
    img->LUT[img->num_colors++] = color;
  }

  // number of tiles
  uint32 wtiles = width / edge;

  // Pixel (0, 0) gets the chosen color label
  for (uint32 i = 0; i < height; i++) {
    uint32 I = i / edge;
    for (uint32 j = 0; j < width; j++) {
      uint32 J = j / edge;
      img->image[i][j] = (I * wtiles + J) % FIXED_LUT_SIZE;
    }
  }

  return img;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
///
/// Ensures: (*imgp)==NULL.
void ImageDestroy(Image* imgp) {
  assert(imgp != NULL);

  Image img = *imgp;

  for (uint32 i = 0; i < img->height; i++) {
    free(img->image[i]);
  }
  free(img->image);
  free(img->LUT);
  free(img);

  *imgp = NULL;
}

/// Create a deep copy of the image pointed to by img.
///   img : address of an Image variable.
///
/// On success, a new copied image is returned.
/// (The caller is responsible for destroying the returned image!)
/*------------------------------------------------------------------
 * ImageCopy
 *  Cria e retorna uma cópia profunda (deep copy) da imagem original.
 *  Copia a LUT e todos os índices de pixel.
 *  Deep copy sem realocar LUT: reaproveita a LUT criada por ImageCreate.
 *-----------------------------------------------------------------*/
Image ImageCopy(const Image img) {
    if (img == NULL) return NULL;

    Image copy = ImageCreate(img->width, img->height);
    if (copy == NULL) return NULL;

    // Copiar LUT
    copy->num_colors = img->num_colors;
    if (img->num_colors > 0) {
        const size_t lutBytes = (size_t)img->num_colors * sizeof(rgb_t);
        memcpy(copy->LUT, img->LUT, lutBytes);
    }

    // Copiar pixels linha por linha (muito mais rápido!)
    const size_t rowBytes = (size_t)img->width * sizeof(uint16);
    for (uint32 v = 0; v < img->height; v++) {
        memcpy(copy->image[v], img->image[v], rowBytes);
        // Incrementar PIXMEM por linha (aproximação: width acessos)
        PIXMEM += img->width;
    }

    return copy;
}



/// Printing on the console

/// These functions do not modify the image and never fail.

/// Output the raw RGB image (i.e., print the integer value of pixel).
void ImageRAWPrint(const Image img) {
  printf("width = %d height = %d\n", (int)img->width, (int)img->height);
  printf("num_colors = %d\n", (int)img->num_colors);
  printf("RAW image\n");

  // Print the pixel labels of each image row
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      printf("%2d", img->image[i][j]);
    }
    // At current row end
    printf("\n");
  }

  printf("LUT:\n");
  // Print the LUT (R,G,B) values
  for (int i = 0; i < (int)img->num_colors; i++) {
    rgb_t color = img->LUT[i];
    int r = color >> 16 & 0xff;
    int g = color >> 8 & 0xff;
    int b = color & 0xff;
    printf("%3d -> (%3d,%3d,%3d)\n", i, r, g, b);
  }

  printf("\n");
}

/// PBM file operations --- For BW images

// See PBM format specification: http://netpbm.sourceforge.net/doc/pbm.html

//
static void unpackBits(int nbytes, const uint8 bytes[], uint8 raw_row[]) {
  // bitmask starts at top bit
  int offset = 0;
  uint8 mask = 1 << (7 - offset);
  while (offset < 8) {  // or (mask > 0)
    for (int b = 0; b < nbytes; b++) {
      raw_row[8 * b + offset] = (bytes[b] & mask) != 0;
    }
    mask >>= 1;
    offset++;
  }
}

static void packBits(int nbytes, uint8 bytes[], const uint8 raw_row[]) {
  // bitmask starts at top bit
  int offset = 0;
  uint8 mask = 1 << (7 - offset);
  while (offset < 8) {  // or (mask > 0)
    for (int b = 0; b < nbytes; b++) {
      if (offset == 0) bytes[b] = 0;
      bytes[b] |= raw_row[8 * b + offset] ? mask : 0;
    }
    mask >>= 1;
    offset++;
  }
}

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PBM file.
/// Only binary PBM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageLoadPBM(const char* filename) {  ///
  int w, h;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  check((f = fopen(filename, "rb")) != NULL, "Open failed");
  // Parse PBM header
  check(fscanf(f, "P%c ", &c) == 1 && c == '4', "Invalid file format");
  skipComments(f);
  check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width");
  skipComments(f);
  check(fscanf(f, "%d", &h) == 1 && h >= 0, "Invalid height");
  check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected");

  // Allocate image
  img = AllocateImageHeader((uint32)w, (uint32)h);

  // Read pixels
  int nbytes = (w + 8 - 1) / 8;  // number of bytes for each row
  // using VLAs...
  uint8 bytes[nbytes];
  uint8 raw_row[nbytes * 8];
  for (uint32 i = 0; i < img->height; i++) {
    check(fread(bytes, sizeof(uint8), nbytes, f) == (size_t)nbytes,
          "Reading pixels");
    unpackBits(nbytes, bytes, raw_row);
    img->image[i] = AllocateRowArray((uint32)w);
    for (uint32 j = 0; j < (uint32)w; j++) {
      img->image[i][j] = (uint16)raw_row[j];
    }
  }

  fclose(f);
  return img;
}

/// Save image to PBM file.
/// On success, returns nonzero.
/// On failure, a partial and invalid file may be left in the system.
int ImageSavePBM(const Image img, const char* filename) {  ///
  assert(img != NULL);
  assert(img->num_colors == 2);

  int w = (int)img->width;
  int h = (int)img->height;
  FILE* f = NULL;

  check((f = fopen(filename, "wb")) != NULL, "Open failed");
  check(fprintf(f, "P4\n%d %d\n", w, h) > 0, "Writing header failed");

  // Write pixels
  int nbytes = (w + 8 - 1) / 8;  // number of bytes for each row
  // using VLAs...
  uint8 bytes[nbytes];
  uint8 raw_row[nbytes * 8];
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      raw_row[j] = (uint8)img->image[i][j];
    }
    // Fill padding pixels with WHITE
    memset(raw_row + w, WHITE, nbytes * 8 - w);
    packBits(nbytes, bytes, raw_row);
    check(fwrite(bytes, sizeof(uint8), nbytes, f) == (size_t)nbytes,
          "Writing pixels failed");
  }

  // Cleanup
  fclose(f);

  return 0;
}

/// PPM file operations --- For RGB images

/// Load a raw PPM file.
/// Only ASCII PPM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageLoadPPM(const char* filename) {
  assert(filename != NULL);
  int w, h;
  int levels;
  char c;
  FILE* f = NULL;

  check((f = fopen(filename, "rb")) != NULL, "Open failed");
  // Parse PPM header
  check(fscanf(f, "P%c ", &c) == 1 && c == '3', "Invalid file format");
  skipComments(f);
  check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width");
  skipComments(f);
  check(fscanf(f, "%d", &h) == 1 && h >= 0, "Invalid height");
  skipComments(f);
  check(fscanf(f, "%d", &levels) == 1 && 0 <= levels && levels <= 255,
        "Invalid depth");
  check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected");

  // Allocate image
  Image img = ImageCreate((uint32)w, (uint32)h);

  // Read pixels
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      int r, g, b;
      check(fscanf(f, "%d %d %d", &r, &g, &b) == 3 && 0 <= r && r <= levels &&
                0 <= g && g <= levels && 0 <= b && b <= levels,
            "Invalid pixel color");
      rgb_t color = r << 16 | g << 8 | b;
      uint16 index = LUTAllocColor(img, color);
      img->image[i][j] = index;
      // printf("[%u][%u]: (%d,%d,%d) -> %u (%6x)\n", i, j, r,g,b, index,
      // color);
    }
    fprintf(f, "\n");
  }

  fclose(f);
  return img;
}

/// Save image to PPM file.
/// On success, returns nonzero.
/// On failure, a partial and invalid file may be left in the system.
int ImageSavePPM(const Image img, const char* filename) {
  assert(img != NULL);

  int w = (int)img->width;
  int h = (int)img->height;
  FILE* f = NULL;

  check((f = fopen(filename, "wb")) != NULL, "Open failed");
  check(fprintf(f, "P3\n%d %d\n255\n", w, h) > 0, "Writing header failed");

  // The pixel RGB values
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      uint16 index = img->image[i][j];
      rgb_t color = img->LUT[index];
      int r = color >> 16 & 0xff;
      int g = color >> 8 & 0xff;
      int b = color & 0xff;
      fprintf(f, "  %3d %3d %3d", r, g, b);
    }
    fprintf(f, "\n");
  }

  // Cleanup
  fclose(f);

  return 0;
}

/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
uint32 ImageWidth(const Image img) {
  assert(img != NULL);
  return img->width;
}

/// Get image height
uint32 ImageHeight(const Image img) {
  assert(img != NULL);
  return img->height;
}

/// Get number of image colors
uint16 ImageColors(const Image img) {
  assert(img != NULL);
  return img->num_colors;
}

/// Image comparison

/// These functions do not modify the images and never fail.

/// Check if img1 and img2 represent equal images.
/// NOTE: The same rgb color may correspond to different LUT labels in
/// different images!
/*------------------------------------------------------------------
 *  Retorna 1 se forem iguais, 0 caso contrário.
 * ImageIsEqual
 *  Compara dimensões, nº de cores, LUT e todos os pixels.
 *  Retorna 1 se forem iguais, 0 caso contrário.
 *  Otimizações:
 *    - early-return em cada discrepância
 *    - cache de width/height em locals (evita loads repetidos)
 *    - memcmp na LUT (uint32) para acelerar
 *    - contagem PIXMEM (+2) por comparação de pixels (dois reads)
 *-----------------------------------------------------------------*/
int ImageIsEqual(const Image img1, const Image img2) {
    if (img1 == NULL || img2 == NULL) return 0;
    if (img1 == img2) return 1;

    const uint32 W = img1->width, H = img1->height;
    if (W != img2->width || H != img2->height) return 0;
    if (img1->num_colors != img2->num_colors) return 0;

    // Comparar LUT
    if (img1->num_colors > 0) {
        const size_t lutBytes = (size_t)img1->num_colors * sizeof(rgb_t);
        if (memcmp(img1->LUT, img2->LUT, lutBytes) != 0) return 0;
    }

    // Comparar pixels linha por linha com memcmp (muito mais rápido!)
    const size_t rowBytes = (size_t)W * sizeof(uint16);
    for (uint32 v = 0; v < H; v++) {
        if (memcmp(img1->image[v], img2->image[v], rowBytes) != 0) {
            PIXMEM += W;  // Contabilizar acessos
            return 0;
        }
        PIXMEM += W;  // Contabilizar acessos da linha
    }
    return 1;
}





int ImageIsDifferent(const Image img1, const Image img2) {
  assert(img1 != NULL);
  assert(img2 != NULL);

  return !ImageIsEqual(img1, img2);
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)

/// Rotate 90 degrees clockwise (CW).
/// Returns a rotated version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)

/*------------------------------------------------------------------
 * ImageRotate90CW
 *  Cria uma nova imagem rotacionada 90° no sentido horário.
 *  O pixel (v, u) da original vai para (u, height - 1 - v).
 *  Rotação 90° CW sem realocar LUT.
 *-----------------------------------------------------------------*/
Image ImageRotate90CW(const Image img) {
    if (img == NULL) return NULL;

    const uint32 W = img->width, H = img->height;

    Image rotated = ImageCreate(H, W);
    if (rotated == NULL) return NULL;

    // Copiar LUT sem realocar
    rotated->num_colors = img->num_colors;
    for (uint16 i = 0; i < img->num_colors; i++)
        rotated->LUT[i] = img->LUT[i];

    // Rotação (v,u)->(u, H-1-v)
    for (uint32 v = 0; v < H; v++)
        for (uint32 u = 0; u < W; u++)
            rotated->image[u][H - 1 - v] = img->image[v][u];

    return rotated;
}



/// Rotate 180 degrees clockwise (CW).
/// Returns a rotated version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/*------------------------------------------------------------------
 * ImageRotate180CW
 *  Cria uma nova imagem rotacionada 180° no sentido horário.
 *  O pixel (v, u) vai para (height - 1 - v, width - 1 - u).
 *  Rotação 180° CW sem realocar LUT.
 *-----------------------------------------------------------------*/
Image ImageRotate180CW(const Image img) {
    if (img == NULL) return NULL;

    const uint32 W = img->width, H = img->height;

    Image rotated = ImageCreate(W, H);
    if (rotated == NULL) return NULL;

    // Copiar LUT sem realocar
    rotated->num_colors = img->num_colors;
    for (uint16 i = 0; i < img->num_colors; i++)
        rotated->LUT[i] = img->LUT[i];

    // (v,u)->(H-1-v, W-1-u)
    for (uint32 v = 0; v < H; v++)
        for (uint32 u = 0; u < W; u++)
            rotated->image[H - 1 - v][W - 1 - u] = img->image[v][u];

    return rotated;
}



/// Check whether pixel coords (u, v) are inside img.
/// ATTENTION
///   u : column index
///   v : row index
int ImageIsValidPixel(const Image img, int u, int v) {
  return 0 <= u && u < (int)img->width && 0 <= v && v < (int)img->height;
}

/// Region Growing

/// The following three *RegionFilling* functions perform region growing
/// using some variation of the 4-neighbors flood-filling algorithm:
///   Given the coordinates (u, v) of a seed pixel,
///   fill all similarly-colored adjacent pixels with a new color label.
///
/// All of these functions receive the same arguments:
///   img: The image to operate on (and modify).
///   u, v: the coordinates of the seed pixel.
///   label: the new color label (LUT index) to fill the region with.
///
/// And return: the number of labeled pixels.

/// Each function carries out a different version of the algorithm.

/// Region growing using the recursive flood-filling algorithm.
/*------------------------------------------------------------------
 * ImageRegionFillingRecursive
 *  Preenche uma região conexa da imagem (4-vizinhos) com uma nova cor.
 *  Usa o algoritmo recursivo de Flood Fill:
 *   - Parte do pixel (u, v)
 *   - Altera todos os pixels adjacentes (cima, baixo, esquerda, direita)
 *     que tenham a mesma cor inicial ("background") para o novo label.
 *
 *  Parâmetros:
 *    img   → imagem a modificar
 *    u,v   → coordenadas do pixel inicial
 *    label → novo índice de cor (LUT index)
 *
 *  Retorna:
 *    o número total de pixels que foram modificados.
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 * NOTA:
 *  Esta versão recursiva do algoritmo Flood Fill é a implementação
 *  mais direta e intuitiva do processo de preenchimento de regiões.
 *
 *  A função chama-se a si própria (recursão) para explorar os 4
 *  vizinhos de cada pixel (cima, baixo, esquerda e direita) que ainda
 *  pertencem à região com a cor de fundo original.
 *
 *  Apesar da sua simplicidade e clareza conceptual, esta abordagem
 *  tem uma limitação importante: para imagens grandes, a profundidade
 *  de recursão pode ser muito elevada, levando a um possível **stack overflow**.
 *
 *  As versões iterativas (com STACK e QUEUE) foram desenvolvidas para
 *  resolver precisamente essa limitação, simulando o mesmo processo
 *  de propagação mas com controlo manual sobre a pilha ou fila.
 *------------------------------------------------------------------*/

static int floodFillRecursive(Image img, int u, int v, uint16 background, uint16 label);

int ImageRegionFillingRecursive(Image img, int u, int v, uint16 label) {
    // Verificar se o pixel é válido
    if (!ImageIsValidPixel(img, u, v))
        return 0;

    // Guardar a cor original (background)
    uint16 background = img->image[v][u];

    // Se o pixel já tem a nova cor, não há nada a fazer
    if (background == label)
        return 0;

    // Chamar a função recursiva auxiliar
    return floodFillRecursive(img, u, v, background, label);
}

/*------------------------------------------------------------------
// Função auxiliar interna (static) — apenas visível dentro deste ficheiro.
// Implementa a parte recursiva do algoritmo Flood Fill, garantindo
// encapsulamento e evitando conflitos de nomes com outros módulos.
*-----------------------------------------------------------------*/
static int floodFillRecursive(Image img, int u, int v, uint16 background, uint16 label) {
    // Parar se estiver fora da imagem
    if (!ImageIsValidPixel(img, u, v))
        return 0;

    // Parar se o pixel não tiver a cor de fundo (background)
    if (img->image[v][u] != background)
        return 0;

    // Atribuir o novo label ao pixel
    img->image[v][u] = label;

    // Contar este pixel
    int count = 1;

    // Propagar recursivamente para os 4 vizinhos
    count += floodFillRecursive(img, u + 1, v, background, label);  // direita
    count += floodFillRecursive(img, u - 1, v, background, label);  // esquerda
    count += floodFillRecursive(img, u, v + 1, background, label);  // baixo
    count += floodFillRecursive(img, u, v - 1, background, label);  // cima

    return count;
}


/// Region growing using a STACK of pixel coordinates to
/// implement the flood-filling algorithm.
/*------------------------------------------------------------------
 * ImageRegionFillingWithSTACK
 *  Preenche uma região conexa da imagem (4-vizinhos) com uma nova cor,
 *  utilizando uma pilha (STACK) para evitar a recursão.
 *
 *  Implementa o algoritmo iterativo de Flood Fill:
 *   - Começa no pixel (u, v)
 *   - Enquanto houver pixels na pilha, remove o do topo e colore-o
 *   - Adiciona à pilha todos os vizinhos válidos com a cor de fundo
 *
 *  Retorna o número total de pixels preenchidos.
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 * NOTA:
 *  Esta implementação substitui a recursão explícita por uma pilha (STACK)
 *  alocada dinamicamente, evitando o risco de **stack overflow** do sistema.
 *
 *  O TAD Stack (PixelCoordsStack) fornece as operações básicas Push e Pop,
 *  permitindo gerir manualmente os pixels que ainda precisam de ser visitados.
 *
 *  Desta forma, o controlo da recursão é feito de forma iterativa —
 *  cada pixel é processado e os seus vizinhos válidos são empilhados —
 *  obtendo o mesmo resultado lógico da versão recursiva,
 *  mas com melhor controlo da memória e maior robustez para imagens grandes.
 * 
 *  (marcar o pixel logo ao empilhar (marca “visitado”),
 *  em vez de só ao tirar da pilha. Isto evita reempilhar o mesmo pixel várias vezes.)
 *------------------------------------------------------------------*/

/*
 * OTIMIZAÇÃO: Processar vizinhos inline sem array temporário
 * Reduz alocações e melhora localidade de cache
 */
int ImageRegionFillingWithSTACK(Image img, int u, int v, uint16 label) {
    if (!ImageIsValidPixel(img, u, v)) return 0;

    const uint16 background = img->image[v][u];
    if (background == label) return 0;

    // Tamanho inicial adaptativo baseado na imagem
    const uint32 initialSize = (img->width * img->height) / 100;
    Stack* stack = StackCreate(initialSize > 100 ? initialSize : 100);
    if (!stack) return 0;

    int count = 0;
    const int32_t W = (int32_t)img->width;
    const int32_t H = (int32_t)img->height;

    // Marcar e empilhar semente
    img->image[v][u] = label;
    count++;
    StackPush(stack, (PixelCoords){u, v});

    while (!StackIsEmpty(stack)) {
        PixelCoords p = StackPop(stack);
        const int32_t x = p.u, y = p.v;

        // Processar 4 vizinhos inline (evita array temporário)
        // Direita
        if (x + 1 < W && img->image[y][x + 1] == background) {
            img->image[y][x + 1] = label;
            count++;
            StackPush(stack, (PixelCoords){x + 1, y});
        }
        // Esquerda
        if (x > 0 && img->image[y][x - 1] == background) {
            img->image[y][x - 1] = label;
            count++;
            StackPush(stack, (PixelCoords){x - 1, y});
        }
        // Baixo
        if (y + 1 < H && img->image[y + 1][x] == background) {
            img->image[y + 1][x] = label;
            count++;
            StackPush(stack, (PixelCoords){y + 1, x});
        }
        // Cima
        if (y > 0 && img->image[y - 1][x] == background) {
            img->image[y - 1][x] = label;
            count++;
            StackPush(stack, (PixelCoords){y - 1, x});
        }
    }

    StackDestroy(&stack);
    return count;
}


/*------------------------------------------------------------------
 * ImageRegionFillingWithQUEUE
 *  Preenche uma região conexa da imagem (4-vizinhos) com uma nova cor,
 *  utilizando uma fila (QUEUE) para processar os pixels por camadas.
 *
 *  Implementa o algoritmo iterativo de Flood Fill na sua forma BFS:
 *   - Começa no pixel (u, v)
 *   - Enquanto houver pixels na fila, remove o da frente e colore-o
 *   - Adiciona à fila todos os vizinhos válidos com a cor de fundo
 *
 *  Retorna o número total de pixels preenchidos.
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 * NOTA:
 *  Esta versão usa o TAD Queue (PixelCoordsQueue) para gerir os
 *  pixels pendentes de visita de forma **FIFO** (First In, First Out).
 *  Isto garante um preenchimento em largura (BFS), expandindo a região
 *  uniformemente a partir do pixel inicial (u, v).
 *  É uma alternativa mais estável em termos de ordem de propagação,
 *  embora ambas as abordagens (STACK e QUEUE) produzam o mesmo resultado.
 * 
 *  BFS “sem duplicados”.
 *------------------------------------------------------------------*/
int ImageRegionFillingWithQUEUE(Image img, int u, int v, uint16 label) {
    if (!ImageIsValidPixel(img, u, v)) return 0;

    const uint16 background = img->image[v][u];
    if (background == label) return 0;

    // Tamanho inicial adaptativo
    const uint32 initialSize = (img->width * img->height) / 100;
    Queue* queue = QueueCreate(initialSize > 100 ? initialSize : 100);
    if (!queue) return 0;

    int count = 0;
    const int32_t W = (int32_t)img->width;
    const int32_t H = (int32_t)img->height;

    // Marcar e enfileirar semente
    img->image[v][u] = label;
    count++;
    QueueEnqueue(queue, (PixelCoords){u, v});

    while (!QueueIsEmpty(queue)) {
        PixelCoords p = QueueDequeue(queue);
        const int32_t x = p.u, y = p.v;

        // Processar 4 vizinhos inline
        if (x + 1 < W && img->image[y][x + 1] == background) {
            img->image[y][x + 1] = label;
            count++;
            QueueEnqueue(queue, (PixelCoords){x + 1, y});
        }
        if (x > 0 && img->image[y][x - 1] == background) {
            img->image[y][x - 1] = label;
            count++;
            QueueEnqueue(queue, (PixelCoords){x - 1, y});
        }
        if (y + 1 < H && img->image[y + 1][x] == background) {
            img->image[y + 1][x] = label;
            count++;
            QueueEnqueue(queue, (PixelCoords){y + 1, x});
        }
        if (y > 0 && img->image[y - 1][x] == background) {
            img->image[y - 1][x] = label;
            count++;
            QueueEnqueue(queue, (PixelCoords){y - 1, x});
        }
    }

    QueueDestroy(&queue);
    return count;
}




/*------------------------------------------------------------------
 * ImageSegmentation
 *  Percorre toda a imagem e aplica a função de preenchimento (Flood Fill)
 *  a cada nova região de pixels brancos (WHITE) ainda não identificada.
 *
 *  Cada região encontrada recebe um novo label (índice na LUT) e uma
 *  nova cor RGB gerada automaticamente pela função GenerateNextColor().
 *
 *  Parâmetros:
 *    img       → imagem a segmentar (modificada no processo)
 *    fillFunct → ponteiro para uma das funções de Region Filling:
 *                (ImageRegionFillingRecursive, WithSTACK ou WithQUEUE)
 *
 *  Retorna:
 *    o número total de regiões identificadas na imagem.
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 * NOTA:
 *  Esta função implementa o processo de **segmentação de imagem**
 *  baseado em "region growing" — deteta todas as regiões conexas de
 *  pixels brancos e preenche-as com cores distintas.
 *
 *  O uso de um ponteiro para função (fillFunct) torna o algoritmo
 *  **modular e reutilizável**, permitindo testar diferentes abordagens
 *  (recursiva, com pilha ou com fila) sem alterar o código principal.
 *
 *  Cada vez que é descoberta uma nova região branca, é atribuído um
 *  novo label e uma nova cor RGB à LUT, obtida através de
 *  GenerateNextColor(). Assim, cada região fica visualmente distinta.
 *
 *  Em suma, esta função demonstra o uso prático da abstração de funções
 *  como parâmetros e consolida os conceitos de TAD e encapsulamento.
 *------------------------------------------------------------------*/
/*
 * OTIMIZAÇÃO: Cache de valores e evitar chamadas repetidas
 */
int ImageSegmentation(Image img, FillingFunction fillFunct) {
    if (img == NULL || fillFunct == NULL) return 0;

    int regionCount = 0;
    rgb_t currentColor = 0x0000FF;
    uint16 currentLabel = img->num_colors;
    
    const uint32 W = img->width;
    const uint32 H = img->height;
    const uint16 maxColors = FIXED_LUT_SIZE;

    for (uint32 v = 0; v < H; v++) {
        uint16* row = img->image[v];  // Cache do ponteiro da linha
        
        for (uint32 u = 0; u < W; u++) {
            // Usar cache da linha em vez de img->image[v][u]
            if (row[u] == WHITE) {
                // Verificar overflow da LUT
                if (currentLabel >= maxColors) {
                    fprintf(stderr, "Warning: LUT overflow at region %d\n", regionCount);
                    return regionCount;
                }

                // Adicionar cor à LUT
                img->LUT[currentLabel] = currentColor;
                img->num_colors++;

                // Preencher região
                fillFunct(img, u, v, currentLabel);
                regionCount++;

                // Próxima cor e label
                currentColor = GenerateNextColor(currentColor);
                currentLabel++;
            }
        }
    }

    return regionCount;
}
