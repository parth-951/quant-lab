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
    src/returns.cpp
    src/historical_var.cpp
    src/historical_risk.cpp
    src/portfolio_historical_risk.cpp
    src/portfolio.cpp
    src/portfolio_exposure.cpp
    src/risk_attribution.cpp

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

    # --------------------------------------------------------
    # Pricing tests
    # --------------------------------------------------------

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


    # --------------------------------------------------------
    # Portfolio tests
    # --------------------------------------------------------

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


    # --------------------------------------------------------
    # Historical returns tests
    # --------------------------------------------------------

    add_executable(
        quant_lab_returns_tests
        tests/returns_tests.cpp
    )

    target_link_libraries(
        quant_lab_returns_tests
        PRIVATE
            quant_lab_core
    )

    add_test(
        NAME quant_lab_returns_tests
        COMMAND quant_lab_returns_tests
    )
    # --------------------------------------------------------
    # Historical VaR tests
    # --------------------------------------------------------

    add_executable(
        quant_lab_historical_var_tests
        tests/historical_var_tests.cpp
    )

    target_link_libraries(
        quant_lab_historical_var_tests
        PRIVATE
            quant_lab_core
    )

    add_test(
        NAME quant_lab_historical_var_tests
        COMMAND quant_lab_historical_var_tests
    )
    # --------------------------------------------------------
    # Historical risk tests
    # --------------------------------------------------------

    add_executable(
        quant_lab_historical_risk_tests
        tests/historical_risk_tests.cpp
    )

    target_link_libraries(
        quant_lab_historical_risk_tests
        PRIVATE
            quant_lab_core
    )

    add_test(
        NAME quant_lab_historical_risk_tests
        COMMAND quant_lab_historical_risk_tests
    )
    add_executable(
        quant_lab_portfolio_historical_risk_tests
        tests/portfolio_historical_risk_tests.cpp
    )

    target_link_libraries(
        quant_lab_portfolio_historical_risk_tests
        PRIVATE quant_lab_core
    )

    add_test(
        NAME quant_lab_portfolio_historical_risk_tests
        COMMAND quant_lab_portfolio_historical_risk_tests
    )
    add_executable(
        quant_lab_portfolio_exposure_tests
        tests/portfolio_exposure_tests.cpp
    )

    target_link_libraries(
        quant_lab_portfolio_exposure_tests
        PRIVATE quant_lab_core
    )

    add_test(
        NAME quant_lab_portfolio_exposure_tests
        COMMAND quant_lab_portfolio_exposure_tests
    )
    add_executable(quant_lab_risk_attribution_tests
        tests/risk_attribution_tests.cpp
    )

    target_link_libraries(quant_lab_risk_attribution_tests
        PRIVATE quant_lab_core
    )

    add_test(
        NAME quant_lab_risk_attribution_tests
        COMMAND quant_lab_risk_attribution_tests
    )

endif()