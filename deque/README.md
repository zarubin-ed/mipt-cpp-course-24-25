# Deque<T> Library (`deque.h`)

Упрощённый аналог `std::deque<T>` с поддержкой случайного доступа, двунаправленных итераторов и амортизированного O(1) добавления/удаления по концам.

---

## Содержание
1. [Обзор](#overview)
2. [Конструкторы и присваивание](#constructors)
3. [Размер и доступ](#access)
4. [Итераторы](#iterators)
5. [Модификаторы](#modifiers)
6. [Исключительная безопасность](#exceptions)
7. [Пример использования](#example)

---

## <a name="overview"></a>1. Обзор

`Deque<T>` stores elements in a static or dynamically resized array of fixed-size blocks (buckets) фиксированного размера. Поддерживает:

- **O(1)** доступ по индексу (`operator[]`, `at`).
- **Амортизированное O(1)** добавление/удаление в начало и конец (`push_back`, `pop_back`, `push_front`, `pop_front`).
- **O(n)** вставка/удаление в середину (`insert`, `erase`).
- **Двунаправленные и обратные итераторы** с поддержкой всех операций RandomAccess.

---

## <a name="constructors"></a>2. Конструкторы и присваивание

| Сигнатура | Описание | Безопасность |
|---|---|---|
| `Deque()` | пустой deque | — |
| `Deque(const Deque& other)` | копирующий конструктор | строгая гарантия |
| `Deque& operator=(const Deque& other)` | копирующее присваивание | строгая гарантия |
| `Deque(int n)` | `n` default-элементов `T()` | строгая гарантия |
| `Deque(int n, const T& val)` | `n` копий `val` | строгая гарантия |

Файл: `deque.h` (без `main`). Компиляция: C++23.

---

## <a name="access"></a>3. Размер и доступ

| Метод / оператор | Сигнатура | Описание | Сложность |
|---|---|---|---|
| `size()` | `size_t size() const` | Число элементов | O(1) |
| `operator[](i)` | `T& operator[](size_t)` | Доступ без проверки | O(1) |
| `operator[](i) const` | `const T& operator[](size_t) const` |  | O(1) |
| `at(i)` | `T& at(size_t)` / `const T& at(size_t) const` | С проверкой, `throw out_of_range` | O(1) |
| `empty()` | `bool empty() const` | Пусто? | O(1) |

---

## <a name="iterators"></a>4. Итераторы

**Внутренний тип** `iterator` и `const_iterator`:
- Поддерживают `RandomAccessIterator`:
  - `++, --`, `+ n, - n`, `it1 - it2`.
  - Сравнение `<, >, <=, >=, ==, !=`.
  - Разыменование `*it` → `T&`, `it->` → `T*`.
  - Конверсия `iterator` → `const_iterator`.
- **reverse_iterator**, методы `rbegin()`, `rend()`, `crbegin()`, `crend()`.
- **Гарантия**: операции доступа не инвалидируют другие итераторы (кроме `insert`/`erase`).

---

## <a name="modifiers"></a>5. Модификаторы

| Метод | Сигнатура | Описание | Сложность | Искл. безопасность |
|---|---|---|---|---|
| `push_back(val)` | `void push_back(const T&)` | Добавить в конец | амортиз. O(1) | строгая |
| `pop_back()` | `void pop_back()` | Удалить с конца | O(1) | н/д |
| `push_front(val)` | `void push_front(const T&)` | Добавить в начало | амортиз. O(1) | строгая |
| `pop_front()` | `void pop_front()` | Удалить с начала | O(1) | н/д |
| `insert(it, val)` | `iterator insert(iterator, const T&)` | Вставить перед `it` | O(n) | строгая |
| `erase(it)` | `iterator erase(iterator)` | Удалить `it` | O(n) | строгая |
| `clear()` | `void clear()` | Удалить все | O(n) | н/д |
| `swap(other)` | `void swap(Deque&)` | Обмен | O(1) | н/д |

---

## <a name="exceptions"></a>6. Исключительная безопасность

Все методы (кроме `insert`/`erase`) возвращают дека в исходное состояние при выбросе исключения из конструктора или оператора присваивания `T`. Обеспечивается копированием прежде чем изменять.

---

## <a name="example"></a>7. Пример использования

```cpp
#include "deque.h"
#include <iostream>

int main() {
    Deque<std::string> dq(2, "hi");
    dq.push_back("world");      // ["hi","hi","world"]
    dq.push_front("hello");    // ["hello","hi","hi","world"]

    for (auto it = dq.begin(); it != dq.end(); ++it)
        std::cout << *it << ' ';
    // hello hi hi world

    auto mid = dq.begin() + 2;
    dq.insert(mid, "dear");     // insert in middle
    dq.erase(mid);               // remove

    std::cout << "\nSize: " << dq.size();
}
````

