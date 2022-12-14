# CMakeLists.txt

add_library(inih STATIC
	"ini.c"
)
target_include_directories(inih PUBLIC ".")
target_compile_definitions(inih PRIVATE "INI_ALLOW_INLINE_COMMENTS=0")
target_compile_definitions(inih PRIVATE "INI_USE_STACK=0")
target_compile_definitions(inih PRIVATE "INI_ALLOW_REALLOC=1")
