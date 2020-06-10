cmake_minimum_required(VERSION 3.10)

project(
	NP2kai_932
	DESCRIPTION "NP2kai language resource library"
	HOMEPAGE_URL "http://domisan.sakura.ne.jp/article/np2kai/np2kai.html"
	LANGUAGES C CXX
)

cmake_policy(SET CMP0076 NEW)

list(APPEND NP2kai_windows_932_sources "np2jp.rc")

add_library(NP2kai_windows_932 MODULE ${NP2kai_windows_932_sources})

