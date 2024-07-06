#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_SUBTITLE_H
#define LVP_SUBTITLE_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		struct LVP_SubtitleContext;

		enum LVP_SubAlignment
		{
			SUB_ALIGN_UNKNOWN,
			SUB_ALIGN_BOTTOM_LEFT, SUB_ALIGN_BOTTOM_CENTER, SUB_ALIGN_BOTTOM_RIGHT,
			SUB_ALIGN_MIDDLE_LEFT, SUB_ALIGN_MIDDLE_CENTER, SUB_ALIGN_MIDDLE_RIGHT,
			SUB_ALIGN_TOP_LEFT,    SUB_ALIGN_TOP_CENTER,    SUB_ALIGN_TOP_RIGHT
		};

		class LVP_Subtitle
		{
		public:
			LVP_Subtitle();
			~LVP_Subtitle();

		public:
			bool          customPos;
			bool          customRotation;
			SDL_Rect      drawRect;
			int           id;
			int           layer;
			SDL_Point     position;
			double        rotation;
			SDL_Point     rotationPoint;
			LVP_SubPTS    pts;
			bool          skip;
			LVP_SubStyle* style;
			LVP_SubRect*  subRect;
			std::string   text;
			std::string   text2;
			std::string   text3;
			Strings       textSplit;
			uint16_t*     textUTF16;

		public:
			void                copy(const LVP_Subtitle &subtitle);
			Graphics::LVP_Color getColor() const;
			Graphics::LVP_Color getColorOutline() const;
			Graphics::LVP_Color getColorShadow() const;
			TTF_Font*           getFont(LVP_SubtitleContext &subContext);
			SDL_Rect            getMargins(const SDL_FPoint &scale);
			int                 getOutline(const SDL_FPoint &scale) const;
			SDL_Point           getShadow(const SDL_FPoint  &scale);
			bool                isAlignedBottom();
			bool                isAlignedCenter();
			bool                isAlignedLeft();
			bool                isAlignedMiddle();
			bool                isAlignedRight();
			bool                isAlignedTop();
			bool                isDuplicate(const LVP_Subtitles& subs) const;
			bool                isExpiredPTS(const LVP_SubtitleContext &subContext, double progress) const;
			bool                isSeekedPTS(const LVP_SubtitleContext  &subContext) const;
			bool                overlaps(LVP_Subtitle* subtitle) const;
			bool                skipRender(bool layered) const;

		private:
			LVP_SubAlignment getAlignment() const;
		};
	}
}

#endif
