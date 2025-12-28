#ifndef MINES_FIELD_H
#define MINES_FIELD_H

#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <stdio.h> // работа с памятью, генерация случайных чисел, exit, malloc, free
#include <string.h> // работа со строками и памятью (memset, memcpy, strlen и т.д.)
#include <stdbool.h>

/* -------------------------------------------------------------------
   Макрос IDX(INDEX): переводит координаты (r,c) в индекс линейного массива.
   ------------------------------------------------------------------- */
#define IDX(f, r, c) ( (int)((r) * (f)->cols + (c)) )

/* Field хранит:
   - rows, cols : размеры поля
   - mines      : текущее число расставленных мин
   - is_mine    : линейный массив длины rows*cols; 1 = мина, 0 = нет
   - count      : линейный массив длины rows*cols; count[i] = число мин вокруг клетки i
*/
typedef struct {
    int rows;
    int cols;
    int mines;
    unsigned char* is_mine;
    unsigned char* count;
} Field;

/* field_create
    - Выделяет память и инициализирует структуру поля.
    - Возвращает NULL при ошибке.
*/
Field* field_create(int rows, int cols);

/* field_free
   - Освобождает память структуры и её массивов.
   - Безопасно вызывать с NULL.
*/
void field_free(Field* f);

/* field_clear
   - Очищает поле: сбрасывает флаги мин, обнуляет счётчики и счётчик мин.
   - Используется перед новой генерацией.
*/
void field_clear(Field* f);

/* compute_counts
   - Для каждой клетки, если она не мина, вычисляет количество мин среди 8 соседей.
   - Результат записывается в f->count.
*/
void compute_counts(Field* f);

/* generate_by_probability
   - Заполняет поле минами с заданной вероятностью (в процентах).
   - После расстановки мин вызывает compute_counts для обновления счётчиков.
*/
void generate_by_probability(Field* f, int percent);

/* print_field_ascii
   - Печатает поле в виде таблицы.
   - show_mines=true: мины отображаются '*'.
   - Значение 0 отображается как '.', 1..8 — цифры.
*/
void print_field_ascii(const Field* f, bool show_mines);

/* save_field_to_file
   - Сохраняет поле в текстовом формате:
     первая строка: rows cols mines
     затем rows строк, по cols символов: 'M' или '0'..'8'
*/
bool save_field_to_file(const Field* f, const char* fname);

/* validate_field
   - Для каждой неминной клетки пересчитывает число соседних мин и сравнивает
     с f->count[i]. Печатает ошибку для несоответствий и возвращает false,
     если хоть одно несоответствие найдено.
*/
bool validate_field(const Field* f);

/* simulate_solver_from
      - Пытаемся логически раскрыть всё поле, начиная со start_r,start_c.
      - Возвращает true, если все безопасные клетки можно открыть, применяя только локальную логику.
      - Вспомогательные массивы:
          state[i]        : -1 = закрыта, >=0 = открыта и содержит число (f->count[i])
          inferred_mine[i]: 0/1 — внутреннее помечение, что клетка точно мина (не игровой флаг)
      - Использует очередь (BFS - (англ. breadth-first search, «поиск в ширину») —
      алгоритм обхода графа в ширину.) для раскрытия нулевых областей.
*/
bool simulate_solver_from(const Field* f, int start_r, int start_c);

/*
  check_solvability
  - Перебирает все неминные клетки и вызывает simulate_solver_from для каждой.
  - При первом успехе возвращает true.
  - Если ни одна стартовая клетка не дала полного решения, возвращает false..
*/
bool check_solvability(const Field* f, int* out_r, int* out_c);

#endif /* MINES_FIELD_H */
