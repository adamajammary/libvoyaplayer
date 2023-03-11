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

		// Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour,
		// Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV,
		// AlphaLevel, Encoding
		enum LVP_SubStyleV4
		{
			SUB_STYLE_V4_NAME,
			SUB_STYLE_V4_FONT_NAME,
			SUB_STYLE_V4_FONT_SIZE,
			SUB_STYLE_V4_COLOR_PRIMARY,
			SUB_STYLE_V4_COLOR_SECONDARY,
			SUB_STYLE_V4_COLOR_BORDER,
			SUB_STYLE_V4_COLOR_SHADOW,
			SUB_STYLE_V4_FONT_BOLD,
			SUB_STYLE_V4_FONT_ITALIC,
			SUB_STYLE_V4_FONT_BORDER_STYLE,
			SUB_STYLE_V4_FONT_OUTLINE,
			SUB_STYLE_V4_FONT_SHADOW,
			SUB_STYLE_V4_FONT_ALIGNMENT,
			SUB_STYLE_V4_FONT_MARGINL,
			SUB_STYLE_V4_FONT_MARGINR,
			SUB_STYLE_V4_FONT_MARGINV,
			SUB_STYLE_V4_FONT_ALPHA_LEVEL,
			SUB_STYLE_V4_FONT_ENCODING,
			NR_OF_V4_SUB_STYLES
		};

		// Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour,
		// Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow,
		// Alignment, MarginL, MarginR, MarginV, Encoding
		enum LVP_SubStyleV4Plus
		{
			SUB_STYLE_V4PLUS_FONT_UNDERLINE = 9,
			SUB_STYLE_V4PLUS_FONT_STRIKEOUT,
			SUB_STYLE_V4PLUS_FONT_SCALE_X,
			SUB_STYLE_V4PLUS_FONT_SCALE_Y,
			SUB_STYLE_V4PLUS_FONT_LETTER_SPACING,
			SUB_STYLE_V4PLUS_FONT_ROTATION_ANGLE,
			SUB_STYLE_V4PLUS_FONT_BORDER_STYLE,
			SUB_STYLE_V4PLUS_FONT_OUTLINE,
			SUB_STYLE_V4PLUS_FONT_SHADOW,
			SUB_STYLE_V4PLUS_FONT_ALIGNMENT,
			SUB_STYLE_V4PLUS_FONT_MARGINL,
			SUB_STYLE_V4PLUS_FONT_MARGINR,
			SUB_STYLE_V4PLUS_FONT_MARGINV,
			SUB_STYLE_V4PLUS_FONT_ENCODING,
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

			#if defined _windows
				std::wstring fontName;
			#else
				std::string  fontName;
			#endif

		public:
			void      copy(const LVP_SubStyle &subStyle);
			TTF_Font* getFont(LVP_SubtitleContext &subContext);
			int       getFontSizeScaled(double scale = 1.0);

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
