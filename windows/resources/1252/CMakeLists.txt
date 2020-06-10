cmake_minimum_required(VERSION 3.10)

project(
	NP2kai_1252
	DESCRIPTION "NP2kai language resource library"
	HOMEPAGE_URL "http://domisan.sakura.ne.jp/article/np2kai/np2kai.html"
	LANGUAGES C CXX
)

cmake_policy(SET CMP0076 NEW)

list(APPEND NP2kai_windows_1252_sources "np2.rc")

add_library(NP2kai_windows_1252 MODULE ${NP2kai_windows_1252_sources})

