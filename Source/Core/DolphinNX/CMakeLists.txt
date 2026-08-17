project(GBAStationDolphinStub)

set(TICO_NRO_VERSION "0.0.0" CACHE STRING "GBAStation Dolphin NRO package version")

# Link directories MUST come before add_executable / target_link_libraries
link_directories(
  ${DEVKITPRO}/devkitA64/lib/
  ${DEVKITPRO}/libnx/lib/
  ${DEVKITPRO}/portlibs/switch/lib/
)

add_executable(dolphin-nx
  main.cpp
  Host.cpp
  Audio.cpp
  Input.cpp
  ControllerProfiles.cpp
  NetPlayStubs.cpp
  TicoCore.cpp
  Overlay/Overlay.cpp
  Overlay/TranslationManager.cpp
  Overlay/VulkanOverlay.cpp
)

target_compile_definitions(dolphin-nx PRIVATE
  __SWITCH__
  SWITCH
  HAVE_LIBNX
  AARCH64
  NX
  VK_USE_PLATFORM_VI_NN
  TICO_NRO_VERSION="${TICO_NRO_VERSION}"
)

# Optimization flags for Tegra X1 (Cortex-A57)
target_compile_options(dolphin-nx PRIVATE
  -O3
  -ffunction-sections
  -fdata-sections
  -ffast-math
  -ftree-vectorize
  -fomit-frame-pointer
)

# Include paths
target_include_directories(dolphin-nx PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/Core"
  "${CMAKE_SOURCE_DIR}/Externals/picojson"
  "${CMAKE_SOURCE_DIR}/Externals/Vulkan-Headers/include"
  "${CMAKE_SOURCE_DIR}/Externals/VulkanMemoryAllocator/include"
  "${CMAKE_SOURCE_DIR}/Externals/imgui/imgui"
  "${CMAKE_SOURCE_DIR}/Externals/imgui/imgui/backends"
  "${CMAKE_SOURCE_DIR}/Externals/stb"
  "${DEVKITPRO}/libnx/include"
  "${DEVKITPRO}/portlibs/switch/include"
  "${DEVKITPRO}/portlibs/switch/include/SDL2"
)

# Mesa NVK can be supplied either as an in-tree Mesa build or as the portable
# switchVK SDK package (include/ + lib/libvulkan.a).  Supporting both keeps
# local builds and CI on the same dependency contract.
if(DEFINED MESA_NVK_DIR)
  if(EXISTS "${MESA_NVK_DIR}/lib/libvulkan.a")
    set(GBASTATION_NVK_LIBRARY "${MESA_NVK_DIR}/lib/libvulkan.a")
    target_include_directories(dolphin-nx PRIVATE "${MESA_NVK_DIR}/include")
  else()
    set(GBASTATION_NVK_LIBRARY "${MESA_NVK_DIR}/src/nouveau/vulkan/libvulkan.a")
    target_include_directories(dolphin-nx PRIVATE "${MESA_NVK_DIR}/../include")
    target_link_directories(dolphin-nx PRIVATE
      "${MESA_NVK_DIR}/src/nouveau/vulkan"
      "${MESA_NVK_DIR}/src/vulkan/util"
      "${MESA_NVK_DIR}/src/nouveau/compiler"
      "${MESA_NVK_DIR}/src/compiler/spirv"
      "${MESA_NVK_DIR}/src/util"
      "${MESA_NVK_DIR}/src/nouveau/nil"
      "${MESA_NVK_DIR}/src/nouveau/mme"
      "${MESA_NVK_DIR}/src/nouveau/winsys"
      "${MESA_NVK_DIR}/src/nouveau/headers"
      "${MESA_NVK_DIR}/src/compiler/nir"
      "${MESA_NVK_DIR}/src/compiler"
      "${MESA_NVK_DIR}/src/compiler/rust"
      "${MESA_NVK_DIR}/src/util/blake3"
      "${MESA_NVK_DIR}/src/c11/impl"
    )
  endif()
endif()

# Core Dolphin libraries. videovulkan is linked explicitly so the overlay
# can call VKGfx::SetOverlayCallback and reach g_vulkan_context directly.
target_link_libraries(dolphin-nx PRIVATE
  core
  uicommon
  videovulkan
  imgui
)

# NVK / Vulkan (static) — replaces OGL/EGL Mesa stack
# Use Mesa's public loaderless libvulkan.a package instead of linking
# libnvk.a and every support archive manually.
if(DEFINED GBASTATION_NVK_LIBRARY)
  target_link_libraries(dolphin-nx PRIVATE
    ${GBASTATION_NVK_LIBRARY}
  )
endif()

# Nouveau DRM + compression (system libraries from devkitPro portlibs)
target_link_libraries(dolphin-nx PRIVATE
  -ldrm_nouveau
  -lzstd
  -lz
)

# libnx provides the native audren audio backend used by the Switch frontend.
target_link_libraries(dolphin-nx PRIVATE
  ${DEVKITPRO}/libnx/lib/libnx.a
  m
)

# Linker flags for NRO-compatible ELF
set_target_properties(dolphin-nx PROPERTIES
  OUTPUT_NAME GBAStationDolphinStub
  LINK_FLAGS "-Wl,--gc-sections -Wl,--allow-multiple-definition"
)

# switchVK's libc shim supplies the wrapped implementations. The linker must
# create the corresponding __real_* aliases for the shim to call through.
target_link_options(dolphin-nx PRIVATE
  "-Wl,--wrap=open"
  "-Wl,--wrap=close"
  "-Wl,--wrap=stat"
  "-Wl,--wrap=lstat"
)

install(TARGETS dolphin-nx RUNTIME DESTINATION bin)
