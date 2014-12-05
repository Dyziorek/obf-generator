#ifdef SETUP_DEFAULT_MAP

SETUP_DEFAULT_MAP(INPUT_TEST, In, Booltype, "test", false)
SETUP_DEFAULT_MAP(INPUT_TAG, In, Stringtype, "tag", false)
SETUP_DEFAULT_MAP(INPUT_VALUE, In, Stringtype, "value", false)
SETUP_DEFAULT_MAP(INPUT_ADDITIONAL, In, Stringtype, "additional", false)
SETUP_DEFAULT_MAP(INPUT_MINZOOM, In, Inttype, "minzoom", false)
SETUP_DEFAULT_MAP(INPUT_MAXZOOM, In, Inttype, "maxzoom", false)
SETUP_DEFAULT_MAP(INPUT_NIGHT_MODE, In, Booltype, "nightMode", false)
SETUP_DEFAULT_MAP(INPUT_LAYER, In, Inttype, "layer", false)
SETUP_DEFAULT_MAP(INPUT_POINT, In, Booltype, "point", false)
SETUP_DEFAULT_MAP(INPUT_AREA, In, Booltype, "area", false)
SETUP_DEFAULT_MAP(INPUT_CYCLE, In, Booltype, "cycle", false)

SETUP_DEFAULT_MAP(INPUT_TEXT_LENGTH, In, Inttype, "textLength", false)
SETUP_DEFAULT_MAP(INPUT_NAME_TAG, In, Stringtype, "nameTag", false)

SETUP_DEFAULT_MAP(OUTPUT_DISABLE, Out, Booltype, "disable", false)
SETUP_DEFAULT_MAP(OUTPUT_NAME_TAG2, Out, Stringtype, "nameTag2", false)

SETUP_DEFAULT_MAP(OUTPUT_ATTR_INT_VALUE, Out, Inttype, "attrIntValue", false)
SETUP_DEFAULT_MAP(OUTPUT_ATTR_BOOL_VALUE, Out, Booltype, "attrBoolValue", false)
SETUP_DEFAULT_MAP(OUTPUT_ATTR_COLOR_VALUE, Out, Colortype, "attrColorValue", false)
SETUP_DEFAULT_MAP(OUTPUT_ATTR_STRING_VALUE, Out, Stringtype, "attrStringValue", false)

// order - no sense to make it float
SETUP_DEFAULT_MAP(OUTPUT_ORDER, Out, Inttype, "order", false)
SETUP_DEFAULT_MAP(OUTPUT_OBJECT_TYPE, Out, Inttype, "objectType", false)
SETUP_DEFAULT_MAP(OUTPUT_SHADOW_LEVEL, Out, Inttype, "shadowLevel", false)

// text properties
SETUP_DEFAULT_MAP(OUTPUT_TEXT_WRAP_WIDTH, Out, Inttype, "textWrapWidth", true)
SETUP_DEFAULT_MAP(OUTPUT_TEXT_DY, Out, Inttype, "textDy", true)
SETUP_DEFAULT_MAP(OUTPUT_TEXT_HALO_RADIUS, Out, Inttype, "textHaloRadius", true)
SETUP_DEFAULT_MAP(OUTPUT_TEXT_HALO_Colortype, Out, Colortype, "textHaloColor", true)
SETUP_DEFAULT_MAP(OUTPUT_TEXT_SIZE, Out, Inttype, "textSize", true)
SETUP_DEFAULT_MAP(OUTPUT_TEXT_ORDER, Out, Inttype, "textOrder", false)
SETUP_DEFAULT_MAP(OUTPUT_TEXT_MIN_DISTANCE, Out, Inttype, "textMinDistance", true)
SETUP_DEFAULT_MAP(OUTPUT_TEXT_SHIELD, Out, Stringtype, "textShield", false)

SETUP_DEFAULT_MAP(OUTPUT_TEXT_Colortype, Out, Colortype, "textColor", false)
SETUP_DEFAULT_MAP(OUTPUT_TEXT_BOLD, Out, Booltype, "textBold", false)
SETUP_DEFAULT_MAP(OUTPUT_TEXT_ON_PATH, Out, Booltype, "textOnPath", false)

// point
SETUP_DEFAULT_MAP(OUTPUT_ICON, Out, Stringtype, "icon", false)
SETUP_DEFAULT_MAP(OUTPUT_ICON_ORDER, Out, Inttype, "iconOrder", false)

// polygon/way
SETUP_DEFAULT_MAP(OUTPUT_COLOR, Out, Colortype, "color", false)
SETUP_DEFAULT_MAP(OUTPUT_COLOR_2, Out, Colortype, "color_2", false)
SETUP_DEFAULT_MAP(OUTPUT_COLOR_3, Out, Colortype, "color_3", false)
SETUP_DEFAULT_MAP(OUTPUT_COLOR_0, Out, Colortype, "color_0", false)
SETUP_DEFAULT_MAP(OUTPUT_COLOR__1, Out, Colortype, "color__1", false)
SETUP_DEFAULT_MAP(OUTPUT_STROKE_WIDTH, Out, Floattype, "strokeWidth", true)
SETUP_DEFAULT_MAP(OUTPUT_STROKE_WIDTH_2, Out, Floattype, "strokeWidth_2", true)
SETUP_DEFAULT_MAP(OUTPUT_STROKE_WIDTH_3, Out, Floattype, "strokeWidth_3", true)
SETUP_DEFAULT_MAP(OUTPUT_STROKE_WIDTH_0, Out, Floattype, "strokeWidth_0", true)
SETUP_DEFAULT_MAP(OUTPUT_STROKE_WIDTH__1, Out, Floattype, "strokeWidth__1", true)

SETUP_DEFAULT_MAP(OUTPUT_PATH_EFFECT, Out, Stringtype, "pathEffect", false)
SETUP_DEFAULT_MAP(OUTPUT_PATH_EFFECT_2, Out, Stringtype, "pathEffect_2", false)
SETUP_DEFAULT_MAP(OUTPUT_PATH_EFFECT_3, Out, Stringtype, "pathEffect_3", false)
SETUP_DEFAULT_MAP(OUTPUT_PATH_EFFECT_0, Out, Stringtype, "pathEffect_0", false)
SETUP_DEFAULT_MAP(OUTPUT_PATH_EFFECT__1, Out, Stringtype, "pathEffect__1", false)
SETUP_DEFAULT_MAP(OUTPUT_CAP, Out, Stringtype, "cap", false)
SETUP_DEFAULT_MAP(OUTPUT_CAP_2, Out, Stringtype, "cap_2", false)
SETUP_DEFAULT_MAP(OUTPUT_CAP_3, Out, Stringtype, "cap_3", false)
SETUP_DEFAULT_MAP(OUTPUT_CAP_0, Out, Stringtype, "cap_0", false)
SETUP_DEFAULT_MAP(OUTPUT_CAP__1, Out, Stringtype, "cap__1", false)

SETUP_DEFAULT_MAP(OUTPUT_SHADER, Out, Stringtype, "shader", false)
SETUP_DEFAULT_MAP(OUTPUT_SHADOW_COLOR, Out, Colortype, "shadowColor", false)
SETUP_DEFAULT_MAP(OUTPUT_SHADOW_RADIUS, Out, Inttype, "shadowRadius", true)

#endif // SETUP_DEFAULT_MAP
