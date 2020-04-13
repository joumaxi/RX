//=====================================================================//
/*! @file
    @brief  RX64M/RX71M/RX65N/RX72N Audio サンプル @n
			SD-CARD にある MP3、WAV 形式のサウンドを再生する。@n
			オーディオインターフェースとして、マイコン内蔵 D/A 又は、SSIE を選択できる。@n
			※ D/A を使う場合「#define USE_DAC」@n
			※ SSIE を使う場合「#define USE_SSIE」
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2020 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include "common/renesas.hpp"
#include "common/cmt_io.hpp"
#include "common/fixed_fifo.hpp"
#include "common/sci_io.hpp"
#include "common/format.hpp"
#include "common/command.hpp"

#include "graphics/font8x16.hpp"
#include "graphics/kfont.hpp"
#include "graphics/font.hpp"
#include "graphics/graphics.hpp"

#include "common/dir_list.hpp"
#include "common/shell.hpp"

#include "sound/wav_in.hpp"
#include "sound/mp3_in.hpp"
#include "common/tpu_io.hpp"
#include "sound/sound_out.hpp"


namespace {

	typedef device::cmt_io<device::CMT0> CMT;
	CMT			cmt_;

#if defined(SIG_RX71M)
	typedef device::system_io<12'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT0, device::bitpos::B7> LED;
	typedef device::SCI1 SCI_CH;
	static const char* system_str_ = { "RX71M" };

	#define USE_DAC

#elif defined(SIG_RX64M)
	typedef device::system_io<12'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT0, device::bitpos::B7> LED;
	typedef device::SCI1 SCI_CH;
	static const char* system_str_ = { "RX64M" };

	#define USE_DAC

#elif defined(SIG_RX65N)
	/// for RX65N Envision Kit
	typedef device::system_io<12'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT7, device::bitpos::B0> LED;
	typedef device::PORT<device::PORT0, device::bitpos::B5> SW2;
	typedef device::SCI9 SCI_CH;
	static const char* system_str_ = { "RX65N" };

    typedef device::PORT<device::PORT6, device::bitpos::B4, 0> SDC_POWER;	///< 0 でＯＮ
    typedef device::NULL_PORT SDC_WPRT;		///< 書き込み禁止は使わない
    // RX65N Envision Kit の SDHI ポートは、候補３で指定できる
    typedef fatfs::sdhi_io<device::SDHI, SDC_POWER, SDC_WPRT,
        device::port_map::option::THIRD> SDC;

	#define USE_DAC
	#define USE_GLCDC

#elif defined(SIG_RX72N)
	/// for RX72N Envision Kit
	typedef device::system_io<16'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT4, device::bitpos::B0> LED;
	typedef device::PORT<device::PORT0, device::bitpos::B7> SW2;
	typedef device::SCI2 SCI_CH;
	static const char* system_str_ = { "RX72N" };

    typedef device::PORT<device::PORT4, device::bitpos::B2> SDC_POWER;
    typedef device::NULL_PORT SDC_WPRT;  ///< カード書き込み禁止ポート設定
    // RX72N Envision Kit の SDHI ポートは、候補３で指定できる
    typedef fatfs::sdhi_io<device::SDHI, SDC_POWER, SDC_WPRT,
		device::port_map::option::THIRD> SDC;

	typedef utils::ssie_io<device::SSIE1, device::DMAC1, 8192, 512> SSIE_IO;

	#define USE_SSIE
	#define USE_GLCDC

#endif

	typedef utils::fixed_fifo<char, 512>  RECV_BUFF;
	typedef utils::fixed_fifo<char, 1024> SEND_BUFF;
	typedef device::sci_io<SCI_CH, RECV_BUFF, SEND_BUFF> SCI;
	SCI			sci_;

	typedef utils::command<256> CMD;
	CMD 		cmd_;

	// SD-CARD ハードウェアーコンテキスト
    SDC			sdc_;

	// ディレクトリー・リスト・コンテキスト
	typedef utils::dir_list DLIST;
	DLIST		dlist_;

	// SD-CARD 操作コンテキスト
	typedef utils::shell<CMD> SHELL;
	SHELL		shell_(cmd_);

	// オーディオ・コーデック・コンテキスト
	typedef sound::mp3_in MP3_IN;
	MP3_IN		mp3_in_;
	typedef sound::wav_in WAV_IN;
	WAV_IN		wav_in_;

#ifdef USE_DAC
	typedef utils::sound_out<8192, 1024> SOUND_OUT;
	SOUND_OUT	sound_out_;

	class tpu_task {
	public:
		void operator() () {
			uint32_t tmp = wpos_;
			++wpos_;
			if((tmp ^ wpos_) & 64) {
				sound_out_.service(64);
			}
		}
	};

	typedef device::tpu_io<device::TPU0, tpu_task> TPU0;
	TPU0		tpu0_;

	void start_audio_()
	{
		{  // DMAC マネージャー開始
			uint8_t intr_level = 4;
			bool cpu_intr = true;
			auto ret = dmac_mgr_.start(tpu0_.get_intr_vec(), DMAC_MGR::trans_type::SP_DN_32,
				reinterpret_cast<uint32_t>(sound_out_.get_wave()), DAC::DADR0.address(),
				sound_out_.size(), intr_level, cpu_intr);
			if(!ret) {
				utils::format("DMAC Not start...\n");
			}
		}
	}
#endif

#ifdef USE_SSIE
	SSIE_IO		ssie_io_;
	SSIE_IO::SOUND_OUT&	sound_out_ = ssie_io_.at_sound_out();

	void start_audio_()
	{
		{  // SSIE 設定 RX72N Envision kit では、I2S, 48KHz, 32/24 ビットフォーマット
			uint8_t intr = 5;
			uint8_t adiv = 24'576'000 / 48'000 / (32 + 32);
			auto ret = ssie_io_.start(adiv,
				utils::ssie_t::FORM::I2S,
				utils::ssie_t::D_NUM::_32, utils::ssie_t::S_NUM::_32, intr);
///				utils::ssie_core::D_NUM::_24, utils::ssie_core::S_NUM::_32, intr);
			if(ret) {
				ssie_io_.enable_mute(false);
				ssie_io_.enable_send();  // 送信開始
				uint32_t bclk = 24'576'000 / static_cast<uint32_t>(adiv);
				utils::format("SSIE Start: BCLK: %u Hz\n") % bclk;
			} else {
				utils::format("SSIE No start...\n");
			}
		}
	}
#endif

	void update_led_()
	{
		static uint8_t n = 0;
		++n;
		if(n >= 30) {
			n = 0;
		}
		if(n < 10) {
			LED::P = 0;
		} else {
			LED::P = 1;
		}
	}


	sound::af_play::CTRL sound_ctrl_task_()
	{
		auto ctrl = sound::af_play::CTRL::NONE;

		if(sci_.recv_length() > 0) {
			auto ch = sci_.getch();
			if(ch == ' ') {			
				ctrl = sound::af_play::CTRL::PAUSE;
			} else if(ch == 0x08) {  // BS
				ctrl = sound::af_play::CTRL::REPLAY;
			} else if(ch == 0x0D) {  // RETURN
				ctrl = sound::af_play::CTRL::STOP;
			} else if(ch == 0x1b) {  // ESC
				ctrl = sound::af_play::CTRL::STOP;
				dlist_.stop();
			}
		}
		update_led_();

		return ctrl;
	}


	void sound_tag_task_(utils::file_io& fin, const sound::tag_t& tag)
	{
		utils::format("Album:  '%s'\n") % tag.get_album().c_str();
		utils::format("Title:  '%s'\n") % tag.get_title().c_str();
		utils::format("Artist: '%s'\n") % tag.get_artist().c_str();
		utils::format("Year:    %s\n") % tag.get_year().c_str();
		utils::format("Disc:    %s\n") % tag.get_disc().c_str();
		utils::format("Track:   %s\n") % tag.get_track().c_str();
	}


	void sound_update_task_(uint32_t t)
	{
		uint16_t sec = t % 60;
		uint16_t min = (t / 60) % 60;
		uint16_t hor = (t / 3600) % 24;
		utils::format("\r%02d:%02d:%02d") % hor % min % sec;
	}


	bool play_mp3_(const char* fname)
	{
		utils::file_io fin;
		if(!fin.open(fname, "rb")) {
			return false;
		}
		mp3_in_.set_ctrl_task(sound_ctrl_task_);
		mp3_in_.set_tag_task(sound_tag_task_);
		mp3_in_.set_update_task(sound_update_task_);
		bool ret = mp3_in_.decode(fin, sound_out_);
		fin.close();
		return ret;
	}


	bool play_wav_(const char* fname)
	{
		utils::file_io fin;
		if(!fin.open(fname, "rb")) {
			return false;
		}
		wav_in_.set_ctrl_task(sound_ctrl_task_);
		wav_in_.set_tag_task(sound_tag_task_);
		wav_in_.set_update_task(sound_update_task_);
		bool ret = wav_in_.decode(fin, sound_out_);
		fin.close();
		return ret;
	}


	void play_loop_(const char*, const char*);

	struct loop_t {
		const char*	start;
		bool	enable;
	};
	loop_t		loop_t_;

	void play_loop_func_(const char* name, const FILINFO* fi, bool dir, void* option)
	{
		loop_t* t = static_cast<loop_t*>(option);
		if(t->enable) {
			if(strcmp(name, t->start) != 0) {
				return;
			} else {
				t->enable = false;
			}
		}
		if(dir) {
			play_loop_(name, "");
		} else {
			const char* ext = strrchr(name, '.');
			if(ext != nullptr) {
				bool ret = true; 
				if(utils::str::strcmp_no_caps(ext, ".mp3") == 0) {
					ret = play_mp3_(name);
				} else if(utils::str::strcmp_no_caps(ext, ".wav") == 0) {
					ret = play_wav_(name);
				}
				if(!ret) {
					utils::format("Can't open audio file: '%s'\n") % name;
				}
			}
		}
	}


	void play_loop_(const char* root, const char* start)
	{
		loop_t_.start = start;
		if(strlen(start) != 0) {
			loop_t_.enable = true;
		} else {
			loop_t_.enable = false;
		}
		dlist_.start(root);
	}


	void cmd_service_()
	{
		// コマンド入力と、コマンド解析
		if(!cmd_.service()) {
			return;
		}
		if(shell_.analize()) {
			return;
		}

		auto cmdn = cmd_.get_words();
		if(cmd_.cmp_word(0, "play")) {  // play [xxx]
			if(cmdn >= 2) {
				char tmp[FF_MAX_LFN + 1];
				cmd_.get_word(1, tmp, sizeof(tmp));
				if(std::strcmp(tmp, "*") == 0) {
					play_loop_("", "");
				} else {
					play_loop_("", tmp);
				}
			} else {
				play_loop_("", "");
			}
		} else if(cmd_.cmp_word(0, "help") || cmd_.cmp_word(0, "?")) {
			shell_.help();
			utils::format("    play file-name\n");
		} else {
			utils::format("Command error: '%s'\n") % cmd_.get_command();
		}
	}
}


extern "C" {

	void set_sample_rate(uint32_t freq)
	{
#ifdef USE_SSIE
		ssie_io_.set_sampling_freq(freq);
#endif
#ifdef USE_DAC
		uint8_t intr_level = 5;
		if(!tpu0_.start(freq, intr_level)) {
			utils::format("TPU0 start error...\n");
		}
#endif
	}


	void sci_putch(char ch)
	{
		sci_.putch(ch);
	}


	void sci_puts(const char* str)
	{
		sci_.puts(str);
	}


	char sci_getch(void)
	{
		return sci_.getch();
	}


	uint16_t sci_length()
	{
		return sci_.recv_length();
	}


	DSTATUS disk_initialize(BYTE drv) {
		return sdc_.disk_initialize(drv);
	}


	DSTATUS disk_status(BYTE drv) {
		return sdc_.disk_status(drv);
	}


	DRESULT disk_read(BYTE drv, BYTE* buff, DWORD sector, UINT count) {
		return sdc_.disk_read(drv, buff, sector, count);
	}


	DRESULT disk_write(BYTE drv, const BYTE* buff, DWORD sector, UINT count) {
		return sdc_.disk_write(drv, buff, sector, count);
	}


	DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void* buff) {
		return sdc_.disk_ioctl(drv, ctrl, buff);
	}


	DWORD get_fattime(void) {
		return 0;
	}
}


int main(int argc, char** argv);

int main(int argc, char** argv)
{
	SYSTEM_IO::setup_system_clock();

	{  // SCI 設定
		uint8_t intr_lvl = 2;
		sci_.start(115200, intr_lvl);
	}

	{  // 時間計測タイマー（60Hz）
		uint8_t intr_lvl = 4;
		cmt_.start(60, intr_lvl);
	}

	utils::format("\r%s Start for Audio Sample\n") % system_str_;

	cmd_.set_prompt("# ");

	LED::DIR = 1;

	start_audio_();

	while(1) {
		cmt_.sync();

///		service_audio_();

		sdc_.service();

		dlist_.service(1, play_loop_func_, true, &loop_t_);

		cmd_service_();

		update_led_();
	}
}