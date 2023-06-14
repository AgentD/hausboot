mkfatimg_path = join_paths(meson.current_source_dir(), 'mkfatimage.sh')
mkdiskimg_path = join_paths(meson.current_source_dir(), 'mkdiskimage.sh')
bootcfg_path = join_paths(meson.current_source_dir(), 'boot.cfg')

fatpart = custom_target(
	'fatpart',
	depends: [
		vbr,
		stage2,
		kernel,
		fatedit,
		installfat
	],
	depend_files: [
		mkfatimg_path,
		bootcfg_path,
	],
	output: 'fatpart.img',
	command: [
		mkfatimg_path,
		vbr,
		stage2,
		kernel,
		bootcfg_path,
		fatedit,
		installfat,
		'@OUTPUT0@'
	],
	install: false,
	build_by_default: true,
)

diskimg = custom_target(
	'diskimage',
	depends: [
		mbr,
		fatpart,
	],
	depend_files: [
		mkdiskimg_path,
	],
	output: 'disk.img',
	command: [
		mkdiskimg_path,
		fatpart,
		mbr,
		'@OUTPUT0@'
	],
	install: false,
	build_by_default: true,
)

conf_data = configuration_data()
conf_data.set('diskimg', diskimg.full_path())

configure_file(
	input: join_paths(meson.current_source_dir(), 'bochsrc.txt'),
	output: 'bochsrc.txt',
	configuration: conf_data
)
