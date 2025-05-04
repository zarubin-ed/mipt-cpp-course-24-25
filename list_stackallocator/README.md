# StackAllocator & List Library (`list.h`)  

Реализует стековый аллокатор для STL-контейнеров и двусвязный список с поддержкой пользовательского аллокатора.

---

## Содержание
1. [StackStorage](#stackstorage)
2. [StackAllocator](#stackallocator)
3. [List (AllocatorAware)](#list)
4. [Пример использования](#usage)

---

## <a name="stackstorage"></a>1. StackStorage<N>

**Суть:**
Контейнер фиксированного размера N байт, размещённый на стеке. Предоставляет метод выделения подотрезков памяти с учётом выравнивания.

**Шаблон:** `StackStorage<size_t N>`  
**Поля:**
- `alignas(max_align_t) byte bytes_[N]` — внутренний буфер.
- `size_t space_` — оставшийся объём.

**Методы:**
| Метод | Сигнатура | Описание |
|---|---|---|
| `getMemory(bytes, alignment)` | `byte* getMemory(size_t, size_t)` | Выдаёт указатель на `bytes` байт памяти, выровненной по `alignment`, или бросает `bad_alloc`.

---

## <a name="stackallocator"></a>2. StackAllocator<T,N>

**Суть:**
STL-совместимый аллокатор, который выдаёт память из `StackStorage<N>` без единого обращения к heap.

**Шаблон:** `StackAllocator<typename T, size_t N>`  
**Внутренности:**
- `StackStorage<N>* storage_` — указатель на поставщик памяти.

**Обязательные члены Allocator-req:**
| Член | Сигнатура | Описание |
|---|---|---|
| `value_type` | `using value_type = T;` | Тип элемента |
| `allocate(n)` | `T* allocate(size_t)` | Выделение `n * sizeof(T)` байт из `storage_` |
| `deallocate(p,n)` | `void deallocate(T*, size_t) noexcept` | Ничего не делает (память освобождается вместе со StackStorage) |
| `rebind<U>` | `template<typename U> struct rebind { using other = StackAllocator<U,N>; };` | Для адаптации к другому типу |

**Конструкторы:**
- `StackAllocator(StackStorage<N>& storage)` — связывает с поставщиком.
- Шаблон копирования из `StackAllocator<U,N>` — переносит `storage_`.

**Сравнение:**
- `operator==` true, если `storage_` совпадают; `operator!=` по-обратному.

---

## <a name="list"></a>3. List<T,Allocator>

**Суть:**
Двусвязный список, полностью соответствующий AllocatorAwareContainer. Поддерживает передачу любого подходящего аллокатора (включая `StackAllocator`).

**Шаблон:** `List<typename T, typename Allocator = std::allocator<T>>`  

### Внутренние узлы

```cpp
struct BaseNode { BaseNode *prev, *next; /* кольцевая связность */ };
struct Node : BaseNode { T data; };
````

### Типы

| Тип                           | Определение                                                     |
| ----------------------------- | --------------------------------------------------------------- |
| `value_type`                  | `T`                                                             |
| `allocator_type`              | `typename std::allocator_traits<Allocator>::rebind_alloc<Node>` |
| `iterator` / `const_iterator` | двунаправленные, на основе `BaseNode*`                          |

### Конструкторы

| Сигнатура                          | Описание                                             |
| ---------------------------------- | ---------------------------------------------------- |
| `List()`                           | пустой список, без аллокаций                         |
| `List(size_t n, Allocator)`        | `n` default-элементов                                |
| `List(n, const T& val, Allocator)` | `n` копий `val`                                      |
| `List(const List& other)`          | копирующий с `select_on_container_copy_construction` |
| `List(const List&, Allocator)`     | копия с указанным аллокатором                        |

### Деструктор

* `~List()` — удаляет все узлы, возвращает память аллокатору

### Методы

| Метод                                | Сигнатура              | Описание                   | Сложность |
| ------------------------------------ | ---------------------- | -------------------------- | --------- |
| `size()`                             | `size_t size() const`  | Текущее число элементов    | O(1)      |
| `empty()`                            | `bool empty() const`   | Пуст ли список             | O(1)      |
| `get_allocator()`                    | `allocator_type`       | Текущий аллокатор          | O(1)      |
| `begin(), end()`                     | `iterator`             | Пределы последовательности | O(1)      |
| `cbegin(), cend(), rbegin(), rend()` | аналоги const/reverse  | O(1)                       |           |
| `push_back(val)`, `push_front(val)`  | вставка в конец/начало | O(1)                       |           |
| `pop_back()`, `pop_front()`          | удаление               | O(1)                       |           |
| `insert(pos, val)`                   | вставка перед `pos`    | O(1) (алокация)            |           |
| `erase(pos)`                         | удаление `pos`         | O(1) (деалокация)          |           |

### Исключительная безопасность

* Все операции либо полностью выполняются, либо при исключении возвращают список в исходное состояние.

---

## <a name="usage"></a>4. Пример использования

```cpp
#include "list.h"
#include <list>

int main() {
    // 1) Стековый аллокатор + std::list
    StackStorage<100000> storage;
    StackAllocator<int,100000> alloc(storage);
    std::list<int, decltype(alloc)> stdl(alloc);
    for(int i=0;i<1000;++i) stdl.push_back(i);

    // 2) Собственный List с тем же аллокатором
    List<int, decltype(alloc)> myList(alloc);
    for(int i=0;i<1000;++i) myList.push_back(i);
}
```

