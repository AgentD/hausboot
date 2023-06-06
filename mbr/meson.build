mbr = executable(
	'mbr',
	name_suffix: 'bin',
	sources: [
		'abi.S',
		'mbr.cpp',
	],
	link_args: [
		'-Wl,-T' + join_paths(meson.current_source_dir(), 'mbr.ld'),
		'-nostdlib',
	],
	link_depends: [
		'mbr.ld',
	],
	cpp_args: realmode_cpp_args,
	install: false,
	implicit_include_directories: false,
	include_directories: incs,
	pie: false,
)
