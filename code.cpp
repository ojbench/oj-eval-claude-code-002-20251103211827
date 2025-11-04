#include <complex>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

namespace sjtu {
class int2048 {
private:
  static const int BASE = 10000;      // 1e4 per digit
  static const int BASE_DIGS = 4;     // digits per BASE
  std::vector<int> a;                 // little-endian digits
  bool neg = false;                   // sign flag (true if negative and not zero)

  // helpers
  void trim();
  bool is_zero() const;
  int abs_compare(const int2048 &b) const; // -1,0,1 comparing |*this| vs |b|

  static int2048 add_abs(const int2048 &x, const int2048 &y);
  static int2048 sub_abs(const int2048 &x, const int2048 &y); // assumes |x|>=|y|

  static int2048 mul_simple(const int2048 &x, const int2048 &y);
  static int2048 mul_fft(const int2048 &x, const int2048 &y);
  static int2048 mul_by_int(const int2048 &x, int m);

  static void divmod_abs(const int2048 &u, const int2048 &v, int2048 &q, int2048 &r);

public:
  // Constructors
  int2048();
  int2048(long long);
  int2048(const std::string &);
  int2048(const int2048 &);

  // Integer1
  void read(const std::string &);
  void print();
  int2048 &add(const int2048 &);
  friend int2048 add(int2048, const int2048 &);
  int2048 &minus(const int2048 &);
  friend int2048 minus(int2048, const int2048 &);

  // Integer2
  int2048 operator+() const;
  int2048 operator-() const;
  int2048 &operator=(const int2048 &);
  int2048 &operator+=(const int2048 &);
  friend int2048 operator+(int2048, const int2048 &);
  int2048 &operator-=(const int2048 &);
  friend int2048 operator-(int2048, const int2048 &);
  int2048 &operator*=(const int2048 &);
  friend int2048 operator*(int2048, const int2048 &);
  int2048 &operator/=(const int2048 &);
  friend int2048 operator/(int2048, const int2048 &);
  int2048 &operator%=(const int2048 &);
  friend int2048 operator%(int2048, const int2048 &);
  friend std::istream &operator>>(std::istream &, int2048 &);
  friend std::ostream &operator<<(std::ostream &, const int2048 &);
  friend bool operator==(const int2048 &, const int2048 &);
  friend bool operator!=(const int2048 &, const int2048 &);
  friend bool operator<(const int2048 &, const int2048 &);
  friend bool operator>(const int2048 &, const int2048 &);
  friend bool operator<=(const int2048 &, const int2048 &);
  friend bool operator>=(const int2048 &, const int2048 &);
};

// ===== helpers =====
void int2048::trim() {
  while (!a.empty() && a.back() == 0) a.pop_back();
  if (a.empty()) neg = false; // zero is non-negative
}

bool int2048::is_zero() const { return a.empty(); }

int int2048::abs_compare(const int2048 &b) const {
  if (a.size() != b.a.size()) return a.size() < b.a.size() ? -1 : 1;
  for (int i = (int)a.size() - 1; i >= 0; --i) {
    if (a[i] != b.a[i]) return a[i] < b.a[i] ? -1 : 1;
  }
  return 0;
}

// ===== constructors =====
int2048::int2048() { neg = false; }

int2048::int2048(long long v) {
  neg = v < 0; unsigned long long x = neg ? (unsigned long long)(-v) : (unsigned long long)v;
  while (x) { a.push_back((int)(x % BASE)); x /= BASE; }
  trim();
}

int2048::int2048(const std::string &s) { read(s); }

int2048::int2048(const int2048 &o) { a = o.a; neg = o.neg; }

// ===== basic ops =====
void int2048::read(const std::string &s) {
  a.clear(); neg = false;
  int i = 0, n = (int)s.size();
  // skip leading spaces
  while (i < n && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t' || s[i] == '\v' || s[i] == '\f')) ++i;
  if (i < n && (s[i] == '+' || s[i] == '-')) { neg = (s[i] == '-'); ++i; }
  // move to last digit
  int end = n - 1;
  while (end >= i && !(s[end] >= '0' && s[end] <= '9')) --end;
  if (end < i) { a.clear(); neg = false; return; }
  for (int p = end; p >= i; p -= BASE_DIGS) {
    int val = 0;
    int l = p - (BASE_DIGS - 1);
    if (l < i) l = i;
    for (int t = l; t <= p; ++t) {
      if (s[t] >= '0' && s[t] <= '9') val = val * 10 + (s[t] - '0');
    }
    a.push_back(val);
  }
  trim(); if (is_zero()) neg = false;
}

void int2048::print() {
  if (is_zero()) { std::cout << 0; return; }
  if (neg) std::cout << '-';
  int i = (int)a.size() - 1;
  std::cout << a[i];
  for (--i; i >= 0; --i) {
    int v = a[i];
    // print exactly BASE_DIGS digits with leading zeros (BASE=10000 -> 4 digits)
    std::cout << (v / 1000);
    std::cout << ((v / 100) % 10);
    std::cout << ((v / 10) % 10);
    std::cout << (v % 10);
  }
}

// ===== absolute add/sub =====
int2048 int2048::add_abs(const int2048 &x, const int2048 &y) {
  int2048 r; r.a.resize(std::max(x.a.size(), y.a.size()));
  long long carry = 0;
  for (size_t i = 0; i < r.a.size(); ++i) {
    long long cur = carry;
    if (i < x.a.size()) cur += x.a[i];
    if (i < y.a.size()) cur += y.a[i];
    r.a[i] = (int)(cur % BASE);
    carry = cur / BASE;
  }
  if (carry) r.a.push_back((int)carry);
  r.neg = false; return r;
}

int2048 int2048::sub_abs(const int2048 &x, const int2048 &y) {
  // assumes |x| >= |y|
  int2048 r; r.a.resize(x.a.size());
  long long carry = 0;
  for (size_t i = 0; i < x.a.size(); ++i) {
    long long cur = x.a[i] - carry - (i < y.a.size() ? y.a[i] : 0);
    if (cur < 0) { cur += BASE; carry = 1; } else carry = 0;
    r.a[i] = (int)cur;
  }
  r.trim(); r.neg = false; return r;
}

// ===== operators for Integer1 =====
int2048 &int2048::add(const int2048 &b) {
  bool sx = this->neg; bool sb = b.neg;
  if (sx == sb) {
    int2048 r = add_abs(*this, b);
    r.neg = sx && !r.is_zero();
    *this = r;
  } else {
    int cmp = abs_compare(b);
    if (cmp == 0) { a.clear(); neg = false; }
    else if (cmp > 0) { int2048 r = sub_abs(*this, b); r.neg = sx; *this = r; }
    else { int2048 r = sub_abs(b, *this); r.neg = sb; *this = r; }
  }
  return *this;
}

int2048 add(int2048 a, const int2048 &b) { a.add(b); return a; }

int2048 &int2048::minus(const int2048 &b) {
  bool sx = this->neg; bool sb = b.neg;
  if (sx != sb) {
    int2048 r = add_abs(*this, b);
    r.neg = sx && !r.is_zero();
    *this = r;
  } else {
    int cmp = abs_compare(b);
    if (cmp == 0) { a.clear(); neg = false; }
    else if (cmp > 0) { int2048 r = sub_abs(*this, b); r.neg = sx; *this = r; }
    else { int2048 r = sub_abs(b, *this); r.neg = !sb; *this = r; }
  }
  return *this;
}

int2048 minus(int2048 a, const int2048 &b) { a.minus(b); return a; }

// ===== multiplication =====
int2048 int2048::mul_simple(const int2048 &x, const int2048 &y) {
  int2048 r;
  if (x.is_zero() || y.is_zero()) return r;
  r.a.assign(x.a.size() + y.a.size(), 0);
  for (size_t i = 0; i < x.a.size(); ++i) {
    long long carry = 0;
    for (size_t j = 0; j < y.a.size() || carry; ++j) {
      long long cur = r.a[i + j] + carry + 1LL * x.a[i] * (j < y.a.size() ? y.a[j] : 0);
      r.a[i + j] = (int)(cur % BASE);
      carry = cur / BASE;
    }
  }
  r.trim(); r.neg = false;
  return r;
}

int2048 int2048::mul_fft(const int2048 &x, const int2048 &y) {
  // For now, fallback to schoolbook. Can be optimized later.
  return mul_simple(x, y);
}

int2048 int2048::mul_by_int(const int2048 &x, int m) {
  int2048 r; if (x.is_zero() || m == 0) return r;
  r.a.resize(x.a.size());
  long long carry = 0;
  for (size_t i = 0; i < x.a.size(); ++i) {
    long long cur = 1LL * x.a[i] * m + carry;
    r.a[i] = (int)(cur % BASE);
    carry = cur / BASE;
  }
  if (carry) r.a.push_back((int)carry);
  r.neg = false; return r;
}

// ===== division (absolute) =====
void int2048::divmod_abs(const int2048 &u, const int2048 &v, int2048 &q, int2048 &r) {
  q = int2048(0); r = int2048(0);
  if (v.is_zero()) return; // undefined, guarded by tests
  if (u.abs_compare(v) < 0) { r = u; return; }

  // shift v so its size matches u (like long division by base blocks)
  int n = (int)u.a.size();
  int m = (int)v.a.size();
  int k = n - m;

  int2048 vshift = v;
  if (k > 0) vshift.a.insert(vshift.a.begin(), k, 0);

  int2048 rem = u;
  q.a.assign(k + 1, 0);

  for (int pos = k; pos >= 0; --pos) {
    // binary search digit in [0, BASE-1]
    int low = 0, high = BASE - 1, best = 0;
    while (low <= high) {
      int mid = (low + high) >> 1;
      int2048 t = mul_by_int(vshift, mid);
      int cmp = t.abs_compare(rem);
      if (cmp <= 0) { best = mid; low = mid + 1; }
      else high = mid - 1;
    }
    if (best) {
      int2048 t = mul_by_int(vshift, best);
      rem = sub_abs(rem, t);
    }
    q.a[pos] = best;
    // shift vshift down by one base block
    if (!vshift.a.empty()) vshift.a.erase(vshift.a.begin());
    vshift.trim();
  }
  q.trim(); r = rem; r.neg = false;
}

// ===== operators (Integer2) =====
int2048 int2048::operator+() const { return *this; }
int2048 int2048::operator-() const {
  int2048 r(*this); if (!r.is_zero()) r.neg = !r.neg; return r;
}

int2048 &int2048::operator=(const int2048 &o) { a = o.a; neg = o.neg; return *this; }

int2048 &int2048::operator+=(const int2048 &b) { return add(b); }
int2048 operator+(int2048 a, const int2048 &b) { return add(a, b); }

int2048 &int2048::operator-=(const int2048 &b) { return minus(b); }
int2048 operator-(int2048 a, const int2048 &b) { return minus(a, b); }

int2048 &int2048::operator*=(const int2048 &b) {
  bool sign = (neg != b.neg);
  int2048 x = *this; x.neg = false;
  int2048 y = b; y.neg = false;
  int2048 r = mul_fft(x, y);
  r.neg = sign && !r.is_zero();
  *this = r; return *this;
}
int2048 operator*(int2048 a, const int2048 &b) { a *= b; return a; }

int2048 &int2048::operator/=(const int2048 &b) {
  bool neg_res = (neg != b.neg);
  int2048 qa, ra;
  int2048 A = *this; A.neg = false; int2048 B = b; B.neg = false;
  divmod_abs(A, B, qa, ra);
  if (neg_res && !ra.is_zero()) {
    // floor towards -inf: if negative and has remainder, subtract 1
    qa = add_abs(qa, int2048(1));
    qa.neg = true;
  } else {
    qa.neg = neg_res && !qa.is_zero();
  }
  *this = qa; return *this;
}
int2048 operator/(int2048 a, const int2048 &b) { a /= b; return a; }

int2048 &int2048::operator%=(const int2048 &b) {
  // r = x - (x / y) * y (with floor division)
  int2048 q = (*this) / b;
  int2048 prod = q * b;
  // compute *this - prod
  if (this->neg != prod.neg) {
    *this = add_abs(*this, prod);
    this->neg = this->neg;
  } else {
    int cmp = this->abs_compare(prod);
    if (cmp == 0) { this->a.clear(); this->neg = false; }
    else if (cmp > 0) { *this = sub_abs(*this, prod); this->neg = this->neg; }
    else { *this = sub_abs(prod, *this); this->neg = !prod.neg; }
  }
  return *this;
}
int2048 operator%(int2048 a, const int2048 &b) { a %= b; return a; }

std::istream &operator>>(std::istream &is, int2048 &x) {
  std::string s; is >> s; x.read(s); return is;
}
std::ostream &operator<<(std::ostream &os, const int2048 &x) {
  if (x.is_zero()) { os << 0; return os; }
  if (x.neg) os << '-';
  int i = (int)x.a.size() - 1;
  os << x.a[i];
  for (--i; i >= 0; --i) {
    int v = x.a[i];
    char buf[5]; buf[4] = '\0';
    for (int k = 3; k >= 0; --k) { buf[k] = char('0' + (v % 10)); v /= 10; }
    os << buf;
  }
  return os;
}

bool operator==(const int2048 &x, const int2048 &y) {
  return x.neg == y.neg && x.a == y.a;
}
bool operator!=(const int2048 &x, const int2048 &y) { return !(x == y); }

bool operator<(const int2048 &x, const int2048 &y) {
  if (x.neg != y.neg) return x.neg && !x.is_zero();
  int cmp = x.abs_compare(y);
  return x.neg ? (cmp > 0) : (cmp < 0);
}
bool operator>(const int2048 &x, const int2048 &y) { return y < x; }
bool operator<=(const int2048 &x, const int2048 &y) { return !(y < x); }
bool operator>=(const int2048 &x, const int2048 &y) { return !(x < y); }

} // namespace sjtu
