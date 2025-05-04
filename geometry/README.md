# Geometry Library (`geometry.h`)

Набор классов для работы с геометрическими фигурами на плоскости: точки, прямые, полигоны, эллипсы, круги, прямоугольники, квадраты, треугольники и общие операции над ними.

---

## Содержание

1. [Point](#point)
2. [Line](#line)
3. [Shape (абстрактный)](#shape)
4. [Polygon](#polygon)
5. [Ellipse](#ellipse)
6. [Circle](#circle)
7. [Rectangle](#rectangle)
8. [Square](#square)
9. [Triangle](#triangle)
10. [Общие операции над Shape](#common-operations)

---

## Point

**Суть:** представляет точку на плоскости с координатами `(x, y)`.

**Поля:**
- `fType x, y` — координаты точки.

**Конструкторы:**
- `Point(fType x_, fType y_)`
- `Point()` — по умолчанию `(0,0)`.
- `Point(const MathFunc::Vector& v)`

**Операции и методы:**
| Операция / метод | Сигнатура | Описание |
|---|---|---|
| Сложение/вычитание векторов | `operator+, operator-` | Поэлементные |
| Масштабирование | `operator*(fType), operator/(fType)` | Умножение/деление координат на scalar |
| Сравнение точек | `operator==, operator!=` | С учетом погрешности `1e-9` |
| `rotate(angle)` | `Point rotate(fType angle) const` | Вращение вокруг `(0,0)` на angle (рад)
| `rotate(center, angle)` | `Point rotate(const Point& center, fType angle) const` | Вращение против часовой от центра (гр/переводится в рад)
| `reflect(point)` | `Point reflect(const Point& point) const` | Симметрия относительно точки
| `scale(point, scale)` | `Point scale(const Point& point, fType scale) const` | Гомотетия с центром `point`
| `perpendicular()` | `Point perpendicular() const` | Перпендикулярный вектор `(-y, x)` |
| `dotProduct(another)` | `fType dotProduct(const Point&) const` | Скалярное произведение |
| `crossProduct(another)` | `fType crossProduct(const Point&) const` | Векторное (2D) |
| `length2()` | `fType length2() const` | Квадрат длины вектора |
| `length()` | `fType length() const` | Длина вектора |
| `bisector(rhs)` | `Point bisector(Point rhs) const` | Биссектриса угла между векторами |
| `angleBetweenTwoVectors(another)` | `fType angleBetweenTwoVectors(const Point&) const` | Угол между векторами (рад)

---

## Line

**Суть:** прямая на плоскости в общем виде `Ax + By + C = 0`.

**Поля (protected):**
- `fType cA_, cB_, cC_` — коэффициенты A, B, C.

**Конструкторы:**
- `Line(const Point& p1, const Point& p2)` — по двум точкам.
- `Line(fType k, fType b)` — по угловому коэффициенту `k` и сдвигу `b`.
- `Line(const Point& point, fType k)` — по точке и `k`.
- `Line(const Point& p, const Vector& v)` — по точке и направляющему вектору.

**Методы:**
| Метод | Сигнатура | Описание |
|---|---|---|
| `getCoefficients()` | `tuple<fType,fType,fType> getCoefficients() const` | Возвращает `(A,B,C)` |
| `intersection(another)` | `Point intersection(const Line&) const` | Точка пересечения двух прямых |
| `reflect(point)` | `Point reflect(const Point&) const` | Симметрия точки относительно прямой |

**Операторы:**
- `operator==, operator!=` — проверка на совпадение прямых (пропорциональность коэффициентов).
- `operator<<` — вывод `A B C`.

---

## Shape (абстрактный)

**Суть:** базовый класс для всех фигур, задает интерфейс.

**Чисто виртуальные методы:**
| Метод | Описание |
|---|---|
| `perimeter()` | Периметр |
| `area()` | Площадь |
| `isCongruentTo(Shape&)` | Конгруэнтность |
| `isSimilarTo(Shape&)` | Подобие |
| `containsPoint(Point&)` | Проверка, лежит ли точка внутри |
| `rotate(center,angle)` | Поворот |
| `reflect(center)` | Симметрия относительно точки |
| `reflect(axis)` | Симметрия относительно прямой |
| `scale(center,coef)` | Гомотетия |

**Общий оператор:** `operator==` вызывает `isEqual` (проверка множества точек).

---

## Polygon

**Наследует:** `Shape`

**Суть:** простой многоугольник без самопересечений.

**Поля:**
- `vector<Point> vertices_` — вершины в порядке обхода.

**Конструкторы:**
- По `vector<Point>`;
- По списку `{Point, ...}`;
- Шаблонный для параметров через запятую.

**Методы:**
| Метод | Сигнатура | Описание |
|---|---|---|
| `getVertices()` | `vector<Point> getVertices() const` |
| `verticesCount()` | `size_t verticesCount() const` |
| `isConvex()` | `bool isConvex() const` | Выпуклость |
| `perimeter()` | override | Сумма длин сторон |
| `area()` | override | По формуле треугольников от точки 0 |
| `isCongruentTo`, `isSimilarTo` | override | Через коэффициент подобия |
| `containsPoint(point)` | override | Лучевой метод (знаки векторного произведения) |
| `rotate`, `reflect`, `scale` | override | Трансформации всех вершин |

---

## Ellipse

**Наследует:** `Shape`

**Суть:** эллипс с фокусами `focus1_`, `focus2_` и большой осью `diameter_`.

**Поля (protected):**
- `Point focus1_, focus2_`;
- `fType diameter_`.

**Конструктор:**
- `Ellipse(Point f1, Point f2, fType diameter)`.

**Методы:**
| Метод | Сигнатура | Описание |
|---|---|---|
| `focuses()` | `pair<Point,Point>` | Фокусы |
| `directrices()` | `pair<Line,Line>` | Директрисы |
| `eccentricity()` | `fType` | Эксцентриситет `c/d` |
| `perimeter()` | override | Приближенная формула окружности Рамануджана |
| `area()` | override | `πab` |
| `isCongruentTo`, `isSimilarTo`, `isEqual` | override | Сравнение осей и эксцентриситета |
| `containsPoint(point)` | override | Сумма расстояний до фокусов ≤ диаметра |
| `rotate`, `reflect`, `scale` | override | Трансформации фокусов и диаметра |

---

## Circle

**Наследует:** `Ellipse`

**Суть:** круг как частный случай эллипса с равными фокусами.

**Конструктор:**
- `Circle(Point center, fType radius)`.

**Методы:**
| Метод | Описание |
|---|---|
| `radius()` | Радиус |
| `perimeter()` | `2πr` |
| `area()` | `πr²` |
| `center()` | Центр |

---

## Rectangle

**Наследует:** `Polygon`

**Суть:** произвольный прямоугольник.

**Конструкторы:**
- По двум противоположным вершинам и отношению сторон `ratio`.

**Методы:**
| Метод | Сигнатура | Описание |
|---|---|---|
| `center()` | `Point` | Центр (средняя точка диагонали) |
| `diagonals()` | `pair<Line,Line>` | Диагонали |
| `perimeter()`, `area()` | override | По сторонам |

---

## Square

**Наследует:** `Rectangle`

**Суть:** квадрат, частный случай прямоугольника со `ratio=1`.

**Конструктор:**
- По двум противоположным вершинам.

**Методы:**
| Метод | Описание |
|---|---|
| `circumscribedCircle()` | Описанная окружность |
| `inscribedCircle()` | вписанная окружность |

---

## Triangle

**Наследует:** `Polygon`

**Суть:** треугольник.

**Методы:**
| Метод | Описание |
|---|---|
| `circumscribedCircle()` | Описанная окружность |
| `inscribedCircle()` | Вписанная окружность |
| `centroid()` | Центроид (центр масс) |
| `orthocenter()` | Ортоцентр |
| `EulerLine()` | Прямая Эйлера |
| `ninePointsCircle()` | Окружность Эйлера |

---

## Common Operations for any `Shape`

| Метод | Описание |
|---|---|
| `perimeter()` | Периметр |
| `area()` | Площадь |
| `operator==` | Совпадение как множеств точек |
| `isCongruentTo` | Конгруэнтность (движение) |
| `isSimilarTo` | Подобие |
| `containsPoint` | Принадлежность точки |
| `rotate(center,angle)` | Поворот вокруг точки |
| `reflect(center)` | Симметрия точки |
| `reflect(axis)` | Симметрия прямой |
| `scale(center,coef)` | Гомотетия |

---

*Файл:* `geometry.h`  
*Компиляция:* Make + включение через `#include "geometry.h"`.