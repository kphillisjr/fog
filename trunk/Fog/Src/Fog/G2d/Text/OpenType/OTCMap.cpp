// [Fog-G2d]
//
// [License]
// MIT, See COPYING file in package

// [Dependencies]
#include <Fog/Core/Memory/MemMgr.h>
#include <Fog/Core/Memory/MemOps.h>
#include <Fog/Core/Tools/Logger.h>
#include <Fog/G2d/Text/OpenType/OTCMap.h>
#include <Fog/G2d/Text/OpenType/OTEnum.h>

namespace Fog {

// [Byte-Pack]
#include <Fog/Core/C++/PackByte.h>

// ============================================================================
// [Fog::OTCMapFormat0]
// ============================================================================

// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt16       | 0            | format               | Format number is set to 0.
// UInt16       | 2            | length               | Table length in bytes.
// UInt16       | 4            | macLanguageCode      | Mac language code.
// UInt8[256]   | 6            | glyphIdArray         | Glyph index array (256).

// ============================================================================
// [Fog::OTCMapFormat2]
// ============================================================================

// ============================================================================
// [Fog::OTCMapFormat4]
// ============================================================================

// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt16       | 0            | format               | Format number is set to 4.
// UInt16       | 2            | length               | Table length in bytes.
// UInt16       | 4            | macLanguageCode      | Mac language code.
// UInt16       | 6            | numSegX2 (N * 2)     | Number of segments * 2.
// UInt16       | 8            | searchRange          | (2^floor(log2(N))) * 2   == 2*1(LogSegs).
// UInt16       | 10           | entrySelector        | log2(searchRange / 2)    == LogSegs.
// UInt16       | 12           | rangeShift           | N*2 - searchRange.
// UInt16[N]    | 14           | endCount             | End characterCode for each segment, last=0xFFFF.
// UInt16       | 14+N*2       | reservedPad          | Padding, set to 0.
// UInt16[N]    | 16+N*2       | startCount           | Start character code for each segment.
// UInt16[N]    | 16+N*4       | idDelta              | Delta for all character codes in segment.
// UInt16[N]    | 16+N*6       | idRangeOffset        | Offsets into glyphIdArray or 0.
// UInt16[]     | 16+N*8       | glyphIdArray         | Glyph index array (arbitrary length).

struct FOG_NO_EXPORT CMapFormat4
{
  OTUInt16 format;
  OTUInt16 length;
  OTUInt16 macLanguageCode;
  OTUInt16 numSegX2;
  OTUInt16 searchRange;
  OTUInt16 entrySelector;
  OTUInt16 rangeShift;
};

static size_t FOG_CDECL OTCMapContext_getGlyphPlacement4(OTCMapContext* ctx,
  uint32_t* glyphList, size_t glyphAdvance, const uint16_t* sData, size_t sLength)
{
  const uint8_t* data = static_cast<const uint8_t*>(ctx->_data);
  const CMapFormat4* tab = reinterpret_cast<const CMapFormat4*>(data);

  FOG_ASSERT(tab->format.getValueA() == 4);

  uint32_t length = tab->length.getValueA();
  uint32_t numSeg = tab->numSegX2.getValueA() >> 1;
  uint32_t searchRange = tab->searchRange.getValueA() >> 1;
  uint32_t entrySelector = tab->entrySelector.getValueA();
  uint32_t rangeShift = tab->rangeShift.getValueA() >> 1;

  const uint16_t* sMark = sData;
  const uint16_t* sEnd = sData + sLength;

  while (sData < sEnd)
  {
    uint32_t uc = sData[0];
    uint32_t ut;
    uint32_t glyphId = 0;

    const uint8_t* p;

    if (uc > 0xFFFF)
      goto _GlyphDone;

    uint32_t start, end;
    uint32_t endCount = 14;
    uint32_t search = endCount;
    
    // [endCount, endCount + segCount].
    p = data + search;
    ut = FOG_OT_UINT16(p + rangeShift * 2)->getValueA();
    if (uc >= ut)
      search += rangeShift * 2;

    search -= 2;
    while (entrySelector)
    {
      searchRange >>= 1;
      start = FOG_OT_UINT16(p + searchRange * 2 + numSeg * 2 + 2)->getValueA();
      end   = FOG_OT_UINT16(p + searchRange * 2)->getValueA();

      if (uc > end)
        search += searchRange * 2;
      entrySelector--;
    }

    search += 2;
    ut = ((search - endCount) >> 1) & 0xFFFF;
    FOG_ASSERT(uc <= FOG_OT_UINT16(data + endCount + ut * 2)->getValueA());

    p = data + 16 + ut * 2;
    start = FOG_OT_UINT16(p + numSeg * 2)->getValueA();
    end   = FOG_OT_UINT16(p)->getValueA();

    if (uc < start)
      goto _GlyphDone;

    uint32_t offset = FOG_OT_UINT16(p + numSeg * 6)->getValueA();
    if (offset == 0)
      glyphId = (uc + FOG_OT_UINT16(p + numSeg * 4)->getValueA()) & 0xFFFF;
    else
      glyphId = FOG_OT_UINT16(p + offset + (uc - start) * 2 + numSeg * 6)->getValueA();

_GlyphDone:
    glyphList[0] = glyphId;
    glyphList = reinterpret_cast<uint32_t*>((uint8_t*)glyphList + glyphAdvance);

    sData++;
  }

  return (size_t)(sData - sMark);
}

// ============================================================================
// [Fog::OTCMapFormat6]
// ============================================================================

// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt16       | 0            | format               | Format number is set to 6.
// UInt16       | 2            | length               | Table length in bytes.
// UInt16       | 4            | macLanguageCode      | Mac language code.
// UInt16       | 6            | first                | First segment code.
// UInt16       | 8            | count (N)            | Segment size in bytes.
// UInt16[N]    | 10           | glyphIdArray         | Glyph index array.

// ============================================================================
// [Fog::OTCMapFormat8]
// ============================================================================

// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt16       | 0            | format               | Format number is set to 8.
// UInt16       | 2            | reserved             | Reserved.
// UInt32       | 4            | length               | Table length in bytes.
// UInt32       | 8            | macLanguageCode      | Mac language code.
// UInt8[8192]  | 12           | is32                 | Tightly packed array of bits indicating
//              |              |                      | whether the particular 16-bit (index) value
//              |              |                      | is the start of a 32-bit character code.
// UInt32       | 8204         | count (N)            | Number of groupings which follow.
//
// The header is followed by N groups in the following format:
//
// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt32       | 0            | startChar            | Start character code.
// UInt32       | 4            | lastChar             | Last character code.
// UInt32       | 8            | startGlyph           | Start glyph ID for the group.

// ============================================================================
// [Fog::OTCMapFormat10]
// ============================================================================

// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt16       | 0            | format               | Format number is set to 10.
// UInt16       | 2            | reserved             | Reserved.
// UInt32       | 4            | length               | Table length in bytes.
// UInt32       | 8            | macLanguageCode      | Mac language code.
// UInt32       | 12           | start                | First character in range.
// UInt32       | 16           | count (N)            | Number of characters in range.
// UInt16[N]    | 20           | glyphIdArray         | Glyph index array.

// ============================================================================
// [Fog::OTCMapFormat12]
// ============================================================================

// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt16       | 0            | format               | Format number is set to 12.
// UInt16       | 2            | reserved             | Reserved.
// UInt32       | 4            | length               | Table length in bytes.
// UInt32       | 8            | macLanguageCode      | Mac language code.
// UInt32       | 12           | count (N)            | Number of groups.
//
// The header is followed by N groups in the following format:
//
// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt32       | 0            | start                | First character code.
// UInt32       | 4            | end                  | Last character code.
// UInt32       | 8            | startId              | Start glyph ID for the group.

// ============================================================================
// [Fog::OTCMapFormat13]
// ============================================================================

// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt16       | 0            | format               | Format number is set to 13.
// UInt16       | 2            | reserved             | Reserved.
// UInt32       | 4            | length               | Table length in bytes.
// UInt32       | 8            | macLanguageCode      | Mac language code.
// UInt32       | 12           | count (N)            | Number of groups.
//
// The header is followed by N groups in the following format:
//
// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt32       | 0            | start                | First character code.
// UInt32       | 4            | end                  | Last character code.
// UInt32       | 8            | glyphId              | Glyph ID for the whole group.

// ============================================================================
// [Fog::OTCMapFormat14]
// ============================================================================

// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt16       | 0            | format               | Format number is set to 14.
// UInt32       | 2            | length               | Table length in bytes.
// UInt32       | 8            | numSelector (N)      | Number of variation selection records.
//
// The header is followed by N selection records in the following format:
//
// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt24       | 0            | varSelector          | Unicode code-point of selection.
// UInt32       | 3            | defaultOffset        | Offset to a default UVS table.
// UInt32       | 7            | nonDefaultOffset     | Offset to a non-default UVS table.
//
// Selectors are sorted by code-point.
//
// A default Unicode Variation Selector (UVS) subtable is just a list of ranges
// of code-points which are to be found in the standard cmap. No glyph IDs here:
//
// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt32       | 0            | numRanges            | Number of ranges following.
// -------------+--------------+----------------------+------------------------
// UInt24       | 0            | uniStart             | First code-point in range.
// UInt8        | 3            | additionalCount      | Count of additional characters (can be zero).
//
// Ranges are sorted by uniStart:
//
// Type         | Offset       | Name                 | Description
// -------------+--------------+----------------------+------------------------
// UInt32       | 0            | numMappings          | Number of mappings.
// -------------+--------------+----------------------+------------------------
// UInt24       | 0            | uniStart             | First code-point in range.
// UInt16       | 3            | glyphID              | Glyph ID of the first character.

// [Byte-Pack]
#include <Fog/Core/C++/PackRestore.h>

// ============================================================================
// [Fog::OTCMapTable]
// ============================================================================

static void FOG_CDECL OTCMapTable_destroy(OTCMapTable* self);

static err_t OTCMapContext_init(OTCMapContext* ctx, const OTCMapTable* cmap, uint32_t language);

static err_t FOG_CDECL OTCMapTable_init(OTCMapTable* self)
{
  const uint8_t* data = self->getData();
  uint32_t dataLength = self->getDataLength();

#if defined(FOG_OT_DEBUG)
  Logger::info("Fog::OTCMapTable", "init", 
    "Init 'cmap' table, length %u bytes.", dataLength);
#endif // FOG_OT_DEBUG

  FOG_ASSERT_X(self->_tag == FOG_OT_TAG('c', 'm', 'a', 'p'),
    "Fog::OTCMapTable::init() - Called on invalid table.");

  self->count = 0;
  self->items = NULL;
  self->_destroy = (OTTableDestroyFunc)OTCMapTable_destroy;

  // --------------------------------------------------------------------------
  // [Header]
  // --------------------------------------------------------------------------

  const OTCMapHeader* header = reinterpret_cast<const OTCMapHeader*>(data);
  if (header->version.getRawA() != 0x0000)
  {
#if defined(FOG_OT_DEBUG)
    Logger::info("Fog::OTCMapTable", "init",
      "Unsupported 'cmap' version %d.", header->version.getRawA());
#endif // FOG_OT_DEBUG

    return ERR_FONT_OT_CMAP_UNSUPPORTED_FORMAT;
  }

  // --------------------------------------------------------------------------
  // [Encoding]
  // --------------------------------------------------------------------------

  uint32_t count = header->count.getValueA();
  if (count == 0 || count > dataLength / sizeof(OTCMapEncoding))
  {
#if defined(FOG_OT_DEBUG)
    Logger::info("Fog::OTCMapTable", "init",
      "Corrupted 'cmap' table, count of subtables reported to by %u.", count);
#endif // FOG_OT_DEBUG

    return ERR_FONT_OT_CMAP_CORRUPTED;
  }

  const OTCMapEncoding* encTable = reinterpret_cast<const OTCMapEncoding*>(
    data + sizeof(OTCMapHeader));

  OTCMapItem* items = static_cast<OTCMapItem*>(
    MemMgr::alloc(count * sizeof(OTCMapItem)));

  if (FOG_IS_NULL(items))
    return ERR_RT_OUT_OF_MEMORY;

#if defined(FOG_OT_DEBUG)
  Logger::info("Fog::OTCMapTable", "init",
    "Found %u subtables.", count);
#endif // FOG_OT_DEBUG

  uint32_t unicodeEncodingIndex = 0xFFFFFFFF;
  uint32_t unicodeEncodingPriority = 0xFFFFFFFF;

  uint32_t minOffsetValue = sizeof(OTCMapHeader) + count * sizeof(OTCMapEncoding);

  self->count = count;
  self->items = items;

  for (uint32_t i = 0; i < count; i++, items++, encTable++)
  {
    uint16_t platformId = encTable->platformId.getValueA();
    uint16_t specificId = encTable->specificId.getValueA();
    uint32_t offset     = encTable->offset.getValueA();

    // This is our encodingId which can match several platformId/specificId
    // combinations. The purpose is to simplify matching of subtables we are
    // interested it. Generally we are interested mainly of unicode tables.
    uint32_t encodingId = OT_ENCODING_ID_NONE;

    // The priority of encodingId when matching it against encodingId in tables.
    // Higher priority is set by subtables with more features. For example MS
    // UCS4 encoding has higher priority than UNICODE.
    uint32_t priority = 0;

    switch (platformId)
    {
      case OT_PLATFORM_ID_UNICODE:
        encodingId = OT_ENCODING_ID_UNICODE;
        break;

      case OT_PLATFORM_ID_MAC:
        if (specificId == OT_MAC_ID_ROMAN)
          encodingId = OT_ENCODING_ID_MAC_ROMAN;
        break;

      case OT_PLATFORM_ID_MS:
        switch (specificId)
        {
          case OT_MS_ID_SYMBOL : encodingId = OT_ENCODING_ID_MS_SYMBOL; break;
          case OT_MS_ID_UNICODE: encodingId = OT_ENCODING_ID_UNICODE  ; priority = 1; break;
          case OT_MS_ID_SJIS   : encodingId = OT_ENCODING_ID_SJIS     ; break;
          case OT_MS_ID_GB2312 : encodingId = OT_ENCODING_ID_GB2312   ; break;
          case OT_MS_ID_BIG5   : encodingId = OT_ENCODING_ID_BIG5     ; break;
          case OT_MS_ID_WANSUNG: encodingId = OT_ENCODING_ID_WANSUNG  ; break;
          case OT_MS_ID_JOHAB  : encodingId = OT_ENCODING_ID_JOHAB    ; break;
          case OT_MS_ID_UCS4   : encodingId = OT_ENCODING_ID_UNICODE  ; break;
          default: break;
        }
        break;

      default:
        // Unknown.
        break;
    }

    if ((encodingId == FOG_OT_TAG('u', 'n', 'i', 'c')) &&
        (unicodeEncodingIndex == 0xFFFFFFFF || unicodeEncodingPriority < priority))
    {
      unicodeEncodingIndex = i;
      unicodeEncodingPriority = priority;
    }

    items->encodingId = encodingId;
    items->priority = priority;
    items->status = OT_NOT_VALIDATED;

#if defined(FOG_OT_DEBUG)
    Logger::info("Fog::OTCMapTable", "init", 
      "#%02u - platformId=%u, specificId=%u => [encoding='%c%c%c%c', priority=%u], offset=%u",
      i,
      platformId,
      specificId,
      (encodingId >> 24) & 0xFF,
      (encodingId >> 16) & 0xFF,
      (encodingId >>  8) & 0xFF,
      (encodingId      ) & 0xFF,
      priority,
      offset);
#endif // FOG_OT_DEBUG

    if (offset < minOffsetValue || offset >= dataLength)
    {
#if defined(FOG_OT_DEBUG)
      Logger::info("Fog::OTCMapTable", "init", 
        "#%02u - corrupted offset=%u", i, offset);
#endif // FOG_OT_DEBUG
      items->status = ERR_FONT_OT_CMAP_CORRUPTED;
    }
  }

  self->unicodeEncodingIndex = unicodeEncodingIndex;
  self->_initContext = OTCMapContext_init;

  return ERR_OK;
}

static void FOG_CDECL OTCMapTable_destroy(OTCMapTable* self)
{
  if (self->items)
  {
    MemMgr::free(self->items);
    self->items = NULL;
  }

  // This results in crash in case that destroy is called twice by accident.
  self->_destroy = NULL;
}

// ============================================================================
// [Fog::OTCMapContext]
// ============================================================================

static err_t FOG_CDECL OTCMapContext_init(OTCMapContext* ctx, const OTCMapTable* cMapTable, uint32_t encodingId)
{
  uint32_t index = 0xFFFFFFFF;

  const OTCMapItem* items = cMapTable->items;
  uint32_t count = cMapTable->count;

  if (encodingId == FOG_OT_TAG('u', 'n', 'i', 'c'))
  {
    index = cMapTable->unicodeEncodingIndex;
    FOG_ASSERT(index < count);
  }
  else
  {
    uint32_t priority = 0xFFFFFFFF;

    for (uint32_t i = 0; i < count; i++)
    {
      if (items[i].encodingId == encodingId)
      {
        if (index == 0xFFFFFFFF || priority < items[i].priority)
        {
          index = i;
          priority = items[i].priority;
        }
      }
    }
  }

  if (index == 0xFFFFFFFF)
    return ERR_FONT_OT_CMAP_ENCODING_NOT_FOUND;

  const uint8_t* data = cMapTable->getData();
  const OTCMapEncoding* encTable = reinterpret_cast<const OTCMapEncoding*>(
    data + sizeof(OTCMapHeader) + index * sizeof(OTCMapEncoding));

  uint32_t offset = encTable->offset.getValueA();
  FOG_ASSERT(offset < cMapTable->getDataLength());

  // Format of the table is the first UInt16 data entry in that offset.
  uint16_t format = reinterpret_cast<const OTUInt16*>(data + offset)->getValueA();
  switch (format)
  {
    case 4:
      ctx->_getGlyphPlacementFunc = OTCMapContext_getGlyphPlacement4;
      ctx->_data = (void*)(data + offset);
      return ERR_OK;

    default:
      return ERR_FONT_OT_CMAP_UNSUPPORTED_VERSION;
  }
}

// ============================================================================
// [Init / Fini]
// ============================================================================

FOG_NO_EXPORT void OTCMap_init(void)
{
  OTApi& api = fog_ot_api;

  // --------------------------------------------------------------------------
  // [OTCMap]
  // --------------------------------------------------------------------------
  
  api.otcmaptable_init = OTCMapTable_init;
}

} // Fog namespace