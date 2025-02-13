//=====================================================================//
/*! @file
    @brief  CPU 内蔵 Data Flash 操作サンプル @n
			RX64M, RX71M: @n
					12MHz のベースクロックを使用する @n
			　　　　P07 ピンにLEDを接続する @n
			RX65N (Renesas Envision kit RX65N): @n
					12MHz のベースクロックを使用する @n
			　　　　P70 に接続された LED を利用する @n
			RX24T: @n
					10MHz のベースクロックを使用する @n
			　　　　P00 ピンにLEDを接続する @n
			RX66T: @n
					10MHz のベースクロックを使用する @n
			　　　　P00 ピンにLEDを接続する @n
			RX72T: @n
					16MHz のベースクロックを使用する @n
			　　　　P01 ピンにLEDを接続する @n
			RX72N (Renesas Envision kit RX72N): @n
					16MHz のベースクロックを使用する @n
			　　　　P40 ピンにLEDを接続する
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2018, 2021 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include "common/renesas.hpp"

#include "common/cmt_mgr.hpp"
#include "common/fixed_fifo.hpp"
#include "common/sci_io.hpp"
#include "common/format.hpp"
#include "common/input.hpp"
#include "common/command.hpp"

namespace {

#if defined(SIG_RX71M)
	static const char* system_str_ = { "RX71M" };
	typedef device::PORT<device::PORT0, device::bitpos::B7> LED;
	typedef device::SCI1 SCI_CH;
#elif defined(SIG_RX64M)
	static const char* system_str_ = { "RX64M" };
	typedef device::PORT<device::PORT0, device::bitpos::B7> LED;
	typedef device::SCI1 SCI_CH;
#elif defined(SIG_RX65N)
	static const char* system_str_ = { "RX65N Envision Kit" };
	typedef device::PORT<device::PORT7, device::bitpos::B0> LED;
	typedef device::SCI9 SCI_CH;
#elif defined(SIG_RX24T)
	static const char* system_str_ = { "RX24T" };
	typedef device::PORT<device::PORT0, device::bitpos::B0> LED;
	typedef device::SCI1 SCI_CH;
	#define SINGLE
#elif defined(SIG_RX66T)
	static const char* system_str_ = { "RX66T" };
	typedef device::PORT<device::PORT0, device::bitpos::B0> LED;
	typedef device::SCI1 SCI_CH;
#elif defined(SIG_RX72T)
	static const char* system_str_ = { "RX72T" };
	typedef device::PORT<device::PORT0, device::bitpos::B1> LED;
	typedef device::SCI1 SCI_CH;
#elif defined(SIG_RX72N)
	static const char* system_str_ = { "RX72N Envision Kit" };
	typedef device::PORT<device::PORT4, device::bitpos::B0> LED;
	typedef device::SCI2 SCI_CH;
#endif
	typedef device::system_io<> SYSTEM_IO;

	typedef utils::fixed_fifo<char, 256> RXB;  ///< SCI/RX (RECV) バッファの定義
	typedef utils::fixed_fifo<char, 512> TXB;  ///< SCI/TX (SEND) バッファの定義

	typedef device::sci_io<SCI_CH, RXB, TXB> SCI;
	SCI			sci_;

	typedef device::cmt_mgr<device::CMT0, utils::null_task> CMT;
	CMT			cmt_;

	typedef utils::command<256> COMMAND;
	COMMAND		command_;

	typedef device::flash_io FLASH_IO;
	FLASH_IO	flash_io_;


	void dump_(uint16_t org, uint16_t len)
	{
		bool adr = true;
		for(uint16_t i = 0; i < len; ++i) {
			if(adr) {
				utils::format("0x%04X:") % static_cast<uint32_t>(org);
				adr = false;
			}
			uint8_t dat = flash_io_.read(org);
			utils::format(" %02X%") % static_cast<uint32_t>(dat);
			uint16_t a = org;
			++org;
			if(((a ^ org) & 16) != 0) {
				utils::format("\n");
				adr = true;
			}
		}
		if(!adr) {
			utils::format("\n");
		}
	}


	void command_service_()
	{
		// コマンド入力と、コマンド解析
		if(!command_.service()) {
			return;
		}

		auto n = command_.get_words();
		if(command_.cmp_word(0, "erase") && n >= 2) {
			bool f = false;
			char buff[16];
			if(command_.get_word(1, buff, sizeof(buff))) {
				int bank = 0;
				if((utils::input("%d", buff) % bank).status()) {
					if(static_cast<uint32_t>(bank) < FLASH_IO::data_flash_bank) {
						f = flash_io_.erase(bank);
						if(!f) {
							utils::format("Erase error: bank %d\n") % bank;
							f = true;
						} else {
							f = flash_io_.erase_check(bank);
							if(!f) {
								utils::format("Erase Check error: bank %d\n") % bank;
								f = true;
							}
						}
					}
				}
			}
			if(!f) {
				utils::format("Erase param error: '%s'\n") % buff;
			}
		} else if((command_.cmp_word(0, "r") || command_.cmp_word(0, "read")) && n >= 2) {
			bool f = false;
			uint32_t org = 0;
			char buff[16];
			if(command_.get_word(1, buff, sizeof(buff))) {
				if((utils::input("%x", buff) % org).status()) {
					f = true;
				}
			}
			uint32_t len = 1;
			if(n >= 3) {
				if(command_.get_word(2, buff, sizeof(buff))) {
					if((utils::input("%x", buff) % len).status()) {
						f = true;
					}
				}
			}
			if(f) {
				dump_(org, len);
			} else {
				utils::format("Read param error: %s\n") % command_.get_command();
			}
		} else if(command_.cmp_word(0, "write") && n >= 3) {
			char buff[16];
			if(command_.get_word(1, buff, sizeof(buff))) {
				bool f = false;
				uint32_t org = 0;
				if((utils::input("%x", buff) % org).status()) {
					for(uint8_t i = 2; i < n; ++i) {
						if(command_.get_word(i, buff, sizeof(buff))) {
							uint32_t data = 0;
							if(!(utils::input("%x", buff) % data).status()) {
								break;
							}
#ifdef SINGLE							
							uint32_t inc = 1;
							uint8_t tmp[1];
							tmp[0] = data;
							if(!flash_io_.write(org, tmp, 1)) {
#else
							uint32_t inc = 4;
							uint8_t tmp[4];
							tmp[3] = data;
							tmp[2] = data >> 8;
							tmp[1] = data >> 16;
							tmp[0] = data >> 24;
							if(!flash_io_.write(org, tmp, 4)) {
#endif

#ifdef SINGLE
								utils::format("Write error: %04X: %02X\n")
#else
								utils::format("Write error: %04X: %08X\n")
#endif
									% static_cast<uint32_t>(org) % data;
							}
							org += inc;
						}
					}
					f = true;
				}
				if(!f) {
					utils::format("Write param error: %s\n") % command_.get_command();
				}
			}
		} else if(command_.cmp_word(0, "uid")) {
			if(FLASH_IO::get_uid_num() == 0) {
				utils::format("Unique ID not define.\n");
			} else {
				for(uint32_t i = 0; i < FLASH_IO::get_uid_num(); ++i) {
					auto id = FLASH_IO::get_uid(i);
					utils::format("Unique ID%d: %08X\n") % i % id;
				}
			}
		} else if(command_.cmp_word(0, "?") || command_.cmp_word(0, "help")) {
			utils::format("erase [bank] (erase 0 to %d)\n") % FLASH_IO::data_flash_bank;
			utils::format("r[ead] org [end] (read)\n");
			utils::format("write org data... (write)\n");
			utils::format("uid (unique ID list)\n");
		} else {
			const char* p = command_.get_command();
			if(p[0]) {
				utils::format("command error: '%s'\n") % p;
			}
		}
	}
}


extern "C" {

	// syscalls.c から呼ばれる、標準出力（stdout, stderr）
	void sci_putch(char ch)
	{
		sci_.putch(ch);
	}

	void sci_puts(const char* str)
	{
		sci_.puts(str);
	}

	// syscalls.c から呼ばれる、標準入力（stdin）
	char sci_getch(void)
	{
		return sci_.getch();
	}

	uint16_t sci_length()
	{
		return sci_.recv_length();
	}
}


int main(int argc, char** argv);

int main(int argc, char** argv)
{
	SYSTEM_IO::boost_master_clock();

	{  // タイマー設定（100Hz）
		uint8_t intr = 4;
		uint32_t freq = 100;
		cmt_.start(freq, intr);
	}

	{  // SCI の開始
		uint8_t intr = 2;        // 割り込みレベル
		uint32_t baud = 115200;  // ボーレート
		sci_.start(baud, intr);
	}

	{
		auto clk = device::clock_profile::ICLK / 1'000'000;
		utils::format("Start Data Flash sample for '%s' %u [MHz]\n") % system_str_ % clk;
		auto fclk = device::clock_profile::FCLK / 1'000'000;
		utils::format("Flash drive clock: %u [MHz]\n") % fclk;
		utils::format("Data Flash size: %08X\n") % FLASH_IO::data_flash_size;
	}

	{  // DataFlash 開始
		flash_io_.start();
	}

	command_.set_prompt("# ");

	LED::DIR = 1;
	uint8_t cnt = 0;
	while(1) {
		cmt_.sync();

		command_service_();

		++cnt;
		if(cnt >= 50) {
			cnt = 0;
		}
		if(cnt < 25) {
			LED::P = 0;
		} else {
			LED::P = 1;
		}
	}
}
