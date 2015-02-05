#ifndef MBGL_STYLE_TYPES
#define MBGL_STYLE_TYPES

#include <mbgl/util/enum.hpp>

#include <string>
#include <array>

namespace mbgl {

// Stores a premultiplied color, with all four channels ranging from 0..1
typedef std::array<float, 4> Color;

// -------------------------------------------------------------------------------------------------

enum class StyleLayerType : uint8_t {
    Unknown,
    Fill,
    Line,
    Symbol,
    Raster,
    Background
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(StyleLayerTypeClass, StyleLayerType, {
    { StyleLayerType::Unknown, "unknown" },
    { StyleLayerType::Fill, "fill" },
    { StyleLayerType::Line, "line" },
    { StyleLayerType::Symbol, "symbol" },
    { StyleLayerType::Raster, "raster" },
    { StyleLayerType::Background, "background" },
    { StyleLayerType(-1), "unknown" },
});
#else



MBGL_DEFINE_ENUM_CLASS_NAME(StyleLayerTypeClass, StyleLayerType, 7, { StyleLayerType::Unknown, "unknown" },
{ StyleLayerType::Fill, "fill" },
{ StyleLayerType::Line, "line" },
{ StyleLayerType::Symbol, "symbol" },
{ StyleLayerType::Raster, "raster" },
{ StyleLayerType::Background, "background" },
{ StyleLayerType(-1), "unknown" });
//MBGL_DEFINE_ENUM_CLASS_INIT(StyleLayerTypeClass, StyleLayerType, 0, StyleLayerType::Unknown, "unknown");
//MBGL_DEFINE_ENUM_CLASS_INIT(StyleLayerTypeClass, StyleLayerType, 1, StyleLayerType::Fill, "fill");
//MBGL_DEFINE_ENUM_CLASS_INIT(StyleLayerTypeClass, StyleLayerType, 2, StyleLayerType::Line, "line");
//MBGL_DEFINE_ENUM_CLASS_INIT(StyleLayerTypeClass, StyleLayerType, 3, StyleLayerType::Symbol, "symbol");
//MBGL_DEFINE_ENUM_CLASS_INIT(StyleLayerTypeClass, StyleLayerType, 4, StyleLayerType::Raster, "raster");
//MBGL_DEFINE_ENUM_CLASS_INIT(StyleLayerTypeClass, StyleLayerType, 5, StyleLayerType::Background, "background");
//MBGL_DEFINE_ENUM_CLASS_INIT(StyleLayerTypeClass, StyleLayerType, 6, StyleLayerType(-1), "unknown");
//
#endif

// -------------------------------------------------------------------------------------------------

enum class SourceType : uint8_t {
    Vector,
    Raster,
    GeoJSON,
    Video
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(SourceTypeClass, SourceType, {
    { SourceType::Vector, "vector" },
    { SourceType::Raster, "raster" },
    { SourceType::GeoJSON, "geojson" },
    { SourceType::Video, "video" },
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(SourceTypeClass, SourceType, 4, 
	{ SourceType::Vector, "vector" },
	{ SourceType::Raster, "raster" },
	{ SourceType::GeoJSON, "geojson" },
	{ SourceType::Video, "video" });
#endif
// -------------------------------------------------------------------------------------------------

enum class WindingType : bool {
    EvenOdd,
    NonZero,
};
#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(WindingTypeClass, WindingType, {
    { WindingType::EvenOdd, "even-odd" },
    { WindingType::NonZero, "non-zero" },
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(WindingTypeClass, WindingType, 2, 
	{ WindingType::EvenOdd, "even-odd" },
	{ WindingType::NonZero, "non-zero" });
#endif
// -------------------------------------------------------------------------------------------------

enum class CapType : uint8_t {
    Round,
    Butt,
    Square,
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(CapTypeClass, CapType, {
    { CapType::Round, "round" },
    { CapType::Butt, "butt" },
    { CapType::Square, "square" },
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(CapTypeClass, CapType, 3, 
	{ CapType::Round, "round" },
	{ CapType::Butt, "butt" },
	{ CapType::Square, "square" }
);
#endif
// -------------------------------------------------------------------------------------------------

enum class JoinType : uint8_t {
    Miter,
    Bevel,
    Round
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(JoinTypeClass, JoinType, {
    { JoinType::Miter, "miter" },
    { JoinType::Bevel, "bevel" },
    { JoinType::Round, "round" },
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(JoinTypeClass, JoinType, 3, 
	{ JoinType::Miter, "miter" },
	{ JoinType::Bevel, "bevel" },
	{ JoinType::Round, "round" }
);
#endif
// -------------------------------------------------------------------------------------------------

enum class TranslateAnchorType : bool {
    Map,
    Viewport
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(TranslateAnchorTypeClass, TranslateAnchorType, {
    { TranslateAnchorType::Map, "map" },
    { TranslateAnchorType::Viewport, "viewport" },
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(TranslateAnchorTypeClass, TranslateAnchorType, 2, 
	{ TranslateAnchorType::Map, "map" },
	{ TranslateAnchorType::Viewport, "viewport" }
);
#endif
// -------------------------------------------------------------------------------------------------

enum class RotateAnchorType : bool {
    Map,
    Viewport,
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(RotateAnchorTypeClass, RotateAnchorType, 2, 
    { RotateAnchorType::Map, "map" },
    { RotateAnchorType::Viewport, "viewport" });
#else
MBGL_DEFINE_ENUM_CLASS_NAME(RotateAnchorTypeClass, RotateAnchorType, 2,
	{ RotateAnchorType::Map, "map" },
	{ RotateAnchorType::Viewport, "viewport" });
#endif
// -------------------------------------------------------------------------------------------------

enum class PlacementType : bool {
    Point,
    Line,
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(PlacementTypeClass, PlacementType, 2,
    { PlacementType::Point, "point" },
    { PlacementType::Line, "line" });
#else
MBGL_DEFINE_ENUM_CLASS_NAME(PlacementTypeClass, PlacementType, 2,
	{ PlacementType::Point, "point" },
	{ PlacementType::Line, "line" });
#endif
// -------------------------------------------------------------------------------------------------

enum class RotationAlignmentType : bool {
    Map,
    Viewport,
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(RotationAlignmentTypeClass, RotationAlignmentType, {
    { RotationAlignmentType::Map, "map" },
    { RotationAlignmentType::Viewport, "viewport" },
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(RotationAlignmentTypeClass, RotationAlignmentType, 2,
	{ RotationAlignmentType::Map, "map" },
	{ RotationAlignmentType::Viewport, "viewport" });
#endif
// -------------------------------------------------------------------------------------------------

enum class TextJustifyType : uint8_t {
    Center,
    Left,
    Right
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(TextJustifyTypeClass, TextJustifyType, {
    { TextJustifyType::Center, "center" },
    { TextJustifyType::Left, "left" },
    { TextJustifyType::Right, "right" },
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(TextJustifyTypeClass, TextJustifyType, 3,
	{ TextJustifyType::Center, "center" },
	{ TextJustifyType::Left, "left" },
	{ TextJustifyType::Right, "right" });
#endif
// -------------------------------------------------------------------------------------------------

enum class TextAnchorType : uint8_t {
    Center,
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(TextAnchorTypeClass, TextAnchorType, {
    { TextAnchorType::Center, "center" },
    { TextAnchorType::Left, "left" },
    { TextAnchorType::Right, "right" },
    { TextAnchorType::Top, "top" },
    { TextAnchorType::Bottom, "bottom" },
    { TextAnchorType::TopLeft, "top-left" },
    { TextAnchorType::TopRight, "top-right" },
    { TextAnchorType::BottomLeft, "bottom-left" },
    { TextAnchorType::BottomRight, "bottom-right" }
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(TextAnchorTypeClass, TextAnchorType, 9,
	{ TextAnchorType::Center, "center" },
	{ TextAnchorType::Left, "left" },
	{ TextAnchorType::Right, "right" },
	{ TextAnchorType::Top, "top" },
	{ TextAnchorType::Bottom, "bottom" },
	{ TextAnchorType::TopLeft, "top-left" },
	{ TextAnchorType::TopRight, "top-right" },
	{ TextAnchorType::BottomLeft, "bottom-left" },
	{ TextAnchorType::BottomRight, "bottom-right" });
#endif
// -------------------------------------------------------------------------------------------------

enum class TextTransformType : uint8_t {
    None,
    Uppercase,
    Lowercase,
};

#ifndef _MSC_VER
MBGL_DEFINE_ENUM_CLASS(TextTransformTypeClass, TextTransformType, {
    { TextTransformType::None, "none" },
    { TextTransformType::Uppercase, "uppercase" },
    { TextTransformType::Lowercase, "lowercase" },
});
#else
MBGL_DEFINE_ENUM_CLASS_NAME(TextTransformTypeClass, TextTransformType, 3,
	{ TextTransformType::None, "none" },
	{ TextTransformType::Uppercase, "uppercase" },
	{ TextTransformType::Lowercase, "lowercase" });
#endif
}

#endif

