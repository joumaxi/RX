#pragma once
//=====================================================================//
/*!	@file
	@brief	ポート・マッピング・オーダー型 @n
            ポートの機能設定において、どのピンを利用するかを、選択する「型」
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2021 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include <cstdint>

namespace device {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  ポート・マッピング・オーダー型
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	class port_map_order {
    public:
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  ポート・マッピング・オーダー型
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		enum class ORDER : uint8_t {
			BYPASS,		///< ポートマップの設定をバイパスする場合
			FIRST,		///< 第１候補
			SECOND,		///< 第２候補
			THIRD,		///< 第３候補
			FOURTH,		///< 第４候補
			FIFTH,		///< 第５候補
			SIXTH,		///< 第６候補
			SEVENTH,	///< 第７候補
			EIGHTH,		///< 第８候補
			NINTH,		///< 第９候補
			TENTH,		///< 第１０候補

			LOCAL0,		///< 独自の特殊な設定０
			LOCAL1,		///< 独自の特殊な設定１

			FIRST_I2C,	///< SCI ポートを簡易 I2C として使う場合、第１候補
			SECOND_I2C,	///< SCI ポートを簡易 I2C として使う場合、第２候補
			THIRD_I2C,	///< SCI ポートを簡易 I2C として使う場合、第３候補

			FIRST_SPI,	///< SCI ポートを簡易 SPI として使う場合、第１候補
			SECOND_SPI,	///< SCI ポートを簡易 SPI として使う場合、第２候補
			THIRD_SPI,	///< SCI ポートを簡易 SPI として使う場合、第３候補

			FIRST_MII,	///< ETHERC MII 接続、第１候補
			SECOND_MII,	///< ETHERC MII 接続、第２候補

			FIRST_RMII,	///< ETHERC RMII 接続、第１候補
			SECOND_RMII,///< ETHERC RMII 接続、第２候補
		};
    };
}