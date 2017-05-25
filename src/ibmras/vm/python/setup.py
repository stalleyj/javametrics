from distutils.core import setup, Extension
import site
import glob

site_packages = site.getsitepackages()[0]
install_dir = '"' + site_packages + '/healthcenter' + '"'
plugins_dir = '"' + site_packages + '/healthcenter' + '/plugins' + '"'

setup(name='healthcenter', version='0.0.1',  \
	ext_modules=[Extension('healthcenter', 
	['healthcenter.cpp'
	],
	extra_link_args=['-lrt',
			'-lstdc++',
			'-ldl',
			'-lm',
			'-lc',
			'-lpthread',
			'-lcrypt',
			'-L.',
			'-rdynamic',
			'-export-dynamic',
#			'-Wl,--no-undefined',
			'-Wl,-R',
		'-Wl,-export-dynamic',
			'-Wl,-rpath=$ORIGIN/healthcenter'],
	define_macros=[('LINUX', None),
                       ('_LINUX', None),
                       ('IBMRAS_DEBUG_LOGGING', None),
                       ('_64BIT', None),
                       ('EXPORT', None),
			('INSTALL_DIR', install_dir),
			('PLUGINS_DIR', plugins_dir)],
	include_dirs=[('.')],
	libraries=['agentcore']
	)],
#       data_files=[('plugins', glob.glob('plugins/*.so')),
        packages=['healthcenter'],
	package_dir={'healthcenter' : '.'},
	package_data={'healthcenter' : ['plugins/*.so','healthcenter.properties','libagentcore.so']}

#		    ('', ['healthcenter.properties'])
#			]
    )
