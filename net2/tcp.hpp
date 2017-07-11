#pragma once
//=========================================================================//
/*! @file
    @brief  TCP Protocol @n
			Copyright 2017 Kunihito Hiramatsu
    @author 平松邦仁 (hira@rvf-rc45.net)
*/
//=========================================================================//
#include "net2/net_st.hpp"
#include "common/fixed_block.hpp"

#define TCP_DEBUG

extern "C" {
	uint32_t get_counter();
}

namespace net {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  TCP プロトコロ・クラス
		@param[in]	ETHD	イーサーネット・ドライバー・クラス
		@param[in]	NMAX	管理最大数
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template<class ETHD, uint32_t NMAX>
	class tcp {

#ifndef TCP_DEBUG
		typedef utils::null_format debug_format;
#else
		typedef utils::format debug_format;
#endif

		static const uint16_t SEND_MAX      = 1460;      ///< パケットの最大数
		static const uint16_t SYN_TIMEOUT   = 30 * 100;  ///< SYN_RCVD を送って、ACK が返るまでの最大時間
		static const uint16_t RESEND_WAIT   = 90;        ///< 0.9 sec (unit: 10ms)再送
		static const uint16_t RESEND_SPAN   = 20;        ///< 再送に対する揺らぎ
		static const uint16_t RESEND_LIMIT  = 7;         ///< 再送の最大回数

		static const uint16_t CLOSE_TIME_OUT = 5 * 1000 / 10;  // 5 sec (unit: 10ms)

		ETHD&		ethd_;

		net_info&	info_;

		net_state	last_state_;

		enum class recv_task : uint8_t {
			idle,

			listen_server,
			syn_rcvd,
			established_server,

			syn_cent,
			established_client,
			fin_wait1,
			fin_wait2,
			time_wait,

			sync_close,
			close,
		};

		enum class send_task : uint8_t {
			idle,
			sync_mac,

			main,

			sync_close,
			close,
		};

		typedef memory<4096>  RECV_B;
		typedef memory<4096>  SEND_B;

		struct context {
			ip_adrs		adrs_;
			uint8_t		mac_[6];
			uint16_t	cn_port_;
			uint16_t	port_;
			uint16_t	send_time_;

			// IPV4 関係
			uint16_t	send_max_;
			uint16_t	id_;
			uint16_t	offset_;
			uint8_t		life_;

			send_task	send_task_;

			RECV_B		recv_;
			SEND_B		send_;

			uint32_t	timer_ref_;
			uint32_t	net_time_ref_;
			uint32_t	send_time_org_;
			uint32_t	recv_seq_;
			uint32_t	recv_ack_;
			uint32_t	send_seq_;
			uint32_t	send_ack_;
			uint16_t	flags_;
			uint16_t	window_;
			uint16_t	urgent_ptr_;
			uint16_t	timeout_;
			uint16_t	resend_cnt_;

			volatile uint16_t	send_len_;
			volatile uint16_t	send_wait_;
			volatile recv_task	recv_task_;
			volatile bool		server_;
			volatile bool		recv_fin_;
			volatile bool		send_fin_;
			bool	close_req_;

			void reset(const ip_adrs& adrs, uint16_t port, bool server)
			{
				adrs_ = adrs;
				std::memset(mac_, 0x00, 6);
				cn_port_ = port;
				port_ = 0;  // 初期は「０」
				send_time_ = 0;
				
				send_max_ = SEND_MAX;  // 通常の最大転送バイト
				id_ = 0;  // 識別子の初期値
				life_ = 255;  // 生存時間初期値（ルーターの通過台数）
				offset_ = 0;  // フラグメント・オフセット

				send_task_ = send_task::idle;

				recv_.clear();
				send_.clear();

				timer_ref_ = 0;
				net_time_ref_ = 0;
				send_time_org_ = 0;
				recv_seq_ = 0;
				recv_ack_ = 0;
				send_seq_ = tools::rand() & 0xff7fffff;
				send_ack_ = 0;
				flags_ = 0x00;
				window_ = 0xffff;
				urgent_ptr_ = 0;
				timeout_ = 0;
				resend_cnt_ = 0;

				send_len_ = 0;
				send_wait_ = 0;

				recv_task_ = recv_task::idle;
				server_ = server;

				recv_fin_ = false;
				send_fin_ = false;

				close_req_ = false;
			}
		};

		typedef udp_tcp_common<context, NMAX> COMMON;
		COMMON		common_;


		struct frame_t {
			eth_h	eh_;
			ipv4_h	ipv4_;
			tcp_h	tcp_;

			void* next(frame_t* org) {
				return static_cast<void*>(reinterpret_cast<uint8_t*>(org) + sizeof(frame_t));
			}

			const void* next(const frame_t* org) {
				return static_cast<const void*>(reinterpret_cast<const uint8_t*>(org) + sizeof(frame_t));
			}
		} __attribute__((__packed__));


		// TCP checksum header 
		struct csum_h {
			ip_adrs		src_;
			ip_adrs		dst_;
			uint16_t	fix_;
			uint16_t	len_;
		};


		uint16_t make_send_wait_()
		{
			return RESEND_WAIT - (rand() % RESEND_SPAN); 
		}


		uint32_t delta_time_(uint32_t ref)
		{
			uint32_t n = get_counter();
			uint32_t ret = 0;
			if(n > ref) {  // unit 10ms
				ret = n - ref;
			} else {
				ret = (~ref) + 1 + n;
			}
			return ret;
		}


		uint16_t make_seg_(context& ctx, const uint8_t* dst_mac, const uint8_t* dst_ip, frame_t& t, bool data)
		{
			t.eh_.set_dst(dst_mac);  // 転送先の MAC
			t.eh_.set_src(info_.mac);      // 転送元の MAC
			t.eh_.set_type(eth_type::IPV4);

			uint16_t all = sizeof(frame_t);
			uint8_t* p = reinterpret_cast<uint8_t*>(&t) + all;

			// 送信データを上乗せする場合
			uint16_t send_len = 0;
			if(data && ctx.send_len_ == 0 && ctx.recv_task_ == recv_task::established_server
			  && ctx.send_task_ == send_task::main) {
				send_len = ctx.send_.length();
				if(send_len > 0) {
					uint16_t max_len = ctx.send_max_ - sizeof(frame_t);
					if(max_len < send_len) {
						send_len = max_len;
					}
					ctx.send_len_ = send_len;
					ctx.send_.get(p, send_len, false);
//					debug_format("TCP Send: %d\n") % send_len;
					all += send_len;
					p += send_len;
					ctx.flags_ |= tcp_h::MASK_PSH;
					ctx.send_wait_ = make_send_wait_();
					ctx.send_time_org_ = get_counter();
				}
			}

			t.ipv4_.set_ver_hlen(0x45);
			t.ipv4_.set_type(0x00);
			t.ipv4_.set_length(all - sizeof(eth_h));
			t.ipv4_.set_id(ctx.id_);
			t.ipv4_.set_f_offset(ctx.offset_);
			t.ipv4_.set_life(ctx.life_);
			t.ipv4_.set_protocol(ipv4_h::protocol::TCP);
			t.ipv4_.set_csum(0);
			t.ipv4_.set_src_ipa(info_.ip.get());
			t.ipv4_.set_dst_ipa(dst_ip);
			t.ipv4_.set_csum(tools::calc_sum(&t.ipv4_, sizeof(ipv4_h)));

			uint16_t tcp_len = all - sizeof(eth_h) - sizeof(ipv4_h);
			t.tcp_.set_src_port(ctx.cn_port_);
			t.tcp_.set_dst_port(ctx.port_);
			t.tcp_.set_seq(ctx.send_seq_);
			t.tcp_.set_ack(ctx.send_ack_);
			t.tcp_.set_length(tcp_len - send_len);  // TCP Header Length
			t.tcp_.set_flags(ctx.flags_);
			t.tcp_.set_window(ctx.window_);
			t.tcp_.set_csum(0x0000);
			t.tcp_.set_urgent_ptr(ctx.urgent_ptr_);

			// ６０バイトに満たない場合は、ダミー・データ（０）を追加する。
			while(all < 60) {
				*p++ = 0;
				++all;
			}

			csum_h smh;
			smh.src_.set(info_.ip.get());
			smh.dst_.set(dst_ip);
			smh.fix_ = 0x0600;
			smh.len_ = tools::htons(tcp_len);
			uint16_t sum = tools::calc_sum(&smh, sizeof(csum_h));
			sum = tools::calc_sum(&t.tcp_, tcp_len, ~sum);
			t.tcp_.set_csum(sum);

			return all;
		}


		frame_t* get_send_frame_()
		{
			void* dst;
			uint16_t max;
			if(ethd_.send_buff(&dst, max) != 0) {
				debug_format("TCP Frame ether_io fail\n");
				return nullptr;
			}
			return static_cast<frame_t*>(dst);
		}


		bool recv_(context& ctx, const eth_h& eh, const ipv4_h& ih, const tcp_h* tcp)
		{
			// TCP サムの計算
			uint16_t len = ih.get_length() - sizeof(ipv4_h);
			csum_h smh;
			smh.src_.set(ih.get_src_ipa());
			smh.dst_.set(ih.get_dst_ipa());
			smh.fix_ = 0x0600;
			smh.len_ = tools::htons(len);
			uint16_t sum = tools::calc_sum(&smh, sizeof(smh));
			sum = tools::calc_sum(tcp, len, ~sum);
			if(sum != 0) {
				utils::format("\nTCP Frame(%d) sum error: %04X -> %04X\n")
					% len % tcp->get_csum() % sum;
				return false;
			}
			uint16_t opt_len = tcp->get_length() - sizeof(tcp_h);  // TCP ヘッダー・オプション・サイズ
			uint16_t recv_len = len - tcp->get_length();  // 受信データサイズ
			bool send = false;
			bool data = false;  // データ転送を「相乗り」しない
			ctx.flags_ = 0;
			ctx.recv_seq_ = tcp->get_seq();
			ctx.recv_ack_ = tcp->get_ack();
			ctx.recv_fin_ = tcp->get_flag_fin(); 

			switch(ctx.recv_task_) {

			case recv_task::listen_server:
				if(ctx.server_ && ctx.adrs_.is_any()) {
					std::memcpy(ctx.mac_, eh.get_src(), 6);
					ctx.adrs_ = ih.get_src_ipa();
				}
// dump(*tcp, " (LISTEN)");
// utils::format("(LIS) RECV:   SEQ: 0x%08X, ACK: 0x%08X (%d)\n") % ctx.recv_seq_ % ctx.recv_ack_ % recv_len;
// utils::format("(LIS) SERVER: SEQ: 0x%08X, ACK: 0x%08X\n") % ctx.send_seq_ % ctx.send_ack_;
				if(tcp->get_flag_syn()) {
					send = true;
					ctx.send_ack_ = ctx.recv_seq_;
					ctx.flags_ |= tcp_h::MASK_SYN | tcp_h::MASK_ACK;
					++ctx.send_ack_;
					ctx.timer_ref_ = get_counter();
					ctx.recv_task_ = recv_task::syn_rcvd;
				}
				if(ctx.recv_fin_) {
					send = true;
					ctx.flags_ |= tcp_h::MASK_ACK;
					ctx.recv_task_ = recv_task::sync_close;
				}
				break;

			case recv_task::syn_rcvd:
// utils::format("(SYN) RECV:   SEQ: 0x%08X, ACK: 0x%08X (%d)\n") % ctx.recv_seq_ % ctx.recv_ack_ % recv_len;
// utils::format("(SYN) SERVER: SEQ: 0x%08X, ACK: 0x%08X\n") % ctx.send_seq_ % ctx.send_ack_;
				if(tcp->get_flag_ack()
						&& ctx.recv_seq_ == ctx.send_ack_
						&& ctx.recv_ack_ == (ctx.send_seq_ + 1)) {
					ctx.net_time_ref_ = delta_time_(ctx.timer_ref_);
					if(ctx.net_time_ref_ == 0) ++ctx.net_time_ref_;  // ０の場合、最低値を設定
					++ctx.send_seq_;
					ctx.recv_task_ = recv_task::established_server;
				} else {
					++ctx.timeout_;
					if(ctx.timeout_ >= SYN_TIMEOUT) {
						debug_format("TCP SYN_RCVD Timeout\n");
						send = true;
						ctx.flags_ |= tcp_h::MASK_FIN;
						ctx.recv_task_ = recv_task::sync_close;
					}
				}
				if(ctx.recv_fin_) {
					send = true;
					ctx.flags_ |= tcp_h::MASK_ACK;
					ctx.recv_task_ = recv_task::sync_close;
				}
				break;

			case recv_task::established_server:
// utils::format("(EST) RECV:   SEQ: 0x%08X, ACK: 0x%08X (%d)\n") % ctx.recv_seq_ % ctx.recv_ack_ % recv_len;
// utils::format("(EST) SERVER: SEQ: 0x%08X, ACK: 0x%08X (%d)\n") % ctx.send_seq_ % ctx.send_ack_ % ctx.send_len_;
				if(tcp->get_flag_ack()) {
					if(ctx.send_len_ > 0) {  // 転送データがあるなら ACK 確認
						if(ctx.recv_seq_ == ctx.send_ack_
						  && ctx.recv_ack_ >= (ctx.send_seq_ + ctx.send_len_)) {
							ctx.send_.get_go(ctx.send_len_);  // 転送データが無事送れたので、バッファを進める
							ctx.send_seq_ += ctx.send_len_;
							ctx.send_len_ = 0;
							ctx.send_wait_ = 0;
							ctx.resend_cnt_ = 0;
						}
					}
				} else {
					// データ転送したが、ACK が来ない場合に、再送を行う
					if(ctx.send_len_ > 0) {
						uint32_t d = delta_time_(ctx.send_time_org_);
						if(d >= ctx.send_wait_) {
							send = true;
							ctx.send_len_ = 0;  // 再送
							++ctx.resend_cnt_;
							// 再送回数がリミットに達したらリセットを送って強制終了
							if(ctx.resend_cnt_ >= RESEND_LIMIT) {
								ctx.flags_ |= tcp_h::MASK_RST;
								ctx.recv_task_ = recv_task::close;
								ctx.send_task_ = send_task::close;
								debug_format("TCP RST\n");
							} else {
								debug_format("TCP ReSend: %d\n") % ctx.send_len_;
							}
						}
					}
				}

				if(tcp->get_flag_psh()) {  // データ受信
// utils::format("PSH:    SEQ: 0x%08X, ACK: 0x%08X (%d)\n") % ctx.recv_seq_ % ctx.recv_ack_ % recv_len;
// utils::format("SERVER: SEQ: 0x%08X, ACK: 0x%08X\n") % ctx.send_seq_ % ctx.send_ack_;
					if(recv_len > 0 && recv_len < (ctx.recv_.size() - ctx.recv_.length() - 1)) {
						if(ctx.recv_ack_ == ctx.send_seq_ && ctx.recv_seq_ >= ctx.send_ack_) {
							send = true;
							const uint8_t* org = reinterpret_cast<const uint8_t*>(tcp);
							org += tcp->get_length();
							ctx.recv_.put(org, recv_len);
							ctx.send_ack_ += recv_len;
							ctx.flags_ |= tcp_h::MASK_ACK;
						}
					}
				}
				if(ctx.recv_fin_) {
					send = true;
					ctx.recv_fin_ = true;
					++ctx.send_ack_;
					ctx.flags_ |= tcp_h::MASK_ACK;
					if(!ctx.send_fin_) {
						ctx.flags_ |= tcp_h::MASK_FIN;
					}
					ctx.recv_task_ = recv_task::sync_close;
				}
				if(tcp->get_flag_rst()) {  // リセット
					ctx.recv_task_ = recv_task::close;
					ctx.send_task_ = send_task::close;
				}
				break;

			case recv_task::syn_cent:


				break;



			case recv_task::sync_close:
// debug_format("sync_close (Recv Len: %d)\n") % recv_len;
// utils::format("RECV:      SEQ: 0x%08X, ACK: 0x%08X\n") % ctx.recv_seq_ % ctx.recv_ack_;
// utils::format("SEND(ACK): SEQ: 0x%08X, ACK: 0x%08X\n") % ctx.send_seq_ % ctx.send_ack_;
				if(tcp->get_flag_ack()) {
					if(ctx.recv_ack_ == (ctx.send_seq_ + 1) && ctx.recv_seq_ == ctx.send_ack_) { 
						++ctx.send_seq_;
						ctx.recv_task_ = recv_task::close;
					}
				}
				if(ctx.recv_fin_) {  // FIN に対するACKを返す
					ctx.recv_fin_ = true;
					send = true;
					++ctx.send_ack_;
					ctx.flags_ |= tcp_h::MASK_ACK;
				}
				if(tcp->get_flag_psh()) {
					debug_format("sync_close PSH (Recv Len: %d)\n") % recv_len;
				}
				break;

			case recv_task::established_client:
				break;

			case recv_task::close:
				break;

			default:
				break;
			}

			if(send) {
				frame_t* t = get_send_frame_();
				if(t == nullptr) {
					return false;
				}
				auto all = make_seg_(ctx, eh.get_src(), ih.get_src_ipa(), *t, data);
				ethd_.send(all);
			}
			return true;
		}


		void send_(context& ctx)
		{
			if(ctx.recv_task_ != recv_task::established_server) {
				return;
			}

			if(ctx.send_len_ > 0) {

				// データ転送したが、ACK が来ない場合に、再送を行う
				uint32_t d = delta_time_(ctx.send_time_org_);
				if(d >= ctx.send_wait_) {
					debug_format("TCP ReSend: %d\n") % ctx.send_len_;
					ctx.send_len_ = 0;  // 再送
					++ctx.resend_cnt_;
				} else {
					return;
				}
			}

			ethd_.enable_interrupt(false);

			uint16_t flags = ctx.flags_;
			ctx.flags_ = 0;

			uint16_t len = ctx.send_.length();
			if(len == 0) {
				ethd_.enable_interrupt();
				return;
			}

			frame_t* t = get_send_frame_();
			if(t == nullptr) {
				ethd_.enable_interrupt();
				return;
			}

			if(ctx.resend_cnt_ >= RESEND_LIMIT) {  // 再送がリミットになったら、RSTを送る
				ctx.flags_ |= tcp_h::MASK_RST;
				auto all = make_seg_(ctx, ctx.mac_, ctx.adrs_.get(), *t, false);
				ethd_.send(all);

			} else {
				ctx.flags_ |= tcp_h::MASK_ACK;
				auto all = make_seg_(ctx, ctx.mac_, ctx.adrs_.get(), *t, true);
				ethd_.send(all);
// dump(t->eh_);
// dump(t->ipv4_);
// dump(t->tcp_);
				char* str = static_cast<char*>(t->next(t));
				str[len] = 0;
				utils::format("Send: '%s' (%d)\n") % str % len;
			}

			ctx.flags_ = flags;

			ethd_.enable_interrupt();
		}


		// 割り込み「外」からの FIN の送信
		void send_flags_(context& ctx, uint8_t flags)
		{
			ethd_.enable_interrupt(false);

			frame_t* t = get_send_frame_();
			if(t != nullptr) {
///				debug_format("Send FIN at out of INTR\n");
				ctx.flags_ = flags;
				auto all = make_seg_(ctx, ctx.mac_, ctx.adrs_.get(), *t, false);
				ethd_.send(all);
				ctx.send_fin_ = true;
				if(ctx.recv_task_ != recv_task::close) {
				   	ctx.recv_task_ = recv_task::sync_close;  // FIN に対する ACK を待つ
				}
			}

			ethd_.enable_interrupt();
		}


	public:
		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
			@param[in]	ethd	イーサーネット・ドライバー
			@param[in]	info	ネット情報
			@param[in]	seq		シーケンス番号初期値
		*/
		//-----------------------------------------------------------------//
		tcp(ETHD& ethd, net_info& info, uint32_t seq = 1) noexcept : ethd_(ethd), info_(info),
			last_state_(net_state::OK)
		{ }


		//-----------------------------------------------------------------//
		/*!
			@brief  TCP の同時接続数を返す
			@return TCP の同時接続数
		*/
		//-----------------------------------------------------------------//
		uint32_t capacity() const noexcept { return NMAX; }


		//-----------------------------------------------------------------//
		/*!
			@brief  オープン
			@param[in]	adrs	アドレス
			@param[in]	port	ポート
			@param[in]	server	サーバーの場合「true」
			@param[out]	ディスクリプタ
			@return ネット・ステートを返す
		*/
		//-----------------------------------------------------------------//
		net_state open(const ip_adrs& adrs, uint16_t port, bool server, uint32_t& desc) noexcept
		{
			if(port == 0) {  // ０番ポートは無効
				auto st = net_state::FAIL_PORT;
				if(last_state_ != st) {
					debug_format("TCP Open fail port: %d\n") % port;
					last_state_ = st;
				}
				desc = NMAX;
				return st;
			}
			if(adrs.is_brodcast()) {  // ブロードキャストアドレスは無効
				auto st = net_state::FAIL_ADRS;
				if(last_state_ != st) {
					debug_format("TCP Open fail brodcast address: %s\n") % adrs.c_str();
					last_state_ = st;
				}
				desc = NMAX;
				return st;
			}

			if(adrs.is_any() && !server) {  // クライアント接続では、ANY アドレスは無効
				auto st = net_state::FAIL_ANY;
				if(last_state_ != st) {
					debug_format("TCP Open fail any address for client: %s\n") % adrs.c_str();
					last_state_ = st;
				}
				desc = NMAX;
				return st;
			}

			// 同じポートがある場合は無効
			for(uint32_t i = 0; i < NMAX; ++i) {
				if(!common_.at_blocks().is_alloc(i)) continue;
				const context& ctx = common_.get_blocks().get(i);
				if(ctx.cn_port_ == port) {
					auto st = net_state::EVEN_PORT;
					if(last_state_ != st) {
						debug_format("TCP Open fail even port as: %d\n") % port;
						last_state_ = st;
					}
					desc = NMAX;
					return st;
				}
			}

			// コンテキスト・スペースが無い
			uint32_t idx = common_.at_blocks().alloc();
			if(!common_.at_blocks().is_alloc(idx)) {
				auto st = net_state::CONTEXT_EMPTY;
				if(last_state_ != st) {
					debug_format("TCP Open fail context empty\n"); 
					last_state_ = st;
				}
				desc = NMAX;
				return st;
			}

			last_state_ = net_state::OK;

			context& ctx = common_.at_blocks().at(idx);

			// コンテキスト初期化
			ctx.reset(adrs, port, server);

			if(server) {
				ctx.recv_task_ = recv_task::listen_server;
				ctx.send_task_ = send_task::main;
			} else {
				ctx.recv_task_ = recv_task::idle;
				ctx.send_task_ = send_task::sync_mac;
			}

			common_.at_blocks().unlock(idx);

			desc = idx;

			return net_state::OK;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  ディスクリプタの検査
			@param[in]	desc	ディスクリプタ
			@return ディスクリプタが無効「false」
		*/
		//-----------------------------------------------------------------//
		bool probe(int desc) const
		{
			uint32_t idx = static_cast<uint32_t>(desc);
			return common_.get_blocks().is_alloc(idx);
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  接続の検査
			@param[in]	desc	ディスクリプタ
			@return 接続状態「true」、切断状態、ディスクリプタが無効「false」
		*/
		//-----------------------------------------------------------------//
		bool connected(int desc) const noexcept
		{
			uint32_t idx = static_cast<uint32_t>(desc);
			if(!common_.get_blocks().is_alloc(idx)) return false;

			const context& ctx = common_.get_blocks().get(idx);
			if(ctx.server_) {
				return ctx.recv_task_ == recv_task::established_server;
			} else {
				return ctx.recv_task_ == recv_task::established_client;
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  クローズの検査
			@param[in]	desc	ディスクリプタ
			@return 接続状態「true」、切断状態、ディスクリプタが無効「false」
		*/
		//-----------------------------------------------------------------//
		bool is_fin(int desc) const noexcept
		{
			uint32_t idx = static_cast<uint32_t>(desc);
			if(!common_.get_blocks().is_alloc(idx)) return false;

			const context& ctx = common_.get_blocks().get(idx);
			if(!ctx.recv_fin_ && !ctx.send_fin_) return true;
			else return false;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  接続ＩＰの取得
			@param[in]	desc	ディスクリプタ
			@return 接続ＩＰ（ANYなら無効）
		*/
		//-----------------------------------------------------------------//
		const ip_adrs& get_ip(int desc) const
		{
			static ip_adrs tmp;
			uint32_t idx = static_cast<uint32_t>(desc);
			if(!common_.get_blocks().is_alloc(idx)) return tmp;

			const context& ctx = common_.get_blocks().get(idx);
			return ctx.adrs_;			
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  接続ポートの取得
			@param[in]	desc	ディスクリプタ
			@return 接続ポート
		*/
		//-----------------------------------------------------------------//
		uint16_t get_port(int desc) const
		{
			static ip_adrs tmp;
			uint32_t idx = static_cast<uint32_t>(desc);
			if(!common_.get_blocks().is_alloc(idx)) return tmp;

			const context& ctx = common_.get_blocks().get(idx);
			return ctx.cn_port_;			
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  送信
			@param[in]	desc	ディスクリプタ
			@param[in]	src		ソース
			@param[in]	len		送信バイト数
			@return 送信バイト（負の値はエラー）
		*/
		//-----------------------------------------------------------------//
		inline int send(int desc, const void* src, uint16_t len) noexcept
		{
			return common_.send(desc, src, len);
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  送信バッファの残量取得
			@param[in]	desc	ディスクリプタ
			@return 送信バッファの残量（負の値はエラー）
		*/
		//-----------------------------------------------------------------//
		inline int get_send_length(int desc) const noexcept
		{
			return common_.get_send_length(desc);
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  受信
			@param[in]	desc	ディスクリプタ
			@param[in]	dst		ソース
			@param[in]	len		受信バイト数
			@return 受信バイト（負の値はエラー）
		*/
		//-----------------------------------------------------------------//
		inline int recv(int desc, void* dst, uint16_t len) noexcept
		{
			return common_.recv(desc, dst, len);
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  受信バッファの残量取得
			@param[in]	desc	ディスクリプタ
			@return 受信バッファの残量（負の値はエラー）
		*/
		//-----------------------------------------------------------------//
		inline int get_recv_length(int desc) const noexcept
		{
			return common_.get_recv_length(desc);
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  クローズ
			@param[in]	desc	ディスクリプタ
		*/
		//-----------------------------------------------------------------//
		void close(int desc) noexcept
		{
			uint32_t idx = static_cast<uint32_t>(desc);
			if(!common_.at_blocks().is_alloc(idx)) return;

			context& ctx = common_.at_blocks().at(idx);

			if(!ctx.send_fin_) {  // FIN を送っていない
				send_flags_(ctx, tcp_h::MASK_FIN);
			}
			ctx.close_req_ = true;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  プロセス（割り込みから呼ばれる）
			@param[in]	eh	イーサーネット・ヘッダー
			@param[in]	ih	IPV4 ヘッダー
			@param[in]	tcp	TCP ヘッダー
			@param[in]	len	メッセージ長
			@return エラーが無い場合「true」
		*/
		//-----------------------------------------------------------------//
		bool process(const eth_h& eh, const ipv4_h& ih, const tcp_h* tcp, int32_t len) noexcept
		{
			// 該当するコンテキストを探す
			uint32_t idx = NMAX;
			for(uint32_t i = 0; i < NMAX; ++i) {
				if(!common_.at_blocks().is_alloc(i)) continue;  // alloc: 有効
				if(common_.at_blocks().is_lock(i)) continue;  // lock:  無効
				context& ctx = common_.at_blocks().at(i);  // コンテキスト取得

				uint16_t sum = tools::calc_sum(&ih, sizeof(ipv4_h));
				if(sum != 0) {
					debug_format("TCP IPV4 Header Sum Error: %04X -> %04X\n") % ih.get_csum() % sum;
					continue;
				}

				// 転送先の確認
				if(info_.ip != ih.get_dst_ipa()) continue;
				// 転送元の確認
				if(!ctx.adrs_.is_any() && ctx.adrs_ != ih.get_src_ipa()) continue; 

				// ポート番号の確認
				if(ctx.port_ != 0) {
					if(ctx.port_ != tcp->get_src_port()) {
						continue;
					}
				} else {
					ctx.port_ = tcp->get_src_port();
					debug_format("TCP connection key port: %d\n") % ctx.port_;
				}
				if(ctx.cn_port_ != tcp->get_dst_port()) {
					continue;
				}

				return recv_(ctx, eh, ih, tcp);
			}
			return false;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  サービス（１０ｍｓ毎に呼ぶ）@n
					※割り込み外から呼ぶ事
		*/
		//-----------------------------------------------------------------//
		void service() noexcept
		{
			for(uint32_t i = 0; i < NMAX; ++i) {

				if(!common_.at_blocks().is_alloc(i)) continue;

				context& ctx = common_.at_blocks().at(i);
				switch(ctx.send_task_) {
				case send_task::sync_mac:
					if(common_.check_mac(ctx, info_)) {
						ctx.recv_task_ = recv_task::syn_cent;
						send_flags_(ctx, tcp_h::MASK_SYN);
						ctx.send_task_ = send_task::main;
					}
					break;

				case send_task::main:
					send_(ctx);
					// FIN を受け取った、又は、クローズ要求なら「クローズ同期」を始める
					if(ctx.recv_fin_ || ctx.close_req_) {
						ctx.send_task_ = send_task::sync_close;
						ctx.send_time_ = CLOSE_TIME_OUT;
					}
					break;

				case send_task::sync_close:
					// 送信動作、及びレシーブ動作が「クローズ」なら終了
					if(ctx.close_req_ && ctx.recv_task_ == recv_task::close) {
						common_.at_blocks().lock(i);  // ロックする（割り込みで利用不可にする）
						common_.at_blocks().erase(i);
					} else if(!ctx.recv_fin_) {  // FIN を受け取ったら、「送信」は出来ない。
						send_(ctx);
					}
					if(ctx.send_time_ > 0) {
						ctx.send_time_--;
						if(ctx.send_time_ == 0) {
							debug_format("TCP Close TimeOut (%d)\n") % i;
						}
					} else {
						common_.at_blocks().lock(i);  // ロックする（割り込みで利用不可にする）
						common_.at_blocks().erase(i);
					}
					break;

				case send_task::close:  // 強制クローズ
					common_.at_blocks().lock(i);
					common_.at_blocks().erase(i);
					break;

				default:
					break;
				}
			}
		}
	};
}
