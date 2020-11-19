#pragma once
//=====================================================================//
/*! @file
    @brief  GUI 関数電卓・クラス @n
			RX65N Envision Kit @n
			RX72N Envision Kit
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2020 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include "common/renesas.hpp"

#include "common/fixed_fifo.hpp"
#include "common/sci_i2c_io.hpp"
#include "chip/FT5206.hpp"

#include "graphics/font8x16.hpp"
#include "graphics/kfont.hpp"
#include "graphics/graphics.hpp"
#include "graphics/simple_dialog.hpp"
#include "graphics/widget_director.hpp"
#include "graphics/scaling.hpp"

#include "common/format.hpp"
#include "common/basic_arith.hpp"
#include "common/fixed_string.hpp"

namespace app {

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
    /*!
        @brief  CALC GUI クラス
    */
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
    class calc_gui {
	public:
		static const int16_t LCD_X = 480;
		static const int16_t LCD_Y = 272;
		static const auto PIX = graphics::pixel::TYPE::RGB565;

		typedef utils::fixed_fifo<uint8_t, 64> RB64;
		typedef utils::fixed_fifo<uint8_t, 64> SB64;

#if defined(SIG_RX65N)
		typedef device::PORT<device::PORT6, device::bitpos::B3> LCD_DISP;
		typedef device::PORT<device::PORT6, device::bitpos::B6> LCD_LIGHT;
		static const uint32_t LCD_ORG = 0x0000'0100;
		typedef device::PORT<device::PORT0, device::bitpos::B7> FT5206_RESET;
		typedef device::sci_i2c_io<device::SCI6, RB64, SB64, device::port_map::option::FIRST_I2C> FT5206_I2C;

#elif defined(SIG_RX72N)
		typedef device::PORT<device::PORTB, device::bitpos::B3> LCD_DISP;
		typedef device::PORT<device::PORT6, device::bitpos::B7> LCD_LIGHT;
		static const uint32_t LCD_ORG = 0x0080'0000;
		typedef device::PORT<device::PORT6, device::bitpos::B6> FT5206_RESET;
		typedef device::sci_i2c_io<device::SCI6, RB64, SB64, device::port_map::option::THIRD_I2C> FT5206_I2C;
#endif
		typedef device::glcdc_mgr<device::GLCDC, LCD_X, LCD_Y, PIX> GLCDC;

		typedef graphics::font8x16 AFONT;
		typedef graphics::kfont<16, 16> KFONT;
		typedef graphics::font<AFONT, KFONT> FONT;

		// ソフトウェアーレンダラー
		typedef graphics::render<GLCDC, FONT> RENDER;
		// 標準カラーインスタンス
		typedef graphics::def_color DEF_COLOR;

	private:

		GLCDC	glcdc_;
		AFONT	afont_;
		KFONT	kfont_;
		FONT	font_;
		RENDER	render_;

		FT5206_I2C	ft5206_i2c_;
		typedef chip::FT5206<FT5206_I2C> TOUCH;
		TOUCH	touch_;
		// RX65N Envision Kit: INT to P02(IRQ10), not use
		// RX72N Envision Kit: INT to P34(IRQ4), not use

		typedef gui::simple_dialog<RENDER, TOUCH> DIALOG;
		DIALOG	dialog_;

		typedef gui::widget_director<RENDER, TOUCH, 40> WIDD;
		WIDD	widd_;

		static const int16_t BTN_W = 38;
		static const int16_t BTN_H = 38;
		static const int16_t ORG_X = 10;
		static const int16_t ORG_Y = 94;
		static const int16_t SPC_X = 44;
		static const int16_t SPC_Y = 44;

		constexpr int16_t LOC_X(int16_t x)
		{
			return ORG_X + SPC_X * x;
		}

		constexpr int16_t LOC_Y(int16_t y)
		{
			return ORG_Y + SPC_Y * y;
		}

		typedef gui::button BUTTON;
		BUTTON	no0_;
		BUTTON	no1_;
		BUTTON	no2_;
		BUTTON	no3_;
		BUTTON	no4_;
		BUTTON	no5_;
		BUTTON	no6_;
		BUTTON	no7_;
		BUTTON	no8_;
		BUTTON	no9_;

		BUTTON	del_;
		BUTTON	ac_;

		BUTTON	mul_;
		BUTTON	div_;
		BUTTON	add_;
		BUTTON	sub_;

		BUTTON	poi_;
		BUTTON	pin_;
		BUTTON	pot_;
		BUTTON	equ_;

		typedef utils::basic_arith<float> ARITH;
		ARITH	arith_;

		typedef utils::fixed_string<128> STR;
		STR		cbuff_;

		static const int16_t limit_ = 3;
		vtx::spos	cur_pos_;

		void clear_win_()
		{
			cbuff_.clear();
			cur_pos_.set(0);
			render_.set_fore_color(DEF_COLOR::Darkgray);
			render_.round_box(vtx::srect(0, 0, 480, 16 * 5 + 6), 8);
		}

		typedef utils::fixed_string<512> OUTSTR;


		void conv_cha_(char ch, OUTSTR& out)
		{
			switch(ch) {
			case '0': out += "０"; break;
			case '1': out += "１"; break;
			case '2': out += "２"; break;
			case '3': out += "３"; break;
			case '4': out += "４"; break;
			case '5': out += "５"; break;
			case '6': out += "６"; break;
			case '7': out += "７"; break;
			case '8': out += "８"; break;
			case '9': out += "９"; break;
			case '+': out += "＋"; break;
			case '-': out += "－"; break;
			case '/': out += "÷"; break;
			case '*': out += "×"; break;
			default:
				out += ch; break;
			}
		}


		OUTSTR conv_str_(const char* str)
		{
			OUTSTR out;
			char ch;
			while((ch = *str++) != 0) {
				conv_cha_(ch, out);
			}
			return out;
		}


		void update_calc_()
		{
			if(cur_pos_.x != static_cast<int32_t>(cbuff_.size())) {
				if(cur_pos_.x > static_cast<int32_t>(cbuff_.size())) {
					render_.set_fore_color(DEF_COLOR::Darkgray);
					auto x = cur_pos_.x;
					if(x > 0) --x;
					render_.fill_box(vtx::srect(6 + x * 16, 6 + cur_pos_.y * 20, 16, 16));
				} else {
					OUTSTR tmp;
					conv_cha_(cbuff_.back(), tmp);
					render_.set_fore_color(DEF_COLOR::White);
					render_.draw_text(vtx::spos(6 + cur_pos_.x * 16, 6 + cur_pos_.y * 20),
						tmp.c_str());
				}
				cur_pos_.x = cbuff_.size();
			}
		}


		void update_ans_()
		{
			arith_.analize(cbuff_.c_str());
			auto ans = arith_.get();
			char tmp[20];
			utils::sformat("%7.6f\n", tmp, sizeof(tmp)) % ans;
			auto out = conv_str_(tmp);
			render_.set_back_color(DEF_COLOR::Darkgray);
			render_.set_fore_color(DEF_COLOR::White);
			render_.draw_text(vtx::spos(6, 6 + 20 * 3), out.c_str(), false, true);
			cbuff_.clear();
			cur_pos_.y++;
			if(cur_pos_.y >= limit_) {
				render_.move(vtx::srect(6, 6 + 20, 480 - 12, 20 * 2), vtx::spos(6, 6));
				render_.set_fore_color(DEF_COLOR::Darkgray);
				render_.fill_box(vtx::srect(6, 6 + 20 * 2, 480 - 12, 20));
				cur_pos_.y = limit_ - 1;
			}
		}

	public:
		//-------------------------------------------------------------//
		/*!
			@brief  AUDIO GUI コンストラクタ
		*/
		//-------------------------------------------------------------//
		calc_gui() noexcept :
			glcdc_(nullptr, reinterpret_cast<void*>(LCD_ORG)),
			afont_(), kfont_(), font_(afont_, kfont_),
			render_(glcdc_, font_),
			ft5206_i2c_(), touch_(ft5206_i2c_),
			dialog_(render_, touch_),
			widd_(render_, touch_),
			no0_(vtx::srect(LOC_X(0), LOC_Y(3), BTN_W, BTN_H), "０"),
			no1_(vtx::srect(LOC_X(0), LOC_Y(2), BTN_W, BTN_H), "１"),
			no2_(vtx::srect(LOC_X(1), LOC_Y(2), BTN_W, BTN_H), "２"),
			no3_(vtx::srect(LOC_X(2), LOC_Y(2), BTN_W, BTN_H), "３"),
			no4_(vtx::srect(LOC_X(0), LOC_Y(1), BTN_W, BTN_H), "４"),
			no5_(vtx::srect(LOC_X(1), LOC_Y(1), BTN_W, BTN_H), "５"),
			no6_(vtx::srect(LOC_X(2), LOC_Y(1), BTN_W, BTN_H), "６"),
			no7_(vtx::srect(LOC_X(0), LOC_Y(0), BTN_W, BTN_H), "７"),
			no8_(vtx::srect(LOC_X(1), LOC_Y(0), BTN_W, BTN_H), "８"),
			no9_(vtx::srect(LOC_X(2), LOC_Y(0), BTN_W, BTN_H), "９"),
			del_(vtx::srect(LOC_X(3), LOC_Y(0), BTN_W, BTN_H), "DEL"),
			ac_ (vtx::srect(LOC_X(4), LOC_Y(0), BTN_W, BTN_H), "AC"),
			mul_(vtx::srect(LOC_X(3), LOC_Y(1), BTN_W, BTN_H), "×"),
			div_(vtx::srect(LOC_X(4), LOC_Y(1), BTN_W, BTN_H), "÷"),
			add_(vtx::srect(LOC_X(3), LOC_Y(2), BTN_W, BTN_H), "＋"),
			sub_(vtx::srect(LOC_X(4), LOC_Y(2), BTN_W, BTN_H), "－"),
			poi_(vtx::srect(LOC_X(1), LOC_Y(3), BTN_W, BTN_H), "・"),
			pin_(vtx::srect(LOC_X(2), LOC_Y(3), BTN_W, BTN_H), "（"),
			pot_(vtx::srect(LOC_X(3), LOC_Y(3), BTN_W, BTN_H), "）"),
			equ_(vtx::srect(LOC_X(4), LOC_Y(3), BTN_W, BTN_H), "＝"),
			arith_(), cbuff_(), cur_pos_(0)
		{ }


		//-------------------------------------------------------------//
		/*!
			@brief  widget 追加
			@param[in]	w	widget
			@return 正常なら「true」
		*/
		//-------------------------------------------------------------//
		bool insert_widget(gui::widget* w) noexcept {
			return widd_.insert(w);
		}


		//-------------------------------------------------------------//
		/*!
			@brief  widget 削除
			@param[in]	w	widget
			@return 正常なら「true」
		*/
		//-------------------------------------------------------------//
		void remove_widget(gui::widget* w) noexcept {
			widd_.remove(w);
		}


		//-------------------------------------------------------------//
		/*!
			@brief  レンダーの参照
			@return レンダー
		*/
		//-------------------------------------------------------------//
		RENDER& at_render() noexcept { return render_; }


		//-------------------------------------------------------------//
		/*!
			@brief  開始
		*/
		//-------------------------------------------------------------//
		void start() noexcept
		{
			{  // GLCDC の初期化
				LCD_DISP::DIR  = 1;
				LCD_LIGHT::DIR = 1;
				LCD_DISP::P  = 0;  // DISP Disable
				LCD_LIGHT::P = 0;  // BackLight Disable (No PWM)
				if(glcdc_.start()) {
					utils::format("Start GLCDC\n");
					LCD_DISP::P  = 1;  // DISP Enable
					LCD_LIGHT::P = 1;  // BackLight Enable (No PWM)
					if(!glcdc_.control(GLCDC::CONTROL_CMD::START_DISPLAY)) {
						utils::format("GLCDC ctrl fail...\n");
					}
				} else {
					utils::format("GLCDC Fail\n");
				}
			}

			{  // FT5206 touch screen controller
				TOUCH::reset<FT5206_RESET>();
				uint8_t intr_lvl = 1;
				if(!ft5206_i2c_.start(FT5206_I2C::SPEED::STANDARD, intr_lvl)) {
					utils::format("FT5206 I2C Start Fail...\n");
				}
				if(!touch_.start()) {
					utils::format("FT5206 Start Fail...\n");
				}
			}
		}


		//-------------------------------------------------------------//
		/*!
			@brief  タッチ・パネルの設定
		*/
		//-------------------------------------------------------------//
		void setup_touch_panel() noexcept
		{
			render_.sync_frame();
			dialog_.modal(vtx::spos(400, 60),
				"Touch panel device wait...\nPlease touch it with some screen.");
			uint8_t nnn = 0;
			while(1) {
				render_.sync_frame();
				touch_.update();
				auto num = touch_.get_touch_num();
				if(num == 0) {
					++nnn;
					if(nnn >= 60) break;
				} else {
					nnn = 0;
				}
			}
			render_.clear(DEF_COLOR::Black);
		}


		//-------------------------------------------------------------//
		/*!
			@brief  GUI のセットアップ
		*/
		//-------------------------------------------------------------//
		void setup() noexcept
		{
			no0_.enable();
			no0_.at_select_func() = [=](uint32_t id) { cbuff_ += '0'; };
			no1_.enable();
			no1_.at_select_func() = [=](uint32_t id) { cbuff_ += '1'; };
			no2_.enable();
			no2_.at_select_func() = [=](uint32_t id) { cbuff_ += '2'; };
			no3_.enable();
			no3_.at_select_func() = [=](uint32_t id) { cbuff_ += '3'; };
			no4_.enable();
			no4_.at_select_func() = [=](uint32_t id) { cbuff_ += '4'; };
			no5_.enable();
			no5_.at_select_func() = [=](uint32_t id) { cbuff_ += '5'; };
			no6_.enable();
			no6_.at_select_func() = [=](uint32_t id) { cbuff_ += '6'; };
			no7_.enable();
			no7_.at_select_func() = [=](uint32_t id) { cbuff_ += '7'; };
			no8_.enable();
			no8_.at_select_func() = [=](uint32_t id) { cbuff_ += '8'; };
			no9_.enable();
			no9_.at_select_func() = [=](uint32_t id) { cbuff_ += '9'; };

			del_.enable();
			del_.at_select_func() = [=](uint32_t id) { cbuff_.pop_back(); };
			ac_.enable();
			ac_.at_select_func() = [=](uint32_t id) { clear_win_(); };

			mul_.enable();
			mul_.at_select_func() = [=](uint32_t id) { cbuff_ += '*'; };
			div_.enable();
			div_.at_select_func() = [=](uint32_t id) { cbuff_ += '/'; };
			add_.enable();
			add_.at_select_func() = [=](uint32_t id) { cbuff_ += '+'; };
			sub_.enable();
			sub_.at_select_func() = [=](uint32_t id) { cbuff_ += '-'; };

			poi_.enable();
			poi_.at_select_func() = [=](uint32_t id) { cbuff_ += '.'; };
			pin_.enable();
			pin_.at_select_func() = [=](uint32_t id) { cbuff_ += '('; };
			pot_.enable();
			pot_.at_select_func() = [=](uint32_t id) { cbuff_ += ')'; };

			equ_.enable();
			equ_.at_select_func() = [=](uint32_t id) { update_ans_(); };

			clear_win_();
		}


		//-------------------------------------------------------------//
		/*!
			@brief  アップデート
		*/
		//-------------------------------------------------------------//
		void update() noexcept
		{
			render_.sync_frame();
			touch_.update();
			widd_.update();

			update_calc_();
		}
	};
}