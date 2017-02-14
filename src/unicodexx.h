#pragma once

namespace uxx {

enum category {
  CATEGORY_CN
, CATEGORY_LU, CATEGORY_LL, CATEGORY_LT, CATEGORY_LM, CATEGORY_LO
, CATEGORY_MN, CATEGORY_MC, CATEGORY_ME
, CATEGORY_ND, CATEGORY_NL, CATEGORY_NO
, CATEGORY_PC, CATEGORY_PD, CATEGORY_PS, CATEGORY_PE, CATEGORY_PI, CATEGORY_PF, CATEGORY_PO
, CATEGORY_SM, CATEGORY_SC, CATEGORY_SK, CATEGORY_SO
, CATEGORY_ZS, CATEGORY_ZL, CATEGORY_ZP
, CATEGORY_CC, CATEGORY_CF, CATEGORY_CS, CATEGORY_CO
};

enum bidi_class {
  BIDI_CLASS_L = 1, BIDI_CLASS_LRE, BIDI_CLASS_LRO
, BIDI_CLASS_R, BIDI_CLASS_AL, BIDI_CLASS_RLE, BIDI_CLASS_RLO
, BIDI_CLASS_PDF
, BIDI_CLASS_EN, BIDI_CLASS_ES, BIDI_CLASS_ET
, BIDI_CLASS_AN, BIDI_CLASS_CS
, BIDI_CLASS_NSM
, BIDI_CLASS_BN
, BIDI_CLASS_B, BIDI_CLASS_S
, BIDI_CLASS_WS
, BIDI_CLASS_ON
, BIDI_CLASS_LRI, BIDI_CLASS_RLI, BIDI_CLASS_FSI, BIDI_CLASS_PDI
};

enum bound_class {
  BOUND_CLASS_START
, BOUND_CLASS_OTHER
, BOUND_CLASS_CR, BOUND_CLASS_LF
, BOUND_CLASS_CONTROL
, BOUND_CLASS_EXTEND
, BOUND_CLASS_L, BOUND_CLASS_V, BOUND_CLASS_T, BOUND_CLASS_LV, BOUND_CLASS_LVT
, BOUND_CLASS_REGIONAL_INDICATOR
, BOUND_CLASS_SPACINGMARK
, BOUND_CLASS_PREPEND
, BOUND_CLASS_ZWJ
, BOUND_CLASS_E_BASE, BOUND_CLASS_E_MODIFIER
, BOUND_CLASS_GAZ, BOUND_CLASS_E_BASE_GAZ  // glue-after-zwj
};

enum decomp_type {
  DECOMP_TYPE_FONT = 1
, DECOMP_TYPE_NOBREAK
, DECOMP_TYPE_INITIAL, DECOMP_TYPE_MEDIAL, DECOMP_TYPE_FINAL
, DECOMP_TYPE_ISOLATED
, DECOMP_TYPE_CIRCLE
, DECOMP_TYPE_SUPER, DECOMP_TYPE_SUB
, DECOMP_TYPE_VERTICAL
, DECOMP_TYPE_WIDE, DECOMP_TYPE_NARROW, DECOMP_TYPE_SMALL
, DECOMP_TYPE_SQUARE
, DECOMP_TYPE_FRACTION
, DECOMP_TYPE_COMPAT
};

enum decomp_flag {
  DECOMP_STRIP_IGNORABLE  = (1 << 1)
, DECOMP_CASEFOLD         = (1 << 2)
, DECOMP_COMPAT           = (1 << 3)
, DECOMP_CANONICALIZE     = (1 << 4)
, DECOMP_STRIP            = (1 << 5)
};

struct flags {
  u8 char_width:2;
  bool bidi_mirrored:1;
  bool comp_exclude:1;
  bool default_ignore:1;
  bool ctrl_boundary:1;
  u8 pad:2;
};

class codepoint {
private:
  uint32_t cp;
public:
  explicit codepoint(uint32_t ch) : cp(ch) {}
  explicit operator uint32_t() { return cp; }
  codepoint &operator=(uint32_t ch) { cp = ch; return *this; }

  friend bool operator==(codepoint a, codepoint w) { return a.cp == w.cp; }
  friend bool operator!=(codepoint a, codepoint w) { return a.cp != w.cp; }
  friend bool operator<(codepoint a, codepoint w) { return a.cp < w.cp; }
  friend bool operator>(codepoint a, codepoint w) { return a.cp > w.cp; }
  friend bool operator<=(codepoint a, codepoint w) { return a.cp <= w.cp; }
  friend bool operator>=(codepoint a, codepoint w) { return a.cp >= w.cp; }

  codepoint &operator+=(uint32_t a) { cp += a; return *this; }
  codepoint &operator-=(uint32_t a) { cp -= a; return *this; }
  friend codepoint operator+(codepoint a, uint32_t w) { return a += w; }
  friend codepoint operator+(uint32_t a, codepoint w) { return w += a; }
  friend codepoint operator-(codepoint a, uint32_t w) { return a -= w; }
  codepoint &operator++() { ++cp; return *this; }
  codepoint operator++(int) { codepoint x(*this); ++cp; return x; }
  codepoint &operator--() { --cp; return *this; }
  codepoint operator--(int) { codepoint x(*this); --cp; return x; }
};

template<typename OutputIter>
OutputIter utf8_encode(codepoint cp, OutputIter dest)
{
  uint32_t c = uint32_t(cp);
  if (c < 0x80) { *dest++ = (uint8_t)c; }
  else if (c < 0x800) { *dest++ = 0xC0 + uint8_t(c>>6); *dest++ = 0x80 + uint8_t(c&0x3F); }
  else if (c < 0x10000)
  {
    *dest++ = 0xE0 + uint8_t(c>>12);
    *dest++ = 0x80 + uint8_t((c>>6)&0x3F);
    *dest++ = 0x80 + uint8_t(c&0x3F);
  }
  else if (c < 0x110000)
  {
    *dest++ = 0xF0 + uint8_t(c>>18);
    *dest++ = 0x80 + uint8_t((c>>12)&0x3F);
    *dest++ = 0x80 + uint8_t((c>>6)&0x3F);
    *dest++ = 0x80 + uint8_t(c&0x3F);
  }
  // else invalid codepoint
  return dest;
}

template<typename InputIter>
class utf8_codepoints_view {
private:
  InputIter first, last;

  class iterator_base<bool is_const> : std::iterator<
    std::input_iterator_tag,
    std::conditional<is_const, const codepoint, codepoint>::type
  > {
  private:
    typedef typename std::conditional<is_const, const InputIter, InputIter>::type wrapped_iter;
    wrapped_iter it;
    codepoint cp;
  public:
    iterator_base(wrapped_iter pos) : it(pos) {}
    iterator_base(const iterator_base<false> &a) : v(a.v) {}

    iterator_base &operator++()
    {
// is valid in a multibyte utf-8 sequence
#define valid_cont(c) (((c) & 0xC0) == 0x80)
#define assert_valid(ex) if (!(ex)) return UTF8_INVALID_UTF8
  uint32_t c = *it; ++it;
  if (c < 0x80)
  {
    cp = c;
  }
  assert_valid(cp - 0xC2 <= 0x32);
  switch ((cp < 0xE0) + (cp < 0xF0))
  {
  case 2: // 2-byte sequence
    assert_valid(str < end && valid_cont(str[0]));
    *out_cp = ((cp & 0x1F) << 6) | (str[0] & 0x3F);
    return 2;
  case 1: // 3-byte sequence
    assert_valid(str + 1 < end && valid_cont(str[0]) && valid_cont(str[1]));
    assert_valid(cp != 0xED || str[0] <= 0x9F);
    cp = ((cp & 0xF) << 12) | ((str[0] & 0x3F) << 6) | (str[1] & 0x3F);
    assert_valid(cp >= 0x800);
    *out_cp = cp;
    return 3;
  default: // 4-byte sequence
    assert_valid(   str + 2 < end && valid_cont(str[0])
                 && valid_cont(str[1]) && valid_cont(str[2]));
    assert_valid(!(cp == 0xF0 && str[0] < 0x90) && !(cp == 0xF4 && str[0] > 0x8F));
    *out_cp = ((cp & 7) << 18) | ((str[0] & 0x3F) << 12)
            | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
    return 4;
  }
}
#undef assert_valid
#undef valid_cont
    }
    iterator_base operator++(int) { iterator_base x(*this); operator++(); return x; }

    friend bool operator==(const iterator_base &a, const iterator_base &w);
    friend bool operator!=(const iterator_base &a, const iterator_base &w) { return !(a == w); }
  }
public:
  utf8_codepoints_view(InputIter first_, InputIter last_) : first(first_), last(last_) {}
  template<typename Range>
  utf8_codepoints_view(Range r) : first(std::begin(r)), last(std::end(r)) {}

  typedef iterator_base<false> iterator;
  typedef iterator_base<true> const_iterator;

  iterator begin() { return iterator(first); }
  iterator end() { return iterator(last); }
  const_iterator cbegin() const { return const_iterator(first); }
  const_iterator cend() const { return const_iterator(last); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
};

ssz utf8_iterate_cp(usz, const u8 * restrict, unicp OUT);

typedef struct {
  unibound_class bc_state;
} utf8_g_iterator;

void utf8_g_iterator_init(utf8_g_iterator OUT);
ssz utf8_iterate_g(utf8_g_iterator INOUT, usz, const u8 * restrict, unicp OUT, bool OUT);

ssz unidecomposed_sz(unicp, unidecomp_flag);
ssz unidecompose(unicp, usz, unicp OUT, unidecomp_flag);

unicp uniupcase(unicp);
unicp unidowncase(unicp);
unicp unititlecase(unicp);

unicategory unigetcategory(unicp);
unibidi_class unigetbidi_class(unicp);
unibound_class unigetbound_class(unicp);
unidecomp_type unigetdecomp_type(unicp);
uniflags unigetflags(unicp);

}
