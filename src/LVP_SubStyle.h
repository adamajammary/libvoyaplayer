#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_SUBSTYLE_H
#define LVP_SUBSTYLE_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		struct LVP_SubtitleContext;

		enum LVP_SubBorderStyle
		{
			SUB_BORDER_STYLE_OUTLINE = 1, SUB_BORDER_STYLE_BOX = 3
		};

		enum LVP_SubStyleVersion
		{
			SUB_STYLE_VERSION_UNKNOWN = -1, SUB_STYLE_VERSION_4_SSA, SUB_STYLE_VERSION_4PLUS_ASS
		};

		// Name, Fontname, Fontsize,
		// PrimaryColour, SecondaryColour, OutlineColour, BackColour,
		// Bold, Italic, Underline,
		// BorderStyle, Outline, Shadow, Alignment,
		// MarginL, MarginR, MarginV,
		// AlphaLevel, Encoding
		enum LVP_SubStyleV4
		{
			SUB_STYLE_NAME,
			SUB_STYLE_FONT_NAME,
			SUB_STYLE_FONT_SIZE,
			SUB_STYLE_COLOR_PRIMARY,
			SUB_STYLE_COLOR_SECONDARY,
			SUB_STYLE_COLOR_BORDER,
			SUB_STYLE_COLOR_SHADOW,
			SUB_STYLE_BOLD,
			SUB_STYLE_ITALIC,
			SUB_STYLE_UNDERLINE,
			SUB_STYLE_V4_BORDER_STYLE = 10,
			SUB_STYLE_V4_OUTLINE,
			SUB_STYLE_V4_SHADOW,
			SUB_STYLE_V4_ALIGNMENT,
			SUB_STYLE_V4_MARGINL,
			SUB_STYLE_V4_MARGINR,
			SUB_STYLE_V4_MARGINV,
			SUB_STYLE_V4_ALPHA_LEVEL,
			SUB_STYLE_V4_ENCODING,
			NR_OF_V4_SUB_STYLES
		};

		// Name, ..., Underline,
		// StrikeOut,
		// ScaleX, ScaleY,
		// Spacing, Angle, BorderStyle, Outline, Shadow, Alignment,
		// MarginL, MarginR, MarginV,
		// Encoding
		enum LVP_SubStyleV4Plus
		{
			SUB_STYLE_V4PLUS_STRIKEOUT = 10,
			SUB_STYLE_V4PLUS_SCALE_X,
			SUB_STYLE_V4PLUS_SCALE_Y,
			SUB_STYLE_V4PLUS_LETTER_SPACING,
			SUB_STYLE_V4PLUS_ROTATION_ANGLE,
			SUB_STYLE_V4PLUS_BORDER_STYLE,
			SUB_STYLE_V4PLUS_OUTLINE,
			SUB_STYLE_V4PLUS_SHADOW,
			SUB_STYLE_V4PLUS_ALIGNMENT,
			SUB_STYLE_V4PLUS_MARGINL,
			SUB_STYLE_V4PLUS_MARGINR,
			SUB_STYLE_V4PLUS_MARGINV,
			SUB_STYLE_V4PLUS_ENCODING,
			NR_OF_V4PLUS_SUB_STYLES
		};

		class LVP_SubStyle
		{
		public:
			LVP_SubStyle();
			LVP_SubStyle(Strings data, LVP_SubStyleVersion version);
			~LVP_SubStyle() {}

		public:
			LVP_SubAlignment    alignment;
			int                 blur;
			LVP_SubBorderStyle  borderStyle;
			Graphics::LVP_Color colorOutline;
			Graphics::LVP_Color colorPrimary;
			Graphics::LVP_Color colorShadow;
			SDL_FPoint          fontScale;
			int                 fontSize;
			int                 fontStyle;
			int                 marginL;
			int                 marginR;
			int                 marginV;
			std::string         name;
			int                 outline;
			double              rotation;
			SDL_Point           shadow;
			LVP_SubStyleVersion version;

			#if defined _windows
				std::wstring fontName;
			#else
				std::string  fontName;
			#endif

		public:
			void      copy(const LVP_SubStyle &subStyle);
			TTF_Font* getFont(LVP_SubtitleContext &subContext);
			int       getFontSizeScaled(double scale = 1.0) const;

			static int              GetOffsetX(Graphics::LVP_SubTexture* subTexture, LVP_SubtitleContext &subContext);
			static LVP_SubStyle*    GetStyle(const std::string &name, const LVP_SubStyles &subStyles);
			static bool             IsAlignedBottom(LVP_SubAlignment a);
			static bool             IsAlignedCenter(LVP_SubAlignment a);
			static bool             IsAlignedLeft(LVP_SubAlignment a);
			static bool             IsAlignedMiddle(LVP_SubAlignment a);
			static bool             IsAlignedRight(LVP_SubAlignment a);
			static bool             IsAlignedTop(LVP_SubAlignment a);
			static LVP_SubAlignment ToSubAlignment(int alignment);

		private:
			TTF_Font* openFontArial(int fontSize);
			TTF_Font* openFontInternal(const LVP_SubtitleContext &subContext, int fontSize);

		};
	}
}

#endif
