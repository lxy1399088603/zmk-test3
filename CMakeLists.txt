if(CONFIG_CUSTOM_CALCULATOR)
  zephyr_include_directories(include)

  zephyr_library_named(custom_calculator)
  zephyr_library_sources(
    src/calculator/calc_engine.c
    src/calculator/calc_display.c
    src/calculator/key_display.c
    src/behaviors/behavior_calc.c
  )
  zephyr_library_include_directories(src)
  target_include_directories(custom_calculator PRIVATE ${APPLICATION_SOURCE_DIR}/include)
endif()
