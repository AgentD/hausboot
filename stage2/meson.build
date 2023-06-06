stage2 = executable(
	'stage2',
	name_suffix: 'bin',
	sources: [
		'abi.S',
		'heap.cpp',
		'stage2.cpp',
	],
	link_args: [
		'-Wl,-T' + join_paths(meson.current_source_dir(), 'stage2.ld'),
		'-nostdlib',
	],
	link_depends: [
		'stage2.ld',
	],
	link_with: [
		libBIOS,
		libpm86,
		libcxxabi,
	],
	cpp_args: realmode_cpp_args,
	install: false,
	implicit_include_directories: false,
	include_directories: incs,
	pie: false,
)
