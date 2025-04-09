#pragma once
#include <algorithm>
#include <cstring>
#include <iostream>

class CowString {
public:
  CowString()
    :  length_ (0)
    , capacity_ (kServiceInformationByteLen)
    , stringData (new char[capacity_])
  {
    get_buffer_counter() = 1;
    stringData[kCounterByteLen + length_] = '\0';
  }

  CowString(const char* cStyleString)
    : length_ (strlen(cStyleString))
    , capacity_ (length_ + kServiceInformationByteLen)
    , stringData (new char[capacity_])
  {
    std::copy(cStyleString, cStyleString + length_, stringData + kCounterByteLen);
    stringData[kCounterByteLen + length_] = '\0';
    get_buffer_counter() = 1;
  }

  CowString(size_t length, char symbol)
    : length_ (length)
    , capacity_ (length_ + kServiceInformationByteLen)
    , stringData (new char[capacity_])
  {
    stringData[length_ + kCounterByteLen] = '\0';
    get_buffer_counter() = 1;
    std::fill(stringData + kCounterByteLen, stringData + kCounterByteLen + length_, symbol);
  }

  CowString (char symbol)
  : CowString(1, symbol)
  {}

  CowString(const CowString& other)
    : length_ (other.length_)
    , capacity_ (other.capacity_)
    , stringData (other.stringData)
  {
    ++get_buffer_counter();
  }

  void shrink_to_fit() {
    if (capacity_ > length_ + kServiceInformationByteLen) {
      reallocate(length_ + kServiceInformationByteLen);
    }
  }

  void clear() {
    make_copy();
    length_ = 0;
    stringData[kCounterByteLen] = '\0';
  }

  bool empty() const {
    return length_ == 0;
  }

  char* data() {
    make_copy();
    return stringData + kCounterByteLen;
  }

  const char* data() const {
    return stringData + kCounterByteLen;
  }

  size_t length() const {
    return length_;
  }

  size_t size() const {
    return length_;
  }

  size_t capacity() const {
    return capacity_ - kServiceInformationByteLen;
  }

  char& operator[](size_t i) {
    make_copy();
    return stringData[i + kCounterByteLen];
  }

  const char& operator[](size_t i) const {
    return stringData[i + kCounterByteLen];
  }

  char& back() {
    make_copy();
    return operator[](length_ - 1);
  }

  const char& back() const {
    return operator[](length_ - 1);
  }

  char& front() {
    make_copy();
    return operator[](0);
  }

  const char& front() const {
    return operator[](0);
  }

  void push_back(char symbol) {
    if (length_ + 1 + kServiceInformationByteLen > capacity_) {
      reallocate(kServiceInformationByteLen + (length_ + 1) * 2);
    } else {
      make_copy();
    }
    stringData[length_ + kCounterByteLen] = symbol;
    stringData[length_ + kCounterByteLen + 1] = '\0';
    ++length_;
  }

  void pop_back() {
    make_copy();
    --length_;
    stringData[length_ + kCounterByteLen] = '\0';
  }

  void swap(CowString& other) {
    std::swap(stringData, other.stringData);
    std::swap(length_, other.length_);
    std::swap(capacity_, other.capacity_);
  }

  size_t find(const CowString& other) const {
    size_t other_length = other.length();
    for (size_t i = 0; i + other_length <= length_; ++i) {
      if (memcmp(stringData + i + kCounterByteLen, other.stringData + kCounterByteLen, other_length) == 0) {
        return i;
      }
    }
    return length_;
  }

  size_t rfind(const CowString& other) const {
    size_t other_length = other.length();
    for (ssize_t i = length_ - other_length; i >= 0; --i) {
      if (memcmp(stringData + i + kCounterByteLen, other.stringData + kCounterByteLen, other_length) == 0) {
        return i;
      }
    }
    return length_;
  }

  CowString substr(size_t start, size_t length) const {
    return CowString(stringData + kCounterByteLen + start , std::min(length, length_ - start));
  }

  CowString& operator=(const CowString& other) {
    if (this->get_string_info() == other.get_string_info()) {
      return *this;
    }
    CowString temp(other);
    swap(temp);
    return *this;
  }

  CowString& operator+=(char symbol) {
    push_back(symbol);
    return *this;
  }

  CowString& operator+=(const CowString& other) {
    if (length_ + other.length_ + kServiceInformationByteLen > capacity_) {
      reallocate(length_ + other.length_ + kServiceInformationByteLen);
    } else {
      make_copy();
    }
    std::copy(other.stringData + kCounterByteLen, other.stringData + kCounterByteLen + other.length_, stringData + length_ + kCounterByteLen);
    length_ += other.length_;
    stringData[length_ + kCounterByteLen] = '\0';
    return *this;
  }

  ~CowString() {
    if (is_alone()) {
      delete [] stringData;
    } else {
      --get_buffer_counter();
    }
  }

private:
  size_t length_ = 0;
  size_t capacity_ = 0;
  char* stringData = nullptr;
  static constexpr int kCounterByteLen = 8;
  static constexpr int kTerminateSymbolByteLen = 1;
  static constexpr int kServiceInformationByteLen = kCounterByteLen + kTerminateSymbolByteLen;

  void make_copy() {
    if (!is_alone()) {
      reallocate(capacity_ + kServiceInformationByteLen);
    }
  }

  bool is_alone() {
    return get_buffer_counter() == 1;
  }

  CowString(const char* cStyleString, const size_t length)
    : length_ (length)
    , capacity_ (length + kServiceInformationByteLen)
    , stringData (new char[capacity_])
  {
    std::copy(cStyleString, cStyleString + length, stringData + kCounterByteLen);
    stringData[length_ + kCounterByteLen] = '\0';
    get_buffer_counter() = 1;
  }

  std::tuple<char*, size_t, size_t> get_string_info() const {
    return std::make_tuple(stringData, capacity_, length_);
  }

  size_t& get_buffer_counter() const {
    return *reinterpret_cast<size_t*>(stringData);
  }

  void reallocate(size_t length) {
    --get_buffer_counter();
    char* temp = new char[length];
    std::copy(stringData, stringData + std::min(capacity_, length), temp);
    capacity_ = length;
    if (get_buffer_counter() == 0) {
      delete[] stringData;
    }
    stringData = temp;
    get_buffer_counter() = 1;
  }
};

bool operator<(const CowString& lhs, const CowString& rhs) {
  const int comparison = memcmp(lhs.data(), rhs.data(), std::min(lhs.length(), rhs.length()));
  return comparison == 0 ? lhs.length() < rhs.length() : comparison < 0;
}

bool operator>=(const CowString& lhs, const CowString& rhs) {
  return !(lhs < rhs);
}

bool operator<=(const CowString& lhs, const CowString& rhs) {
  return rhs >= lhs;
}

bool operator>(const CowString& lhs, const CowString& rhs) {
  return !(lhs <= rhs);
}

bool operator==(const CowString& lhs, const CowString& rhs) {
  return lhs.length() != rhs.length() ? false : memcmp(lhs.data(), rhs.data(), lhs.length()) == 0;
}

bool operator!=(const CowString& lhs, const CowString& rhs) {
  return !(lhs == rhs);
}

CowString operator+(const CowString& lhs, const CowString& rhs) {
  CowString temp(lhs);
  temp += rhs;
  return temp;
}

std::istream& operator>>(std::istream& inStream, CowString& other) {
  other.clear();
  do {
    const char currentSymbol = inStream.get();
    if(currentSymbol == EOF) {
      break;
    }
    if(std::isspace(currentSymbol)) {
      if(other.empty()) {
        continue;
      }
      break;
    }
    other.push_back(currentSymbol);
  } while(true);
  return inStream;
}

std::ostream& operator<<(std::ostream& outStream, const CowString& str) {
  for (size_t i = 0; i < str.length(); ++i) {
    outStream << str[i];
  }
  return outStream;
}