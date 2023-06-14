kernel = executable(
	'KRNL386',
	name_suffix: 'SYS',
	sources: [
		'abi.S',
		'kernel.cpp',
	],
	link_args: [
		'-Wl,-T' + join_paths(meson.current_source_dir(), 'kernel.ld'),
		'-nostdlib',
	],
	link_depends: [
		'kernel.ld',
	],
	cpp_args: pm32_cpp_args,
	install: false,
	implicit_include_directories: false,
	include_directories: incs,
	pie: false,
)
