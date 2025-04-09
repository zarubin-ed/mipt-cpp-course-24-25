# Документация класса `BigInteger`

Класс `BigInteger` реализует длинную целочисленную арифметику произвольной точности, поддерживая знаковые целые числа. Цель класса — предоставить интерфейс и реализацию базовых арифметических операций, операций сравнения и преобразования. Умножение производится с помощью алгоритма быстрого преобразования Фурье (FFT), обеспечивая временную сложность O(n log n). Операция деления реализована с временной сложностью O(n²).

## Конструкторы

- `BigInteger()`: конструктор по умолчанию. Инициализирует значение `0`.
- `BigInteger(int64_t x)`: инициализирует `BigInteger` целым числом `x`.
- `BigInteger(const std::string& s)`: инициализирует `BigInteger` строкой десятичного представления числа.

## Арифметические операторы

- `BigInteger operator+(const BigInteger& other) const`: возвращает сумму двух чисел. **Сложность:** O(n)
- `BigInteger operator-(const BigInteger& other) const`: возвращает разность. **Сложность:** O(n)
- `BigInteger operator*(const BigInteger& other) const`: возвращает произведение двух чисел, реализованное через FFT. **Сложность:** O(n log n)
- `BigInteger operator/(const BigInteger& other) const`: возвращает результат деления текущего числа на `other`. **Сложность:** O(n²)
- `BigInteger& operator+=(const BigInteger& other)`: прибавляет `other` к текущему числу. **Сложность:** O(n)
- `BigInteger& operator-=(const BigInteger& other)`: вычитает `other` из текущего числа. **Сложность:** O(n)
- `BigInteger& operator*=(const BigInteger& other)`: умножает текущее число на `other` с использованием FFT. **Сложность:** O(n log n)
- `BigInteger& operator/=(const BigInteger& other)`: делит текущее число на `other`. **Сложность:** O(n²)

## Операторы сравнения

- `bool operator==(const BigInteger& other) const`
- `bool operator!=(const BigInteger& other) const`
- `bool operator<(const BigInteger& other) const`
- `bool operator>(const BigInteger& other) const`
- `bool operator<=(const BigInteger& other) const`
- `bool operator>=(const BigInteger& other) const`

## Унарные операторы

- `BigInteger operator-() const`: унарный минус (инвертирует знак).
- `BigInteger operator+() const`: унарный плюс (возвращает копию).

## Преобразования

- `explicit operator bool() const`: возвращает `true`, если число не равно нулю.
- `std::string to_string() const`: преобразует число к строке в десятичной системе счисления.
- `friend std::ostream& operator<<(std::ostream& os, const BigInteger& x)`: вывод в поток.
- `friend std::istream& operator>>(std::istream& is, BigInteger& x)`: ввод из потока.

---

# Документация по классу `Rational`

Класс `Rational` реализует арифметику рациональных чисел произвольной точности, используя класс `BigInteger` для представления числителя и знаменателя. Все рациональные числа хранятся в несократимом виде с положительным знаменателем.

## Конструкторы

- `Rational()`: Конструктор по умолчанию. Инициализирует объект значением `0/1`.
- `Rational(const BigInteger& numerator)`: Инициализирует рациональное число с заданным числителем и знаменателем, равным `1`.
- `Rational(const BigInteger& numerator, const BigInteger& denominator)`: Инициализирует рациональное число с заданными числителем и знаменателем. Если знаменатель отрицательный, знак переносится в числитель. Выполняется сокращение дроби.

## Арифметические операторы

- `Rational operator+(const Rational& other) const`: Возвращает сумму двух рациональных чисел. **Сложность:** зависит от сложности операций сложения и умножения в `BigInteger`.
- `Rational operator-(const Rational& other) const`: Возвращает разность двух рациональных чисел. **Сложность:** аналогично оператору сложения.
- `Rational operator*(const Rational& other) const`: Возвращает произведение двух рациональных чисел. **Сложность:** зависит от сложности операций умножения в `BigInteger`.
- `Rational operator/(const Rational& other) const`: Возвращает частное от деления на другое рациональное число. **Сложность:** зависит от сложности операций умножения и деления в `BigInteger`.
- `Rational& operator+=(const Rational& other)`: Прибавляет к текущему числу другое рациональное число. **Сложность:** аналогично оператору сложения.
- `Rational& operator-=(const Rational& other)`: Вычитает из текущего числа другое рациональное число. **Сложность:** аналогично оператору сложения.
- `Rational& operator*=(const Rational& other)`: Умножает текущее число на другое рациональное число. **Сложность:** аналогично оператору умножения.
- `Rational& operator/=(const Rational& other)`: Делит текущее число на другое рациональное число. **Сложность:** аналогично оператору деления.

## Операторы сравнения

- `bool operator==(const Rational& other) const`
- `bool operator!=(const Rational& other) const`
- `bool operator<(const Rational& other) const`
- `bool operator>(const Rational& other) const`
- `bool operator<=(const Rational& other) const`
- `bool operator>=(const Rational& other) const`

Сравнение осуществляется на основе числителя и знаменателя, учитывая, что дроби всегда хранятся в несократимом виде.

## Унарные операторы

- `Rational operator-() const`: Унарный минус (инвертирует знак числителя).
- `Rational operator+() const`: Унарный плюс (возвращает копию текущего объекта).

## Преобразования

- `explicit operator double() const`: Преобразует рациональное число в число с плавающей запятой типа `double`. Может привести к потере точности при больших числах.
- `std::string to_string() const`: Преобразует рациональное число в строку формата "числитель/знаменатель". Если знаменатель равен `1`, возвращается только числитель.

---