#pragma once
//=====================================================================//
/*!	@file
	@brief	RX24T システム制御
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2017, 2018 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include "RX24T/system.hpp"
#include "RX24T/flash.hpp"
#include "RX24T/clock_profile.hpp"

#ifndef SIG_RX24T
#  error "system_io.hpp: Not available on not RX24T"
#endif

namespace device {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  systen_io クラス
		@param[in]	BASE_CLOCK	ベース・クロック周波数（１０ＭＨｚ）
		@param[in]	INTR_CLOCK	内臓クロック周波数（８０ＭＨｚ）
		@param[in]	EXT_CLOCK	外部クロック入力を使う場合「true」@n
								※クリスタル利用は「false」
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template <bool EXT_CLOCK = false>
	struct system_io {

		static uint8_t clock_div_(uint32_t clk) noexcept
		{
			uint8_t div = 0;
			while(clk < clock_profile::PLL_BASE) {
				++div;
				clk <<= 1;
			}
			return div;
		}


		//-------------------------------------------------------------//
		/*!
			@brief  マスター・クロックのブースト
			@return エラーなら「false」
		*/
		//-------------------------------------------------------------//
		static bool boost_master_clock()
		{
			device::SYSTEM::PRCR = 0xA500 | 0b0111;	// クロック、低消費電力、関係書き込み許可

			// メモリーの WAIT 設定
			if(clock_profile::ICLK > 64000000) {
				device::SYSTEM::MEMWAIT = 0b10; // 64MHz 以上 wait 設定
			} else if(clock_profile::ICLK > 32000000) {
				device::SYSTEM::MEMWAIT = 0b01; // 32MHz 以上 64MHz 以下 wait 設定 
			} else {
				device::SYSTEM::MEMWAIT = 0b00; // wait 無し
			}

			while(device::SYSTEM::OPCCR.OPCMTSF() != 0) asm("nop");
			device::SYSTEM::OPCCR = 0;  // 高速モード選択
			while(device::SYSTEM::OPCCR.OPCMTSF() != 0) asm("nop");

			device::SYSTEM::MOSCWTCR = 9;	// 4ms wait
			// メインクロック・ドライブ能力設定、内部発信
			bool xtal = clock_profile::BASE >= 10000000;
			device::SYSTEM::MOFCR = device::SYSTEM::MOFCR.MODRV21.b(xtal)
				| device::SYSTEM::MOFCR.MOSEL.b(EXT_CLOCK);
			device::SYSTEM::MOSCCR.MOSTP = 0;  // メインクロック発振器動作
			while(device::SYSTEM::OSCOVFSR.MOOVF() == 0) asm("nop");

			// Min: (x4.0) 0b000111, Max: (x15.5) 0b111110
			uint32_t n = clock_profile::PLL_BASE * 2 / clock_profile::BASE;
			if(n < 8) n = 8;
			else if(n > 31) n = 31;
			device::SYSTEM::PLLCR.STC = n - 1;

			device::SYSTEM::PLLCR2.PLLEN = 0;			// PLL 動作
			while(device::SYSTEM::OSCOVFSR.PLOVF() == 0) asm("nop");

			device::SYSTEM::SCKCR = device::SYSTEM::SCKCR.FCK.b(clock_div_(clock_profile::FCLK))
								  | device::SYSTEM::SCKCR.ICK.b(clock_div_(clock_profile::ICLK))
								  | device::SYSTEM::SCKCR.PCKA.b(clock_div_(clock_profile::PCLKA))
								  | device::SYSTEM::SCKCR.PCKB.b(clock_div_(clock_profile::PCLKB))
								  | device::SYSTEM::SCKCR.PCKD.b(clock_div_(clock_profile::PCLKD));

			device::SYSTEM::SCKCR3.CKSEL = 0b100;	// PLL 選択

			// クロック関係書き込み不許可
			device::SYSTEM::PRCR = 0xA500;

			// ROM キャッシュを有効（標準）
			device::FLASH::ROMCE = 1;

			return true;
		}
	};


	//-------------------------------------------------------------//
	/*!
		@brief  MTU マスタークロック取得
		@return MTU マスタークロック
	*/
	//-------------------------------------------------------------//
	inline uint32_t get_mtu_master_clock() noexcept
	{
		return clock_profile::PCLKA;
	}
}
