template<typename T>
class Deque {
  class Bucket;
public:
  Deque()
    : size_(0)
    , numberOfBuckets_(0)
    , data_(new Bucket[1])
    , begin_(nullptr, nullptr)
  {}

  Deque(const Deque& other)
    : size_(other.size_)
    , numberOfBuckets_((other.begin_.indexInBlock() + size_ + Bucket::kBucketSize - 1) / Bucket::kBucketSize)
    , data_(new Bucket[numberOfBuckets_ + 1])
  {
    allocateBuckets();
    size_t i = 0;

    try {
      for (; i < size_; ++i) {
        data_[i / Bucket::kBucketSize].setValue(i % Bucket::kBucketSize, other[i]);
      }
    } catch (...) {
      constructorExceptionProcessing(i);
      throw;
    }

    begin_ = iterator(data_, data_->data_ + other.begin_.indexInBlock());
  }

  Deque(size_t size) // to not call odd default constructor
    : size_(size)
    , numberOfBuckets_((size_ + Bucket::kBucketSize - 1) / Bucket::kBucketSize)
    , data_(new Bucket[numberOfBuckets_ + 1])
  {
    allocateBuckets();
    size_t i = 0;

    try {
      for (; i < size_; ++i) {
        new (data_[i / Bucket::kBucketSize].getElementPointer(i % Bucket::kBucketSize)) T();
      }
    } catch (...) {
      constructorExceptionProcessing(i);
      throw;
    }

    begin_ = iterator(data_, data_->data_);
  }

  Deque(size_t size, const T& value)
    : size_(size)
    , numberOfBuckets_((size + Bucket::kBucketSize - 1) / Bucket::kBucketSize)
    , data_ (new Bucket[numberOfBuckets_ + 1])
  {
    allocateBuckets();
    size_t i = 0;
    try {
      for (; i < size_; ++i) {
        data_[i / Bucket::kBucketSize].setValue(i % Bucket::kBucketSize, value);
      }
    } catch (...) {
      constructorExceptionProcessing(i);
      throw;
    }

    begin_ = iterator(data_, data_->data_);
  }

  Deque& operator=(const Deque& other) {
    if(other.data_ != data_) {
      Deque temp(other);
      swap(temp);
    }
    return *this;
  }

  void swap(Deque& other) {
    std::swap(size_, other.size_);
    std::swap(numberOfBuckets_, other.numberOfBuckets_);
    std::swap(data_, other.data_);
    std::swap(begin_, other.begin_);
  }

  ~Deque() {
    for (auto it = begin(); it != end(); ++it) {
      it->~T();
    }
    delete [] data_;
  }

  size_t size() const {
    return size_;
  }

  T& operator[](size_t index) {
    return *(begin_ + index);
  }

  const T& operator[](size_t index) const {
    return *(begin_ + index);
  }

  T& at(size_t index) {
    if (index >= size_) {
      throw std::out_of_range("");
    }
    return operator[](index);
  }

  const T& at(size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("");
    }
    return operator[](index);
  }

  template<bool IsConst>
  class Iterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename std::conditional<IsConst, const T, T>::type;
    using reference = value_type&;
    using pointer = value_type*;
    using difference_type = std::ptrdiff_t;

    Iterator& operator+=(difference_type diff) {
      difference_type blockDiff;
      ssize_t blockIndex = indexInBlock();

      if (diff < 0) {
        blockDiff = -(-diff / Bucket::kBucketSize);
        blockIndex -= -diff % Bucket::kBucketSize;
        if (blockIndex < 0) {
          blockIndex += Bucket::kBucketSize;
          --blockDiff;
        }
      } else {
        blockDiff = diff / Bucket::kBucketSize;
        blockIndex += diff % Bucket::kBucketSize;
        if (static_cast<size_t>(blockIndex) >= Bucket::kBucketSize) {
          blockIndex -= Bucket::kBucketSize;
          ++blockDiff;
        }
      }

      if (bucketPointer_) {
        bucketPointer_ += blockDiff;
        elementPointer_ = bucketPointer_->data_ + blockIndex;
      }

      return *this;
    }

    Iterator& operator-=(difference_type diff) {
      return *this += -diff;
    }

    Iterator& operator++() {
      return *this += 1;
    }

    Iterator& operator--() {
      return *this -= 1;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }

    Iterator operator--(int) {
      Iterator tmp = *this;
      --*this;
      return tmp;
    }

    Iterator operator+(difference_type diff) const {
      Iterator tmp = *this;
      tmp += diff;
      return tmp;
    }

    Iterator operator-(difference_type diff) const {
      Iterator tmp = *this;
      tmp -= diff;
      return tmp;
    }

    difference_type operator-(const Iterator rhs) const {
      if (bucketPointer_ < rhs.bucketPointer_) {
        return -(rhs - *this);
      }

      if (bucketPointer_ > rhs.bucketPointer_) {
        difference_type result = (bucketPointer_ - rhs.bucketPointer_ - 1) * Bucket::kBucketSize;
        result += indexInBlock();
        result += Bucket::kBucketSize - rhs.indexInBlock();
        return result;
      }

      return elementPointer_ - rhs.elementPointer_;
    }

    reference operator*() const {
      return *operator->();
    }

    pointer operator->() const {
      return elementPointer_;
    }

    reference operator[](difference_type diff) const {
      return *(*this + diff);
    }

    operator Iterator<true>() const {
      return Iterator<true>(bucketPointer_, elementPointer_);
    }

    std::strong_ordering operator<=>(const Iterator& other) const = default;

    bool operator==(const Iterator& other) const = default;

  private:
    Bucket* bucketPointer_ = nullptr;
    pointer elementPointer_ = nullptr;

    Iterator() = default;

    Iterator (Bucket* bucketPointer, pointer elementPointer)
      : bucketPointer_(bucketPointer)
      , elementPointer_(elementPointer)
    {}

    ssize_t indexInBlock() const {
      return bucketPointer_ ? elementPointer_ - bucketPointer_->data_ : 0;
    }

    friend class Deque;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = typename iterator::value_type;
  using reference = typename iterator::reference;
  using const_reference = const reference;
  using size_type = size_t;

  iterator begin() {
    return begin_;
  }

  iterator end() {
    return begin_ + size_;
  }

  const_iterator begin() const {
    return const_iterator(begin_);
  }

  const_iterator end() const {
    return const_iterator(begin_ + size_);
  }

  const_iterator cbegin() const {
    return begin();
  }

  const_iterator cend() const {
    return end();
  }

  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rbegin() const {
    return reverse_iterator(end());
  }

  const_reverse_iterator rend() const {
    return reverse_iterator(begin());
  }

  const_reverse_iterator crbegin() const {
    return rbegin();
  }

  const_reverse_iterator crend() const {
    return rend();
  }

  bool empty() const {
    return size_ == 0;
  }

  void pop_back() {
    --size_;
    end()->~T();
  }

  void pop_front() {
    begin()->~T();
    ++begin_;
    --size_;
  }

  void push_front(const T& value) {
    if (numberOfBuckets_ == 0 || begin() == iterator(data_, data_->data_)) {
      Bucket tempBucket;
      tempBucket.allocate();
      tempBucket.setValue(Bucket::kBucketSize - 1, value);
      reallocate(&tempBucket, true);
    } else {
      --begin_;
      try {
        new (&*begin()) T(value);
      } catch (...) {
        ++begin_;
        throw;
      }
    }
    ++size_;
  }

  void push_back(const T& value) {
    if(numberOfBuckets_ == 0 || end() - 1 == iterator(data_ + numberOfBuckets_ - 1, (data_ + numberOfBuckets_ - 1)->data_ + Bucket::kBucketSize - 1)) {
      Bucket tempBucket;
      tempBucket.allocate();
      tempBucket.setValue(0, value);
      reallocate(&tempBucket, false);
    } else {
      new (&*end()) T(value);
    }
    ++size_;
  }

  void insert(const_iterator pos, const T& value) {
    if (pos == cbegin()) {
      push_front(value);
      return;
    }

    size_t index = pos - begin();
    if (numberOfBuckets_ == 0 || end() - 1 == iterator(data_ + numberOfBuckets_ - 1, (data_ + numberOfBuckets_ - 1)->data_ + Bucket::kBucketSize - 1)) {
      reallocate(); // might cause iterator invalidation
    }

    iterator border = begin() + index;
    iterator it = end();
    new (&*it) T(value);
    for (; it != border; --it) {
      std::swap(*it, *(it - 1));
    }
    ++size_;
  }

  void erase(const_iterator pos) {
    if (pos == cbegin()) {
      pop_front();
      return;
    }

    iterator it = begin() + (pos - cbegin());
    for (; it + 1 != end(); ++it) {
      std::swap(*it, *(it + 1));
    }

    --size_;
    end()->~T();
  }

private:
  void reallocate(Bucket* tempBucket = nullptr, bool isNewBucketLhs = false) {
    size_t newCap = newCapacity(numberOfBuckets_);

    if (newCap == 1 && tempBucket) { // if buffer was empty it is corner case
      data_ = new Bucket[newCap];
      *data_ = *tempBucket;
      begin_.bucketPointer_ = data_;
      begin_.elementPointer_ = begin_.bucketPointer_->data_ + (isNewBucketLhs ? Bucket::kBucketSize - 1 : 0);
    } else {
      Bucket* newData = new Bucket[newCap + 1];

      const size_t startIndex = numberOfBuckets_ * (kIncreaseCoefficient - 1) / 2;
      ssize_t beginInsideBlockPosition = begin_.indexInBlock();
      ssize_t beginBlockPosition = begin_.bucketPointer_ ? begin_.bucketPointer_ - data_ : 0;

      for (size_t i = startIndex; i < startIndex + numberOfBuckets_; ++i) {
        newData[i] = data_[i - startIndex];
        data_[i - startIndex] = nullptr;
      }

      for (size_t j = 0; j < startIndex - 1; ++j) { // allocate prefix of buffer
        (newData + j)->allocate();
      }
      if (tempBucket && isNewBucketLhs) {
        newData[startIndex - 1] = *tempBucket; // adding allocated block
      } else {
        (newData + startIndex - 1)->allocate();
      }

      for (size_t j = startIndex + numberOfBuckets_ + 1; j < newCap + 1; ++j) {// suffix
        (newData + j)->allocate();
      }
      if (tempBucket && !isNewBucketLhs) {
        newData[startIndex + numberOfBuckets_] = *tempBucket;// adding allocated block
      } else {
        (newData + startIndex + numberOfBuckets_)->allocate();
      }

      if (tempBucket && isNewBucketLhs) {//begin calculation in unusual only in case of push_front
        begin_.bucketPointer_ = newData + startIndex - 1;
        begin_.elementPointer_ = begin_.bucketPointer_->data_ + Bucket::kBucketSize - 1;
      } else {
        begin_.bucketPointer_ = newData + startIndex + beginBlockPosition;
        begin_.elementPointer_ = begin_.bucketPointer_->data_ + beginInsideBlockPosition;
      }
      delete [] data_;
      data_ = newData;
    }

    if (tempBucket) {
      tempBucket->data_ = nullptr;
    }
    numberOfBuckets_ = newCap;
  }

  void constructorExceptionProcessing(size_t i) {
    for (size_t j = 0; j < i; ++j) {
      data_[i / Bucket::kBucketSize].getElementPointer(i % Bucket::kBucketSize)->~T();
    }
    delete[] data_;
  }

  void allocateBuckets() {
    for (size_t j = 0 ; j < numberOfBuckets_ + 1; ++j) {
      (data_ + j)->allocate();
    }
  }

  static size_t newCapacity(size_t oldCapacity) {
    return oldCapacity > 0 ?  oldCapacity * kIncreaseCoefficient : 1;
  }

  class Bucket {
  public:

    Bucket() = default;

    Bucket(T* data)
      : data_(data)
    {}

    void allocate() {
      data_ = static_cast<T*>(operator new(kBytesForStorage, std::align_val_t(kCustomAlignment)));
    }

    void setValue(size_t index, const T& value) {
      new (getElementPointer(index)) T(value);
    }
    
    T* getElementPointer(size_t index) const {
      return data_ + index;
    }

    ~Bucket() {
      operator delete(data_, std::align_val_t(kCustomAlignment));
    }

    T* data_ = nullptr;

    static constexpr size_t kBucketSize = 64;
    static constexpr size_t kBytesForStorage = sizeof(T) * kBucketSize;
    static constexpr size_t kCustomAlignment = std::max(static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__), alignof(T));
  };

  static constexpr size_t kIncreaseCoefficient = 3; // greater than one
  size_t size_ = 0;
  size_t numberOfBuckets_ = 0;
  Bucket* data_ = nullptr;
  iterator begin_;
};
