# Smart Pointers Library (`smart_pointers.h`)

Реализация разделяемого (`SharedPtr`) и слабого (`WeakPtr`) умных указателей, а также функций `makeShared`, `allocateShared` и поддержки `EnableSharedFromThis`.

---

## Содержание
1. [SharedPtr<T>](#sharedptr)
2. [WeakPtr<T>](#weakptr)
3. [makeShared и allocateShared](#makers)
4. [EnableSharedFromThis<T>](#enable)

---

## <a name="sharedptr"></a>1. SharedPtr<T>

**Concept:**
Умный указатель с разделяемым владением. Несёт ответственность за удаление объекта, когда последний владелец уничтожается или сбрасывает указатель.

**Объявление:**
```cpp
template<typename T>
class SharedPtr;
````

**Основные возможности:**

* **Разделяемое владение**: несколько `SharedPtr<T>` могут указывать на один объект.
* **Пользовательский удалитель (Deleter)**: можно задать, как уничтожать объект.
* **Пользовательский аллокатор**: можно задать, как выделять/освобождать память для управляющего блока.
* **Aliasing-конструктор**: разделяет владение, но хранит произвольный указатель.
* **Поддержка полиморфизма**: `SharedPtr<Base>` из `SharedPtr<Derived>`.

**Публичный интерфейс:**

| Член                                              | Сигнатура                                                                                                                                                                                                                                                               | Описание                                                   |
| ------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------- |
| **Конструкторы**                                  | `SharedPtr() noexcept`<br>`SharedPtr(U* p, Deleter d, Alloc a)`<br>`SharedPtr(const SharedPtr& )`<br>`SharedPtr(SharedPtr&& )`<br>`template<U> SharedPtr(const SharedPtr<U>&)`<br>`template<U> SharedPtr(SharedPtr<U>&&)`<br>`template<U> SharedPtr(const WeakPtr<U>&)` | создание, копирование, перемещение, конверсия, алиасинг    |
| **Деструктор**                                    | `~SharedPtr()`                                                                                                                                                                                                                                                          | уменьшает счётчик; при 0 удаляет объект и управляющий блок |
| `operator=`                                       | копирующее и перемещающее присваивание                                                                                                                                                                                                                                  | сильная гарантия через copy-and-swap                       |
| `reset()` / `reset(U* p, D, A)`                   | сброс к null или на новый указатель с удалителем/аллокатором                                                                                                                                                                                                            |                                                            |
| `swap(SharedPtr&) noexcept`                       | обмен содержимым                                                                                                                                                                                                                                                        |                                                            |
| `T* get() const noexcept`                         | получить сырой указатель                                                                                                                                                                                                                                                |                                                            |
| `T& operator*() const`<br>`T* operator->() const` | разыменование                                                                                                                                                                                                                                                           |                                                            |
| `size_t use_count() const noexcept`               | число владельцев                                                                                                                                                                                                                                                        |                                                            |
| `bool unique() const noexcept`                    | единственный владелец?                                                                                                                                                                                                                                                  |                                                            |
| `explicit operator bool() const noexcept`         | проверка на непустой указатель                                                                                                                                                                                                                                          |                                                            |

---

## <a name="weakptr"></a>2. WeakPtr<T>

**Концепция:**
Слабый указатель, не влияющий на время жизни объекта, но позволяющий проверить, жив ли объект, и получить `SharedPtr` при необходимости.

**Объявление:**

```cpp
template<typename T>
class WeakPtr;
```

**Публичный интерфейс:**

| Член                                 | Сигнатура                                                                                                               | Описание                                                       |
| ------------------------------------ | ----------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- |
| **Конструкторы**                     | `WeakPtr() noexcept`<br>`template<U> WeakPtr(const SharedPtr<U>&)`<br>`WeakPtr(const WeakPtr&)`<br>`WeakPtr(WeakPtr&&)` | создание из `SharedPtr` или другого `WeakPtr`                  |
| **Деструктор**                       | `~WeakPtr()`                                                                                                            | уменьшает weak-счётчик; при обоих счётчиках=0 освобождает блок |
| `operator=`                          | копирующее/перемещающее присваивание                                                                                    |                                                                |
| `size_t use_count() const noexcept`  | число `SharedPtr` владельцев                                                                                            |                                                                |
| `bool expired() const noexcept`      | объект уже удалён?                                                                                                      |                                                                |
| `SharedPtr<T> lock() const noexcept` | получить `SharedPtr`, или null если истёк                                                                               |                                                                |
| `void reset() noexcept`              | сброс                                                                                                                   |                                                                |
| `void swap(WeakPtr&) noexcept`       | обмен                                                                                                                   |                                                                |

---

## <a name="makers"></a>3. makeShared и allocateShared

### makeShared<T>(Args&&...)

* Выделяет одну область памяти для управления и объекта.
* Конструирует `T` напрямую в контрол-блоке.
* Возвращает `SharedPtr<T>`.

```cpp
auto p = makeShared<MyClass>(arg1, arg2);
```

### allocateShared<T>(Alloc const&, Args&&...)

* То же, но использует пользовательский аллокатор `Alloc` для всего.

```cpp
auto p = allocateShared<MyClass>(myAlloc, arg1, arg2);
```

---

## <a name="enable"></a>4. EnableSharedFromThis<T>

**Концепция:**
Позволяет объекту, управляемому `SharedPtr`, получить `SharedPtr` на себя.

**Использование:**

```cpp
struct Widget : EnableSharedFromThis<Widget> { ... };
auto p = makeShared<Widget>();
auto self = p->shared_from_this();
```

**Метод:**

| Член                              | Сигнатура                                                                            | Описание |
| --------------------------------- | ------------------------------------------------------------------------------------ | -------- |
| `SharedPtr<T> shared_from_this()` | возвращает `SharedPtr<T>` на `*this`, или бросает `bad_weak_ptr` если нет владельцев |          |

---

*Файл:* `smart_pointers.h`
*Компиляция:* C++17 или новее.
Подключение: `#include "shared_ptr.h"`.