// [Fog/Graphics Library - C++ API]
//
// [Licence] 
// MIT, See COPYING file in package

// [Guard]
#ifndef _FOG_GRAPHICS_IMAGEIO_GIF_H
#define _FOG_GRAPHICS_IMAGEIO_GIF_H

// [Dependencies]
#include <Fog/Graphics/ImageIO.h>

//! @addtogroup Fog_Graphics_ImageIO
//! @{

namespace Fog {
namespace ImageIO {

// ============================================================================
// [Fog::ImageIO::GifDecoderDevice]
// ============================================================================

struct FOG_API GifDecoderDevice : public DecoderDevice
{
  GifDecoderDevice();
  virtual ~GifDecoderDevice();

  virtual void reset();
  virtual uint32_t readHeader();
  virtual uint32_t readImage(Image& image);

private:
  void* _context;

  bool openGif();
  void closeGif();
};

// ============================================================================
// [Fog::ImageIO::GifEncoderDevice]
// ============================================================================

struct FOG_API GifEncoderDevice : public EncoderDevice
{
  GifEncoderDevice();
  virtual ~GifEncoderDevice();

  virtual uint32_t writeImage(const Image& image);
};

} // ImageIO namespace
} // Fog namespace

//! @}

// [Guard]
#endif // _FOG_GRAPHICS_IMAGEIO_GIF_H
