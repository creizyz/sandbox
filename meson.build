project('sandbox', 'cpp',
        version : '1.0.0',
        default_options : ['warning_level=3', 'cpp_std=c++20'])

sources = [
    'src/main.cpp',
    'src/core/module.cpp'
]

sandbox = executable('sandbox', sources, install : true)

test('test', sandbox)