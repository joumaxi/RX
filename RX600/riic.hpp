#pragma once
//=====================================================================//
/*!	@file
	@brief	RX24T/RX66T/RX64M/RX71M/RX65N RIICa 定義
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2016, 2022 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include "common/device.hpp"

namespace device {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  I 2 C バスインタフェース 定義
		@param[in]	base	ベース・アドレス
		@param[in]	per		ペリフェラル型
		@param[in]	INT		割り込みベクター型
		@param[in]	eev		「通信エラー、通信イベント」ベクター
		@param[in]	rxv		「受信データフル」ベクター
		@param[in]	txv		「送信データエンプティ」ベクター
		@param[in]	tev		「送信終了」ベクター
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev>
	struct riic_t {

		static constexpr auto PERIPHERAL = per;	///< ペリフェラル型
		static constexpr auto TX_VEC = txv;		///< 受信割り込みベクター
		static constexpr auto RX_VEC = rxv;		///< 送信割り込みベクター
		static constexpr auto EE_VEC = eev;		///< 通信エラー、イベント割り込みベクター
		static constexpr auto TE_VEC = tev;		///< 送信終了割り込みベクター

		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスコントロールレジスタ 1 (ICCR1)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct iccr1_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t<io_, bitpos::B0> SDAI;
			bit_rw_t<io_, bitpos::B1> SCLI;
			bit_rw_t<io_, bitpos::B2> SDAO;
			bit_rw_t<io_, bitpos::B3> SCLO;
			bit_rw_t<io_, bitpos::B4> SOWP;
			bit_rw_t<io_, bitpos::B5> CLO;
			bit_rw_t<io_, bitpos::B6> IICRST;
			bit_rw_t<io_, bitpos::B7> ICE;
		};
		typedef iccr1_t<base + 0x00> ICCR1_;
		static ICCR1_ ICCR1;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスコントロールレジスタ 2 (ICCR2)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct iccr2_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t<io_, bitpos::B1> ST;
			bit_rw_t<io_, bitpos::B2> RS;
			bit_rw_t<io_, bitpos::B3> SP;
			bit_rw_t<io_, bitpos::B5> TRS;
			bit_rw_t<io_, bitpos::B6> MST;
			bit_rw_t<io_, bitpos::B7> BBSY;
		};
		typedef iccr2_t<base + 0x01> ICCR2_;
		static ICCR2_ ICCR2;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスモードレジスタ 1 (ICMR1)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icmr1_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bits_rw_t<io_, bitpos::B0, 3> BC;
			bit_rw_t <io_, bitpos::B3>    BCWP;
			bits_rw_t<io_, bitpos::B4, 3> CKS;
			bit_rw_t <io_, bitpos::B7>    MTWP;
		};
		typedef icmr1_t<base + 0x02> ICMR1_;
		static ICMR1_ ICMR1;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスモードレジスタ 2 (ICMR2)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icmr2_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t <io_, bitpos::B0>    TMOS;
			bit_rw_t <io_, bitpos::B1>    TMOL;
			bit_rw_t <io_, bitpos::B2>    TMOH;
			bits_rw_t<io_, bitpos::B4, 3> SDDL;
			bit_rw_t <io_, bitpos::B7>    DLCS;
		};
		typedef icmr2_t<base + 0x03> ICMR2_;
		static ICMR2_ ICMR2;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスモードレジスタ 3 (ICMR3)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icmr3_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bits_rw_t<io_, bitpos::B0, 2> NF;
			bit_rw_t <io_, bitpos::B2>    ACKBR;
			bit_rw_t <io_, bitpos::B3>    ACKBT;
			bit_rw_t <io_, bitpos::B4>    ACKWP;
			bit_rw_t <io_, bitpos::B5>    RDRFS;
			bit_rw_t <io_, bitpos::B6>    WAIT;
			bit_rw_t <io_, bitpos::B7>    SMBS;
		};
		typedef icmr3_t<base + 0x04> ICMR3_;
		static ICMR3_ ICMR3;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスファンクション許可レジスタ (ICFER)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icfer_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t<io_, bitpos::B0> TMOE;
			bit_rw_t<io_, bitpos::B1> MALE;
			bit_rw_t<io_, bitpos::B2> NALE;
			bit_rw_t<io_, bitpos::B3> SALE;
			bit_rw_t<io_, bitpos::B4> NACKE;
			bit_rw_t<io_, bitpos::B5> NFE;
			bit_rw_t<io_, bitpos::B6> SCLE;
			bit_rw_t<io_, bitpos::B7> FMPE;
		};
		typedef icfer_t<base + 0x05> ICFER_;
		static ICFER_ ICFER;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスステータス許可レジスタ (ICSER)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icser_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t<io_, bitpos::B0> SAR0E;
			bit_rw_t<io_, bitpos::B1> SAR1E;
			bit_rw_t<io_, bitpos::B2> SAR2E;
			bit_rw_t<io_, bitpos::B3> GCAE;
			bit_rw_t<io_, bitpos::B5> DIDE;
			bit_rw_t<io_, bitpos::B7> HOAE;
		};
		typedef icser_t<base + 0x06> ICSER_;
		static ICSER_ ICSER;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バス割り込み許可レジスタ (ICIER)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icier_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t<io_, bitpos::B0> TMOIE;
			bit_rw_t<io_, bitpos::B1> ALIE;
			bit_rw_t<io_, bitpos::B2> STIE;
			bit_rw_t<io_, bitpos::B3> SPIE;
			bit_rw_t<io_, bitpos::B4> NAKIE;
			bit_rw_t<io_, bitpos::B5> RIE;
			bit_rw_t<io_, bitpos::B6> TEIE;
			bit_rw_t<io_, bitpos::B7> TIE;
		};
		typedef icier_t<base + 0x07> ICIER_;
		static ICIER_ ICIER;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスステータスレジスタ 1 (ICSR1)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icsr1_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t<io_, bitpos::B0> AAS0;
			bit_rw_t<io_, bitpos::B1> AAS1;
			bit_rw_t<io_, bitpos::B2> AAS2;
			bit_rw_t<io_, bitpos::B3> GCA;
			bit_rw_t<io_, bitpos::B5> DID;
			bit_rw_t<io_, bitpos::B7> HOA;
		};
		typedef icsr1_t<base + 0x08> ICSR1_;
		static ICSR1_ ICSR1;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスステータスレジスタ 2 (ICSR2)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icsr2_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t<io_, bitpos::B0> TMOF;
			bit_rw_t<io_, bitpos::B1> AL;
			bit_rw_t<io_, bitpos::B2> START;
			bit_rw_t<io_, bitpos::B3> STOP;
			bit_rw_t<io_, bitpos::B4> NACKF;
			bit_rw_t<io_, bitpos::B5> RDRF;
			bit_rw_t<io_, bitpos::B6> TEND;
			bit_rw_t<io_, bitpos::B7> TDRE;
		};
		typedef icsr2_t<base + 0x09> ICSR2_;
		static ICSR2_ ICSR2;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  スレーブアドレスレジスタ Ly (SARLy) (y=0 ～ 2)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct sarly_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t <io_, bitpos::B0>    SVA0;
			bits_rw_t<io_, bitpos::B1, 7> SVA;
		};
		typedef sarly_t<base + 0x0A> SARL0_;
		typedef sarly_t<base + 0x0C> SARL1_;
		typedef sarly_t<base + 0x0E> SARL2_;
		static SARL0_ SARL0;
		static SARL1_ SARL1;
		static SARL2_ SARL2;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  スレーブアドレスレジスタ Uy (SARUy) (y=0 ～ 2)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct saruy_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bit_rw_t <io_, bitpos::B0>    FS;
			bits_rw_t<io_, bitpos::B1, 2> SVA;
		};
		typedef saruy_t<base + 0x0B> SARU0_;
		typedef saruy_t<base + 0x0D> SARU1_;
		typedef saruy_t<base + 0x0F> SARU2_;
		static SARU0_ SARU0;
		static SARU1_ SARU1;
		static SARU2_ SARU2;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスビットレート Low レジスタ (ICBRL)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icbrl_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bits_rw_t <io_, bitpos::B0, 5> BRL;
		};
		typedef icbrl_t<base + 0x10> ICBRL_;
		static ICBRL_ ICBRL;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バスビットレート High レジスタ (ICBRH)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		template <uint32_t ofs>
		struct icbrh_t : public rw8_t<ofs> {
			typedef rw8_t<ofs> io_;
			using io_::operator =;
			using io_::operator ();
			using io_::operator |=;
			using io_::operator &=;

			bits_rw_t <io_, bitpos::B0, 5> BRH;
		};
		typedef icbrh_t<base + 0x11> ICBRH_;
		static ICBRH_ ICBRH;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バス送信データレジスタ (ICDRT)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		typedef rw8_t<base + 0x12> ICDRT_;
		static ICDRT_ ICDRT;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  I 2 C バス受信データレジスタ (ICDRR)
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		typedef rw8_t<base + 0x13> ICDRR_;
		static ICDRR_ ICDRR;


		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief  チャネルを返す
			@return チャネル
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		static uint32_t get_chanel() {
			return (base >> 5) & 1;
		}
	};
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICCR1_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICCR1;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICCR2_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICCR2;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICMR1_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICMR1;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICMR2_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICMR2;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICMR3_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICMR3;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICFER_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICFER;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICSER_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICSER;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICIER_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICIER;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICSR1_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICSR1;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICSR2_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICSR2;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::SARL0_
		riic_t<base, per, txv, rxv, INT, eev, tev>::SARL0;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::SARL1_
		riic_t<base, per, txv, rxv, INT, eev, tev>::SARL1;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::SARL2_
		riic_t<base, per, txv, rxv, INT, eev, tev>::SARL2;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::SARU0_
		riic_t<base, per, txv, rxv, INT, eev, tev>::SARU0;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::SARU1_
		riic_t<base, per, txv, rxv, INT, eev, tev>::SARU1;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::SARU2_
		riic_t<base, per, txv, rxv, INT, eev, tev>::SARU2;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICBRL_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICBRL;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICBRH_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICBRH;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICDRT_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICDRT;
	template <uint32_t base, peripheral per, ICU::VECTOR txv, ICU::VECTOR rxv,
		typename INT, INT eev, INT tev> typename riic_t<base, per, txv, rxv, INT, eev, tev>::ICDRR_
		riic_t<base, per, txv, rxv, INT, eev, tev>::ICDRR;

#if defined(SIG_RX24T)
	typedef riic_t<0x00088300, peripheral::RIIC0, ICU::VECTOR::RIIC_TXI0, ICU::VECTOR::RIIC_RXI0,
		ICU::VECTOR, ICU::VECTOR::RIIC_EEI0, ICU::VECTOR::RIIC_TEI0> RIIC0;
#elif defined(SIG_RX64M) || defined(SIG_RX71M)
	typedef riic_t<0x00088300, peripheral::RIIC0, ICU::VECTOR::RIIC_TXI0, ICU::VECTOR::RIIC_RXI0,
		ICU::VECTOR_BL1, ICU::VECTOR_BL1::EEI0, ICU::VECTOR_BL1::TEI0> RIIC0;
	typedef riic_t<0x00088340, peripheral::RIIC2, ICU::VECTOR::RIIC_TXI2, ICU::VECTOR::RIIC_RXI2,
		ICU::VECTOR_BL1, ICU::VECTOR_BL1::EEI2, ICU::VECTOR_BL1::TEI2> RIIC2;
#elif defined(SIG_RX65N) || defined(SIG_RX72M) || defined(SIG_RX72N) || defined(SIG_RX66N)
	typedef riic_t<0x00088300, peripheral::RIIC0, ICU::VECTOR::RIIC_TXI0, ICU::VECTOR::RIIC_RXI0,
		ICU::VECTOR_BL1, ICU::VECTOR_BL1::EEI0, ICU::VECTOR_BL1::TEI0> RIIC0;
	typedef riic_t<0x00088320, peripheral::RIIC1, ICU::VECTOR::RIIC_TXI1, ICU::VECTOR::RIIC_RXI1,
		ICU::VECTOR_BL1, ICU::VECTOR_BL1::EEI1, ICU::VECTOR_BL1::TEI1> RIIC1;
	typedef riic_t<0x00088340, peripheral::RIIC2, ICU::VECTOR::RIIC_TXI2, ICU::VECTOR::RIIC_RXI2,
		ICU::VECTOR_BL1, ICU::VECTOR_BL1::EEI2, ICU::VECTOR_BL1::TEI2> RIIC2;
#elif defined(SIG_RX66T) || defined(SIG_RX72T)
	typedef riic_t<0x00088300, peripheral::RIIC0, ICU::VECTOR::RIIC_TXI0, ICU::VECTOR::RIIC_RXI0,
		ICU::VECTOR_BL1, ICU::VECTOR_BL1::EEI0, ICU::VECTOR_BL1::TEI0> RIIC0;
#endif
}
