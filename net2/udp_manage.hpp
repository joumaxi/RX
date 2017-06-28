#pragma once
//=========================================================================//
/*! @file
    @brief  UDP Protocol 全体管理 @n
			Copyright 2017 Kunihito Hiramatsu
    @author 平松邦仁 (hira@rvf-rc45.net)
*/
//=========================================================================//
#include "net2/udp.hpp"

namespace net {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  UDP マネージメント・クラス
		@param[in]	ETHER	イーサーネット・ドライバー・クラス
		@param[in]	NMAX	管理最大数（通常４個）
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template<class ETHER, uint32_t NMAX = 4>
	class udp_manage {

		ETHER&		eth_;

		udp			udp_[NMAX];

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
			@param[in]	eth		イーサーネット・ドライバー
		*/
		//-----------------------------------------------------------------//
		udp_manage(ETHER& eth) : eth_(eth)
		{ }


		//-----------------------------------------------------------------//
		/*!
			@brief  格納可能な最大サイズを返す
			@return 格納可能な最大サイズ
		*/
		//-----------------------------------------------------------------//
		uint32_t capacity() const noexcept { return NMAX; }


		//-----------------------------------------------------------------//
		/*!
			@brief  パース
			@param[in]	eh	イーサーネット・ヘッダー
			@param[in]	ih	IPV4 ヘッダー
			@param[in]	msg	メッセージ先頭
			@param[in]	len	メッセージ長
			@return エラーが無い場合「true」
		*/
		//-----------------------------------------------------------------//
		bool parse(const eth_h& eh, const ipv4_h& ih, const void* msg, int32_t len)
		{
			return true;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  オープン
			@param[in]	info	アドレス、MAC アドレス情報
			@param[in]	port	ポート
			@return ディスクリプタ
		*/
		//-----------------------------------------------------------------//
		int open(const arp_info& info, uint16_t port)
		{
			int ds = -1;


			return ds;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  サービス（１０ｍｓ毎に呼ぶ）
		*/
		//-----------------------------------------------------------------//
		void service()
		{
			for(uint32_t i = 0; i < NMAX; ++i) {
				udp_[i].service();
			}
		}
	};
}
