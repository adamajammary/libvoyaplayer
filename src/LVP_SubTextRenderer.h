#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_SUBTEXTRENDERER_H
#define LVP_SUBTEXTRENDERER_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		// [Start, End] ReadOrder, Marked, Style, Text
		enum LVP_SubDialoguePropertyV4
		{
			SUB_DIALOGUE_READORDER,
			SUB_DIALOGUE_V4_MARKED,
			SUB_DIALOGUE_STYLE,
			SUB_DIALOGUE_V4_TEXT,
			NR_OF_SUB_DIALOGUE_V4_PROPERTIES
		};

		// [Start, End] ReadOrder, Layer, Style, Name, MarginL, MarginR, MarginV, Effect, Text
		enum LVP_SubDialoguePropertyV4Plus
		{
			SUB_DIALOGUE_V4PLUS_LAYER   = 1,
			SUB_DIALOGUE_V4PLUS_SPEAKER = 3,
			SUB_DIALOGUE_V4PLUS_MARGINL,
			SUB_DIALOGUE_V4PLUS_MARGINR,
			SUB_DIALOGUE_V4PLUS_MARGINV,
			SUB_DIALOGUE_V4PLUS_EFFECTS,
			SUB_DIALOGUE_V4PLUS_TEXT,
			NR_OF_SUB_DIALOGUE_V4PLUS_PROPERTIES
		};

		class LVP_SubTextRenderer
		{
		private:
			LVP_SubTextRenderer()  {}
			~LVP_SubTextRenderer() {}

		private:
			static Graphics::LVP_Color         drawColor;
			static Graphics::LVP_SubTexturesId subsBottom;
			static Graphics::LVP_SubTexturesId subsMiddle;
			static Graphics::LVP_SubTexturesId subsPosition;
			static Graphics::LVP_SubTexturesId subsTop;

		public:
			static std::string   RemoveFormatting(const std::string &subtitleString);
			static void          RemoveSubs(size_t id);
			static void          Render(SDL_Renderer* renderer, LVP_SubtitleContext &subContext);
			static LVP_Subtitles SplitAndFormatSub(const Strings &subTexts, LVP_SubtitleContext &subContext);

		private:
			static Graphics::LVP_SubTexture* createSubFill(uint16_t* subString16, LVP_Subtitle* sub, SDL_Renderer* renderer, LVP_SubtitleContext &subContext);
			static Graphics::LVP_SubTexture* createSubOutline(Graphics::LVP_SubTexture* subFill, SDL_Renderer* renderer, LVP_SubtitleContext &subContext);
			static Graphics::LVP_SubTexture* createSubShadow(Graphics::LVP_SubTexture* subFill, SDL_Renderer* renderer, LVP_SubtitleContext &subContext);
			static bool                      formatAnimationsContain(const Strings &animations, const std::string &string);
			static void                      formatDrawCommand(const std::string &subText, const Strings &subSplit, int subID, LVP_Subtitles &subs, const LVP_SubtitleContext &subContext);
			static Strings                   formatGetAnimations(const std::string &subString);
			static void                      formatOverrideStyleCat1(const Strings &animations, LVP_Subtitle* sub, const LVP_SubStyles &subStyles);
			static void                      formatOverrideStyleCat2(const Strings &animations, LVP_Subtitle* sub, const LVP_SubStyles &subStyles);
			static std::string               formatRemoveAnimations(const std::string &subString);
			static Strings                   formatSplitStyling(const std::string &subText, LVP_SubStyle* subStyle, LVP_SubtitleContext &subContext);
			static SDL_Rect                  getDrawRect(const std::string &subLine, LVP_SubStyle* style);
			static LVP_SubStyle*             getSubStyle(const LVP_SubStyles &subStyles, const Strings &subSplit);
			static std::string               getSubText(const std::string &dialogueText, size_t nrStyles, LVP_SubStyleVersion version);
			static void                      handleSubCollisions(const Graphics::LVP_SubTextureId &subTextures, const Graphics::LVP_SubTexturesId &subs);
			static void                      handleSubsOutOfBound(const Graphics::LVP_SubTextureId &subTextures);
			static std::string               removeInvalidFormatting(const std::string &subtitleString);
			static void                      removeSubs(Graphics::LVP_SubTexturesId &subs, size_t id);
			static void                      renderSub(Graphics::LVP_SubTexture* subTexture, SDL_Renderer* renderer, const LVP_SubtitleContext &subContext);
			static void                      renderSubBorderShadow(Graphics::LVP_SubTexture* subTexture, SDL_Renderer* renderer, LVP_SubtitleContext &subContext);
			static void                      renderSubs(Graphics::LVP_SubTexturesId &subs, SDL_Renderer* renderer, LVP_SubtitleContext &subContext);
			static void                      setSubPositionAbsolute(const Graphics::LVP_SubTexturesId &subs, LVP_SubtitleContext &subContext);
			static void                      setSubPositionRelative(const Graphics::LVP_SubTexturesId &subs, LVP_SubtitleContext &subContext);
			static void                      setTotalWidth(const Graphics::LVP_SubTexturesId &subs);
			static Strings16                 splitSub(uint16_t* subStringUTF16, int subStringWidth, LVP_Subtitle* sub, TTF_Font* font, int subWidth, int maxWidth);
			static size_t                    splitSubGetNrLines(const Strings &words, TTF_Font* font, int maxWidth);
			static Strings16                 splitSubDistributeByLines(const Strings &words, size_t nrLines, TTF_Font* font, int maxWidth);
			static Strings16                 splitSubDistributeByWidth(const Strings &words, TTF_Font* font, int remainingWidth, int maxWidth);

		};
	}
}

#endif
