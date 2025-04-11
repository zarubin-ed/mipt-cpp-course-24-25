#include "string.h"
#include <iostream>

int main() {
  CowString s1("Example");
  CowString s2 = s1; // Копирование с использованием COW (счетчик ссылок увеличивается)
  s1.push_back('!');

  std::cout << "s1: " << s1 << "\n"; // Вывод: "Example!"
  std::cout << "s2: " << s2 << "\n"; // Вывод: "Example"

  if (s1 != s2)
    std::cout << "Строки различаются\n";

  CowString s3 = s1.substr(0, 4); // "Exam"
  std::cout << "s3: " << s3 << "\n";

  return 0;
}