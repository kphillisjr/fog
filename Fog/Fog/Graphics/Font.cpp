// [Fog/Graphics Library - C++ API]
//
// [Licence] 
// MIT, See COPYING file in package

// [Precompiled headers]
#ifdef FOG_PRECOMP
#include FOG_PRECOMP
#endif

// [Dependencies]
#include <Fog/Core/AutoLock.h>
#include <Fog/Core/FileSystem.h>
#include <Fog/Core/FileUtil.h>
#include <Fog/Core/Hash.h>
#include <Fog/Core/Lock.h>
#include <Fog/Core/Memory.h>
#include <Fog/Core/OS.h>
#include <Fog/Core/Static.h>
#include <Fog/Core/String.h>
#include <Fog/Core/UserInfo.h>
#include <Fog/Graphics/Constants.h>
#include <Fog/Graphics/Error.h>
#include <Fog/Graphics/Font.h>
#include <Fog/Graphics/Font_FreeType.h>
#include <Fog/Graphics/Font_Win.h>
#include <Fog/Graphics/Glyph.h>
#include <Fog/Graphics/GlyphCache.h>
#include <Fog/Graphics/Rgba.h>

namespace Fog {

// ============================================================================
// [Fog::Font_Local]
// ============================================================================

struct Font_Local
{
  Lock lock;
  Vector<String32> paths;
  Vector<String32> list;
  bool listInitialized;
};

static Static<Font_Local> font_local;

// ============================================================================
// [Fog::FontFace]
// ============================================================================

FontFace::FontFace()
{
  refCount.init(1);
  memset(&metrics, 0, sizeof(FontMetrics));
  memset(&attributes, 0, sizeof(FontAttributes));
}

FontFace::~FontFace()
{
}

FontFace* FontFace::ref()
{
  refCount.inc();
  return this;
}

void FontFace::deref()
{
  if (refCount.deref()) delete this;
}

// ============================================================================
// [Fog::Font]
// ============================================================================

Font::Data* Font::sharedNull;
FontEngine* Font::_engine;

static err_t _setFace(Font* self, FontFace* face)
{
  if (!face) return Error::FontInvalidFace;
  if (self->_d->face == face) return Error::Ok;

  err_t err = self->detach();
  if (err) return err;

  if (self->_d->face) self->_d->face->deref();
  self->_d->face = face;

  return Error::Ok;
}

Font::Font() :
  _d(sharedNull->ref())
{
}

Font::Font(const Font& other) :
  _d(other._d->ref())
{
}

Font::Font(Data* d) :
  _d(d)
{
}

Font::~Font()
{
  _d->deref();
}

err_t Font::_detach()
{
  if (_d->refCount.get() > 1)
  {
    Data* newd = Data::copy(_d);
    if (!newd) return Error::OutOfMemory;
    AtomicBase::ptr_setXchg(&_d, newd)->deref();
  }
  return Error::Ok;
}

void Font::free()
{
  AtomicBase::ptr_setXchg(&_d, sharedNull->ref())->deref();
}

err_t Font::setFamily(const String32& family)
{
  return _setFace(this, _engine->getFace(family, size(), attributes()));
}

err_t Font::setFamily(const String32& family, uint32_t size)
{
  return _setFace(this, _engine->getFace(family, size, attributes()));
}

err_t Font::setSize(uint32_t size)
{
  return _setFace(this, _engine->getFace(family(), size, attributes()));
}

err_t Font::setAttributes(const FontAttributes& a)
{
  return _setFace(this, _engine->getFace(family(), size(), a));
}

err_t Font::setBold(bool val)
{
  if (isBold() == val) return Error::Ok;

  FontAttributes a = attributes();
  a.bold = val;

  return _setFace(this, _engine->getFace(family(), size(), a));
}

err_t Font::setItalic(bool val)
{
  if (isItalic() == val) return Error::Ok;

  FontAttributes a = attributes();
  a.italic = val;

  return _setFace(this, _engine->getFace(family(), size(), a));
}

err_t Font::setStrike(bool val)
{
  if (isStrike() == val) return Error::Ok;

  FontAttributes a = attributes();
  a.strike = val;

  return _setFace(this, _engine->getFace(family(), size(), a));
}

err_t Font::setUnderline(bool val)
{
  if (isUnderline() == val) return Error::Ok;

  FontAttributes a = attributes();
  a.underline = val;

  return _setFace(this, _engine->getFace(family(), size(), a));
}

err_t Font::set(const Font& other)
{
  AtomicBase::ptr_setXchg(&_d, other._d->ref())->deref();
  return Error::Ok;
}

err_t Font::getTextWidth(const String32& str, TextWidth* textWidth) const
{
  return _d->face->getTextWidth(str.cData(), str.length(), textWidth);
}

err_t Font::getTextWidth(const Char32* str, sysuint_t length, TextWidth* textWidth) const
{
  return _d->face->getTextWidth(str, length, textWidth);
}

err_t Font::getGlyphs(const Char32* str, sysuint_t length, GlyphSet& glyphSet) const
{
  return _d->face->getGlyphs(str, length, glyphSet);
}

err_t Font::getPath(const Char32* str, sysuint_t length, Path& dst) const
{
  return _d->face->getPath(str, length, dst);
}

bool Font::addFontPath(const String32& path)
{
  AutoLock locked(font_local.instance().lock);

  if (!font_local.instance().paths.contains(path) &&
      FileSystem::isDirectory(path))
  {
    font_local.instance().paths.append(path);
    return true;
  }
  else
    return false;
}

void Font::addFontPaths(const Sequence<String32>& paths)
{
  AutoLock locked(font_local.instance().lock);

  Sequence<String32>::ConstIterator it(paths);
  for (; it.isValid(); it.toNext())
  {
    String32 path = it.value();
    if (!font_local.instance().paths.contains(path) &&
        FileSystem::isDirectory(path))
    {
      font_local.instance().paths.append(path);
    }
  }
}

bool Font::removeFontPath(const String32& path)
{
  AutoLock locked(font_local.instance().lock);
  return font_local.instance().paths.remove(path) != 0;
}

bool Font::hasFontPath(const String32& path)
{
  AutoLock locked(font_local.instance().lock);
  return font_local.instance().paths.contains(path);
}

bool Font::findFontFile(const String32& fileName, String32& dest)
{
  AutoLock locked(font_local.instance().lock);
  return FileSystem::findFile(font_local.instance().paths, fileName, dest);
}

Vector<String32> Font::fontPaths()
{
  AutoLock locked(font_local.instance().lock);
  return font_local.instance().paths;
}

Vector<String32> Font::fontList()
{
  AutoLock locked(font_local.instance().lock);
  if (!font_local.instance().listInitialized)
  {
    font_local.instance().list.append(_engine->getFonts());
    font_local.instance().listInitialized = true;
  }
  return font_local.instance().list;
}

// ============================================================================
// [Font::Data]
// ============================================================================

Font::Data::Data()
{
  refCount.init(1);
  face = NULL;
}

Font::Data::~Data()
{
  if (face) face->deref();
}

Font::Data* Font::Data::ref() const
{
  refCount.inc();
  return const_cast<Data*>(this);
}

void Font::Data::deref()
{
  if (refCount.deref()) delete this;
}

Font::Data* Font::Data::copy(Data* d)
{
  Data* newd = new(std::nothrow) Data();
  if (!newd) return NULL;

  newd->face = d->face ? d->face->ref() : NULL;
  return newd;
}

// ============================================================================
// [Fog::FontEngine]
// ============================================================================

FontEngine::FontEngine(const Fog::String32& name) :
  _name(name)
{
}

FontEngine::~FontEngine()
{
}

} // Fog namespace

// ============================================================================
// [Library Initializers]
// ============================================================================

FOG_INIT_EXTERN void fog_font_shutdown(void);

FOG_INIT_DECLARE err_t fog_font_init(void)
{
  using namespace Fog;

  // [Local]

  font_local.init();
  font_local.instance().listInitialized = false;

  err_t initResult = Error::Ok;

  // [Font Shared Null]

  Font::sharedNull = new Font::Data();

  // [Font Paths]

  // add $HOME and $HOME/fonts directories (standard in linux)
  // (can be for example symlink to real font path)
  String32 home = UserInfo::directory(UserInfo::Home);
  String32 homeFonts;

  FileUtil::joinPath(homeFonts, home, Ascii8("fonts"));

  font_local.instance().paths.append(home);
  font_local.instance().paths.append(homeFonts);

#if defined(FOG_OS_WINDOWS)
  // Add Windows standard font directory.
  String32 winFonts = OS::windowsDirectory();
  FileUtil::joinPath(winFonts, winFonts, Ascii8("fonts"));
  font_local.instance().paths.append(winFonts);
#endif // FOG_OS_WINDOWS

  // [Font Face Cache]

  // [Font Engine]

  Font::_engine = NULL;

#if defined(FOG_FONT_WINDOWS)
  if (!Font::_engine) Font::_engine = new FontEngineWin();
#endif // FOG_FONT_WINDOWS

#if defined(FOG_FONT_FREETYPE)
  if (!Font::_engine) Font::_engine = new FontEngineFT();
#endif // FOG_FONT_FREETYPE

  Font::sharedNull->face = Font::_engine->getDefaultFace();

  if (Font::sharedNull->face == NULL)
  {
    initResult = Error::FontCantLoadDefaultFace;
    goto __fail;
  }

  return initResult;

__fail:
  fog_font_shutdown();
  return initResult;
}

FOG_INIT_DECLARE void fog_font_shutdown(void)
{
  using namespace Fog;

  // [Font Shared Null]
  delete Font::sharedNull;
  Font::sharedNull = NULL;

  // [Font Face Cache]

  //Fog_Font_cleanup();
  //Fog_FontFace_cache->~Fog_FontFaceCache();

  // [Font Engine]

  if (Font::_engine)
  {
    delete Font::_engine;
    Font::_engine = NULL;
  }

  // [Local]

  font_local.destroy();
}






































































































#if 0









// mutex
// static Fog::Static<Fog::Lock> Fog_Font_mutex;

// Fog_Font_paths
// static uint8_t Fog_Font_paths_storage[sizeof(Fog::StringList)];
// static Fog::StringList* Fog_Font_paths;

// Fog_Font_list
// static uint8_t Fog_Font_list_storage[sizeof(Fog::StringList)];
// static Fog::StringList* Fog_Font_list;
// static bool Fog_Font_listInitialized = 0;

// ---------------------------------------------------------------------------
// Fog_FontFaceCache
// ---------------------------------------------------------------------------

struct Fog_FontFaceCacheEntry
{
  /*! @brief Font family name. */
  Fog::String32 _family;
  /*! @brief Font size. */
  uint32_t _size;
  /*! @brief Font attributes. Only BOLD and ITALIC!. */
  uint32_t _attributes;

  FOG_INLINE Fog_FontFaceCacheEntry() {}
  FOG_INLINE Fog_FontFaceCacheEntry(const Fog::String32& family, uint32_t size, uint32_t attributes)
    : _family(family), _size(size), _attributes(attributes & Fog::Font::Attribute_FontFaceMask) {}
  FOG_INLINE Fog_FontFaceCacheEntry(const Fog_FontFaceCacheEntry& other)
    : _family(other.family()), _size(other.size()), _attributes(other.attributes()) {}
  FOG_INLINE ~Fog_FontFaceCacheEntry() {}

  FOG_INLINE const Fog::String32& family() const { return _family; }
  FOG_INLINE uint32_t size() const { return _size; }
  FOG_INLINE uint32_t attributes() const { return _attributes; }
};

FOG_DECLARE_TYPEINFO(Fog_FontFaceCacheEntry, Fog::MoveableType)

static FOG_INLINE bool operator==(const Fog_FontFaceCacheEntry &a, const Fog_FontFaceCacheEntry &b)
{
  return a.family() == b.family() && a.size() == b.size() && a.attributes() == b.attributes();
}

static FOG_INLINE uint32_t Wde_hash(const Fog_FontFaceCacheEntry& entry)
{
  return Wde_hash(entry.family()) ^ (entry.size() * 133) ^ entry.attributes();
}

typedef Fog::Hash<Fog_FontFaceCacheEntry, Fog_FontFace*> Fog_FontFaceCache;

// font face cache storage
static uint8_t Fog_FontFace_cache_storage[sizeof(Fog_FontFaceCache)];
static Fog_FontFaceCache *Fog_FontFace_cache;

// returns a font face from cache
static Fog_FontFace* Fog_FontFaceCache_get(const Fog::String32& family, uint32_t size, uint32_t attributes)
{
  Fog::AutoLock locked(local.instance().cs);

  Fog_FontFaceCache::Node* node;
  Fog_FontFace* result = NULL;
  
  if ((node = Fog_FontFace_cache->get(Fog_FontFaceCacheEntry(family, size, attributes))) != NULL)
  {
    result = node->value();
    result->refCount.inc();
  }

  return result;
}

// puts a font face into cache
static void Fog_FontFaceCache_put(Fog_FontFace* face)
{
  Fog::AutoLock locked(local.instance().cs);

  Fog_FontFace_cache->put(Fog_FontFaceCacheEntry(face->family, face->metrics.size, face->attributes), face);
}

// cleans all font faces in cache that have reference count 0
static void Fog_FontFaceCache_cleanup(void)
{
  Fog::AutoLock locked(local.instance().cs);

  Fog_FontFaceCache::MutableIterator iterator(*Fog_FontFace_cache);
  for (iterator.begin(); iterator.exist();)
  {
    Fog_FontFace* face = iterator.value();
    if (face->refCount.get() == 0)
    {
      delete face;
      iterator.removeAndNext();
    }
    else
      iterator.next();
  }
}















// ---------------------------------------------------------------------------
// Fog::Font
// ---------------------------------------------------------------------------

// variaous text width functions
FOG_CAPI_DECLARE void Fog_Font_getTextWidth(const Fog::Font* self, const Fog::Char* text, sysuint_t count, Fog::TextWidth* textWidth)
{
  if (count)
  {
    Fog_Glyphs glyphs;
    glyphs.getGlyphs(*self, text, count);

    textWidth->beginWidth = glyphs.glyphs()[0].beginWidth();
    textWidth->advance = glyphs.advance();
    textWidth->endWidth = glyphs.glyphs()[count-1].endWidth();
  }
  else
  {
    textWidth->beginWidth = 0;
    textWidth->advance = 0;
    textWidth->endWidth = 0;
  }
}

FOG_CAPI_DECLARE void Fog_Font_getFontGlyphsWidth(const Fog::Font* self, const Fog::Glyph* glyphs, sysuint_t count, Fog::TextWidth* textWidth)
{
  if (count)
  {
    sysuint_t i;
    int advance = 0;

    for (i = 0; i != count; i++)
    {
      advance += glyphs[i].advance();
    }

    textWidth->beginWidth = glyphs[0].beginWidth();
    textWidth->advance = advance;
    textWidth->endWidth = glyphs[count-1].endWidth();
  }
  else
  {
    textWidth->beginWidth = 0;
    textWidth->advance = 0;
    textWidth->endWidth = 0;
  }
}






// ---------------------------------------------------------------------------
// font cleanup
// ---------------------------------------------------------------------------

FOG_CAPI_DECLARE void Fog_Font_cleanup(void)
{
  Fog_FontFaceCache_cleanup();
}
#endif
