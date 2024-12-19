project('chip8emu', 'c',
  version : '1.0',
  default_options : ['warning_level=3'])

sdl2_dep = dependency('sdl2')
sdl2_ttf_dep = dependency('SDL2_ttf')
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

dependencies = [
  m_dep,
  sdl2_dep,
  sdl2_ttf_dep,
]

if build_machine.system() == 'windows'
  winpthread_dep = cc.find_library('winpthread', required: false)
  dependencies += winpthread_dep
endif

sources = [
  'src/chip8.c',
  'src/panic.c',
  'src/machine.c',
  'src/backends/sdl.c'
]

exe = executable('chip8', sources,
  install : true, dependencies: [m_dep, sdl2_dep, sdl2_ttf_dep], include_directories: include_directories('src'), win_subsystem: 'windows')
