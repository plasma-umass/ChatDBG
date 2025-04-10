--
-- create user defined conversion
--
CREATE USER conversion_test_user WITH NOCREATEDB NOCREATEUSER;
SET SESSION AUTHORIZATION conversion_test_user;
CREATE CONVERSION myconv FOR 'LATIN1' TO 'UNICODE' FROM iso8859_1_to_utf8;
--
-- cannot make same name conversion in same schema
--
CREATE CONVERSION myconv FOR 'LATIN1' TO 'UNICODE' FROM iso8859_1_to_utf8;
--
-- create default conversion with qualified name
--
CREATE DEFAULT CONVERSION public.mydef FOR 'LATIN1' TO 'UNICODE' FROM iso8859_1_to_utf8;
--
-- cannot make default conversion with same shcema/for_encoding/to_encoding
--
CREATE DEFAULT CONVERSION public.mydef2 FOR 'LATIN1' TO 'UNICODE' FROM iso8859_1_to_utf8;
--
-- drop user defined conversion
--
DROP CONVERSION myconv;
DROP CONVERSION mydef;
--
-- make sure all pre-defined conversions are fine.
-- SQL_ASCII --> MULE_INTERNAL
SELECT CONVERT('foo' USING ascii_to_mic);
SELECT CONVERT('foo', 'SQL_ASCII', 'MULE_INTERNAL');
-- MULE_INTERNAL --> SQL_ASCII
SELECT CONVERT('foo' USING mic_to_ascii);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'SQL_ASCII');
-- KOI8R --> MULE_INTERNAL
SELECT CONVERT('foo' USING koi8_r_to_mic);
SELECT CONVERT('foo', 'KOI8R', 'MULE_INTERNAL');
-- MULE_INTERNAL --> KOI8R
SELECT CONVERT('foo' USING mic_to_koi8_r);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'KOI8R');
-- ISO-8859-5 --> MULE_INTERNAL
SELECT CONVERT('foo' USING iso_8859_5_to_mic);
SELECT CONVERT('foo', 'ISO-8859-5', 'MULE_INTERNAL');
-- MULE_INTERNAL --> ISO-8859-5
SELECT CONVERT('foo' USING mic_to_iso_8859_5);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'ISO-8859-5');
-- WIN1251 --> MULE_INTERNAL
SELECT CONVERT('foo' USING windows_1251_to_mic);
SELECT CONVERT('foo', 'WIN1251', 'MULE_INTERNAL');
-- MULE_INTERNAL --> WIN1251
SELECT CONVERT('foo' USING mic_to_windows_1251);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'WIN1251');
-- ALT --> MULE_INTERNAL
SELECT CONVERT('foo' USING windows_866_to_mic);
SELECT CONVERT('foo', 'ALT', 'MULE_INTERNAL');
-- MULE_INTERNAL --> ALT
SELECT CONVERT('foo' USING mic_to_windows_866);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'ALT');
-- KOI8R --> WIN1251
SELECT CONVERT('foo' USING koi8_r_to_windows_1251);
SELECT CONVERT('foo', 'KOI8R', 'WIN1251');
-- WIN1251 --> KOI8R
SELECT CONVERT('foo' USING windows_1251_to_koi8_r);
SELECT CONVERT('foo', 'WIN1251', 'KOI8R');
-- KOI8R --> ALT
SELECT CONVERT('foo' USING koi8_r_to_windows_866);
SELECT CONVERT('foo', 'KOI8R', 'ALT');
-- ALT --> KOI8R
SELECT CONVERT('foo' USING windows_866_to_koi8_r);
SELECT CONVERT('foo', 'ALT', 'KOI8R');
-- ALT --> WIN1251
SELECT CONVERT('foo' USING windows_866_to_windows_1251);
SELECT CONVERT('foo', 'ALT', 'WIN1251');
-- WIN1251 --> ALT
SELECT CONVERT('foo' USING windows_1251_to_windows_866);
SELECT CONVERT('foo', 'WIN1251', 'ALT');
-- ISO-8859-5 --> KOI8R
SELECT CONVERT('foo' USING iso_8859_5_to_koi8_r);
SELECT CONVERT('foo', 'ISO-8859-5', 'KOI8R');
-- KOI8R --> ISO-8859-5
SELECT CONVERT('foo' USING koi8_r_to_iso_8859_5);
SELECT CONVERT('foo', 'KOI8R', 'ISO-8859-5');
-- ISO-8859-5 --> WIN1251
SELECT CONVERT('foo' USING iso_8859_5_to_windows_1251);
SELECT CONVERT('foo', 'ISO-8859-5', 'WIN1251');
-- WIN1251 --> ISO-8859-5
SELECT CONVERT('foo' USING windows_1251_to_iso_8859_5);
SELECT CONVERT('foo', 'WIN1251', 'ISO-8859-5');
-- ISO-8859-5 --> ALT
SELECT CONVERT('foo' USING iso_8859_5_to_windows_866);
SELECT CONVERT('foo', 'ISO-8859-5', 'ALT');
-- ALT --> ISO-8859-5
SELECT CONVERT('foo' USING windows_866_to_iso_8859_5);
SELECT CONVERT('foo', 'ALT', 'ISO-8859-5');
-- EUC_CN --> MULE_INTERNAL
SELECT CONVERT('foo' USING euc_cn_to_mic);
SELECT CONVERT('foo', 'EUC_CN', 'MULE_INTERNAL');
-- MULE_INTERNAL --> EUC_CN
SELECT CONVERT('foo' USING mic_to_euc_cn);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'EUC_CN');
-- EUC_JP --> SJIS
SELECT CONVERT('foo' USING euc_jp_to_sjis);
SELECT CONVERT('foo', 'EUC_JP', 'SJIS');
-- SJIS --> EUC_JP
SELECT CONVERT('foo' USING sjis_to_euc_jp);
SELECT CONVERT('foo', 'SJIS', 'EUC_JP');
-- EUC_JP --> MULE_INTERNAL
SELECT CONVERT('foo' USING euc_jp_to_mic);
SELECT CONVERT('foo', 'EUC_JP', 'MULE_INTERNAL');
-- SJIS --> MULE_INTERNAL
SELECT CONVERT('foo' USING sjis_to_mic);
SELECT CONVERT('foo', 'SJIS', 'MULE_INTERNAL');
-- MULE_INTERNAL --> EUC_JP
SELECT CONVERT('foo' USING mic_to_euc_jp);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'EUC_JP');
-- MULE_INTERNAL --> SJIS
SELECT CONVERT('foo' USING mic_to_sjis);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'SJIS');
-- EUC_KR --> MULE_INTERNAL
SELECT CONVERT('foo' USING euc_kr_to_mic);
SELECT CONVERT('foo', 'EUC_KR', 'MULE_INTERNAL');
-- MULE_INTERNAL --> EUC_KR
SELECT CONVERT('foo' USING mic_to_euc_kr);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'EUC_KR');
-- EUC_TW --> BIG5
SELECT CONVERT('foo' USING euc_tw_to_big5);
SELECT CONVERT('foo', 'EUC_TW', 'BIG5');
-- BIG5 --> EUC_TW
SELECT CONVERT('foo' USING big5_to_euc_tw);
SELECT CONVERT('foo', 'BIG5', 'EUC_TW');
-- EUC_TW --> MULE_INTERNAL
SELECT CONVERT('foo' USING euc_tw_to_mic);
SELECT CONVERT('foo', 'EUC_TW', 'MULE_INTERNAL');
-- BIG5 --> MULE_INTERNAL
SELECT CONVERT('foo' USING big5_to_mic);
SELECT CONVERT('foo', 'BIG5', 'MULE_INTERNAL');
-- MULE_INTERNAL --> EUC_TW
SELECT CONVERT('foo' USING mic_to_euc_tw);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'EUC_TW');
-- MULE_INTERNAL --> BIG5
SELECT CONVERT('foo' USING mic_to_big5);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'BIG5');
-- LATIN2 --> MULE_INTERNAL
SELECT CONVERT('foo' USING iso_8859_2_to_mic);
SELECT CONVERT('foo', 'LATIN2', 'MULE_INTERNAL');
-- MULE_INTERNAL --> LATIN2
SELECT CONVERT('foo' USING mic_to_iso_8859_2);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'LATIN2');
-- WIN1250 --> MULE_INTERNAL
SELECT CONVERT('foo' USING windows_1250_to_mic);
SELECT CONVERT('foo', 'WIN1250', 'MULE_INTERNAL');
-- MULE_INTERNAL --> WIN1250
SELECT CONVERT('foo' USING mic_to_windows_1250);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'WIN1250');
-- LATIN2 --> WIN1250
SELECT CONVERT('foo' USING iso_8859_2_to_windows_1250);
SELECT CONVERT('foo', 'LATIN2', 'WIN1250');
-- WIN1250 --> LATIN2
SELECT CONVERT('foo' USING windows_1250_to_iso_8859_2);
SELECT CONVERT('foo', 'WIN1250', 'LATIN2');
-- LATIN1 --> MULE_INTERNAL
SELECT CONVERT('foo' USING iso_8859_1_to_mic);
SELECT CONVERT('foo', 'LATIN1', 'MULE_INTERNAL');
-- MULE_INTERNAL --> LATIN1
SELECT CONVERT('foo' USING mic_to_iso_8859_1);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'LATIN1');
-- LATIN3 --> MULE_INTERNAL
SELECT CONVERT('foo' USING iso_8859_3_to_mic);
SELECT CONVERT('foo', 'LATIN3', 'MULE_INTERNAL');
-- MULE_INTERNAL --> LATIN3
SELECT CONVERT('foo' USING mic_to_iso_8859_3);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'LATIN3');
-- LATIN4 --> MULE_INTERNAL
SELECT CONVERT('foo' USING iso_8859_4_to_mic);
SELECT CONVERT('foo', 'LATIN4', 'MULE_INTERNAL');
-- MULE_INTERNAL --> LATIN4
SELECT CONVERT('foo' USING mic_to_iso_8859_4);
SELECT CONVERT('foo', 'MULE_INTERNAL', 'LATIN4');
-- SQL_ASCII --> UNICODE
SELECT CONVERT('foo' USING ascii_to_utf_8);
SELECT CONVERT('foo', 'SQL_ASCII', 'UNICODE');
-- UNICODE --> SQL_ASCII
SELECT CONVERT('foo' USING utf_8_to_ascii);
SELECT CONVERT('foo', 'UNICODE', 'SQL_ASCII');
-- BIG5 --> UNICODE
SELECT CONVERT('foo' USING big5_to_utf_8);
SELECT CONVERT('foo', 'BIG5', 'UNICODE');
-- UNICODE --> BIG5
SELECT CONVERT('foo' USING utf_8_to_big5);
SELECT CONVERT('foo', 'UNICODE', 'BIG5');
-- UNICODE --> KOI8R
SELECT CONVERT('foo' USING utf_8_to_koi8_r);
SELECT CONVERT('foo', 'UNICODE', 'KOI8R');
-- KOI8R --> UNICODE
SELECT CONVERT('foo' USING koi8_r_to_utf_8);
SELECT CONVERT('foo', 'KOI8R', 'UNICODE');
-- UNICODE --> WIN1251
SELECT CONVERT('foo' USING utf_8_to_windows_1251);
SELECT CONVERT('foo', 'UNICODE', 'WIN1251');
-- WIN1251 --> UNICODE
SELECT CONVERT('foo' USING windows_1251_to_utf_8);
SELECT CONVERT('foo', 'WIN1251', 'UNICODE');
-- UNICODE --> ALT
SELECT CONVERT('foo' USING utf_8_to_windows_866);
SELECT CONVERT('foo', 'UNICODE', 'ALT');
-- ALT --> UNICODE
SELECT CONVERT('foo' USING windows_866_to_utf_8);
SELECT CONVERT('foo', 'ALT', 'UNICODE');
-- EUC_CN --> UNICODE
SELECT CONVERT('foo' USING euc_cn_to_utf_8);
SELECT CONVERT('foo', 'EUC_CN', 'UNICODE');
-- UNICODE --> EUC_CN
SELECT CONVERT('foo' USING utf_8_to_euc_cn);
SELECT CONVERT('foo', 'UNICODE', 'EUC_CN');
-- EUC_JP --> UNICODE
SELECT CONVERT('foo' USING euc_jp_to_utf_8);
SELECT CONVERT('foo', 'EUC_JP', 'UNICODE');
-- UNICODE --> EUC_JP
SELECT CONVERT('foo' USING utf_8_to_euc_jp);
SELECT CONVERT('foo', 'UNICODE', 'EUC_JP');
-- EUC_KR --> UNICODE
SELECT CONVERT('foo' USING euc_kr_to_utf_8);
SELECT CONVERT('foo', 'EUC_KR', 'UNICODE');
-- UNICODE --> EUC_KR
SELECT CONVERT('foo' USING utf_8_to_euc_kr);
SELECT CONVERT('foo', 'UNICODE', 'EUC_KR');
-- EUC_TW --> UNICODE
SELECT CONVERT('foo' USING euc_tw_to_utf_8);
SELECT CONVERT('foo', 'EUC_TW', 'UNICODE');
-- UNICODE --> EUC_TW
SELECT CONVERT('foo' USING utf_8_to_euc_tw);
SELECT CONVERT('foo', 'UNICODE', 'EUC_TW');
-- GB18030 --> UNICODE
SELECT CONVERT('foo' USING gb18030_to_utf_8);
SELECT CONVERT('foo', 'GB18030', 'UNICODE');
-- UNICODE --> GB18030
SELECT CONVERT('foo' USING utf_8_to_gb18030);
SELECT CONVERT('foo', 'UNICODE', 'GB18030');
-- GBK --> UNICODE
SELECT CONVERT('foo' USING gbk_to_utf_8);
SELECT CONVERT('foo', 'GBK', 'UNICODE');
-- UNICODE --> GBK
SELECT CONVERT('foo' USING utf_8_to_gbk);
SELECT CONVERT('foo', 'UNICODE', 'GBK');
-- UNICODE --> LATIN2
SELECT CONVERT('foo' USING utf_8_to_iso_8859_2);
SELECT CONVERT('foo', 'UNICODE', 'LATIN2');
-- LATIN2 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_2_to_utf_8);
SELECT CONVERT('foo', 'LATIN2', 'UNICODE');
-- UNICODE --> LATIN3
SELECT CONVERT('foo' USING utf_8_to_iso_8859_3);
SELECT CONVERT('foo', 'UNICODE', 'LATIN3');
-- LATIN3 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_3_to_utf_8);
SELECT CONVERT('foo', 'LATIN3', 'UNICODE');
-- UNICODE --> LATIN4
SELECT CONVERT('foo' USING utf_8_to_iso_8859_4);
SELECT CONVERT('foo', 'UNICODE', 'LATIN4');
-- LATIN4 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_4_to_utf_8);
SELECT CONVERT('foo', 'LATIN4', 'UNICODE');
-- UNICODE --> LATIN5
SELECT CONVERT('foo' USING utf_8_to_iso_8859_9);
SELECT CONVERT('foo', 'UNICODE', 'LATIN5');
-- LATIN5 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_9_to_utf_8);
SELECT CONVERT('foo', 'LATIN5', 'UNICODE');
-- UNICODE --> LATIN6
SELECT CONVERT('foo' USING utf_8_to_iso_8859_10);
SELECT CONVERT('foo', 'UNICODE', 'LATIN6');
-- LATIN6 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_10_to_utf_8);
SELECT CONVERT('foo', 'LATIN6', 'UNICODE');
-- UNICODE --> LATIN7
SELECT CONVERT('foo' USING utf_8_to_iso_8859_13);
SELECT CONVERT('foo', 'UNICODE', 'LATIN7');
-- LATIN7 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_13_to_utf_8);
SELECT CONVERT('foo', 'LATIN7', 'UNICODE');
-- UNICODE --> LATIN8
SELECT CONVERT('foo' USING utf_8_to_iso_8859_14);
SELECT CONVERT('foo', 'UNICODE', 'LATIN8');
-- LATIN8 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_14_to_utf_8);
SELECT CONVERT('foo', 'LATIN8', 'UNICODE');
-- UNICODE --> LATIN9
SELECT CONVERT('foo' USING utf_8_to_iso_8859_15);
SELECT CONVERT('foo', 'UNICODE', 'LATIN9');
-- LATIN9 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_15_to_utf_8);
SELECT CONVERT('foo', 'LATIN9', 'UNICODE');
-- UNICODE --> LATIN10
SELECT CONVERT('foo' USING utf_8_to_iso_8859_16);
SELECT CONVERT('foo', 'UNICODE', 'LATIN10');
-- LATIN10 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_16_to_utf_8);
SELECT CONVERT('foo', 'LATIN10', 'UNICODE');
-- UNICODE --> ISO-8859-5
SELECT CONVERT('foo' USING utf_8_to_iso_8859_5);
SELECT CONVERT('foo', 'UNICODE', 'ISO-8859-5');
-- ISO-8859-5 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_5_to_utf_8);
SELECT CONVERT('foo', 'ISO-8859-5', 'UNICODE');
-- UNICODE --> ISO-8859-6
SELECT CONVERT('foo' USING utf_8_to_iso_8859_6);
SELECT CONVERT('foo', 'UNICODE', 'ISO-8859-6');
-- ISO-8859-6 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_6_to_utf_8);
SELECT CONVERT('foo', 'ISO-8859-6', 'UNICODE');
-- UNICODE --> ISO-8859-7
SELECT CONVERT('foo' USING utf_8_to_iso_8859_7);
SELECT CONVERT('foo', 'UNICODE', 'ISO-8859-7');
-- ISO-8859-7 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_7_to_utf_8);
SELECT CONVERT('foo', 'ISO-8859-7', 'UNICODE');
-- UNICODE --> ISO-8859-8
SELECT CONVERT('foo' USING utf_8_to_iso_8859_8);
SELECT CONVERT('foo', 'UNICODE', 'ISO-8859-8');
-- ISO-8859-8 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_8_to_utf_8);
SELECT CONVERT('foo', 'ISO-8859-8', 'UNICODE');
-- LATIN1 --> UNICODE
SELECT CONVERT('foo' USING iso_8859_1_to_utf_8);
SELECT CONVERT('foo', 'LATIN1', 'UNICODE');
-- UNICODE --> LATIN1
SELECT CONVERT('foo' USING utf_8_to_iso_8859_1);
SELECT CONVERT('foo', 'UNICODE', 'LATIN1');
-- JOHAB --> UNICODE
SELECT CONVERT('foo' USING johab_to_utf_8);
SELECT CONVERT('foo', 'JOHAB', 'UNICODE');
-- UNICODE --> JOHAB
SELECT CONVERT('foo' USING utf_8_to_johab);
SELECT CONVERT('foo', 'UNICODE', 'JOHAB');
-- SJIS --> UNICODE
SELECT CONVERT('foo' USING sjis_to_utf_8);
SELECT CONVERT('foo', 'SJIS', 'UNICODE');
-- UNICODE --> SJIS
SELECT CONVERT('foo' USING utf_8_to_sjis);
SELECT CONVERT('foo', 'UNICODE', 'SJIS');
-- TCVN --> UNICODE
SELECT CONVERT('foo' USING tcvn_to_utf_8);
SELECT CONVERT('foo', 'TCVN', 'UNICODE');
-- UNICODE --> TCVN
SELECT CONVERT('foo' USING utf_8_to_tcvn);
SELECT CONVERT('foo', 'UNICODE', 'TCVN');
-- UHC --> UNICODE
SELECT CONVERT('foo' USING uhc_to_utf_8);
SELECT CONVERT('foo', 'UHC', 'UNICODE');
-- UNICODE --> UHC
SELECT CONVERT('foo' USING utf_8_to_uhc);
SELECT CONVERT('foo', 'UNICODE', 'UHC');
-- UNICODE --> WIN1250
SELECT CONVERT('foo' USING utf_8_to_windows_1250);
SELECT CONVERT('foo', 'UNICODE', 'WIN1250');
-- WIN1250 --> UNICODE
SELECT CONVERT('foo' USING windows_1250_to_utf_8);
SELECT CONVERT('foo', 'WIN1250', 'UNICODE');
-- UNICODE --> WIN1256
SELECT CONVERT('foo' USING utf_8_to_windows_1256);
SELECT CONVERT('foo', 'UNICODE', 'WIN1256');
-- WIN1256 --> UNICODE
SELECT CONVERT('foo' USING windows_1256_to_utf_8);
SELECT CONVERT('foo', 'WIN1256', 'UNICODE');
-- UNICODE --> WIN874
SELECT CONVERT('foo' USING utf_8_to_windows_874);
SELECT CONVERT('foo', 'UNICODE', 'WIN874');
-- WIN874 --> UNICODE
SELECT CONVERT('foo' USING windows_874_to_utf_8);
SELECT CONVERT('foo', 'WIN874', 'UNICODE');
--
-- return to the super user
--
RESET SESSION AUTHORIZATION;
DROP USER conversion_test_user;
