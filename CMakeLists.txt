cmake_minimum_required(VERSION 3.20)

project(QuantLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# CORE LIBRARY
# ============================================================

add_library(quant_lab_core

    src/random.cpp
    src/gbm.cpp
    src/monte_carlo.cpp
    src/black_scholes.cpp
    src/greeks.cpp
    src/implied_vol.cpp
    src/binomial.cpp
    src/validation.cpp
    src/position.cpp
    src/portfolio.cpp

)

target_include_directories(
    quant_lab_core
    PUBLIC
        include
)


# ============================================================
# MAIN APPLICATION
# ============================================================

add_executable(
    quant_lab
    src/main.cpp
)

target_link_libraries(
    quant_lab
    PRIVATE
        quant_lab_core
)


# ============================================================
# TESTING
# ============================================================

include(CTest)

if(BUILD_TESTING)

    # Pricing tests
    add_executable(
        quant_lab_tests
        tests/pricing_tests.cpp
    )

    target_link_libraries(
        quant_lab_tests
        PRIVATE
            quant_lab_core
    )

    add_test(
        NAME quant_lab_tests
        COMMAND quant_lab_tests
    )


    # Portfolio tests
    add_executable(
        quant_lab_portfolio_tests
        tests/portfolio_tests.cpp
    )

    target_link_libraries(
        quant_lab_portfolio_tests
        PRIVATE
            quant_lab_core
    )

    add_test(
        NAME quant_lab_portfolio_tests
        COMMAND quant_lab_portfolio_tests
    )

endif()
