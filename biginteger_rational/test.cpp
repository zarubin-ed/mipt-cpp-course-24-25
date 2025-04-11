#include "biginteger.h"
#include <cassert>
#include <sstream>

using namespace std;

void testBigIntegerCreation() {
    // Тест создания из int
    BigInteger a = 12345;
    assert(a.toString() == "12345");
    BigInteger b(-67890);
    assert(b.toString() == "-67890");

    // Тест создания из строки
    BigInteger c("12345678901234567890");
    assert(c.toString() == "12345678901234567890");
    BigInteger d("-98765432109876543210");
    assert(d.toString() == "-98765432109876543210");

    // Тест создания из литералов
    assert("12345"_bi.toString() == "12345");
    assert("12345678901234567890"_bi.toString() == "12345678901234567890");
}

void testBigIntegerArithmetic() {
    // Сложение
    BigInteger a = 100;
    BigInteger b = 50;
    assert((a + b).toString() == "150");
    assert((a + (-b)).toString() == "50");

    // Вычитание
    assert((a - b).toString() == "50");
    assert((b - a).toString() == "-50");

    // Умножение
    BigInteger c("100000");
    BigInteger d("99999");
    assert((c * d).toString() == "9999900000");
    assert((c * -d).toString() == "-9999900000");

    // Деление
    BigInteger e("1000000");
    BigInteger f("999");
    assert((e / f).toString() == "1001");
    assert((-e / f).toString() == "-1001");

    // Остаток
    assert((e % f).toString() == "1");
    assert((e % -f).toString() == "1");
}

void testBigIntegerComparison() {
    BigInteger a = 100;
    BigInteger b = 200;
    BigInteger c = 100;

    assert(a < b);
    assert(b > a);
    assert(a == c);
    assert(a != b);
    assert(-a < c);
}

void testBigIntegerIncrementDecrement() {
    BigInteger a = 10;
    assert((++a).toString() == "11");
    assert((a++).toString() == "11");
    assert(a.toString() == "12");

    BigInteger b = 10;
    assert((--b).toString() == "9");
    assert((b--).toString() == "9");
    assert(b.toString() == "8");
}

void testBigIntegerIO() {
    stringstream ss;
    BigInteger a("123456789");
    ss << a;
    BigInteger b;
    ss >> b;
    assert(a == b);

    stringstream ss2("-987654321");
    ss2 >> b;
    assert(b.toString() == "-987654321");
}

void testRationalCreation() {
    Rational a(5);       // 5/1
    Rational b(-3);      // -3/1
    Rational c = 2_bi;

    assert(a.toString() == "5");
    assert(b.toString() == "-3");
    assert(c.toString() == "2");
}

void testRationalArithmetic() {
    Rational a(1);      // 1/1
    Rational b(2);      // 2/1
    Rational half = a / b;  // 1/2

    // Сложение
    assert((half + half).toString() == "1");
    assert((half + Rational(1, 2)).toString() == "1");

    // Вычитание
    assert((Rational(3, 2) - half).toString() == "1");

    // Умножение
    assert((Rational(3, 2) * Rational(2, 3)).toString() == "1");

    // Деление
    assert((Rational(4) / Rational(2)).toString() == "2");
}

void testRationalComparison() {
    Rational a(1, 2);   // 1/2
    Rational b(2, 4);   // 1/2 после сокращения
    Rational c(3, 4);

    assert(a == b);
    assert(a < c);
    assert(c > a);
    assert(a <= b);
}

int main() {
    // Тесты BigInteger
    testBigIntegerCreation();
    testBigIntegerArithmetic();
    testBigIntegerComparison();
    testBigIntegerIncrementDecrement();
    testBigIntegerIO();

    // Тесты Rational
    testRationalCreation();
    testRationalArithmetic();
    testRationalComparison();

    cout << "All tests passed!" << endl;
    return 0;
}