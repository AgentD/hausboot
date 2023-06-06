project('Hausboot', 'cpp', meson_version: '>= 0.54.2')

compiler = meson.get_compiler('cpp')
compiler_native = meson.get_compiler('cpp', native: true)

generic_cpp_args = [
	'-O2',
	'-Os',
	'-Wall',
	'-Wextra',
]

bare_cpp_args = generic_cpp_args
bare_cpp_args += [
	'-Wno-main',
	'-ffreestanding',
	'-fno-exceptions',
	'-fno-rtti',
	'-momit-leaf-frame-pointer',
	'-fno-threadsafe-statics',
]

realmode_cpp_args = bare_cpp_args
realmode_cpp_args += [
	'-m16',
	'-march=i386',
]

pm32_cpp_args = bare_cpp_args
pm32_cpp_args += [
	'-m32',
	'-march=i386',
]

incs = include_directories('include')

subdir('lib/BIOS')
subdir('lib/cxxabi')
subdir('lib/pm86')
subdir('mbr')
subdir('vbr')
subdir('stage2')
subdir('kernel')
subdir('tools')
