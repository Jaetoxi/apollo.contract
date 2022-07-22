add_contract(nftone.buy nftone.buy ${CMAKE_CURRENT_SOURCE_DIR}/src/nftone.buy.cpp)

target_include_directories(nftone.buy
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(nftone.buy
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/nftone.buy.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/nftone.buy.contracts.md @ONLY )

target_compile_options( nftone.buy PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
