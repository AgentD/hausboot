vbr = executable(
	'vbr',
	name_suffix: 'bin',
	sources: [
		'abi.S',
		'vbr.cpp',
	],
	link_args: [
		'-Wl,-T' + join_paths(meson.current_source_dir(), 'vbr.ld'),
		'-nostdlib',
	],
	link_depends: [
		'vbr.ld',
	],
	cpp_args: realmode_cpp_args,
	install: false,
	implicit_include_directories: false,
	include_directories: incs,
	pie: false,
)
