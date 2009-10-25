// [Fog/Core Library - C++ API]
//
// [Licence]
// MIT, See COPYING file in package

// [Precompiled Headers]
#if defined(FOG_PRECOMP)
#include FOG_PRECOMP
#endif // FOG_PRECOMP

// [Dependencies]
#include <Fog/Core/Algorithms.h>
#include <Fog/Core/Assert.h>
#include <Fog/Core/Byte.h>
#include <Fog/Core/ByteArray.h>
#include <Fog/Core/Error.h>
#include <Fog/Core/HashUtil.h>
#include <Fog/Core/Locale.h>
#include <Fog/Core/Memory.h>
#include <Fog/Core/Sequence.h>
#include <Fog/Core/String.h>
#include <Fog/Core/StringFilter.h>
#include <Fog/Core/StringMatcher.h>
#include <Fog/Core/StringUtil.h>
#include <Fog/Core/TextCodec.h>
#include <Fog/Core/Vector.h>

#include <stdarg.h>

namespace Fog {

// ============================================================================
// [Fog::String - Helpers]
// ============================================================================

static FOG_INLINE bool fitToRange(
  const String& s, sysuint_t* _start, sysuint_t* _len, const Range& range)
{
  sysuint_t start = range.index;
  sysuint_t slen = s.getLength();

  if (start >= slen) return false;

  sysuint_t r = slen - start;
  if (r > range.length) r = range.length;

  *_start = start;
  *_len = r;
  return true;
}

// ============================================================================
// [Fog::String - Construction / Destruction]
// ============================================================================

String::String()
{
  _d = sharedNull->refAlways();
}

String::String(Char ch, sysuint_t length)
{
  _d = Data::alloc(length);
  if (!_d) { _d = sharedNull->refAlways(); return; }
  if (!length) return;

  StringUtil::fill(_d->data, ch, length);
  _d->length = length;
  _d->data[length] = 0;
}

String::String(const String& other)
{
  _d = other._d->ref();
}

String::String(const String& other1, const String& other2)
{
  if (other1.getLength() == 0)
  {
    _d = other2._d->ref();
  }
  else
  {
    _d = other1._d->ref();
    append(other2);
  }
}

String::String(const Char* str)
{
  sysuint_t length = StringUtil::len(str);

  _d = Data::alloc(0, str, length);
  if (!_d) _d = sharedNull->refAlways();
}

String::String(const Char* str, sysuint_t length)
{
  if (length == DetectLength) length = StringUtil::len(str);

  _d = Data::alloc(0, str, length);
  if (!_d) _d = sharedNull->refAlways();
}

String::String(const Ascii8& str)
{
  const char* s = str.getData();
  sysuint_t length = (str.getLength() == DetectLength)
    ? StringUtil::len(s) : str.getLength();

  _d = Data::alloc(0, s, length);
  if (!_d) _d = sharedNull->refAlways();
}

String::String(const Utf16& str)
{
  _d = sharedNull->refAlways();
  set(str);
}

String::~String()
{
  _d->derefInline();
}

// ============================================================================
// [Fog::String - Implicit Sharing]
// ============================================================================

err_t String::_detach()
{
  if (isDetached()) return Error::Ok;

  Data* newd = Data::copy(_d);
  if (!newd) return Error::OutOfMemory;

  AtomicBase::ptr_setXchg(&_d, newd)->deref();
  return Error::Ok;
}

// ============================================================================
// [Fog::String::Flags]
// ============================================================================

void String::setIsSharable(bool val)
{
  if (isSharable() == val) return;

  detach();

  if (val)
    _d->flags |= Data::IsSharable;
  else
    _d->flags &= ~Data::IsSharable;
}

void String::setIsStrong(bool val)
{
  if (isStrong() == val) return;

  detach();

  if (val)
    _d->flags |= Data::IsStrong;
  else
    _d->flags &= ~Data::IsStrong;
}

// ============================================================================
// [Fog::String::Data]
// ============================================================================

err_t String::prepare(sysuint_t capacity)
{
  Data* d = _d;

  if (d->refCount.get() == 1 && d->capacity >= capacity)
  {
    d->hashCode = 0;
    d->length = 0;
    d->data[0] = 0;
    return Error::Ok;
  }

  d = Data::alloc(capacity);
  if (!d) return Error::OutOfMemory;

  AtomicBase::ptr_setXchg(&_d, d)->deref();
  return Error::Ok;
}

err_t String::reserve(sysuint_t to)
{
  if (to < _d->length) to = _d->length;
  if (_d->refCount.get() == 1 && _d->capacity >= to) goto done;

  if (_d->refCount.get() > 1)
  {
    Data* newd = Data::alloc(to, _d->data, _d->length);
    if (!newd) return Error::OutOfMemory;

    AtomicBase::ptr_setXchg(&_d, newd)->deref();
  }
  else
  {
    Data* newd = Data::realloc(_d, to);
    if (!newd) return Error::OutOfMemory;

    _d = newd;
  }
done:
  return Error::Ok;
}

err_t String::resize(sysuint_t to)
{
  if (_d->refCount.get() == 1 && _d->capacity >= to) goto done;

  if (_d->refCount.get() > 1)
  {
    Data* newd = Data::alloc(to, _d->data, to < _d->length ? to : _d->length);
    if (!newd) return Error::OutOfMemory;

    AtomicBase::ptr_setXchg(&_d, newd)->deref();
  }
  else
  {
    Data* newd = Data::realloc(_d, to);
    if (!newd) return Error::OutOfMemory;

    _d = newd;
  }

done:
  _d->hashCode = 0;
  _d->length = to;
  _d->data[to] = 0;
  return Error::Ok;
}

err_t String::grow(sysuint_t by)
{
  sysuint_t lengthBefore = _d->length;
  sysuint_t lengthAfter = lengthBefore + by;

  FOG_ASSERT(lengthBefore <= lengthAfter);

  if (_d->refCount.get() == 1 && _d->capacity >= lengthAfter) goto done;

  if (_d->refCount.get() > 1)
  {
    sysuint_t optimalCapacity = Std::calcOptimalCapacity(
      sizeof(String::Data), sizeof(Char), lengthBefore, lengthAfter);

    Data* newd = Data::alloc(optimalCapacity, _d->data, _d->length);
    if (!newd) return Error::OutOfMemory;

    AtomicBase::ptr_setXchg(&_d, newd)->deref();
  }
  else
  {
    sysuint_t optimalCapacity = Std::calcOptimalCapacity(
      sizeof(String::Data), sizeof(Char), lengthBefore, lengthAfter);

    Data* newd = Data::realloc(_d, optimalCapacity);
    if (!newd) return Error::OutOfMemory;

    _d = newd;
  }

done:
  _d->hashCode = 0;
  _d->length = lengthAfter;
  _d->data[lengthAfter] = 0;
  return Error::Ok;
}

void String::squeeze()
{
  sysuint_t i = _d->length;
  sysuint_t c = _d->capacity;

  // Pad to 16 bytes
  i = (i + 7) & ~7;

  if (i < c)
  {
    Data* newd = Data::alloc(0, _d->data, _d->length);
    if (!newd) return;
    AtomicBase::ptr_setXchg(&_d, newd)->deref();
  }
}

void String::clear()
{
  if (_d->refCount.get() > 1)
  {
    AtomicBase::ptr_setXchg(&_d, sharedNull->refAlways())->deref();
    return;
  }

  _d->hashCode = 0;
  _d->length = 0;
  _d->data[0] = 0;
}

void String::free()
{
  AtomicBase::ptr_setXchg(&_d, sharedNull->refAlways())->deref();
}

Char* String::mData()
{
  detach();
  _d->hashCode = 0;
  return _d->data;
}

static Char* _prepareSet(String* self, sysuint_t length)
{
  String::Data* d = self->_d;
  if (FOG_UNLIKELY(length == 0)) goto skip;

  if (d->refCount.get() > 1)
  {
    d = String::Data::alloc(length);
    if (!d) return NULL;
    AtomicBase::ptr_setXchg(&self->_d, d)->derefInline();
  }
  else if (d->capacity < length)
  {
    sysuint_t optimalCapacity = Std::calcOptimalCapacity(
      sizeof(String::Data), sizeof(Char), d->length, length);

    d = String::Data::realloc(d, optimalCapacity);
    if (!d) return NULL;
    self->_d = d;
  }

  d->hashCode = 0;
  d->length = length;
  d->data[d->length] = 0;
skip:
  return d->data;
}

static Char* _prepareAppend(String* self, sysuint_t length)
{
  String::Data* d = self->_d;

  sysuint_t lengthBefore = d->length;
  sysuint_t lengthAfter = lengthBefore + length;

  if (FOG_UNLIKELY(length == 0)) goto skip;

  if (d->refCount.get() > 1)
  {
    d = String::Data::alloc(lengthAfter, d->data, d->length);
    if (!d) return NULL;
    AtomicBase::ptr_setXchg(&self->_d, d)->derefInline();
  }
  else if (d->capacity < lengthAfter)
  {
    sysuint_t optimalCapacity = Std::calcOptimalCapacity(
      sizeof(String::Data), sizeof(Char), lengthBefore, lengthAfter);

    d = String::Data::realloc(d, optimalCapacity);
    if (!d) return NULL;
    self->_d = d;
  }

  d->hashCode = 0;
  d->length = lengthAfter;
  d->data[lengthAfter] = 0;
skip:
  return d->data + lengthBefore;
}

static Char* _prepareInsert(String* self, sysuint_t index, sysuint_t length)
{
  String::Data* d = self->_d;

  sysuint_t lengthBefore = d->length;
  sysuint_t lengthAfter = lengthBefore + length;

  if (index > lengthBefore) index = lengthBefore;
  if (FOG_UNLIKELY(!length)) goto skip;

  if (d->refCount.get() > 1 || d->capacity < lengthAfter)
  {
    sysuint_t optimalCapacity = Std::calcOptimalCapacity(
      sizeof(String::Data), sizeof(Char), lengthBefore, lengthAfter);

    d = String::Data::alloc(optimalCapacity, d->data, index);
    if (!d) return NULL;

    StringUtil::copy(
      d->data + index + length, self->_d->data + index, lengthBefore - index);
    AtomicBase::ptr_setXchg(&self->_d, d)->derefInline();
  }

  d->hashCode = 0;
  d->length = lengthAfter;
  d->data[lengthAfter] = 0;
skip:
  return d->data + index;
}

static Char* _prepareReplace(String* self, sysuint_t index, sysuint_t range, sysuint_t replacementLength)
{
  String::Data* d = self->_d;

  sysuint_t lengthBefore = d->length;

  FOG_ASSERT(index <= lengthBefore);
  if (lengthBefore - index > range) range = lengthBefore - index;

  sysuint_t lengthAfter = lengthBefore - range + replacementLength;

  if (d->refCount.get() > 1 || d->capacity < lengthAfter)
  {
    sysuint_t optimalCapacity = Std::calcOptimalCapacity(
      sizeof(String::Data), sizeof(Char), lengthBefore, lengthAfter);

    d = String::Data::alloc(optimalCapacity, d->data, index);
    if (!d) return NULL;

    StringUtil::copy(
      d->data + index + replacementLength,
      self->_d->data + index + range, lengthBefore - index - range);
    AtomicBase::ptr_setXchg(&self->_d, d)->derefInline();
  }
  else
  {
    StringUtil::move(
      d->data + index + replacementLength,
      d->data + index + range, lengthBefore - index - range);
  }

  d->hashCode = 0;
  d->length = lengthAfter;
  d->data[lengthAfter] = 0;
  return d->data + index;
}

// ============================================================================
// [Fog::String::Set]
// ============================================================================

err_t String::set(Char ch, sysuint_t length)
{
  if (length == DetectLength) return Error::InvalidArgument;

  Char* p = _prepareSet(this, length);
  if (!p) return Error::OutOfMemory;

  StringUtil::fill(p, ch, length);
  return Error::Ok;
}

err_t String::set(const Ascii8& str)
{
  const char* s = str.getData();
  sysuint_t length = (str.getLength() == DetectLength)
    ? StringUtil::len(s) : str.getLength();

  Char* p = _prepareSet(this, length);
  if (!p) return Error::OutOfMemory;

  StringUtil::copy(p, s, length);
  return Error::Ok;
}

err_t String::set(const Utf16& str)
{
  const Char* s = str.getData();
  sysuint_t length = (str.getLength() == DetectLength)
    ? StringUtil::len(s) : str.getLength();

  Char* p = _prepareSet(this, length);
  if (!p) return Error::OutOfMemory;

  StringUtil::copy(p, s, length);
  return Error::Ok;
}

err_t String::set(const String& other)
{
  Data* self_d = _d;
  Data* other_d = other._d;
  if (self_d == other_d) return Error::Ok;

  if ((self_d->flags & Data::IsStrong) != 0 || (other_d->flags & Data::IsSharable) == 0)
  {
    return setDeep(other);
  }
  else
  {
    AtomicBase::ptr_setXchg(&_d, other_d->refAlways())->derefInline();
    return Error::Ok;
  }
}

err_t String::set(const void* str, sysuint_t size, const TextCodec& tc)
{
  return tc.toUnicode(*this, Str8((const char*)str, size));
}

err_t String::setUtf8(const char* s, sysuint_t length)
{
  clear();
  return appendUtf8(s, length);
}

err_t String::setUtf32(const uint32_t* s, sysuint_t length)
{
  clear();
  return appendUtf32(s, length);
}

err_t String::setDeep(const String& other)
{
  Data* self_d = _d;
  Data* other_d = other._d;
  if (self_d == other_d) return Error::Ok;

  Char* p = _prepareSet(this, other_d->length);
  if (!p) return Error::OutOfMemory;

  StringUtil::copy(p, other.cData(), other.getLength());
  return Error::Ok;
}

err_t String::setBool(bool b)
{
  return set(Ascii8(b ? "true" : "false"));
}

err_t String::setInt(int32_t n, int base)
{
  clear();
  return appendInt((int64_t)n, base, FormatFlags());
}

err_t String::setInt(uint32_t n, int base)
{
  clear();
  return appendInt((uint64_t)n, base, FormatFlags());
}

err_t String::setInt(int64_t n, int base)
{
  clear();
  return appendInt(n, base, FormatFlags());
}

err_t String::setInt(uint64_t n, int base)
{
  clear();
  return appendInt(n, base, FormatFlags());
}

err_t String::setInt(int32_t n, int base, const FormatFlags& ff, const Locale* locale)
{
  clear();
  return appendInt((int64_t)n, base, ff, locale);
}

err_t String::setInt(uint32_t n, int base, const FormatFlags& ff, const Locale* locale)
{
  clear();
  return appendInt((uint64_t)n, base, ff, locale);
}

err_t String::setInt(int64_t n, int base, const FormatFlags& ff, const Locale* locale)
{
  clear();
  return appendInt(n, base, ff, locale);
}

err_t String::setInt(uint64_t n, int base, const FormatFlags& ff, const Locale* locale)
{
  clear();
  return appendInt(n, base, ff, locale);
}

err_t String::setDouble(double d, int doubleForm)
{
  clear();
  return appendDouble(d, doubleForm, FormatFlags());
}

err_t String::setDouble(double d, int doubleForm, const FormatFlags& ff, const Locale* locale)
{
  clear();
  return appendDouble(d, doubleForm, ff, locale);
}

err_t String::format(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  clear();
  err_t err = appendVformat(fmt, ap);

  va_end(ap);
  return err;
}

err_t String::vformat(const char* fmt, va_list ap)
{
  clear();
  return appendVformat(fmt, ap);
}

err_t String::wformat(const String& fmt, Char lex, const Sequence<String>& args)
{
  clear();
  return appendWformat(fmt, lex, args.cData(), args.getLength());
}

err_t String::wformat(const String& fmt, Char lex, const String* args, sysuint_t length)
{
  clear();
  return appendWformat(fmt, lex, args, length);
}

// ============================================================================
// [Fog::String::Append]
// ============================================================================

static err_t append_ntoa(String* self, uint64_t n, int base, const FormatFlags& ff, const Locale* locale, StringUtil::NTOAOut* out)
{
  const Locale& l = locale ? *locale : Locale::posix();

  Char prefixBuffer[4];
  Char* prefix = prefixBuffer;

  sysuint_t width = ff.width;
  sysuint_t precision = ff.precision;
  uint32_t fmt = ff.flags;

  if (out->negative)
    *prefix++ = l.getMinus();
  else if (fmt & FormatFlags::ShowSign)
    *prefix++ = l.getPlus();
  else if (fmt & FormatFlags::BlankPositive)
    *prefix++ = l.getSpace();

  if (fmt & FormatFlags::Alternate)
  {
    if (base == 8)
    {
      if (n != 0) { out->result--; *(out->result) = '0'; out->length++; }
    }
    else if (base == 16)
    {
      *prefix++ = '0';
      *prefix++ = (fmt & FormatFlags::CapitalizeEOrX) ? 'X' : 'x';
    }
  }

  sysuint_t prefixLength = (sysuint_t)(prefix - prefixBuffer);
  sysuint_t resultLength = out->length;

  if (width == FormatFlags::NoWidth) width = 0;
  if ((fmt & FormatFlags::ZeroPadded) &&
      precision == FormatFlags::NoPrecision &&
      width > prefixLength + resultLength) precision = width - prefixLength;
  if (precision == FormatFlags::NoPrecision) precision = 0;

  sysuint_t fillLength = (resultLength < precision) ? precision - resultLength : 0;
  sysuint_t fullLength = prefixLength + resultLength + fillLength;
  sysuint_t widthLength = (fullLength < width) ? width - fullLength : 0;

  fullLength += widthLength;

  Char* p = _prepareAppend(self, fullLength);
  if (!p) return Error::OutOfMemory;

  // Left justification
  if (!(fmt & FormatFlags::LeftAdjusted))
  {
    StringUtil::fill(p, Char(' '), widthLength); p += widthLength;
  }

  // Number with prefix and precision
  StringUtil::copy(p, prefixBuffer, prefixLength); p += prefixLength;

  // Body
  if (base == 10 && l.getZero() != Char('0'))
  {
    StringUtil::fill(p, l.getZero(), fillLength); p += fillLength;

    for (sysuint_t i = 0; i != resultLength; i++)
      p[i] = l.getZero() + Char((uint8_t)out->result[i] - (uint8_t)'0');
    p += resultLength;
  }
  else
  {
    StringUtil::fill(p, Char('0'), fillLength); p += fillLength;
    StringUtil::copy(p, out->result, resultLength); p += resultLength;
  }

  // Right justification
  if (fmt & FormatFlags::LeftAdjusted)
  {
    StringUtil::fill(p, Char(' '), widthLength);
  }
}

static Char* append_exponent(Char* dest, uint exp, Char zero)
{
  uint t;

  if (exp > 99) { t = exp / 100; *dest++ = zero + Char(t); exp -= t * 100; }
  t = exp / 10; *dest++ = zero + Char(t); exp -= t * 10;
  *dest++ = zero + Char(exp);

  return dest;
}

err_t String::append(Char ch, sysuint_t length)
{
  if (length == DetectLength) return Error::InvalidArgument;

  Char* p = _prepareAppend(this, length);
  if (!p) return Error::OutOfMemory;

  StringUtil::fill(p, ch, length);
  return Error::Ok;
}

err_t String::append(const Ascii8& str)
{
  const char* s = str.getData();
  sysuint_t length = (str.getLength() == DetectLength)
    ? StringUtil::len(s) : str.getLength();

  Char* p = _prepareAppend(this, length);
  if (!p) return Error::OutOfMemory;

  StringUtil::copy(p, s, length);
  return Error::Ok;
}

err_t String::append(const Utf16& str)
{
  const Char* s = str.getData();
  sysuint_t length = (str.getLength() == DetectLength)
    ? StringUtil::len(s) : str.getLength();

  Char* p = _prepareAppend(this, length);
  if (!p) return Error::OutOfMemory;

  StringUtil::copy(p, s, length);
  return Error::Ok;
}

err_t String::append(const String& _other)
{
  if (getLength() == 0) return set(_other);

  String other(_other);

  Char* p = _prepareAppend(this, other.getLength());
  if (!p) return Error::OutOfMemory;

  StringUtil::copy(p, other.cData(), other.getLength());
  return Error::Ok;
}

err_t String::append(const void* str, sysuint_t size, const TextCodec& tc)
{
  return tc.appendToUnicode(*this, str, size);
}

err_t String::appendUtf8(const char* str, sysuint_t length)
{
  if (length == DetectLength) length = StringUtil::len(str);
  if (length == 0) return Error::Ok;
  
  sysuint_t i = length;

  err_t err;
  if ((err = reserve(getLength() + i))) return err;

  Char* dstCur = _d->data + _d->length;

  while (i)
  {
    uint32_t uc = *str;
    uint32_t utf8Size = utf8LengthTable[uc];

    if (i < utf8Size)
    {
      err = Error::InputTruncated;
      goto end;
    }

    switch (utf8Size)
    {
      // Invalid UTF-8 Sequence.
      case 0:
        err = Error::InvalidUtf8Sequence;
        goto end;
      case 1:
        break;
      case 2:
        uc = ((uc - 192U) << 6U)
           | (uint32_t(str[1]) - 128U);
        break;
      case 3:
        // Remove UTF8-BOM (EFBBBF) - We don't want it.
        if (uc == 0xEF && str[1] == 0xBB && str[2] == 0xBF) goto cont;

        uc = ((uc - 224U) << 12U)
           | ((uint32_t(str[1]) - 128U) << 6)
           |  (uint32_t(str[2]) - 128U);
        break;
      case 4:
        uc = ((uc - 240U) << 24U)
           | ((uint32_t(str[1]) - 128U) << 12)
           | ((uint32_t(str[2]) - 128U) << 6)
           |  (uint32_t(str[3]) - 128U);
        break;
      default:
        err = Error::InvalidUtf8Sequence;
        goto end;
    }

    if (uc >= 0x10000U && uc <= MaxCodePoint)
    {
      Char::toSurrogatePair(uc, &dstCur[0]._ch, &dstCur[1]._ch);
      dstCur += 2;
    }
    else if (Char::isSurrogatePair(uc) && uc >= 0xFFFE)
    {
      err = Error::InvalidUnicodeCharacter;
      break;
    }
    else
    {
      *dstCur++ = (uint16_t)uc;
    }

cont:
    str += utf8Size;
    i -= utf8Size;
  }
end:
  xFinalize(dstCur);
  return err;
}

err_t String::appendUtf32(const uint32_t* str, sysuint_t length)
{
  if (length == DetectLength) length = StringUtil::len(str);
  if (length == 0) return Error::Ok;

  sysuint_t i = length;
  err_t err;

  Char* dstCur;
  Char* dstEnd;

reallocBuffer:
  if ((err = reserve(getLength() + i + (i >> 2) + 4))) return err;

  dstCur = _d->data + _d->length;
  dstEnd = _d->data + _d->capacity - 1;

  while (i)
  {
    uint32_t uc = *str;

    if (uc >= 0x10000U && uc <= MaxCodePoint)
    {
      Char::toSurrogatePair(uc, &dstCur[0]._ch, &dstCur[1]._ch);
      dstCur += 2;
    }
    else if (Char::isSurrogatePair(uc) && uc >= 0xFFFE)
    {
      err = Error::InvalidUnicodeCharacter;
      break;
    }
    else
    {
      *dstCur++ = (uint16_t)uc;
    }

    str++;
    i--;
    if (dstCur >= dstEnd) goto reallocBuffer;
  }

  xFinalize(dstCur);
  return err;
}

err_t String::appendBool(bool b)
{
  return append(Ascii8(b ? "true" : "false"));
}

err_t String::appendInt(int32_t n, int base)
{
  return appendInt((int64_t)n, base, FormatFlags());
}

err_t String::appendInt(uint32_t n, int base)
{
  return appendInt((uint64_t)n, base, FormatFlags());
}

err_t String::appendInt(int64_t n, int base)
{
  return appendInt(n, base, FormatFlags());
}

err_t String::appendInt(uint64_t n, int base)
{
  return appendInt(n, base, FormatFlags());
}

err_t String::appendInt(int32_t n, int base, const FormatFlags& ff, const Locale* locale)
{
  return appendInt((int64_t)n, base, ff, locale);
}

err_t String::appendInt(uint32_t n, int base, const FormatFlags& ff, const Locale* locale)
{
  return appendInt((uint64_t)n, base, ff, locale);
}

err_t String::appendInt(int64_t n, int base, const FormatFlags& ff, const Locale* locale)
{
  StringUtil::NTOAOut out;
  StringUtil::itoa(n, base, (ff.flags & FormatFlags::Capitalize) != 0, &out);

  return append_ntoa(this, (uint64_t)n, base, ff, locale, &out);
}

err_t String::appendInt(uint64_t n, int base, const FormatFlags& ff, const Locale* locale)
{
  StringUtil::NTOAOut out;
  StringUtil::utoa(n, base, (ff.flags & FormatFlags::Capitalize) != 0, &out);

  return append_ntoa(this, n, base, ff, locale, &out);
}

err_t String::appendDouble(double d, int doubleForm)
{
  return appendDouble(d, doubleForm, FormatFlags());
}

// Defined in StringUtil.cpp;
namespace StringUtil { FOG_HIDDEN double _mprec_log10(int dig); }

err_t String::appendDouble(double d, int doubleForm, const FormatFlags& ff, const Locale* locale)
{
  err_t err = Error::Ok;
  const Locale& l = locale ? *locale : Locale::posix();

  StringUtil::NTOAOut out;

  sysuint_t width = ff.width;
  sysuint_t precision = ff.precision;
  uint32_t fmt = ff.flags;

  sysuint_t beginLength = _d->length;
  sysuint_t numberLength;
  sysuint_t i;
  sysuint_t savedPrecision = precision;
  int decpt;

  uint8_t* bufCur;
  uint8_t* bufEnd;

  Char* dest;
  Char sign = Char('\0');
  Char zero = l.getZero() - Char('0');

  if (precision == FormatFlags::NoPrecision) precision = 6;

  if (d < 0.0)
    { sign = l.getMinus(); d = -d; }
  else if (fmt & FormatFlags::ShowSign)
    sign = l.getPlus();
  else if (fmt & FormatFlags::BlankPositive)
    sign = l.getSpace();

  if (sign) append(sign);

  // Decimal form
  if (doubleForm == DF_Decimal)
  {
    StringUtil::dtoa(d, 2, precision, &out);

    decpt = out.decpt;
    if (out.decpt == 9999) goto __InfOrNaN;

    bufCur = reinterpret_cast<uint8_t*>(out.result);
    bufEnd = bufCur + out.length;

    // reserve some space for number
    i = precision + 16;
    if (decpt > 0) i += (sysuint_t)decpt;

    if ( (err = reserve(getLength() + i)) ) return err;
    dest = xData() + getLength();

    while (bufCur != bufEnd && decpt > 0) { *dest++ = zero + Char(*bufCur++); decpt--; }
    // Even if not in buffer
    while (decpt > 0) { *dest++ = zero + Char('0'); decpt--; }

    if ((fmt & FormatFlags::Alternate) != 0 || bufCur != bufEnd)
    {
      if (bufCur == reinterpret_cast<uint8_t*>(out.result)) *dest++ = zero + Char('0');
      *dest++ = l.getDecimalPoint();
      while (decpt < 0 && precision > 0) { *dest++ = zero + Char('0'); decpt++; precision--; }

      // Print rest of stuff
      while (*bufCur && precision > 0) { *dest++ = zero + Char(*bufCur++); precision--; }
      // And trailing zeros
      while (precision > 0) { *dest++ = zero + Char('0'); precision--; }
    }

    xFinalize(dest);
  }
  // Exponential form
  else if (doubleForm == DF_Exponent)
  {
__exponentialForm:
    StringUtil::dtoa(d, 2, precision + 1, &out);

    decpt = out.decpt;
    if (decpt == 9999) goto __InfOrNaN;

    // reserve some space for number, we need +X.{PRECISION}e+123
    if ( (err = reserve(getLength() + precision + 10)) ) return err;
    dest = xData() + getLength();

    bufCur = reinterpret_cast<uint8_t*>(out.result);
    bufEnd = bufCur + out.length;

    *dest++ = zero + Char(*bufCur++);
    if ((fmt & FormatFlags::Alternate) != 0 || precision != 0) *dest++ = l.getDecimalPoint();
    while (bufCur != bufEnd && precision > 0)
    {
      *dest++ = zero + Char(*bufCur++);
      precision--;
    }

    // Add trailing zeroes to fill out to ndigits unless this is
    // DF_SignificantDigits
    if (doubleForm == DF_Exponent)
    {
      for (i = precision; i; i--) *dest++ = zero + Char('0');
    }

    // Add the exponent.
    *dest++ = l.getExponential();
    decpt--;
    if (decpt < 0)
      { *dest++ = l.getMinus(); decpt = -decpt; }
    else
      *dest++ = l.getPlus();

    dest = append_exponent(dest, decpt, zero + Char('0'));

    xFinalize(dest);
  }
  // Significant digits form
  else /* if (doubleForm == DF_SignificantDigits) */
  {
    Char* save;
    if (d <= 0.0001 || d >= StringUtil::_mprec_log10(precision))
    {
      if (precision > 0) precision--;
      goto __exponentialForm;
    }

    if (d < 1.0)
    {
      // what we want is ndigits after the point
      StringUtil::dtoa(d, 3, ++precision, &out);
    }
    else
    {
      StringUtil::dtoa(d, 2, precision, &out);
    }

    decpt = out.decpt;
    if (decpt == 9999) goto __InfOrNaN;

    // reserve some space for number
    i = precision + 16;
    if (decpt > 0) i += (sysuint_t)decpt;

    if ( (err = reserve(getLength() + i)) ) return err;
    dest = save = xData() + getLength();

    bufCur = reinterpret_cast<uint8_t*>(out.result);
    bufEnd = bufCur + out.length;

    while (bufCur != bufEnd && decpt > 0) { *dest++ = zero + Char(*bufCur++); decpt--; precision--; }
    // Even if not in buffer
    while (decpt > 0 && precision > 0) { *dest++ = zero + Char('0'); decpt--; precision--; }

    if ((fmt & FormatFlags::Alternate) != 0 || bufCur != bufEnd)
    {
      if (dest == save) *dest++ = zero + Char('0');
      *dest++ = l.getDecimalPoint();
      while (decpt < 0 && precision > 0) { *dest++ = zero + Char('0'); decpt++; precision--; }

      // Print rest of stuff
      while (bufCur != bufEnd && precision > 0){ *dest++ = zero + Char(*bufCur++); precision--; }
      // And trailing zeros
      while (precision > 0) { *dest++ = zero + Char('0'); precision--; }
    }

    xFinalize(dest);
  }
  goto __ret;

__InfOrNaN:
  err |= append(Ascii8((const char*)out.result, out.length));
__ret:
  // Apply padding
  numberLength = _d->length - beginLength;
  if (width != (sysuint_t)-1 && width > numberLength)
  {
    sysuint_t fill = width - numberLength;

    if ((fmt & FormatFlags::LeftAdjusted) == 0)
    {
      if (savedPrecision == FormatFlags::NoPrecision)
        err |= insert(beginLength + (sign.ch() != 0), zero + Char('0'), fill);
      else
        err |= insert(beginLength, Char(' '), fill);
    }
    else
    {
      err |= append(Char(' '), fill);
    }
  }
  return err;
}

err_t String::appendFormat(const char* fmt, ...)
{
  FOG_ASSERT(fmt);

  clear();

  va_list ap;
  va_start(ap, fmt);
  err_t err = appendVformat(fmt, ap);
  va_end(ap);

  return err;
}

err_t String::appendVformat(const char* fmt, va_list ap)
{
#define __VFORMAT_PARSE_NUMBER(__ptr__, __out__)     \
{                                                    \
  /* ----- Clean-up ----- */                         \
  __out__ = 0;                                       \
                                                     \
  /* ----- Remove zeros ----- */                     \
  while (*__ptr__ == '0') __ptr__++;                 \
                                                     \
  /* ----- Parse number ----- */                     \
  while ((c = (uint8_t)*__ptr__) >= '0' && c <= '9') \
  {                                                  \
    __out__ = 10 * __out__ + (c - '0');              \
    __ptr__++;                                       \
  }                                                  \
}

  if (fmt == NULL) return Error::InvalidArgument;

  const char* fmtBeginChunk = fmt;
  uint32_t c;
  sysuint_t beginLength = getLength();

  for (;;)
  {
    c = (uint8_t)*fmt;

    if (c == '%')
    {
      uint directives = 0;
      uint sizeFlags = 0;
      sysuint_t fieldWidth = FormatFlags::NoWidth;
      sysuint_t precision = FormatFlags::NoPrecision;
      uint base = 10;

      if (fmtBeginChunk != fmt) append(Ascii8(fmtBeginChunk, (sysuint_t)(fmt - fmtBeginChunk)));

      // Parse directives.
      for (;;)
      {
        c = (uint8_t)*(++fmt);

        if      (c == '#')  directives |= FormatFlags::Alternate;
        else if (c == '0')  directives |= FormatFlags::ZeroPadded;
        else if (c == '-')  directives |= FormatFlags::LeftAdjusted;
        else if (c == ' ')  directives |= FormatFlags::BlankPositive;
        else if (c == '+')  directives |= FormatFlags::ShowSign;
        else if (c == '\'') directives |= FormatFlags::ThousandsGroup;
        else break;
      }

      // Parse field width.
      if (Byte::isDigit(c))
      {
        __VFORMAT_PARSE_NUMBER(fmt, fieldWidth)
      }
      else if (c == '*')
      {
        c = *++fmt;

        int _fieldWidth = va_arg(ap, int);
        if (_fieldWidth < 0) _fieldWidth = 0;
        if (_fieldWidth > 4096) _fieldWidth = 4096;
        fieldWidth = (sysuint_t)_fieldWidth;
      }

      // Parse precision.
      if (c == '.')
      {
        c = *++fmt;

        if (Byte::isDigit(c))
        {
          __VFORMAT_PARSE_NUMBER(fmt, precision);
        }
        else if (c == '*')
        {
          c = *++fmt;

          int _precision = va_arg(ap, int);
          if (_precision < 0) _precision = 0;
          if (_precision > 4096) _precision = 4096;
          precision = (sysuint_t)_precision;
        }
      }

      // Parse argument type.
      enum
      {
        Arg_Size_H   = 0x01,
        Arg_Size_HH  = 0x02,
        Arg_Size_L   = 0x04,
        Arg_Size_LL  = 0x08,
        Arg_Size_M   = 0x10,
#if (CORE_ARCH_BITS == 32)
        Arg_Size_64  = Arg_Size_LL
#else
        Arg_Size_64  = Arg_Size_L
#endif
      };

      // 'h' and 'hh'.
      if (c == 'h')
      {
        c = (uint8_t)*(++fmt);
        if (c == 'h')
        {
          c = (uint8_t)*(++fmt);
          sizeFlags |= Arg_Size_HH;
        }
        else
        {
          sizeFlags |= Arg_Size_H;
        }
      }
      // 'L'.
      else if (c == 'L')
      {
        c = (uint8_t)*(++fmt);
        sizeFlags |= Arg_Size_LL;
      }
      // 'l' and 'll'.
      else if (c == 'l')
      {
        c = (uint8_t)*(++fmt);
        if (c == 'l')
        {
          c = (uint8_t)*(++fmt);
          sizeFlags |= Arg_Size_LL;
        }
        else
        {
          sizeFlags |= Arg_Size_L;
        }
      }
      // 'j'.
      else if (c == 'j')
      {
        c = (uint8_t)*(++fmt);
        sizeFlags |= Arg_Size_LL;
      }
      // 'z'.
      else if (c == 'z' || c == 'Z')
      {
        c = (uint8_t)*(++fmt);
        if (sizeof(size_t) > sizeof(long))
          sizeFlags |= Arg_Size_LL;
        else if (sizeof(size_t) > sizeof(int))
          sizeFlags |= Arg_Size_L;
      }
      // 't'.
      else if (c == 't')
      {
        c = (uint8_t)*(++fmt);
        if (sizeof(ptrdiff_t) > sizeof(long))
          sizeFlags |= Arg_Size_LL;
        else if (sizeof(ptrdiff_t) > sizeof(int))
          sizeFlags |= Arg_Size_L;
      }
      // 'M' = max type (Core extension).
      else if (c == 'M')
      {
        c = (uint8_t)*(++fmt);
        sizeFlags |= Arg_Size_M;
      }

      // Parse conversion character.
      switch (c)
      {
        // Signed integers.
        case 'd':
        case 'i':
        {
          int64_t i = (sizeFlags >= Arg_Size_64) ? va_arg(ap, int64_t) : va_arg(ap, int32_t);

          if (precision == FormatFlags::NoPrecision && fieldWidth == FormatFlags::NoWidth && directives == 0)
            appendInt(i, base);
          else
            appendInt(i, base, FormatFlags(precision, fieldWidth, directives));
          break;
        }

        // Unsigned integers.
        case 'o':
          base = 8;
          goto ffUnsigned;
        case 'X':
          directives |= FormatFlags::Capitalize;
        case 'x':
          base = 16;
        case 'u':
ffUnsigned:
        {
          uint64_t i = (sizeFlags >= Arg_Size_64) ? va_arg(ap, uint64_t) : va_arg(ap, uint32_t);

          if (precision == FormatFlags::NoPrecision && fieldWidth == FormatFlags::NoWidth && directives == 0)
            appendInt(i, base);
          else
            appendInt(i, base, FormatFlags(precision, fieldWidth, directives));
          break;
        }

        // Floats, doubles, long doubles.
        case 'F':
        case 'E':
        case 'G':
          directives |= FormatFlags::CapitalizeEOrX;
        case 'f':
        case 'e':
        case 'g':
        {
          double i;
          uint doubleForm = 0; // Be quite

          if (c == 'e' || c == 'E')
            doubleForm = DF_Exponent;
          else if (c == 'f' || c == 'F')
            doubleForm = DF_Decimal;
          else if (c == 'g' || c == 'G')
            doubleForm = DF_SignificantDigits;

          i = va_arg(ap, double);
          appendDouble(i, doubleForm, FormatFlags(precision, fieldWidth, directives));
          break;
        }

        // Characters (latin1 or unicode...).
        case 'C':
          sizeFlags |= Arg_Size_L;
        case 'c':
        {
          if (precision == FormatFlags::NoPrecision) precision = 1;
          if (fieldWidth == FormatFlags::NoWidth) fieldWidth = 0;

          sysuint_t fill = (fieldWidth > precision) ? fieldWidth - precision : 0;

          if (fill && (directives & FormatFlags::LeftAdjusted) == 0) append(Char(' '), fill);
          append(Char(va_arg(ap, uint)), precision);
          if (fill && (directives & FormatFlags::LeftAdjusted) != 0) append(Char(' '), fill);
          break;
        }

        // Strings.
        case 'S':
#if FOG_SIZEOF_WCHAR_T == 2
          sizeFlags |= Arg_Size_L;
#else
          sizeFlags |= Arg_Size_LL;
#endif
        case 's':
          if (fieldWidth == FormatFlags::NoWidth) fieldWidth = 0;

          // TODO: Not correct.
          if (sizeFlags >= Arg_Size_LL)
          {
#if 0
            // UTF-32 string (uint32_t*).
            const uint32_t* s = va_arg(ap, const uint32_t*);
            sysuint_t slen = (precision != FormatFlags::NoPrecision)
              ? (sysuint_t)StringUtil::nlen(s, precision)
              : (sysuint_t)StringUtil::len(s);

            String s16;
            TextCodec::utf32().appendToUnicode(s16, reinterpret_cast<const void*>(s), slen * 4);

            slen = s16.getLength();
            sysuint_t fill = (fieldWidth > slen) ? fieldWidth - slen : 0;

            if (fill && (directives & FormatFlags::LeftAdjusted) == 0) append(Char(' '), fill);
            append(s16);
            if (fill && (directives & FormatFlags::LeftAdjusted) != 0) append(Char(' '), fill);
#endif
          }
          else if (sizeFlags >= Arg_Size_L)
          {
#if 0
            // UTF-16 string (Char*).
            const Char* s = va_arg(ap, const Char*);
            sysuint_t slen = (precision != FormatFlags::NoPrecision)
              ? (sysuint_t)StringUtil::nlen(s, precision)
              : (sysuint_t)StringUtil::len(s);
            sysuint_t fill = (fieldWidth > slen) ? fieldWidth - slen : 0;

            if (fill && (directives & FormatFlags::LeftAdjusted) == 0) append(Char(' '), fill);
            append(s, slen);
            if (fill && (directives & FormatFlags::LeftAdjusted) != 0) append(Char(' '), fill);
#endif
          }
          else
          {
#if 0
            // 8-bit string (char*).
            const char* s = va_arg(ap, const char*);
            sysuint_t slen = (precision != FormatFlags::NoPrecision)
              ? (sysuint_t)StringUtil::nlen(s, precision)
              : (sysuint_t)StringUtil::len(s);
            sysuint_t fill = (fieldWidth > slen) ? fieldWidth - slen : 0;

            if (fill && (directives & FormatFlags::LeftAdjusted) == 0) append(Char(' '), fill);
            append(Local8(s, slen));
            if (fill && (directives & FormatFlags::LeftAdjusted) != 0) append(Char(' '), fill);
#endif
          }
          break;

        // Pointer.
        case 'p':
          directives |= FormatFlags::Alternate;
#if FOG_ARCH_BITS == 32
          sizeFlags = 0;
#elif FOG_ARCH_BITS == 64
          sizeFlags = Arg_Size_LL;
#endif // FOG_ARCH_BITS
          goto ffUnsigned;

        // Position receiver 'n'.
        case 'n':
        {
          void* pointer = va_arg(ap, void*);
          sysuint_t n = getLength() - beginLength;
          switch (sizeFlags) {
            case Arg_Size_M:
            case Arg_Size_LL: *(uint64_t *)pointer = (uint64_t)(n); break;
            case Arg_Size_L:  *(ulong    *)pointer = (ulong   )(n); break;
            case Arg_Size_HH: *(uchar    *)pointer = (uchar   )(n); break;
            case Arg_Size_H:  *(uint16_t *)pointer = (uint16_t)(n); break;
            default:          *(uint     *)pointer = (uint    )(n); break;
          }
          break;
        }

        // Extensions
        case 'W':
        {
          if (fieldWidth == FormatFlags::NoWidth) fieldWidth = 0;

          String* string = va_arg(ap, String*);

          const Char* s = string->cData();
          sysuint_t slen = string->getLength();
          if (precision != FormatFlags::NoPrecision)  slen = Math::min(slen, precision);
          sysuint_t fill = (fieldWidth > slen) ? fieldWidth - slen : 0;

          if (fill && (directives & FormatFlags::LeftAdjusted) == 0) append(Char(' '), fill);
          append(Utf16(s, slen));
          if (fill && (directives & FormatFlags::LeftAdjusted) != 0) append(Char(' '), fill);
          break;
        }

        // Percent.
        case '%':
          // skip one "%" if its legal "%%", otherwise send everything
          // to output.
          if (fmtBeginChunk + 1 == fmt) fmtBeginChunk++;
          break;

        // Unsupported or end of input.
        default:
          goto end;
      }
      fmtBeginChunk = fmt+1;
    }

end:
    if (c == '\0')
    {
      if (fmtBeginChunk != fmt) append(Ascii8(fmtBeginChunk, (sysuint_t)(fmt - fmtBeginChunk)));
      break;
    }

    fmt++;
  }
  return Error::Ok;

#undef __VFORMAT_PARSE_NUMBER
}

err_t String::appendWformat(const String& fmt, Char lex, const Sequence<String>& args)
{
  return appendWformat(fmt, lex, args.cData(), args.getLength());
}

err_t String::appendWformat(const String& fmt, Char lex, const String* args, sysuint_t length)
{
  const Char* fmtBeg = fmt.cData();
  const Char* fmtEnd = fmtBeg + fmt.getLength();
  const Char* fmtCur;

  err_t err = Error::Ok;

  for (fmtCur = fmtBeg; fmtCur != fmtEnd; )
  {
    if (*fmtCur == lex)
    {
      fmtBeg = fmtCur;
      if ( (err = append(Utf16(fmtBeg, (sysuint_t)(fmtCur - fmtBeg)))) ) goto done;

      if (++fmtCur != fmtEnd)
      {
        Char ch = *fmtCur;

        if (ch >= Char('0') && ch <= Char('9'))
        {
          uint32_t n = ch.ch() - (uint32_t)'0';
          if (n < length)
          {
            if ( (err = append(args[n])) ) goto done;
            fmtBeg = fmtCur + 1;
          }
        }
        else if (ch == lex)
          fmtBeg++;
      }
      else
        break;
    }
    fmtCur++;
  }

  if (fmtCur != fmtBeg)
  {
    err = append(Utf16(fmtBeg, (sysuint_t)(fmtCur - fmtBeg)));
  }

done:
  return err;
}

// ============================================================================
// [Fog::String::Prepend]
// ============================================================================

err_t String::prepend(Char ch, sysuint_t length)
{
  return insert(0, ch, length);
}

err_t String::prepend(const Ascii8& str)
{
  return insert(0, str);
}

err_t String::prepend(const Utf16& str)
{
  return insert(0, str);
}

err_t String::prepend(const String& other)
{
  return insert(0, other);
}

// ============================================================================
// [Fog::String::Insert]
// ============================================================================

err_t String::insert(sysuint_t index, Char ch, sysuint_t length)
{
  if (length == DetectLength) return Error::InvalidArgument;

  Char* p = _prepareInsert(this, index, length);
  if (!p) return Error::OutOfMemory;

  StringUtil::fill(p, ch, length);
  return Error::Ok;
}

err_t String::insert(sysuint_t index, const Ascii8& str)
{
  const char* s = str.getData();
  sysuint_t length = (str.getLength() == DetectLength)
    ? StringUtil::len(s) : str.getLength();

  Char* p = _prepareInsert(this, index, length);
  if (!p) return Error::OutOfMemory;

  StringUtil::copy(p, s, length);
  return Error::Ok;
}

err_t String::insert(sysuint_t index, const Utf16& str)
{
  const Char* s = str.getData();
  sysuint_t length = (str.getLength() == DetectLength)
    ? StringUtil::len(s) : str.getLength();

  Char* p = _prepareInsert(this, index, length);
  if (!p) return Error::OutOfMemory;

  StringUtil::copy(p, s, length);
  return Error::Ok;
}

err_t String::insert(sysuint_t index, const String& _other)
{
  String other(_other);

  Char* p = _prepareInsert(this, index, other.getLength());
  if (!p) return Error::OutOfMemory;

  StringUtil::copy(p, other.cData(), other.getLength());
  return Error::Ok;
}

// ============================================================================
// [Fog::String - Remove]
// ============================================================================

sysuint_t String::remove(const Range& range)
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return 0;

  sysuint_t lenPart1 = rstart;
  sysuint_t lenPart2 = getLength() - rstart - rlen;
  sysuint_t lenAfter = getLength() - rlen;

  if (_d->refCount.get() > 1)
  {
    Data* newd = Data::alloc(lenAfter);
    if (!newd) return 0;

    StringUtil::copy(newd->data, _d->data, lenPart1);
    StringUtil::copy(newd->data + rstart, _d->data + rstart + rlen, lenPart2);
    newd->length = lenAfter;
    newd->data[lenAfter] = 0;

    AtomicBase::ptr_setXchg(&_d, newd)->deref();
  }
  else
  {
    StringUtil::move(_d->data + rstart, _d->data + rstart + rlen, lenPart2);
    _d->length = lenAfter;
    _d->data[lenAfter] = 0;
    _d->hashCode = 0;
  }

  return rlen;
}

sysuint_t String::remove(Char ch, uint cs, const Range& range)
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return 0;

  Data* d = _d;
  sysuint_t length = d->length;
  sysuint_t tail;

  Char* strBeg = d->data;
  Char* strCur = strBeg + rstart;
  Char* strEnd = strCur + rlen;
  Char* destCur;

  if (cs == CaseSensitive)
  {
caseSensitive:
    while (strCur != strEnd)
    {
      if (*strCur == ch) goto caseSensitiveRemove;
      strCur++;
    }
    return 0;

caseSensitiveRemove:
    if (d->refCount.get() > 1)
    {
      rstart = strCur - strBeg;
      rlen = strEnd - strCur;

      if (detach() != Error::Ok) return 0;

      d = _d;

      strBeg = d->data;
      strCur = strBeg + rstart;
      strEnd = strCur + rlen;
    }
    destCur = strCur;

    while (strCur != strEnd)
    {
      if (*strCur != ch) *destCur++ = *strCur;
      strCur++;
    }

    tail = length - (rstart + rlen);
    StringUtil::copy(destCur, strCur, tail);
  }
  else
  {
    Char chLower = ch.toLower();
    Char chUpper = ch.toUpper();
    if (chLower == chUpper) goto caseSensitive;

    while (strCur != strEnd)
    {
      if (*strCur == chLower || *strCur == chUpper) goto caseInsensitiveRemove;
      strCur++;
    }
    return 0;

caseInsensitiveRemove:
    if (d->refCount.get() > 1)
    {
      rstart = strCur - strBeg;
      rlen = strEnd - strCur;

      if (detach() != Error::Ok) return 0;
      d = _d;

      strBeg = d->data;
      strCur = strBeg + rstart;
      strEnd = strCur + rlen;
    }
    destCur = strCur;

    while (strCur != strEnd)
    {
      if (*strCur != chLower && *strCur != chUpper) *destCur++ = *strCur;
      strCur++;
    }

    tail = length - (rstart + rlen);
    StringUtil::copy(destCur, strCur, tail);
  }

  d->length = (sysuint_t)(destCur - d->data);
  d->data[d->length] = 0;
  d->hashCode = 0;
  return length - d->length;
}

sysuint_t String::remove(const String& other, uint cs, const Range& range)
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return 0;

  sysuint_t len = other.getLength();
  if (len == 0) return 0;
  if (len == 1) return remove(other.at(0), cs, range);

  if (rlen >= 256)
  {
    // Match using StringMatcher.
    StringMatcher matcher;
    if (matcher.setPattern(other)) return 0;

    return remove(matcher, cs, range);
  }
  else
  {
    // Match using naive algorithm.
    const Char* aStr = cData();
    const Char* bStr = other.cData();

    sysuint_t aLength = getLength();
    sysuint_t bLength = len;

    Range ranges[128]; // Maximal length is 256 and minimal pattern size is 2.
    sysuint_t count = 0;
    sysuint_t rpos = rstart;
    sysuint_t rend = rstart + rlen;

    for (;;)
    {
      sysuint_t i = StringUtil::indexOf(aStr + rpos, rend - rpos, bStr, bLength);
      if (i == InvalidIndex) break;
      rpos += i;

      ranges[count].index = rpos;
      ranges[count].length = bLength;
      count++;

      rpos += bLength;
    }

    return remove(ranges, count);
  }
}

sysuint_t String::remove(const StringFilter& filter, uint cs, const Range& range)
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return 0;

  const Char* str = cData();
  sysuint_t len = getLength();
  sysuint_t rend = rstart + rlen;

  Vector<Range> ranges;

  for (;;)
  {
    Range r = filter.indexOf(str, len, cs, Range(rstart, rstart - rend));
    if (r.index == InvalidIndex) break;

    ranges.append(r);
    rstart = r.index + r.length;
  }

  return remove(ranges.cData(), ranges.getLength());
}

sysuint_t String::remove(const Range* range, sysuint_t count)
{
  if (range == NULL || count == 0) return 0;

  sysuint_t i;
  sysuint_t len = getLength();

  if (_d->refCount.get() == 1)
  {
    i = 0;
    Char* s = _d->data;
    sysuint_t dstPos = range[0].index;
    sysuint_t srcPos = dstPos;

    do {
      srcPos += range[i].length;
      sysuint_t j = ((++i == count) ? len : range[i].index) - srcPos;

      StringUtil::copy(s + dstPos, s + srcPos, j);

      dstPos += j;
      srcPos += j;
    } while (i != count);

    _d->length = dstPos;
    _d->hashCode = 0;
    s[dstPos] = 0;
  }
  else
  {
    sysuint_t deleteLength = 0;
    sysuint_t lengthAfter;

    for (i = 0; i < count; i++) deleteLength += range[i].length;
    FOG_ASSERT(len >= deleteLength);

    lengthAfter = len - deleteLength;

    Data* newd = Data::alloc(lengthAfter);
    if (!newd) return 0;

    i = 0;
    Char* dstData = newd->data;
    Char* srcData = _d->data;

    sysuint_t dstPos = range[0].index;
    sysuint_t srcPos = dstPos;

    StringUtil::copy(dstData, srcData, dstPos);

    do {
      srcPos += range[i].length;
      sysuint_t j = ((++i == count) ? len : range[i].index) - srcPos;

      StringUtil::copy(dstData + dstPos, srcData + srcPos, j);

      dstPos += j;
      srcPos += j;
    } while (i != count);

    newd->length = lengthAfter;
    newd->data[lengthAfter] = 0;

    AtomicBase::ptr_setXchg(&_d, newd)->deref();
  }
  return count;
}

// ============================================================================
// [Fog::String - Replace]
// ============================================================================

err_t String::replace(const Range& range, const String& replacement)
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return Error::Ok;

  const Char* replacementData = replacement.cData();
  sysuint_t replacementLength = replacement.getLength();

  if (_d->refCount.get() == 1 && _d != replacement._d)
  {
    Char* s = _d->data + rstart;
    sysuint_t lengthAfter = _d->length - rlen + replacementLength;
    if (lengthAfter < _d->length) return Error::Overflow;

    if (_d->capacity >= lengthAfter)
    {
      StringUtil::move(s + replacementLength, s + rlen, _d->length - rstart - rlen);
      StringUtil::copy(s, replacementData, replacementLength);

      _d->length = lengthAfter;
      _d->hashCode = 0;
      _d->data[lengthAfter] = 0;
      return Error::Ok;
    }
  }

  Range r(rstart, rlen);
  return replace(&r, 1, replacementData, replacementLength);
}

err_t String::replace(Char before, Char after,
  uint cs, const Range& range)
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return Error::Ok;

  Data* d = _d;
  sysuint_t length = d->length;

  Char* strCur = d->data + rstart;
  Char* strEnd = strCur + rlen;

  if (cs == CaseSensitive)
  {
caseSensitive:
    while (strCur != strEnd)
    {
      if (*strCur == before) goto caseSensitiveReplace;
      strCur++;
    }
    return Error::Ok;

caseSensitiveReplace:
    if (d->refCount.get() > 1)
    {
      rstart = (sysuint_t)(strCur - d->data);
      rlen = (sysuint_t)(strEnd - strCur);

      err_t err;
      if ((err = detach())) return err;

      d = _d;

      strCur = d->data + rstart;
      strEnd = strCur + rlen;
    }

    while (strCur != strEnd)
    {
      if (*strCur == before) *strCur = after;
      strCur++;
    }
  }
  else
  {
    Char beforeLower = before.toLower();
    Char beforeUpper = before.toUpper();

    if (beforeLower == beforeUpper) goto caseSensitive;

    while (strCur != strEnd)
    {
      if (*strCur == beforeLower || *strCur == beforeUpper) goto caseInsensitiveReplace;
      strCur++;
    }
    return Error::Ok;

caseInsensitiveReplace:
    if (d->refCount.get() > 1)
    {
      rstart = (sysuint_t)(strCur - d->data);
      rlen = (sysuint_t)(strEnd - strCur);

      err_t err;
      if ((err = detach())) return err;

      d = _d;

      strCur = d->data + rstart;
      strEnd = strCur + rlen;
    }

    while (strCur != strEnd)
    {
      if (*strCur == beforeLower || *strCur == beforeUpper) *strCur = after;
      strCur++;
    }
  }

  return Error::Ok;
}

err_t String::replace(const String& before, const String& after,
  uint cs, const Range& range)
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return 0;

  sysuint_t len = before.getLength();
  if (len == 0) return 0;

  if (rlen >= 256)
  {
    // Match using StringMatcher.
    StringMatcher matcher;
    if (matcher.setPattern(before)) return 0;

    return replace(matcher, after, cs, range);
  }
  else
  {
    // Match using naive algorithm.
    const Char* aStr = cData();
    const Char* bStr = before.cData();

    sysuint_t aLength = getLength();
    sysuint_t bLength = len;

    Range ranges[256];
    sysuint_t count = 0;
    sysuint_t rpos = rstart;
    sysuint_t rend = rstart + rlen;

    for (;;)
    {
      sysuint_t i = StringUtil::indexOf(aStr + rpos, rend - rpos, bStr, bLength);
      if (i == InvalidIndex) break;
      rpos += i;

      ranges[count].index = rpos;
      ranges[count].length = bLength;
      count++;

      rpos += bLength;
    }

    return replace(ranges, count, after.cData(), after.getLength());
  }
}

err_t String::replace(const StringFilter& filter, const String& after,
  uint cs, const Range& range)
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return 0;

  const Char* str = cData();
  sysuint_t len = getLength();
  sysuint_t rend = rstart + rlen;

  Vector<Range> ranges;

  for (;;)
  {
    Range r = filter.indexOf(str, len, cs, Range(rstart, rstart - rend));
    if (r.index == InvalidIndex) break;

    ranges.append(r);
    rstart = r.index + r.length;
  }

  return replace(ranges.cData(), ranges.getLength(), after.cData(), after.getLength());
}

err_t String::replace(const Range* m, sysuint_t mcount, const Char* after, sysuint_t alen)
{
  sysuint_t i;
  sysuint_t pos = 0;
  sysuint_t len = getLength();
  const Char* cur = cData();

  // Get total count of characters we remove.
  sysuint_t mTotal = 0;
  for (i = 0; i < mcount; i++) mTotal += m[i].length;

  // Get total count of characters we add.
  sysuint_t aTotal = alen * mcount;

  // Get target length.
  sysuint_t lenAfter = len - mTotal + aTotal;

  Data* newd = Data::alloc(lenAfter);
  if (!newd) return Error::OutOfMemory;

  Char* p = newd->data;
  sysuint_t remain = lenAfter;
  sysuint_t t;

  // Serialize
  for (i = 0; i < mcount; i++)
  {
    sysuint_t mstart = m[i].index;
    sysuint_t mlen = m[i].length;

    // Begin
    t = mstart - pos;
    if (t > remain) goto overflow;
    StringUtil::copy(p, cur + pos, t);
    p += t; remain -= t;

    // Replacement
    if (alen > remain) goto overflow;
    StringUtil::copy(p, after, alen);
    p += alen; remain -= alen;

    pos = mstart + mlen;
  }

  // Last piece of string
  t = getLength() - pos;
  if (t > remain) goto overflow;
  StringUtil::copy(p, cur + pos, t);
  p += t;

  // Be sure that calculated length is correct (if this assert fails, the
  // 'm' and 'mcount' parameters are incorrect).
  FOG_ASSERT(p == newd->data + lenAfter);

  newd->length = lenAfter;
  newd->data[lenAfter] = 0;

  AtomicBase::ptr_setXchg(&_d, newd)->deref();
  return Error::Ok;

overflow:
  newd->deref();
  return Error::Overflow;
}

// ============================================================================
// [Fog::String - Lower / Upper]
// ============================================================================

err_t String::lower()
{
  Data* d = _d;

  Char* strCur = d->data;
  Char* strEnd = strCur + d->length;

  for (; strCur != strEnd; strCur++)
  {
    if (strCur->isUpper()) goto modify;
  }
  return Error::Ok;

modify:
  {
    sysuint_t n = (sysuint_t)(strCur - d->data);

    err_t err;
    if ((err = detach())) return err;

    d = _d;

    strCur = d->data + n;
    strEnd = d->data + d->length;

    for (; strCur != strEnd; strCur++)
    {
      *strCur = strCur->toLower();
    }
  }
  d->hashCode = 0;
  return Error::Ok;
}

err_t String::upper()
{
  Data* d = _d;

  Char* strCur = d->data;
  Char* strEnd = strCur + d->length;

  for (; strCur != strEnd; strCur++)
  {
    if (strCur->isLower()) goto modify;
  }
  return Error::Ok;

modify:
  {
    sysuint_t n = (sysuint_t)(strCur - d->data);

    err_t err;
    if ((err = detach())) return err;

    d = _d;

    strCur = d->data + n;
    strEnd = d->data + d->length;

    for (; strCur != strEnd; strCur++)
    {
      *strCur = strCur->toUpper();
    }
  }
  d->hashCode = 0;
  return Error::Ok;
}

String String::lowered() const
{
  String t(*this);
  t.lower();
  return t;
}

String String::uppered() const
{
  String t(*this);
  t.upper();
  return t;
}

// ============================================================================
// [Fog::String - Whitespaces / Justification]
// ============================================================================

err_t String::trim()
{
  Data* d = _d;
  sysuint_t len = d->length;

  if (!len) return Error::Ok;

  const Char* strCur = d->data;
  const Char* strEnd = strCur + len;

  while (strCur != strEnd   && strCur->isSpace()) strCur++;
  while (strCur != strEnd-- && strEnd->isSpace()) continue;

  if (strCur != d->data || ++strEnd != d->data + len)
  {
    len = (sysuint_t)(strEnd - strCur);
    if (d->refCount.get() > 1)
    {
      Data* newd = Data::alloc(len, strCur, len);
      if (!newd) return Error::OutOfMemory;
      AtomicBase::ptr_setXchg(&_d, newd)->deref();
    }
    else
    {
      if (strCur != d->data) StringUtil::move(d->data, strCur, len);
      d->length = len;
      d->data[len] = 0;
      d->hashCode = 0;
    }
  }

  return Error::Ok;
}

err_t String::simplify()
{
  Data* d = _d;
  sysuint_t len = d->length;

  if (!len) return Error::Ok;

  const Char* strBeg;
  const Char* strCur = d->data;
  const Char* strEnd = strCur + len;

  Char* dest;

  while (strCur != strEnd   && strCur->isSpace()) strCur++;
  while (strCur != strEnd-- && strEnd->isSpace()) continue;

  strBeg = strCur;

  // Left and Right trim is complete...

  if (strCur != d->data || strEnd + 1 != d->data + len) goto simp;

  for (; strCur < strEnd; strCur++)
  {
    if (strCur[0].isSpace() && strCur[1].isSpace()) goto simp;
  }
  return Error::Ok;

simp:
  strCur = strBeg;
  strEnd++;

  // this is a bit messy, but I will describe how it works. We can't
  // change string that's shared between another one, so if reference
  // count is larger than 1 we will alloc a new D. At the end of this
  // function we will dereference the D and in case that string
  // is shared it wont be destroyed. Only one problem is that we need
  // to increase reference count if string is detached.
  if (d->refCount.get() > 1)
  {
    Data* newd = Data::alloc((sysuint_t)(strEnd - strCur));
    if (!newd) return Error::OutOfMemory;
    _d = newd;
  }
  else
  {
    d->refCount.inc();
  }

  dest = _d->data;

  do {
    if    (strCur != strEnd &&  strCur->isSpace()) *dest++ = ' ';
    while (strCur != strEnd &&  strCur->isSpace()) strCur++;
    while (strCur != strEnd && !strCur->isSpace()) *dest++ = *strCur++;
  } while (strCur != strEnd);

  _d->length = (dest - _d->data);
  _d->data[_d->length] = 0;
  _d->hashCode = 0;

  d->deref();

  return Error::Ok;
}

err_t String::truncate(sysuint_t n)
{
  Data* d = _d;
  if (d->length <= n) return Error::Ok;

  if (d->refCount.get() > 1)
  {
    Data* newd = Data::alloc(n, d->data, n);
    if (!newd) return Error::OutOfMemory;
    AtomicBase::ptr_setXchg(&_d, newd)->deref();
  }
  else
  {
    d->length = n;
    d->data[d->length] = 0;
    d->hashCode = 0;
  }
  return Error::Ok;
}

err_t String::justify(sysuint_t n, Char fill, uint32_t flags)
{
  Data* d = _d;
  sysuint_t length = d->length;

  if (n <= length) return Error::Ok;

  sysuint_t t = n - length;
  sysuint_t left = 0;
  sysuint_t right = 0;

  if ((flags & CenterJustify) == CenterJustify)
  {
    left = t >> 1;
    right = t - left;
  }
  else if ((flags & LeftJustify) == LeftJustify)
  {
    right = t;
  }
  else if ((flags & RightJustify) == RightJustify)
  {
    left = t;
  }

  err_t err;
  if ( (err = reserve(n)) ) return err;
  if ( (err = prepend(fill, left)) ) return err;
  return append(fill, right);
}

String String::trimmed() const
{
  String t(*this);
  t.trim();
  return t;
}

String String::simplified() const
{
  String t(*this);
  t.simplify();
  return t;
}

String String::truncated(sysuint_t n) const
{
  String t(*this);
  t.truncate(n);
  return t;
}

String String::justified(sysuint_t n, Char fill, uint32_t flags) const
{
  String t(*this);
  t.justify(n, fill, flags);
  return t;
}

// ============================================================================
// [Fog::String - Split / Join]
// ============================================================================

Vector<String> String::split(Char ch, uint splitBehavior, uint cs) const
{
  Vector<String> result;
  Data* d = _d;

  if (d->length == 0) return result;

  const Char* strBeg = d->data;
  const Char* strCur = strBeg;
  const Char* strEnd = strCur + d->length;

  if (cs == CaseSensitive)
  {
__caseSensitive:
    for (;;)
    {
      if (strCur == strEnd || *strCur == ch)
      {
        sysuint_t splitLength = (sysuint_t)(strCur - strBeg);
        if ((splitLength == 0 && splitBehavior == KeepEmptyParts) || splitLength != 0)
        {
          result.append(String(strBeg, splitLength));
        }
        if (strCur == strEnd) break;
        strBeg = ++strCur;
      }
      else
        strCur++;
    }
  }
  else
  {
    Char cLower = ch.toLower();
    Char cUpper = ch.toUpper();
    if (cLower == cUpper) goto __caseSensitive;

    for (;;)
    {
      if (strCur == strEnd || *strCur == cLower || *strCur == cUpper)
      {
        sysuint_t splitLength = (sysuint_t)(strCur - strBeg);
        if ((splitLength == 0 && splitBehavior == KeepEmptyParts) || splitLength != 0)
        {
          result.append(String(strBeg, splitLength));
        }
        if (strCur == strEnd) break;
        strBeg = ++strCur;
      }
      else
        strCur++;
    }
  }
  return result;
}

Vector<String> String::split(const String& pattern, uint splitBehavior, uint cs) const
{
  sysuint_t plen = pattern.getLength();

  if (!plen)
  {
    Vector<String> result;
    result.append(*this);
    return result;
  }
  else if (plen == 1)
  {
    return split(pattern.at(0), splitBehavior, cs);
  }
  else
  {
    StringMatcher matcher(pattern);
    return split(matcher, splitBehavior, cs);
  }
}

Vector<String> String::split(const StringFilter& filter, uint splitBehavior, uint cs) const
{
  Vector<String> result;
  Data* d = _d;

  sysuint_t length = d->length;

  const Char* strCur = d->data;
  const Char* strEnd = strCur + length;

  for (;;)
  {
    sysuint_t remain = (sysuint_t)(strEnd - strCur);
    Range m = filter.match(strCur, remain, cs, Range(0, remain));
    sysuint_t splitLength = (m.index != InvalidIndex) ? m.index : remain;

    if ((splitLength == 0 && splitBehavior == KeepEmptyParts) || splitLength != 0)
      result.append(String(strCur, splitLength));

    if (m.index == InvalidIndex) break;

    strCur += m.index;
    strCur += m.length;
  }

  return result;
}

String String::join(const Sequence<String>& seq, const Char separator)
{
  TemporaryString<1> sept(separator);
  return join(seq, sept);
}

String String::join(const Sequence<String>& seq, const String& separator)
{
  String result;

  sysuint_t seqLength = 0;
  sysuint_t sepLength = separator.getLength();

  Sequence<String>::ConstIterator it(seq);

  for (it.toStart(); it.isValid(); it.toNext())
  {
    sysuint_t len = it.value().getLength();

    // Prevent for possible overflow (shouldn't normally happen)
    if (!it.atStart())
    {
      if (seqLength + sepLength < seqLength) return result;
      seqLength += sepLength;
    }

    // Prevent for possible overflow (shouldn't normally happen)
    if (seqLength + len < seqLength) return result;
    seqLength += len;
  }

  // Allocate memory for all strings in seq and for separators
  if (result.reserve(seqLength) != Error::Ok) return result;

  Char* cur = result._d->data;
  const Char* sep = separator.cData();

  // Serialize
  for (it.toStart(); it.isValid(); it.toNext())
  {
    sysuint_t len = it.value().getLength();

    if (!it.atStart())
    {
      StringUtil::copy(cur, sep, sepLength);
      cur += sepLength;
    }

    StringUtil::copy(cur, it.value().cData(), len);
    cur += len;
  }

  cur[0] = Char(0);
  result._d->length = (sysuint_t)(cur - result._d->data);
  return result;
}

// ============================================================================
// [Fog::String - Substring]
// ============================================================================

String String::substring(const Range& range) const
{
  String ret;

  sysuint_t rstart, rlen;
  if (fitToRange(*this, &rstart, &rlen, range)) ret.set(Utf16(cData() + rstart, rlen));

  return ret;
}

// ============================================================================
// [Fog::String - Conversion]
// ============================================================================

err_t String::atob(bool* dst, int base, sysuint_t* end, uint32_t* parserFlags) const
{
  return StringUtil::atob(cData(), getLength(), dst, end, parserFlags);
}

err_t String::atoi8(int8_t* dst, int base, sysuint_t* end, uint32_t* parserFlags) const
{
  return StringUtil::atoi8(cData(), getLength(), dst, base, end, parserFlags);
}

err_t String::atou8(uint8_t* dst, int base, sysuint_t* end, uint32_t* parserFlags) const
{
  return StringUtil::atou8(cData(), getLength(), dst, base, end, parserFlags);
}

err_t String::atoi16(int16_t* dst, int base, sysuint_t* end, uint32_t* parserFlags) const
{
  return StringUtil::atoi16(cData(), getLength(), dst, base, end, parserFlags);
}

err_t String::atou16(uint16_t* dst, int base, sysuint_t* end, uint32_t* parserFlags) const
{
  return StringUtil::atou16(cData(), getLength(), dst, base, end, parserFlags);
}

err_t String::atoi32(int32_t* dst, int base, sysuint_t* end, uint32_t* parserFlags) const
{
  return StringUtil::atoi32(cData(), getLength(), dst, base, end, parserFlags);
}

err_t String::atou32(uint32_t* dst, int base, sysuint_t* end, uint32_t* parserFlags) const
{
  return StringUtil::atou32(cData(), getLength(), dst, base, end, parserFlags);
}

err_t String::atoi64(int64_t* dst, int base, sysuint_t* end, uint32_t* parserFlags) const
{
  return StringUtil::atoi64(cData(), getLength(), dst, base, end, parserFlags);
}

err_t String::atou64(uint64_t* dst, int base, sysuint_t* end, uint32_t* parserFlags) const
{
  return StringUtil::atou64(cData(), getLength(), dst, base, end, parserFlags);
}

err_t String::atof(float* dst, const Locale* locale, sysuint_t* end, uint32_t* parserFlags) const
{
  if (locale == NULL) locale = &Locale::posix();

  return StringUtil::atof(cData(), getLength(), dst,
    Char(locale->getDecimalPoint().ch()), end, parserFlags);
}

err_t String::atod(double* dst, const Locale* locale, sysuint_t* end, uint32_t* parserFlags) const
{
  if (locale == NULL) locale = &Locale::posix();

  return StringUtil::atod(cData(), getLength(), dst,
    Char(locale->getDecimalPoint().ch()), end, parserFlags);
}

// ============================================================================
// [Fog::String - Contains]
// ============================================================================

bool String::contains(Char ch,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (fitToRange(*this, &rstart, &rlen, range))
    return StringUtil::indexOf(cData() + rstart, rlen, ch, cs) != InvalidIndex;
  else
    return false;
}

bool String::contains(const String& pattern,
  uint cs, const Range& range) const
{
  return indexOf(pattern, cs, range) != InvalidIndex;
}

bool String::contains(const StringFilter& filter,
  uint cs, const Range& range) const
{
  Range m = filter.indexOf(cData(), getLength(), cs, range);
  return m.index != InvalidIndex;
}

// ============================================================================
// [Fog::String - CountOf]
// ============================================================================

sysuint_t String::countOf(Char ch,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (fitToRange(*this, &rstart, &rlen, range))
    return StringUtil::countOf(cData() + rstart, rlen, ch, cs);
  else
    return 0;
}

sysuint_t String::countOf(const String& pattern,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return 0;

  sysuint_t len = pattern.getLength();
  if (len == 0) return 0;
  if (len == 1) return countOf(pattern.at(0), cs, range);

  if (rlen >= 256)
  {
    // Match using StringMatcher.
    StringMatcher matcher;
    if (matcher.setPattern(pattern)) return 0;

    return countOf(matcher, cs, range);
  }
  else
  {
    // Match using naive algorithm.
    const Char* aStr = cData();
    const Char* bStr = pattern.cData();

    sysuint_t aLength = getLength();
    sysuint_t bLength = len;

    sysuint_t rpos = rstart;
    sysuint_t rend = rstart + rlen;

    sysuint_t count = 0;

    for (;;)
    {
      sysuint_t i = StringUtil::indexOf(aStr + rpos, rend - rpos, bStr, bLength);
      if (i == InvalidIndex) break;
      rpos += i;

      count++;
      rpos += bLength;
    }

    return count;
  }
}

sysuint_t String::countOf(const StringFilter& filter,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return 0;

  const Char* str = cData();
  sysuint_t len = getLength();
  sysuint_t rend = rstart + rlen;
  sysuint_t count = 0;

  for (;;)
  {
    Range r = filter.indexOf(str, len, cs, Range(rstart, rstart - rend));
    if (r.index == InvalidIndex) break;

    count++;
    rstart = r.index + r.length;
  }

  return count;
}

// ============================================================================
// [Fog::String - IndexOf / LastIndexOf]
// ============================================================================

sysuint_t String::indexOf(Char ch,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return InvalidIndex;

  sysuint_t i = StringUtil::indexOf(cData() + rstart, rlen, ch, cs);
  return i != InvalidIndex ? i + rstart : i;
}

sysuint_t String::indexOf(const String& pattern,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return InvalidIndex;

  sysuint_t len = pattern.getLength();
  if (len == 0) return InvalidIndex;
  if (len == 1) return indexOf(pattern.at(0), cs, range);

  if (rlen >= 256)
  {
    // Match using StringMatcher.
    StringMatcher matcher;
    if (matcher.setPattern(pattern)) return 0;

    return indexOf(matcher, cs, range);
  }
  else
  {
    // Match using naive algorithm.
    sysuint_t i = StringUtil::indexOf(cData() + rstart, rlen, pattern.cData(), len, cs);
    return (i == InvalidIndex) ? i : i + rstart;
  }
}

sysuint_t String::indexOf(const StringFilter& filter,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return InvalidIndex;

  Range m = filter.match(cData(), getLength(), cs, Range(rstart, rlen));
  return m.index;
}

sysuint_t String::lastIndexOf(Char ch,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return InvalidIndex;

  sysuint_t i = StringUtil::lastIndexOf(cData() + rstart, rlen, ch, cs);
  return i != InvalidIndex ? i + rstart : i;
}

sysuint_t String::lastIndexOf(const String& pattern,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return InvalidIndex;

  sysuint_t len = pattern.getLength();
  if (len == 0) return InvalidIndex;
  if (len == 1) return lastIndexOf(pattern.at(0), cs, range);

  if (rlen >= 256)
  {
    // Match using StringMatcher.
    StringMatcher matcher;
    if (matcher.setPattern(pattern)) return 0;

    return lastIndexOf(matcher, cs, range);
  }
  else
  {
    // Match using naive algorithm.
    const Char* aData = cData();
    const Char* bData = pattern.cData();

    sysuint_t result = InvalidIndex;

    for (;;)
    {
      sysuint_t i = StringUtil::indexOf(aData + rstart, rlen, bData, len);
      if (i == InvalidIndex) break;

      result = i + rstart;

      i += len;
      rstart += i;
      rlen -= i;
    }
    return result;
  }
}

sysuint_t String::lastIndexOf(const StringFilter& filter,
  uint cs, const Range& range) const
{
  sysuint_t rstart, rlen;
  if (!fitToRange(*this, &rstart, &rlen, range)) return InvalidIndex;

  sysuint_t result = InvalidIndex;

  for (;;)
  {
    Range m = filter.match(cData(), getLength(), cs, Range(rstart, rlen));
    if (m.index == InvalidIndex) break;

    result = m.index;

    sysuint_t d = m.index + m.length;
    rstart += d;
    rlen -= d;
  }

  return result;
}

// ============================================================================
// [Fog::String - StartsWith / EndsWith]
// ============================================================================

bool String::startsWith(const Ascii8& str, uint cs) const
{
  const char* s = str.getData();
  sysuint_t len = str.getLength();
  if (len == DetectLength) len = StringUtil::len(s);

  return getLength() >= len && StringUtil::eq(cData(), s, len, cs);
}

bool String::startsWith(const Utf16& str, uint cs) const
{
  const Char* s = str.getData();
  sysuint_t len = str.getLength();
  if (len == DetectLength) len = StringUtil::len(s);

  return getLength() >= len && StringUtil::eq(cData(), s, len, cs);
}

bool String::startsWith(const String& str, uint cs) const
{
  return getLength() >= str.getLength() &&
    StringUtil::eq(cData(), str.cData(), str.getLength(), cs);
}

bool String::startsWith(const StringFilter& filter, uint cs) const
{
  sysuint_t flen = filter.getLength();

  if (flen == InvalidIndex) flen = getLength();
  return filter.match(cData(), getLength(), cs, Range(0, flen)).index == 0;
}

bool String::endsWith(const Ascii8& str, uint cs) const
{
  const char* s = str.getData();
  sysuint_t len = str.getLength();
  if (len == DetectLength) len = StringUtil::len(s);

  return getLength() >= len && StringUtil::eq(cData() + getLength() - len, s, len, cs);
}

bool String::endsWith(const Utf16& str, uint cs) const
{
  const Char* s = str.getData();
  sysuint_t len = str.getLength();
  if (len == DetectLength) len = StringUtil::len(s);

  return getLength() >= len &&
    StringUtil::eq(cData() + getLength() - len, s, len, cs);
}

bool String::endsWith(const String& str, uint cs) const
{
  return getLength() >= str.getLength() &&
    StringUtil::eq(cData() + getLength() - str.getLength(), str.cData(), str.getLength(), cs);
}

bool String::endsWith(const StringFilter& filter, uint cs) const
{
  sysuint_t flen = filter.getLength();

  if (flen == InvalidIndex)
  {
    sysuint_t i = 0;
    sysuint_t len = getLength();

    for (;;)
    {
      Range r = filter.match(cData(), len, cs, Range(i));
      if (r.index == InvalidIndex) return false;
      if (r.index + r.length == len) return true;

      i = r.index + 1;
    }
  }
  else
  {
    return flen <= getLength() &&
      filter.match(
        cData() + getLength() - flen, getLength(), cs, Range(0, flen)).index == 0;
  }
}

// ============================================================================
// [Fog::String - ByteSwap]
// ============================================================================

err_t String::bswap()
{
  if (getLength() == 0) return Error::Ok;

  err_t err;
  if ((err = detach())) return err;

  sysuint_t i, len = getLength();
  Char* ch = _d->data;
  for (i = 0; i < len; i++) ch[i].bswap();

  _d->hashCode = 0;
  return Error::Ok;
}

// ============================================================================
// [Fog::String - Comparison]
// ============================================================================

bool String::eq(const String* a, const String* b)
{
  sysuint_t alen = a->getLength();
  sysuint_t blen = b->getLength();
  if (alen != blen) return false;

  return StringUtil::eq(a->cData(), b->cData(), alen, CaseSensitive);
}

bool String::ieq(const String* a, const String* b)
{
  sysuint_t alen = a->getLength();
  sysuint_t blen = b->getLength();
  if (alen != blen) return false;

  return StringUtil::eq(a->cData(), b->cData(), alen, CaseInsensitive);
}

int String::compare(const String* a, const String* b)
{
  sysuint_t aLen = a->getLength();
  sysuint_t bLen = b->getLength();
  const Char* aCur = a->cData();
  const Char* bCur = b->cData();
  const Char* aEnd = aCur + aLen;

  int c;

  if (bLen < aLen) aEnd = aCur + bLen;

  for (; aCur != aEnd; aCur++, bCur++)
    if ((c = (int)aCur->ch() - (int)bCur->ch())) return c;

  return (int)((sysint_t)aLen - (sysint_t)bLen);
}

int String::icompare(const String* a, const String* b)
{
  sysuint_t aLen = a->getLength();
  sysuint_t bLen = b->getLength();
  const Char* aCur = a->cData();
  const Char* bCur = b->cData();
  const Char* aEnd = aCur + aLen;

  int c;

  if (bLen < aLen) aEnd = aCur + bLen;

  for (; aCur != aEnd; aCur++, bCur++)
    if ((c = (int)aCur->toLower().ch() - (int)bCur->toLower().ch())) return c;

  return (int)((sysint_t)aLen - (sysint_t)bLen);
}

bool String::eq(const Ascii8& other, uint cs) const
{
  sysuint_t len = other.getLength();
  if (len == DetectLength)
  {
    const Char* aCur = cData();
    const char* bCur = other.getData();

    if (cs == CaseSensitive)
    {
      for (sysuint_t i = getLength(); i; i--, aCur++, bCur++)
      {
        if (!bCur) return false;
        if (*aCur != *bCur) return false;
      }
    }
    else
    {
      for (sysuint_t i = getLength(); i; i--, aCur++, bCur++)
      {
        if (!bCur) return false;
        if (aCur->toLower() != Byte::toLower(*bCur)) return false;
      }
    }
    return *bCur == 0;
  }
  else
    return getLength() == len && StringUtil::eq(cData(), other.getData(), len, cs);
}

bool String::eq(const Utf16& other, uint cs) const
{
  sysuint_t len = other.getLength();
  if (len == DetectLength)
  {
    const Char* aCur = cData();
    const Char* bCur = other.getData();

    if (cs == CaseSensitive)
    {
      for (sysuint_t i = getLength(); i; i--, aCur++, bCur++)
      {
        if (FOG_UNLIKELY(bCur->ch() == 0)) return false;
        if (*aCur != *bCur) return false;
      }
    }
    else
    {
      for (sysuint_t i = getLength(); i; i--, aCur++, bCur++)
      {
        if (FOG_UNLIKELY(bCur->ch() == 0)) return false;
        if (aCur->toLower().ch() != bCur->toLower().ch()) return false;
      }
    }
    return bCur->ch() == 0;
  }
  else
    return getLength() == len && StringUtil::eq(cData(), other.getData(), len, cs);
}

bool String::eq(const String& other, uint cs) const
{
  return getLength() == other.getLength() &&
    StringUtil::eq(cData(), other.cData(), getLength(), cs);
}

int String::compare(const Ascii8& other, uint cs) const
{
  sysuint_t aLen = getLength();
  sysuint_t bLen = other.getLength();
  const Char* aCur = cData();
  const Char* aEnd = aCur + aLen;
  const char* bCur = other.getData();

  int c;

  if (bLen == DetectLength)
  {
    if (cs == CaseSensitive)
    {
      for (;;)
      {
        if (FOG_UNLIKELY(aCur == aEnd)) return *bCur ? -1 : 0;
        if ((c = (int)aCur->ch() - (int)(uint8_t)*bCur)) return c;
        aCur++;
        bCur++;
      }
    }
    else
    {
      for (;;)
      {
        if (FOG_UNLIKELY(aCur == aEnd)) return *bCur ? -1 : 0;
        if ((c = (int)aCur->toLower().ch() - (int)(uint8_t)Byte::toLower(*bCur))) return c;
        aCur++;
        bCur++;
      }
    }
  }
  else
  {
    if (bLen < aLen) aEnd = aCur + bLen;

    if (cs == CaseSensitive)
    {
      for (; aCur != aEnd; aCur++, bCur++)
        if ((c = (int)aCur->ch() - (int)(uint8_t)*bCur)) return c;
    }
    else
    {
      for (; aCur != aEnd; aCur++, bCur++)
        if ((c = (int)aCur->toLower().ch() - (int)(uint8_t)Byte::toLower(*bCur))) return c;
    }

    return (int)((sysint_t)aLen - (sysint_t)bLen);
  }
}

int String::compare(const Utf16& other, uint cs) const
{
  sysuint_t aLen = getLength();
  sysuint_t bLen = other.getLength();
  const Char* aCur = cData();
  const Char* aEnd = aCur + aLen;
  const Char* bCur = other.getData();

  int c;

  if (bLen == DetectLength)
  {
    if (cs == CaseSensitive)
    {
      for (;;)
      {
        if (FOG_UNLIKELY(aCur == aEnd)) return *bCur ? -1 : 0;
        if ((c = (int)aCur->ch() - (int)bCur->ch())) return c;
        aCur++;
        bCur++;
      }
    }
    else
    {
      for (;;)
      {
        if (FOG_UNLIKELY(aCur == aEnd)) return *bCur ? -1 : 0;
        if ((c = (int)aCur->toLower().ch() - (int)bCur->toLower().ch())) return c;
        aCur++;
        bCur++;
      }
    }
  }
  else
  {
    if (bLen < aLen) aEnd = aCur + bLen;

    if (cs == CaseSensitive)
    {
      for (; aCur != aEnd; aCur++, bCur++)
        if ((c = (int)aCur->ch() - (int)bCur->ch())) return c;
    }
    else
    {
      for (; aCur != aEnd; aCur++, bCur++)
        if ((c = (int)aCur->toLower().ch() - (int)bCur->toLower().ch())) return c;
    }

    return (int)((sysint_t)aLen - (sysint_t)bLen);
  }
}

int String::compare(const String& other, uint cs) const
{
  sysuint_t aLen = getLength();
  sysuint_t bLen = other.getLength();
  const Char* aCur = cData();
  const Char* aEnd = aCur + aLen;
  const Char* bCur = other.cData();

  int c;
  if (bLen < aLen) aEnd = aCur + bLen;

  if (cs == CaseSensitive)
  {
    for (; aCur != aEnd; aCur++, bCur++)
      if ((c = (int)aCur->ch() - (int)bCur->ch())) return c;
  }
  else
  {
    for (; aCur != aEnd; aCur++, bCur++)
      if ((c = (int)aCur->toLower().ch() - (int)bCur->toLower().ch())) return c;
  }

  return (int)((sysint_t)aLen - (sysint_t)bLen);
}

// ============================================================================
// [Fog::String::Utf16]
// ============================================================================

err_t String::validateUtf16(sysuint_t* invalidPos) const
{
  return StringUtil::validateUtf16(cData(), getLength());
}

err_t String::getNumUtf16Chars(sysuint_t* charsCount) const
{
  return StringUtil::getNumUtf16Chars(cData(), getLength(), charsCount);
}

// ============================================================================
// [Fog::String::FileSystem]
// ============================================================================

err_t String::slashesToPosix()
{
  return replace(Char('\\'), Char('/'));
}

err_t String::slashesToWin()
{
  return replace(Char('/'), Char('\\'));
}

// ============================================================================
// [Fog::String::Hash]
// ============================================================================

uint32_t String::getHashCode() const
{
  uint32_t h = _d->hashCode;
  if (h) return h;

  return (_d->hashCode = HashUtil::hashString(cData(), getLength()));
}

// ============================================================================
// [Fog::String::Data]
// ============================================================================

String::Data* String::Data::ref() const
{
  if ((flags & IsSharable) != 0)
    return refAlways();
  else
    return copy(this);
}

void String::Data::deref()
{
  derefInline();
}

String::Data* String::Data::adopt(void* address, sysuint_t capacity)
{
  if (capacity == 0) return String::sharedNull->refAlways();

  Data* d = (Data*)address;
  d->refCount.init(1);
  d->flags = 0;
  d->hashCode = 0;
  d->length = 0;
  d->capacity = capacity;
  d->data[0] = 0;

  return d;
}

String::Data* String::Data::adopt(void* address, sysuint_t capacity, const char* str, sysuint_t length)
{
  if (length == DetectLength) length = StringUtil::len(str);

  if (length <= capacity)
  {
    Data* d = adopt(address, capacity);
    d->length = length;
    StringUtil::copy(d->data, str, length);
    d->data[length] = 0;
    return d;
  }
  else
  {
    return alloc(0, str, length);
  }
}

String::Data* String::Data::adopt(void* address, sysuint_t capacity, const Char* str, sysuint_t length)
{
  if (length == DetectLength) length = StringUtil::len(str);

  if (length <= capacity)
  {
    Data* d = adopt(address, capacity);
    d->length = length;
    StringUtil::copy(d->data, str, length);
    d->data[length] = Char(0);
    return d;
  }
  else
  {
    return alloc(0, str, length);
  }
}

String::Data* String::Data::alloc(sysuint_t capacity)
{
  if (capacity == 0) return String::sharedNull->refAlways();

  // Pad to 16 bytes (8 chars).
  capacity = (capacity + 7) & ~7;

  sysuint_t dsize = sizeFor(capacity);
  Data* d = (Data *)Memory::alloc(dsize);
  if (!d) return NULL;

  d->refCount.init(1);
  d->flags = IsDynamic | IsSharable;
  d->hashCode = 0;
  d->capacity = capacity;
  d->length = 0;

  return d;
}

String::Data* String::Data::alloc(sysuint_t capacity, const char* str, sysuint_t length)
{
  if (length == DetectLength) length = StringUtil::len(str);
  if (length > capacity) capacity = length;

  if (capacity == 0) return String::sharedNull->refAlways();

  Data* d = alloc(capacity);
  if (!d) return NULL;

  d->length = length;
  StringUtil::copy(d->data, str, length);
  d->data[length] = Char(0);

  return d;
}

String::Data* String::Data::alloc(sysuint_t capacity, const Char* str, sysuint_t length)
{
  if (length == DetectLength) length = StringUtil::len(str);
  if (length > capacity) capacity = length;

  if (capacity == 0) return String::sharedNull->refAlways();

  Data* d = alloc(capacity);
  if (!d) return NULL;

  d->length = length;
  StringUtil::copy(d->data, str, length);
  d->data[length] = Char(0);

  return d;
}

String::Data* String::Data::realloc(Data* d, sysuint_t capacity)
{
  FOG_ASSERT(capacity >= d->length);

  sysuint_t dsize = String::Data::sizeFor(capacity);

  if ((d->flags & String::Data::IsDynamic) != 0)
  {
    if ((d = (Data *)Memory::realloc((void*)d, dsize)) != NULL)
    {
      d->capacity = capacity;
    }
  }
  else
  {
    Data* newd = alloc(capacity, d->data, d->length);
    if (!newd) return NULL;

    d->deref();
    d = newd;
  }

  return d;
}

String::Data* String::Data::copy(const Data* d)
{
  return alloc(0, d->data, d->length);
}

void String::Data::free(Data* d)
{
  Memory::free((void*)d);
}

Static<String::Data> String::sharedNull;

} // Fog namespace

FOG_INIT_DECLARE err_t fog_string_init(void)
{
  using namespace Fog;

  String::Data* d = String::sharedNull.instancep();
  d->refCount.init(1);
  d->flags |= String::Data::IsNull | String::Data::IsSharable;
  d->hashCode = 0;
  d->capacity = 0;
  d->length = 0;
  memset(d->data, 0, sizeof(d->data));

  return Error::Ok;
}

FOG_INIT_DECLARE void fog_string_shutdown(void)
{
}
