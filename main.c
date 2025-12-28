#include "field.h"
#include <locale.h>
#include <stdio.h>
#include <time.h>

/* Максимальное число попыток найти решение.*/
#define MAX_ATTEMPTS 1000

/* ===================================================================
   Основной цикл программы и пользовательский интерфейс
   =================================================================== */

int main(void) {
    setlocale(LC_ALL, "Rus");
    /* инициализация RNG */
    srand((unsigned)time(NULL));

    printf("Здравствуйте! Это генератор поля Сапёр (Mines generator).\n");

    for (;;) { /* внешний бесконечный цикл: после сохранения или отказа можно начать заново */
        int rows = 8, cols = 8, perc = 15;

        /* ---------- Ввод размеров поля ---------- */
        printf("\nВведите размеры поля (строки столбцы), например: 8 8\n");
        if (scanf("%d %d", &rows, &cols) != 2) {
            printf("Ввод некорректен. Установлено 8x8.\n");
            rows = 8; cols = 8;
        }
        while (getchar() != '\n'); // очистка остатка ввода

        /* ---------- Ввод вероятности мин ---------- */
        printf("Введите вероятность заполнения минами (0..100), например: 15\n");
        if (scanf("%d", &perc) != 1 || perc < 0 || perc > 100) {
            printf("Ввод некорректен. Установлено 15%%.\n");
            perc = 15;
        }
        while (getchar() != '\n');

        /* Создаём поле нужного размера */
        Field* field = field_create(rows, cols);
        if (!field) { printf("Ошибка выделения памяти.\n"); return 1; }

    /* ---- Попытки сгенерировать решаемую конфигурацию поля ----
        Выполняется до MAX_ATTEMPTS попыток. Каждая попытка:
            - очищает текущее состояние поля,
            - заново случайно размещает мины,
            - пересчитывает числовые счётчики,
            - проверяет решаемость детерминистическим солвером.
        Память под структуру Field выделяется один раз;
        изменяется только содержимое поля.
        Если за MAX_ATTEMPTS попыток решаемая конфигурация не найдена,
        считается, что с данными параметрами (rows, cols, perc)
        быстро получить решаемое поле не удалось.
        В этом случае пользователю предлагается выбор дальнейших действий.
    */
    generate_again:
        {
            bool solvable = false;
            int start_r = -1, start_c = -1;


            for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
                generate_by_probability(field, perc);
                if (check_solvability(field, &start_r, &start_c)) {
                    solvable = true; /* нашли подходящее поле */
                    break;
                }
            }

            /* Показываем информацию о сгенерированном поле */
            printf("\nСгенерировано поле %dx%d, вероятность %d%%, мин = %d\n",
                field->rows, field->cols, perc, field->mines);

            /* Если не нашли решаемое поле */
            if (!solvable) {
                printf("Поле НЕ решаемо детерминистическим солвером.\n");

                printf("Показать текущее поле для анализа? (Y/N): ");
                int ch = getchar();
                while (getchar() != '\n');

                if (ch == 'Y' || ch == 'y') print_field_ascii(field, true);

                printf("Поле не решаемо. Выберите: (R) сгенерировать заново, (P) ввести новые параметры, (E) выйти\n");
                int opt = getchar();
                while (getchar() != '\n');

                if (opt == 'R' || opt == 'r') {
                    /* перегенерировать с теми же параметрами */
                    field_clear(field);
                    goto generate_again;
                }
                if (opt == 'P' || opt == 'p') {
                    field_free(field);
                    continue;  /* вернуться к вводу размеров и вероятности */
                }
                /* E или любое другое — выход */
                field_free(field);
                printf("Выход.\n");
                return 0;
            }
            else {
                printf("Поле решаемо детерминистическим солвером.\n");
                print_field_ascii(field, true);

                printf("\n(Y) выполнить автоматическую проверку счётчиков и сохранить поле, (R) перегенерировать, (P) новые параметры, (E) выйти\n");
                int choice = getchar();
                while (getchar() != '\n');

                if (choice == 'R' || choice == 'r') {
                    field_clear(field);
                    goto generate_again;
                }
                if (choice == 'P' || choice == 'p') {
                    field_free(field);
                    continue;
                }
                if (choice == 'E' || choice == 'e') {
                    field_free(field);
                    printf("Выход.\n");
                    return 0;
                }

                if (choice == 'Y' || choice == 'y') {
                    printf("Выполняю автоматическую проверку счётчиков...\n");
                    bool ok = validate_field(field);
                    if (!ok) {
                        /* Если автоматическая проверка не пройдена — предлагаем варианты */
                        printf("Автоматическая проверка нашла несоответствия. Поле отброшено.\n");
                        printf("Выберите: (R) перегенерировать, (P) новые параметры, (E) выйти\n");
                        int o2 = getchar();
                        while (getchar() != '\n');
                        if (o2 == 'R' || o2 == 'r') { field_clear(field); goto generate_again; }
                        if (o2 == 'P' || o2 == 'p') { field_free(field); continue; }
                        field_free(field); printf("Выход.\n"); return 0;
                    }
                    else {
                        /* Сохранение поля */
                        char fname[260];
                        printf("Введите имя файла для сохранения (например field.txt): ");
                        if (scanf("%259s", fname) == 1) {
                            if (save_field_to_file(field, fname)) {
                                printf("Поле успешно сохранено в %s\n", fname);
                            }
                            else {
                                printf("Ошибка при сохранении в %s\n", fname);
                            }
                        }
                        while (getchar() != '\n'); // очистка ввода
                        print_field_ascii(field, true);

                        for (;;) {
                            printf("\nМеню после сохранения:  1) Сгенерировать новое поле  2) Выйти\nВыберите: ");
                            int cmd;
                            if (scanf("%d", &cmd) != 1) {
                                int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
                                printf("Неверный ввод.\n");
                                continue;
                            }
                            if (cmd == 1) { field_free(field); break; } /* начать новый цикл */
                            else if (cmd == 2) { field_free(field); printf("Выход.\n"); return 0; }
                            else printf("Неизвестная команда.\n");
                        }
                        continue; /* вернуться к внешнему циклу, предложить новые параметры */
                    }
                }
            }
        }

        field_free(field);
    }

    return 0;
}
