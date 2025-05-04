# CowString Library (`string.h`)

Упрощённый аналог `std::string` с оптимизацией Copy-On-Write (COW). Все операции над строкой выполняются вручную без стандартных контейнеров.

---

## Содержание

1. [CowString](#cowstring)
2. [Конструкторы](#constructors)
3. [Основные методы и свойства](#methods)
4. [Операторы](#operators)
5. [Copy-On-Write оптимизация](#cow)
6. [Использование](#usage)

---

## CowString {#cowstring}

**Суть:**
Строка символов с автоматическим разделением общего буфера (COW). Хранит в одном аллокации счётчик ссылок и данные.

**Файл:** `string.h`  
**Компиляция:** Make, C++17. Подключается через `#include "string.h"`.

Структура памяти:
```

\[ ref\_count (size\_t) | characters... | '\0' ]

````

---

## Конструкторы {#constructors}

| Сигнатура | Описание |
|---|---|
| `CowString()` | Пустая строка
| `CowString(const char* s)` | Из C‑строки
| `CowString(size_t n, char c)` | Из n символов c
| `CowString(char c)` | Из одного символа
| `CowString(const CowString& other)` | Копирующий (инкремент ref_count)

---

## Основные методы и свойства {#methods}

| Метод | Сигнатура | Описание | Сложность |
|---|---|---|---|
| `size()` / `length()` | `size_t` | Длина строки | O(1) |
| `capacity()` | `size_t` | Ёмкость (без \0) | O(1) |
| `empty()` | `bool` | Пустая ли строка | O(1) |
| `clear()` | `void` | Обнулить длину (не free) | O(1) |
| `shrink_to_fit()` | `void` | Освобождение неиспользуемой памяти | O(n) |
| `data()` | `char*` / `const char*` | Указатель на C‑строку | O(1) |
| `front()`, `back()` | `char&` / `const char&` | Первый/последний символ | O(1) |
| `push_back(char)` | `void` | Добавить символ (амортиз. O(1)) | amortized O(1) |
| `pop_back()` | `void` | Удалить последний символ | O(1) |
| `substr(pos, count)` | `CowString` | Подстрока | O(count) alloc+copy |
| `find(sub)` / `rfind(sub)` | `size_t` | Поиск подстроки (лево/право) | O(n·m) |

---

## Операторы {#operators}

| Оператор | Сигнатура | Описание |
|---|---|---|
| `operator=` | присваивание (COW-aware) | O(1) |
| `operator==, !=` | сравнение на равенство | O(n) |
| `<, >, <=, >=` | лексикографич. сравнение | O(n) |
| `operator+=` | добавить символ или строку | амортиз. O(n + m) |
| `operator+` | конкатенация (строка+строка/символ) | O(n + m) |
| `operator[]` | доступ по индексу | O(1) (COW при записи) |
| `operator<<`, `operator>>` | ввод/вывод в поток | O(n) |

---

## Copy-On-Write оптимизация {#cow}

- Буфер содержит счётчик ссылок (`size_t counter`) в первых байтах.
- Копирующий конструктор и присваивание не дублируют данные, а увеличивают `counter`.
- Модифицирующие методы (`operator[]`, `push_back`, `clear`, `data()` неконстантный) вызывают `make_copy()`, который при `counter > 1` выделяет новый буфер и уменьшает старый счётчик.
- Благодаря COW минимизируется число `new`/`delete` при множественных копиях.

---

## Использование {#usage}

```cpp
#include "string.h"
#include <iostream>

int main() {
  CowString s1 = "Hello";
  CowString s2 = s1;            // тот же буфер, ref_count=2
  s2.push_back('!');            // сopy-on-write, s2 получает свой буфер

  std::cout << s1 << '\n';     // Hello
  std::cout << s2 << '\n';     // Hello!

  CowString sub = s2.substr(0, 5);
  std::cout << sub << '\n';    // Hello
}
````
