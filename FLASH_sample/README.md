Renesas RX24T, RX64M, RX71M, RX65N, RX66T, RX72N Data Flash サンプル
=========

## 概要
RX マイコンを使った内臓データフラッシュへのアクセス、サンプルプログラム
   
## プロジェクト・リスト
 - main.cpp
 - RX24T/Makefile
 - RX64M/Makefile
 - RX71M/Makefile
 - RX65N/Makefile
 - RX66T/Makefile
 - RX72T/Makefile
 - RX72N/Makefile
   
## ハードウェアーの準備（全般）
 - ベースクリスタルが異なる場合は、typedef のパラメーターを変更する。
 - Makefile で、各モジュール別の設定周波数を宣言している。
 - RX24T:  80MHz (10MHz)
 - RX64M: 120MHz (12MHz)
 - RX71M: 240MHz (12MHz)
 - RX65N: 120MHz (12MHz)
 - RX66T: 160MHz (10MHz)
 - RX72T: 192MHz (16MHz)
 - RX72N: 240MHz (16MHz)
 - インジケーター LED を指定のポートに接続する。
 - USB シリアルの信号と設定の SCI ポートを接続する。
 - RX6x, RX71M, SCI の標準ポートは、「RX600/port_map.hpp」参照。
 - RX24T, SCI の標準ポートは、「RX24T/port_map.hpp」参照。
```
#if defined(SIG_RX71M)
	static const char* system_str_ = { "RX71M" };
	typedef device::system_io<12'000'000, 240'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT0, device::bitpos::B7> LED;
	typedef device::SCI1 SCI_CH;
#elif defined(SIG_RX64M)
	static const char* system_str_ = { "RX64M" };
	typedef device::system_io<12'000'000, 240'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT0, device::bitpos::B7> LED;
	typedef device::SCI1 SCI_CH;
#elif defined(SIG_RX65N)
	static const char* system_str_ = { "RX65N" };
	typedef device::system_io<12'000'000, 240'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT7, device::bitpos::B0> LED;
	typedef device::SCI9 SCI_CH;
#elif defined(SIG_RX24T)
	static const char* system_str_ = { "RX24T" };
	typedef device::system_io<10'000'000, 80'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT0, device::bitpos::B0> LED;
	typedef device::SCI1 SCI_CH;
	#define SINGLE
#elif defined(SIG_RX66T)
	static const char* system_str_ = { "RX66T" };
	typedef device::system_io<10'000'000, 160'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT0, device::bitpos::B0> LED;
	typedef device::SCI1 SCI_CH;
#elif defined(SIG_RX72T)
	static const char* system_str_ = { "RX72T" };
	typedef device::system_io<16'000'000, 192'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT0, device::bitpos::B1> LED;
	typedef device::SCI1 SCI_CH;
#elif defined(SIG_RX72N)
	static const char* system_str_ = { "RX72N" };
	typedef device::system_io<16'000'000, 240'000'000> SYSTEM_IO;
	typedef device::PORT<device::PORT4, device::bitpos::B0> LED;
	typedef device::SCI2 SCI_CH;
#endif
```
 - RX24T, RX66T の場合「10MHz」、他 CPU は「12MHz」のクリスタル。
 - RX72T は 16MHz のクリスタル（USBで利用する場合 192MHz、最大速度優先の場合 200MHz が可能） 
 - RX75N Envision kit の場合、インジケーター LED はボード上の青色を利用する。
 - RX72N Envision kit の場合、インジケーター LED はボード上の青色を利用する。
   
## ハードウェアーリソースの準備
 - SCI に指定されたポートに USB シリアルなどの変換器を接続する。
 - マイコン側の RXD 端子と、USB シリアルの TXD を接続。
 - マイコン側の TXD 端子と、USB シリアルの RXD を接続。
   
## ビルド方法
 - 各プラットホームディレクトリーに移動、make する。
 - flash_sample.mot ファイルを書き込む。
   
## 動作
 - LED が 0.25 秒間隔で点滅する。
 - SCI に指定されたポートで、TX（送信）、RX（受信）を行う。
 - TeraTerm などで確認。
 - TeraTerm のシリアル設定：１１５２００ボー、８ビットデータ、１ストップ、パリティ無し。
 - main.cpp の中、SCI の初期化でボーレートは自由に設定できる。
 - 設定出来ない「値」の場合、初期化が失敗する。（極端に遅い、早い）
    
## 備考
      
-----
   
License
----

MIT
