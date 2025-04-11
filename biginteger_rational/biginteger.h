#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <deque>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

class BigInteger {
public:
  BigInteger(int64_t number) {
    if (number == 0) {
      isNegative_ = false;
      digits_ = {0};
      return;
    }
    isNegative_ = number < 0;
    number = llabs(number);
    while (number > 0) {
      digits_.push_back(number % kBase);
      number /= kBase;
    }
  }

  BigInteger() : digits_({0}), isNegative_(false) {}

  BigInteger(const std::string &string) {
    ssize_t gapLength = 0;
    if (string.front() == '+') {
      ++gapLength;
    }
    if (string.front() == '-') {
      ++gapLength;
      isNegative_ = true;
    }
    digits_.resize((string.size() - static_cast<long unsigned int>(gapLength)) / kPow + 1);
    size_t index = 0;
    for (ssize_t i = static_cast<ssize_t>(string.size()); i > gapLength; i -= kPow) {
      ssize_t l = std::max<ssize_t>(gapLength, i - kPow);
      digits_[index++] = stoll(string.substr(static_cast<size_t>(l), static_cast<size_t>(i - l)));
    }
    normalize();
  }

  BigInteger(const char* str, size_t size)
    : BigInteger(std::string(str, str + size))
  {}

  std::string toString() const {
    if (signum() == 0) {
      return "0";
    }
    std::string result = (signum() == -1 ? "-" : "");
    for (auto i: digits_) {
      for (int j = 0; j < kPow; ++j) {
        result += static_cast<char>(i % 10 + '0');
        i /= 10;
      }
    }
    while (result.back() == '0') {
      result.pop_back();
    }
    std::reverse(result.begin() + (signum() == -1), result.end());
    return result;
  }

  explicit operator bool() const { return signum() != 0; }

  BigInteger &operator++() {
    *this += 1;
    return *this;
  }

  BigInteger &operator--() {
    *this -= 1;
    return *this;
  }

  BigInteger operator++(int) {
    auto result = *this;
    ++*this;
    return result;
  }

  BigInteger operator--(int) {
    auto result = *this;
    --*this;
    return result;
  }

  BigInteger &operator+=(const BigInteger &rhs) {
    add(rhs, false);
    return *this;
  }

  BigInteger &operator-=(const BigInteger &rhs) {
    add(rhs, true);
    return *this;
  }

  BigInteger &operator*=(const BigInteger &rhs) {
    int resultSign = signum() * rhs.signum();
    if (resultSign == 0) {
      return *this = 0;
    }
    size_t kLength = digits_.size() + rhs.digits_.size();
    if (std::max(digits_.size(), rhs.digits_.size()) < kShortNumber) {
      slowMultiply(rhs, kLength, resultSign);
    } else {
      fastMultiply(rhs, kLength, resultSign);
    }
    normalize();
    return *this;
  }

  BigInteger &operator/=(const BigInteger &operand) {
    divide(operand, true);
    return *this;
  }

  BigInteger &operator%=(const BigInteger &operand) {
    divide(operand, false);
    return *this;
  }

  BigInteger operator-() const { return BigInteger(digits_, !isNegative_); }

  friend BigInteger operator-(const BigInteger &lhs, const BigInteger &rhs) {
    BigInteger result(lhs);
    result -= rhs;
    return result;
  }

  friend BigInteger operator+(const BigInteger &lhs, const BigInteger &rhs) {
    BigInteger result(lhs);
    result += rhs;
    return result;
  }

  friend BigInteger operator*(const BigInteger &lhs, const BigInteger &rhs) {
    BigInteger result(lhs);
    result *= rhs;
    return result;
  }

  friend BigInteger operator/(const BigInteger &lhs, const BigInteger &rhs) {
    BigInteger result(lhs);
    result /= rhs;
    return result;
  }

  friend BigInteger operator%(const BigInteger &lhs, const BigInteger &rhs) {
    BigInteger result(lhs);
    result %= rhs;
    return result;
  }

  friend std::weak_ordering operator<=>(const BigInteger &lhs, const BigInteger &rhs) {
    ssize_t leftSize = static_cast<ssize_t>(lhs.digits_.size());
    ssize_t rightSize = static_cast<ssize_t>(rhs.digits_.size());
    int leftSign = lhs.signum();
    int rightSign = rhs.signum();
    if (leftSign != rightSign) {
      return leftSign <=> rightSign;
    }
    if (leftSign == 0) {
      return std::weak_ordering::equivalent;
    }
    if (leftSize != rightSize) {
      return leftSign * leftSize <=> rightSign * rightSize;
    }
    for (ssize_t i = leftSize - 1; i > -1; --i) {
      if (lhs.digits_[static_cast<size_t>(i)] < rhs.digits_[static_cast<size_t>(i)]) {
        return (leftSign == -1 ? std::weak_ordering::greater : std::weak_ordering::less);
      }
      if (lhs.digits_[static_cast<size_t>(i)] > rhs.digits_[static_cast<size_t>(i)]) {
        return (leftSign == -1 ? std::weak_ordering::less : std::weak_ordering::greater);
      }
    }
    return std::weak_ordering::equivalent;
  }

  friend bool operator==(const BigInteger &lhs, const BigInteger &rhs) {
    if (lhs.signum() != rhs.signum() || lhs.digits_.size() != rhs.digits_.size()) {
      return false;
    }
    return lhs <=> rhs == std::weak_ordering::equivalent;
  }

  friend std::ostream &operator<<(std::ostream &outStream, const BigInteger &number) {
    outStream << number.toString();
    return outStream;
  }

  friend std::istream &operator>>(std::istream &inStream, BigInteger &other) {
    bool haveGotAnything = false;
    bool haveGotSign = false;
    bool isNegative = false;
    std::string temporary;
    do {
      const char currentSymbol = static_cast<char>(inStream.peek());
      if (currentSymbol == EOF) {
        break;
      }
      if (std::isspace(currentSymbol)) {
        if (!haveGotAnything) {
          inStream.get();
          continue;
        }
        break;
      }
      bool haveGotNextSymbol = false;

      if (currentSymbol == '-' && temporary.size() == 0 && !haveGotSign) {
        isNegative = true;
        haveGotNextSymbol = true;
        haveGotSign = true;
      }
      if (currentSymbol == '+' && temporary.size() == 0 && !haveGotSign) {
        haveGotNextSymbol = true;
        haveGotSign = true;
      }

      if (isdigit(currentSymbol)) {
        temporary.push_back(currentSymbol);
        haveGotNextSymbol = true;
      }

      if (haveGotNextSymbol) {
        inStream.get();
      } else {
        break;
      }
      haveGotAnything = true;
    } while (true);

    other = BigInteger(temporary);
    other.isNegative_ = isNegative;

    return inStream;
  }

  void swap(BigInteger &rhs) {
    std::swap(isNegative_, rhs.isNegative_);
    digits_.swap(rhs.digits_);
  }

  friend class Rational;

#ifdef LOCAL
  std::deque<int64_t> getDigits() const { return digits_; }
#endif

private:
  using integerType = int64_t;

  static const int64_t kBase = 1e5;
  static const int64_t kPow = 5;
  static const size_t kShortNumber = 10000;
  std::deque<integerType> digits_;
  bool isNegative_ = false;

  BigInteger(const std::deque<integerType> &digits, bool sign) : digits_(digits), isNegative_(sign) {}

  BigInteger(std::deque<integerType> &digits) : BigInteger(digits, true) {}

  bool isZero() const { return digits_.size() == 1 && digits_.front() == 0; }

  int signum() const {
    if (isZero()) {
      return 0;
    }
    return isNegative_ ? -1 : 1;
  }

  void clear() {
    digits_.clear();
    isNegative_ = false;
  }

  void normalize() {
    while (!digits_.empty() && digits_.back() == 0) {
      digits_.pop_back();
    }
    if (digits_.empty()) {
      isNegative_ = false;
      digits_ = {0};
    }
  }

  static void precalcWs(double phi, size_t kLength, std::vector<std::complex<double>> &rootDegrees) {
    rootDegrees.front() = 1;
    for (size_t i = 1; i < kLength; ++i) {
      rootDegrees[i] = std::complex(cos(phi * static_cast<double>(i)), sin(phi *  static_cast<double>(i)));
    }
  }

  static void fastFourierTransform(std::vector<std::complex<double>> &polynomial, const int kLength,
                                   const int kLogLength, std::vector<std::complex<double>> &rootDegrees,
                                   std::vector<std::complex<double>> &temporaryLayer) {
    reorder(polynomial, static_cast<size_t>(kLength), kLogLength);
    int blockLengthLog = 0;
    int layerIndex = kLength >> 1;
    while ((1 << blockLengthLog) < kLength) {
      int blockLength = 1 << blockLengthLog;
      int newBlockLength = blockLength << 1;
      for (int i = 0; i < kLength; ++i) {
        int startBlockIndex = i - (i & (newBlockLength - 1));
        int indexInBlock = i & (newBlockLength - 1);
        temporaryLayer[static_cast<size_t>(i)] =
          polynomial[static_cast<size_t>(startBlockIndex + (indexInBlock & (blockLength - 1)))] +
            rootDegrees[static_cast<size_t>(1LL * layerIndex * indexInBlock % kLength)] *
              polynomial[static_cast<size_t>((1 << blockLengthLog) + startBlockIndex + (indexInBlock & (blockLength - 1)))];
      }
      polynomial.swap(temporaryLayer);
      blockLengthLog++;
      layerIndex >>= 1;
    }
  }

  static void reorder(std::vector<std::complex<double>> &polynomial, size_t kLength, int kLogLength) {
    for (size_t i = 0; i < kLength; ++i) {
      int reversedIndex = reverseInt(static_cast<int>(i), kLogLength);
      if (reversedIndex < static_cast<int>(i)) {
        std::swap(polynomial[i], polynomial[static_cast<size_t>(reversedIndex)]);
      }
    }
  }

  static int reverseInt(int number, int log2kLength) {
    --log2kLength;
    int result = 0;
    for (int bit = 0; bit < log2kLength; ++bit) {
      if ((number >> bit) & 1)
        result |= (1 << (log2kLength - 1 - bit));
    }
    return result;
  }

  static BigInteger multiply(BigInteger lhs, integerType rhs) {
    lhs.digits_.push_back(0);
    for (ssize_t i = static_cast<ssize_t>(lhs.digits_.size()) - 2; i > -1; --i) {
      int64_t cur = 1LL * lhs.digits_[static_cast<size_t>(i)] * rhs;
      lhs.digits_[static_cast<size_t>(i)] = cur % kBase;
      lhs.digits_[static_cast<size_t>(i + 1)] += cur / kBase;
    }
    for (size_t i = 0; i + 1 < lhs.digits_.size(); ++i) {
      lhs.digits_[i + 1] += lhs.digits_[i] / kBase;
      lhs.digits_[i] %= kBase;
    }
    lhs.normalize();
    return lhs;
  }

  void slowMultiply(const BigInteger& rhs, size_t kLength, int resultSign) {
    std::deque<integerType> result(kLength);
    int index = 0;
    for (integerType digit: rhs.digits_) {
      for (size_t i = 0; i < digits_.size(); ++i) {
        int64_t cur = 1LL * digit * digits_[i];
        result[i + static_cast<size_t>(index)] += cur % kBase;
        result[i + static_cast<size_t>(index) + 1] += cur / kBase;
      }
      for (size_t i = 0; i + 1 < kLength; i++) {
        result[i + 1] += result[i] / kBase;
        result[i] %= kBase;
      }
      ++index;
    }
    digits_.swap(result);
    isNegative_ = resultSign == -1;
  }

  void fastMultiply(const BigInteger& rhs, size_t kLength, int resultSign) {
    while (__builtin_popcount(static_cast<unsigned int>(kLength)) != 1) {
      ++kLength;
    }

    std::vector<std::complex<double>> leftMultiplier(kLength);
    std::vector<std::complex<double>> rightMultiplier(kLength);
    std::vector<std::complex<double>> temporaryLayer(kLength);
    std::vector<std::complex<double>> rootDegrees(kLength);

    for (size_t i = 0; i < kLength; ++i) {
      if (i < digits_.size()) {
        leftMultiplier[i] = static_cast<double>(digits_[i]);
      } else {
        leftMultiplier[i] = 0;
      }
    }
    for (size_t i = 0; i < kLength; ++i) {
      if (i < rhs.digits_.size()) {
        rightMultiplier[i] = static_cast<double>(rhs.digits_[i]);
      } else {
        rightMultiplier[i] = 0;
      }
    }

    const int kLogLength = sizeof(kLength) * 8 - static_cast<size_t>(__builtin_clz(static_cast<unsigned int>(kLength)));
    const double kPhi = 2.0 * acos(-1.0) / static_cast<double>(kLength);

    precalcWs(kPhi, kLength, rootDegrees);
    fastFourierTransform(leftMultiplier, static_cast<int>(kLength), kLogLength, rootDegrees, temporaryLayer);
    fastFourierTransform(rightMultiplier, static_cast<int>(kLength), kLogLength, rootDegrees, temporaryLayer);
    for (size_t i = 0; i < kLength; i++) {
      leftMultiplier[i] = leftMultiplier[i] * rightMultiplier[i];
    }

    precalcWs(-kPhi, kLength, rootDegrees);
    fastFourierTransform(leftMultiplier, static_cast<int>(kLength), kLogLength, rootDegrees, temporaryLayer);
    for (size_t i = 0; i < kLength; i++) {
      leftMultiplier[i] /= static_cast<double>(kLength);
    }

    isNegative_ = resultSign == -1;
    const size_t kDigitsSize = kLength;
    digits_.resize(kDigitsSize);
    int64_t cur = 0;
    for (size_t i = 0; i < kDigitsSize; i++) {
      if (i < kLength) {
        cur += static_cast<int64_t>(round(leftMultiplier[i].real()));
      }
      digits_[i] = cur % kBase;
      cur /= kBase;
    }
  }

  void divide(BigInteger rhs, bool doReturnWholePart) {
    BigInteger lhs(*this);
    int leftSign = lhs.signum();
    int rightSign = rhs.signum();
    int resultSign = leftSign * rightSign;
    if (resultSign == 0) {
      return;
    }
    isNegative_ = resultSign == -1;
    int currentPower = 0;
    lhs.isNegative_ = rhs.isNegative_ = false;
    while (rhs.digits_.size() < lhs.digits_.size()) {
      rhs.digits_.push_front(0);
      currentPower++;
    }
    digits_.assign(static_cast<size_t>(currentPower + 1), 0);
    while (currentPower >= 0) {
      int32_t leftBound = 0, rightBound = kBase;
      while (rightBound - leftBound > 1) {
        int32_t middle = (leftBound + rightBound) >> 1;
        if (lhs >= multiply(rhs, middle)) {
          leftBound = middle;
        } else {
          rightBound = middle;
        }
      }
      digits_[static_cast<size_t>(currentPower)] = leftBound;
      lhs -= rhs * leftBound;
      rhs.digits_.pop_front();
      currentPower--;
    }
    if (doReturnWholePart) {
      normalize();
      return;
    }
    lhs.isNegative_ = leftSign == -1;
    swap(lhs);
  }

  void add(const BigInteger &rhs, bool isSubtraction) {
    const size_t kResulLength = std::max(digits_.size(), rhs.digits_.size());
    digits_.resize(kResulLength + 1);
    int64_t reminder = 0;
    int64_t curDigit = 0;
    for (size_t i = 0; i < kResulLength; ++i) {
      curDigit = digits_[i];

      if (isNegative_) {
        curDigit *= -1;
      }

      curDigit += reminder;
      reminder = 0;

      if (i < rhs.digits_.size()) {
        curDigit += (rhs.isNegative_ != isSubtraction ? -1 : 1) * rhs.digits_[i];
      }

      if (curDigit >= kBase) {
        int64_t wholePart = curDigit / kBase;
        curDigit -= wholePart * kBase;
        reminder += wholePart;
      } else if (curDigit < 0) {
        int64_t wholePartsNeedToAdd = (llabs(curDigit) + kBase - 1) / kBase;
        curDigit += wholePartsNeedToAdd * kBase;
        reminder -= wholePartsNeedToAdd;
      }
      digits_[i] = curDigit;
    }
    digits_.back() = reminder;
    isNegative_ = reminder < 0;
    if (isNegative_) {
      reminder = 0;
      for (size_t i = 0; i < kResulLength + 1; ++i) {
        digits_[i] *= -1;
        digits_[i] += reminder;
        reminder = 0;
        if (digits_[i] < 0) {
          --reminder;
          digits_[i] += kBase;
        }
      }
    }
    normalize();
  }
};

BigInteger operator""_bi(unsigned long long val) {
  return BigInteger(static_cast<int64_t>(val));
}

BigInteger operator"" _bi(const char* str, size_t size) {
  return BigInteger(str, size);
}

BigInteger operator"" _bi(const char* str) {
  return BigInteger(str);
}

class Rational {
public:
  Rational (const BigInteger& number) : numerator_(number), denominator_(1) {}

  Rational (const int number)
  : numerator_(number)
  , denominator_(1) {}

  Rational () : numerator_(0), denominator_(1) {}

  Rational& operator+=(const Rational& rhs) {
    this->numerator_ *= rhs.denominator_;
    this->numerator_ += rhs.numerator_ * denominator_;
    this->denominator_ *= rhs.denominator_;
    return *this;
  }

  Rational& operator-=(const Rational& rhs) {
    this->numerator_ = numerator_ * rhs.denominator_ - rhs.numerator_ * denominator_;
    this->denominator_ = denominator_ * rhs.denominator_;
    return *this;
  }

  Rational& operator/=(const Rational& rhs) {
    this->numerator_ *= rhs.denominator_;
    this->denominator_ *= rhs.numerator_;
    return *this;
  }

  Rational& operator*=(const Rational& rhs) {
    this->numerator_ *= rhs.numerator_;
    this->denominator_ *= rhs.denominator_;
    return *this;
  }

  friend Rational operator+(const Rational& lhs, const Rational& rhs) {
    Rational result(lhs);
    result += rhs;
    return result;
  }

  friend Rational operator-(const Rational& lhs, const Rational& rhs) {
    Rational result(lhs);
    result -= rhs;
    return result;
  }

  friend Rational operator*(const Rational& lhs, const Rational& rhs) {
    Rational result(lhs);
    result *= rhs;
    return result;
  }

  friend Rational operator/(const Rational& lhs, const Rational& rhs) {
    Rational result(lhs);
    result /= rhs;
    return result;
  }

  friend Rational operator-(const Rational& number) {
    return Rational(-number.numerator_, number.denominator_);
  }

  friend std::weak_ordering operator<=>(const Rational& lhs, const Rational& rhs) {
    return lhs.numerator_ * rhs.denominator_ <=> rhs.numerator_ * lhs.denominator_;
  }

  friend bool operator==(const Rational& lhs, const Rational& rhs) {
    return lhs.numerator_ * rhs.denominator_ <=> rhs.numerator_ * lhs.denominator_ == std::weak_ordering::equivalent;
  }

  std::string toString() const {
    if (numerator_ == 0) {
      return numerator_.toString();
    }
    normalize();
    if (denominator_ == 1) {
      return numerator_.toString();
    }
    return numerator_.toString() + "/" + denominator_.toString();
  }

  std::string asDecimal(size_t precision = 0) const {
    normalize();
    auto divisible = numerator_;
    for (size_t i = 0; i < (precision + BigInteger::kPow - 1) / BigInteger::kPow; ++i) {
      divisible.digits_.push_front(0);
    }
    size_t symbolsToDelete = (precision + BigInteger::kPow - 1) / BigInteger::kPow * BigInteger::kPow - precision;
    divisible /= denominator_;
    std::string result = divisible.toString();
    for (size_t i = 0; i < symbolsToDelete ; ++i) {
      result.pop_back();
    }

    if (precision > 0) {
      result += '.';
      std::reverse(result.begin(), result.end());
      bool isNegative = false;
      if (result.back() == '-') {
        isNegative = true;
        result.pop_back();
      }

      size_t pointPointer = 0;
      for (size_t i = 0; i < precision; ++i) {
        if (pointPointer + 1 == result.size()) {
          result.push_back('0');
        }
        std::swap(result[pointPointer], result[pointPointer + 1]);
        pointPointer++;
      }
      if (result.back() == '.') {
        result.push_back('0');
      }
      if (isNegative) {
        result.push_back('-');
      }
      std::reverse(result.begin(), result.end());
    }
    return result;
  }

  void swap(Rational& rhs) {
    numerator_.swap(rhs.numerator_);
    denominator_.swap(rhs.denominator_);
  }

  explicit operator double() {
    return stod(asDecimal(kPrecision));
  }

  Rational (const BigInteger& num, const BigInteger& den)
    : numerator_(num)
    , denominator_(den) {
    normalize();
  }
private:
  mutable BigInteger numerator_ = 0;
  mutable BigInteger denominator_ = 1;

  static const size_t kPrecision = 6;

  static BigInteger greatestCommonDivisor(BigInteger lhs, BigInteger rhs) {
    if (lhs == 0) {
      return rhs;
    }
    return greatestCommonDivisor(rhs %= lhs,lhs);
  }

  void normalize() const {
    int sign = numerator_.signum() * denominator_.signum();
    numerator_.isNegative_ = denominator_.isNegative_ = false;
    auto gcd = greatestCommonDivisor(numerator_, denominator_);
    numerator_ /= gcd;
    denominator_ /= gcd;
    numerator_.isNegative_ = sign == -1;
  }
};