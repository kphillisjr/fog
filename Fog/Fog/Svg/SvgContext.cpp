// [Fog/Svg Library - C++ API]
//
// [Licence]
// MIT, See COPYING file in package

// [Dependencies]
#include <Fog/Svg/SvgContext.h>
#include <Fog/Svg/SvgDom.h>

namespace Fog {

// ============================================================================
// [Fog::SvgContext]
// ============================================================================

SvgContext::SvgContext(Painter* painter) :
  _painter(painter)
{
  _lineStyle.type = SVG_SOURCE_NONE;
  _fillStyle.type = SVG_SOURCE_COLOR;
  _fillMode = FILL_NON_ZERO;

  setDpi(90.0);
}

SvgContext::~SvgContext()
{
}

void SvgContext::setDpi(double dpi)
{
  _dpi = dpi;

  _translateCoordData[SVG_UNIT_NONE] = 1.0;
  _translateCoordData[SVG_UNIT_PERCENT] = 1.0;

  // SVG TODO: Em and Ex units.
  _translateCoordData[SVG_UNIT_CM] = dpi * 0.3937007777777778;
  _translateCoordData[SVG_UNIT_EM] = 1.0;
  _translateCoordData[SVG_UNIT_EX] = 1.0;
  _translateCoordData[SVG_UNIT_IN] = dpi;
  _translateCoordData[SVG_UNIT_MM] = dpi * 0.0393700777777778;
  _translateCoordData[SVG_UNIT_PC] = dpi * 0.1666666666666667;
  _translateCoordData[SVG_UNIT_PT] = dpi * 0.0138888888888889;
  _translateCoordData[SVG_UNIT_PX] = 1.0;
}

void SvgContext::drawEllipse(const PointD& cp, const PointD& r)
{
  if (_fillStyle.type != SVG_SOURCE_NONE)
  {
    setupFillStyle();
    _painter->fillEllipse(cp, r);
  }

  if (_lineStyle.type != SVG_SOURCE_NONE)
  {
    setupStrokeStyle();
    _painter->drawEllipse(cp, r);
  }
}

void SvgContext::drawLine(const PointD& p1, const PointD& p2)
{
  if (_lineStyle.type != SVG_SOURCE_NONE)
  {
    setupStrokeStyle();
    _painter->drawLine(p1, p2);
  }
}

void SvgContext::drawRect(const RectD& rect)
{
  if (_fillStyle.type != SVG_SOURCE_NONE)
  {
    setupFillStyle();
    _painter->fillRect(rect);
  }

  if (_lineStyle.type != SVG_SOURCE_NONE)
  {
    setupStrokeStyle();
    _painter->drawRect(rect);
  }
}

void SvgContext::drawRound(const RectD& rect, const PointD& r)
{
  if (_fillStyle.type != SVG_SOURCE_NONE)
  {
    setupFillStyle();
    _painter->fillRound(rect, r);
  }

  if (_lineStyle.type != SVG_SOURCE_NONE)
  {
    setupStrokeStyle();
    _painter->drawRound(rect, r);
  }
}

void SvgContext::drawPath(const Path& path)
{
  if (_fillStyle.type != SVG_SOURCE_NONE)
  {
    setupFillStyle();
    _painter->fillPath(path);
  }

  if (_lineStyle.type != SVG_SOURCE_NONE)
  {
    setupStrokeStyle();
    _painter->drawPath(path);
  }
}

void SvgContext::blitImage(const PointD& pt, const Image& im)
{
  _painter->blitImage(pt, im);
}

} // Fog namespace
