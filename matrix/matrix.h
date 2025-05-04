#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <complex>
#include <deque>
#include <iomanip>
#include <initializer_list>
#include <string>
#include <vector>
#include "../biginteger_rational/biginteger.h"

#define CPP23

template<size_t N, size_t T, bool Cond = (T * T > N)>
struct IsPrimeHelper {
  static constexpr bool value = N % T != 0 && IsPrimeHelper<N, T + 1>::value;
};

template<size_t N, size_t T>
struct IsPrimeHelper<N, T, true> {
  static constexpr bool value = true;
};

template<size_t N>
struct IsPrime {
  static constexpr bool value = N > 1 && IsPrimeHelper<N, 2, N < 2 * 2>::value;
};

template<size_t Modulus>
class Residue {
  ssize_t remain_ = 0;
public:
  Residue& operator+=(const Residue& other) {
    return *this = Residue(remain_ += other.remain_);
  }

  Residue& operator-=(const Residue& other) {
    return *this = Residue(remain_ -= other.remain_);
  }

  Residue& operator*=(const Residue& other) {
    return *this = Residue(remain_ *= other.remain_);
  }

  Residue& operator/=(const Residue& other) {
    static_assert(IsPrime<Modulus>::value, "An attempt to divide in the ring of remainders by composite modulus");
    assert(other.remain_ != 0 && "Dividing by zero");

    ssize_t power = Modulus - 2;
    ssize_t answer = 1;
    ssize_t base = other.remain_;

    while (power > 0) {
      if (power & 1) {
        answer *= base;
        answer %= Modulus;
      }
      base *= base;
      base %= Modulus;
      power >>= 1;
    }

    return *this *= answer;
  }

  friend Residue operator+(const Residue& lhs, const Residue& rhs) {
    Residue result(lhs);
    result += rhs;
    return result;
  }

  friend Residue operator-(const Residue& lhs, const Residue& rhs) {
    Residue result(lhs);
    result -= rhs;
    return result;
  }

  friend Residue operator*(const Residue& lhs, const Residue& rhs) {
    Residue result(lhs);
    result *= rhs;
    return result;
  }

  friend Residue operator/(const Residue& lhs, const Residue& rhs) {
    Residue result(lhs);
    result /= rhs;
    return result;
  }

  friend bool operator==(const Residue& lhs, const Residue& rhs) {
    return lhs.remain_ == rhs.remain_;
  }

  friend std::ostream& operator<<(std::ostream& os, const Residue& rhs)  {
    os << rhs.remain_;
    return os;
  }

  friend std::istream& operator>>(std::istream& is, Residue& rhs) {
    is >> rhs.remain_;
    return is;
  }

  Residue(ssize_t x) {
    x %= static_cast<ssize_t>(Modulus);
    if (x < 0) {
      x += static_cast<ssize_t>(Modulus);
    }
    remain_ = x;
  }

  Residue() = default;

  explicit operator int() {
    return static_cast<int>(remain_);
  }
};

template<size_t Rows, size_t Сolumns, typename Field = Rational>
class Matrix {
  std::array<std::array<Field, Сolumns>, Rows> data_;

  inline static const Field kZero = Field(0);
  inline static const Field kOne = Field(1);
  inline static const Field kROne = kZero - kOne;

  template<bool MakeInverseGauss, bool CalculateDet, size_t OtherRows, size_t OtherCols>
  static std::pair<Matrix<OtherRows, OtherCols, Field>, Field> makeGauss(Matrix<OtherRows, OtherCols, Field> matrix) {
    std::vector<size_t> nonZeroColumns;
    int row = 0;
    Field det = kOne;
    for (ssize_t column = 0; column < static_cast<ssize_t>(OtherRows); ++column) {
      size_t nonZeroValueRow = static_cast<size_t>(row);
      for (; nonZeroValueRow < OtherRows; ++nonZeroValueRow) { //finding nonZero value in current column
        if (matrix[nonZeroValueRow, static_cast<size_t>(column)] != kZero) {
          break;
        }
      }

      if (nonZeroValueRow == OtherRows) {
        if constexpr (CalculateDet) {
          return std::make_pair(matrix, kZero);
        }
        continue;
      }

      if (static_cast<size_t>(row) != nonZeroValueRow) {//swap two rows
        for (size_t i = 0; i < OtherCols; ++i) {
          std::swap(matrix[static_cast<size_t>(row), i], matrix[nonZeroValueRow, i]);
        }
        if constexpr (CalculateDet) {
          det *= kROne;
        }
      }

      if (matrix[static_cast<size_t>(row), static_cast<size_t>(column)] != kOne) { //make first element in line equal to 1
        if constexpr (CalculateDet) {
          det *= matrix[static_cast<size_t>(row), static_cast<size_t>(column)];
        }
        for (ssize_t i = OtherCols - 1; i >= column; --i) {
          matrix[static_cast<size_t>(row), static_cast<size_t>(i)] /= matrix[static_cast<size_t>(row), static_cast<size_t>(column)];
        }
      }

      for (size_t i = nonZeroValueRow + 1; i < OtherRows; ++i) {//making zero column
        if (matrix[i, static_cast<size_t>(column)] != kZero) {
          for (ssize_t j = OtherCols - 1; j >= column; --j) {
            matrix[i, static_cast<size_t>(j)] -= matrix[static_cast<size_t>(row), static_cast<size_t>(j)] * matrix[i, static_cast<size_t>(column)];
          }
        }
      }

      if constexpr (MakeInverseGauss) { //memorizing main columns
        nonZeroColumns.push_back(static_cast<size_t>(column));
      }
      ++row;
    }

    if constexpr (!MakeInverseGauss || CalculateDet) {
      return std::make_pair(matrix, det);
    }

    --row;
    for (; row >= 0; --row) {
      ssize_t column = static_cast<ssize_t>(nonZeroColumns[static_cast<size_t>(row)]);
      for (ssize_t i = row - 1; i >= 0; --i) { //making upper triangular
        if (matrix[static_cast<size_t>(i), static_cast<size_t>(column)] != kZero) {
          for (ssize_t j = OtherCols - 1; j >= column; --j) {
            matrix[static_cast<size_t>(i), static_cast<size_t>(j)] -= matrix[static_cast<size_t>(row), static_cast<size_t>(j)] * matrix[static_cast<size_t>(i), static_cast<size_t>(column)];
          }
        }
      }
    }
    return std::make_pair(matrix, det);
  }

  template<bool isAddition>
  void arithmeticOperation(const Matrix& rhs) {
    for (size_t i = 0; i < Rows; ++i) {
      for (size_t j = 0; j < Сolumns; ++j) {
        if constexpr (isAddition) {
          data_[i][j] += rhs.data_[i][j];
        } else {
          data_[i][j] -= rhs.data_[i][j];
        }
      }
    }
  }
public:
  Matrix() = default;

  Matrix(std::initializer_list<std::initializer_list<Field>> values) {
    auto rowIt = values.begin();
    for (std::size_t i = 0; i < Rows; ++i, ++rowIt) {
      std::copy(rowIt->begin(), rowIt->end(), data_[i].begin());
    }
  }

  void print(std::ostream& os) const {
    for (size_t i = 0; i < Rows; ++i) {
      for (size_t j = 0; j < Сolumns; ++j) {
        os << data_[i][j] << " ";
      }
      os << '\n';
    }
  }

  static Matrix unityMatrix() {
    static_assert(Сolumns == Rows, "the matrix must be square");
    Matrix result;
    for (size_t i = 0; i < Rows; ++i) {
      result.data_[i][i] = kOne;
    }
    return result;
  }

  Matrix& operator+=(const Matrix& rhs) {
    arithmeticOperation<true>(rhs);
    return *this;
  }

  Matrix& operator-=(const Matrix& rhs) {
    arithmeticOperation<false>(rhs);
    return *this;
  }

  Matrix& operator*=(const Field& rhs) {
    for (auto& row : data_) {
      for (auto& item : row) {
        item *= rhs;
      }
    }
    return *this;
  }

  friend Matrix operator+(const Matrix& lhs, const Matrix& rhs) {
    Matrix result(lhs);
    result += rhs;
    return result;
  }

  friend Matrix operator-(const Matrix& lhs, const Matrix& rhs) {
    Matrix result(lhs);
    result -= rhs;
    return result;
  }

  friend Matrix operator*(const Matrix& lhs, const Field& rhs) {
    Matrix result(lhs);
    result += rhs;
    return result;
  }

  friend Matrix operator*(const Field& lhs, const Matrix& rhs) {
    Matrix result(rhs);
    result *= lhs;
    return result;
  }

  Matrix& operator*=(const Matrix& rhs) {
    *this = *this * rhs;
    return *this;
  }

  Field det() const {
    static_assert(Сolumns == Rows, "the matrix must be square");
    return makeGauss<false, true>(*this).second;
  }

  Matrix<Сolumns, Rows, Field> transposed() const {
    Matrix<Сolumns, Rows, Field> result;

    for (size_t i = 0; i < Rows; i++) {
      for (size_t j = 0; j < Сolumns; j++) {
        result[j, i] = data_[i][j];
      }
    }

    return result;
  }

  size_t rank() const {
    Matrix result = makeGauss<false, false>(*this).first;
    size_t rank = 0;
    size_t row = 0;
    size_t column = 0;

    while (row < Rows && column < Сolumns) {
      while (column < Сolumns && result[row, column] == kZero) {
        ++column;
      }
      if (column != Сolumns) {
        ++rank;
      }
      ++row;
    }

    return rank;
  }

  Matrix<Сolumns, Rows, Field> inverted() const {
    static_assert(Сolumns == Rows, "the matrix must be square");
    Matrix<Rows, Rows * 2, Field> temporary;
    for (size_t i = 0; i < Rows; ++i) {
      for (size_t j = 0; j < Rows; ++j) {
        temporary[i, j] = data_[i][j];
      }
    }
    for (size_t i = 0; i < Rows; ++i) {
      temporary[i, i + Сolumns] = kOne;
    }

    temporary = makeGauss<true, false>(temporary).first;

    Matrix<Сolumns, Rows, Field> result;
    for (size_t i = 0; i < Rows; ++i) {
      for (size_t j = 0; j < Сolumns; ++j) {
        result[i, j] = temporary[i, j + Rows];
      }
    }

    return result;
  }

  void invert() {
    *this = inverted();
  }

  Field trace() const {
    static_assert(Сolumns == Rows, "the matrix must be square");
    Field result = kZero;
    for (size_t i = 0; i < Rows; ++i) {
      result += data_[i][i];
    }
    return result;
  }

  std::array<Field, Rows> getColumn(size_t column) const {
    std::array<Field, Rows> result;
    for (size_t i = 0; i < Rows; ++i) {
      result[i] = data_[i][column];
    }
    return result;
  }

  std::array<Field, Сolumns> getRow(size_t row) const {
    return data_[row];
  }

  const Field& operator[](size_t x, size_t y) const {
    return data_[x][y];
  }

  Field& operator[](size_t x, size_t y) {
    return data_[x][y];
  }

  friend bool operator == (const Matrix& lhs,const Matrix& rhs) {
    return lhs.data_ == rhs.data_;
  }
};

template<size_t ResultRows, size_t ResultColumns, size_t CommonDimension, typename  Field_>
Matrix<ResultRows, CommonDimension, Field_> operator*(const Matrix<ResultRows, ResultColumns, Field_>& lhs, const Matrix<ResultColumns, CommonDimension, Field_>& rhs) {
  Matrix<ResultRows, CommonDimension, Field_> result;

  for (size_t i = 0; i < ResultRows; ++i) {
    for (size_t j = 0; j < CommonDimension; ++j) {
      for (size_t k = 0; k < ResultColumns; ++k) {
        result[i, j] += lhs[i, k] * rhs[k, j];
      }
    }
  }

  return result;
}

template<size_t Size, typename  Field = Rational>
using SquareMatrix = Matrix<Size, Size, Field>;

