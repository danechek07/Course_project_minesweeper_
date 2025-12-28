#include "field.h"
#include <time.h>

   /* Создаёт поле, выделяет память */
Field* field_create(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;
    Field* f = (Field*)malloc(sizeof(Field));
    if (!f) return NULL;

    f->rows = rows;
    f->cols = cols;
    f->mines = 0;
    int n = rows * cols;
    f->is_mine = (unsigned char*)calloc(n, sizeof(unsigned char));
    f->count = (unsigned char*)calloc(n, sizeof(unsigned char));
    if (!f->is_mine || !f->count) {
        free(f->is_mine);
        free(f->count);
        free(f);
        return NULL;
    }
    return f;
}

/* Освобождение памяти */
void field_free(Field* f) {
    if (!f) return;
    free(f->is_mine);
    free(f->count);
    free(f);
}

/* Очистка поля  */
void field_clear(Field* f) {
    if (!f) return;
    int n = f->rows * f->cols;
    memset(f->is_mine, 0, n * sizeof(unsigned char));
    memset(f->count, 0, n * sizeof(unsigned char));
    f->mines = 0;
}

/* Подсчёт счётчиков вокруг клетки */
void compute_counts(Field* f) {
    if (!f) return;
    int R = f->rows, C = f->cols;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if (f->is_mine[i]) { f->count[i] = 0; continue; }

            int cnt = 0;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int rr = r + dr, cc = c + dc;
                    if (rr >= 0 && rr < R && cc >= 0 && cc < C)
                        if (f->is_mine[IDX(f, rr, cc)]) ++cnt;
                }
            f->count[i] = (unsigned char)cnt;
        }
    }
}

/* Генерация по вероятности */
void generate_by_probability(Field* f, int percent) {
    if (!f) return;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    field_clear(f);
    int R = f->rows, C = f->cols;
    int placed = 0;

    for (int r = 0; r < R; ++r)
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if ((rand() % 100) < percent) {
                f->is_mine[i] = 1;
                ++placed;
            }
        }
    f->mines = placed;
    compute_counts(f);
}

/* Печать поля в ASCII */
void print_field_ascii(const Field* f, bool show_mines) {
    if (!f) return;
    int R = f->rows, C = f->cols;

    printf("+");
    for (int c = 0; c < C; ++c) printf("---+");
    printf("\n");

    for (int r = 0; r < R; ++r) {
        printf("|");
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            char ch;
            if (show_mines && f->is_mine[i]) ch = '*';
            else if (f->is_mine[i]) ch = '*';
            else ch = (f->count[i] == 0) ? '.' : (char)('0' + f->count[i]);
            printf(" %c |", ch);
        }
        printf("\n+");
        for (int c = 0; c < C; ++c) printf("---+");
        printf("\n");
    }
}

/* Сохранение в файл */
bool save_field_to_file(const Field* f, const char* fname) {
    if (!f || !fname) return false;
    FILE* out = fopen(fname, "w");
    if (!out) return false;

    fprintf(out, "%d %d %d\n", f->rows, f->cols, f->mines);
    for (int r = 0; r < f->rows; ++r) {
        for (int c = 0; c < f->cols; ++c) {
            int i = IDX(f, r, c);
            fputc(f->is_mine[i] ? 'M' : '0' + f->count[i], out);
        }
        fputc('\n', out);
    }
    fclose(out);
    return true;
}

/* Валидация счётчиков */
bool validate_field(const Field* f) {
    if (!f) return false;
    int R = f->rows, C = f->cols;
    bool ok = true;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int i = IDX(f, r, c);
            if (f->is_mine[i]) continue;

            int cnt = 0;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int rr = r + dr, cc = c + dc;
                    if (rr >= 0 && rr < R && cc >= 0 && cc < C)
                        if (f->is_mine[IDX(f, rr, cc)]) ++cnt;
                }

            if (cnt != f->count[i]) {
                printf("Ошибка: клетка (%d,%d) имеет count=%d, а должно быть %d\n",
                    r, c, f->count[i], cnt);
                ok = false;
            }
        }
    }

    if (ok) printf("Валидация пройдена: все счетчики корректны.\n");
    return ok;
}

/* ============ Детерминистический солвер (локальные правила) ============ */
   /*
     Идея (простыми словами):
       - Мы моделируем, как играет человек, применяя только очевидные локальные правила,
         без угадывания и сложных дедукций.
       - Правила те же, что в реальной игре:
           * Если число в клетке n == (количество помеченных мин) + (количество закрытых соседних клеток),
             то все эти закрытые клетки — мины.
           * Если число в клетке n == (количество помеченных мин), то все остальные закрытые
             соседние клетки безопасны и их можно открыть.
       - Алгоритм стартует с одной стартовой клетки (как будто игрок кликнул на неё).
         Мы будем пробовать разные стартовые клетки (в check_solvability).
   */
bool simulate_solver_from(const Field* f, int start_r, int start_c) {
    if (!f) return false;
    int R = f->rows, C = f->cols, N = R * C;

    /* state: -1 закрыта, 0..8 открыта */
    int* state = (int*)malloc(N * sizeof(int));
    if (!state) return false;
    for (int i = 0; i < N; ++i) state[i] = -1;

    /* inferred_mine: внутренние пометки о том, что клетка наверняка мина */
    unsigned char* inferred_mine = (unsigned char*)calloc(N, sizeof(unsigned char));
    if (!inferred_mine) { free(state); return false; }

    /* сколько безопасных клеток должно быть открыто в конце */
    int safe_total = 0;
    for (int i = 0; i < N; ++i) if (!f->is_mine[i]) ++safe_total;

    int start_idx = IDX(f, start_r, start_c);
    if (f->is_mine[start_idx]) { free(state); free(inferred_mine); return false; }

    /* очередь для BFS раскрытия нулей */
    int* queue = (int*)malloc(N * sizeof(int));
    if (!queue) { free(state); free(inferred_mine); return false; }
    int qh = 0, qt = 0;

    /* открываем стартовую клетку */
    state[start_idx] = f->count[start_idx];
    if (f->count[start_idx] == 0) {
        /* если ноль — расширяем нулевую область */
        queue[qt++] = start_idx;
        while (qh < qt) {
            int cur = queue[qh++];
            int rr = cur / C, cc = cur % C;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int r2 = rr + dr, c2 = cc + dc;
                    if (r2 >= 0 && r2 < R && c2 >= 0 && c2 < C) {
                        int p2 = IDX(f, r2, c2);
                        if (f->is_mine[p2]) continue;
                        if (state[p2] == -1) {
                            state[p2] = f->count[p2];
                            if (f->count[p2] == 0) queue[qt++] = p2;
                        }
                    }
                }
        }
    }

    /* основной итеративный цикл применения локальных правил */
    bool changed = true;
    while (changed) {
        changed = false;

        /* для каждой открытой клетки считаем соседей и пробуем применить правила */
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < C; ++c) {
                int p = IDX(f, r, c);
                if (state[p] < 0) continue; /* если закрыта — пропускаем */

                int n = state[p]; /* число на этой клетке */
				int inferred_neighbors = 0; /* сколько соседей помечено как мины */
				int unknown_neighbors = 0; /* сколько соседей закрыто и не помечено */
                int unknown_list[8];
                int unknown_k = 0;

                for (int dr = -1; dr <= 1; ++dr)
                    for (int dc = -1; dc <= 1; ++dc) {
                        if (dr == 0 && dc == 0) continue;
                        int rr = r + dr, cc = c + dc;
                        if (rr >= 0 && rr < R && cc >= 0 && cc < C) {
                            int p2 = IDX(f, rr, cc);
                            if (inferred_mine[p2]) inferred_neighbors++;
                            else if (state[p2] == -1) {
                                unknown_list[unknown_k++] = p2;
                                unknown_neighbors++;
                            }
                        }
                    }

                /* Правило A: если число == known + unknown -> все unknown — мины */
                if (unknown_neighbors > 0 && n == inferred_neighbors + unknown_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (!inferred_mine[p2]) {
                            inferred_mine[p2] = 1;
                            changed = true;
                        }
                    }
                }
                
                /* Правило B: если число == known -> все unknown безопасны -> открываем их */
                if (unknown_neighbors > 0 && n == inferred_neighbors) {
                    for (int k = 0; k < unknown_k; ++k) {
                        int p2 = unknown_list[k];
                        if (state[p2] == -1 && !inferred_mine[p2]) {
                            state[p2] = f->count[p2];
                            changed = true;
                            /* если новый открытый — ноль, добавляем в очередь для flood-fill */
                            if (f->count[p2] == 0) queue[qt++] = p2;
                        }
                    }
                }
            }
        }

        /* если очередь пополнилась нулями — расширяем их (BFS) */
        while (qh < qt) {
            int cur = queue[qh++];
            int rr = cur / C, cc = cur % C;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int r2 = rr + dr, c2 = cc + dc;
                    if (r2 >= 0 && r2 < R && c2 >= 0 && c2 < C) {
                        int p2 = IDX(f, r2, c2);
                        if (f->is_mine[p2]) continue;
                        if (state[p2] == -1 && !inferred_mine[p2]) {
                            state[p2] = f->count[p2];
                            if (f->count[p2] == 0) queue[qt++] = p2;
                            changed = true;  /* открытие изменило состояние */
                        }
                    }
                }
        }
        /* после обработки очереди снова пойдёт полный проход — цикл завершится, когда
   ни одно правило не сможет ничего нового пометить/открыть */
    }

    /* Считаем открытые безопасные клетки */
    int opened = 0;
    for (int i = 0; i < N; ++i) if (!f->is_mine[i] && state[i] >= 0) ++opened;

    free(queue);
    free(inferred_mine);
    free(state);

    /* Возвращаем true только если открыты все безопасные клетки */
    return opened == safe_total;
}

/* Перебираем все возможные стартовые клетки */
bool check_solvability(const Field* f, int* out_r, int* out_c) {
    if (!f) return false;
    int R = f->rows, C = f->cols;
    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            int idx = IDX(f, r, c);
            if (f->is_mine[idx]) continue;
            if (simulate_solver_from(f, r, c)) {
                if (out_r) *out_r = r;
                if (out_c) *out_c = c;
                return true;
            }
        }
    }
    return false;
}
