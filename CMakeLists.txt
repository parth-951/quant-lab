cmake_minimum_required(VERSION 3.20)

project(QuantLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_library(quant_lab_core
    src/binomial.cpp
    src/black_scholes.cpp
    src/gbm.cpp
    src/greeks.cpp
    src/historical_risk.cpp
    src/historical_var.cpp
    src/implied_vol.cpp
    src/monte_carlo.cpp
    src/portfolio.cpp
    src/position.cpp
    src/portfolio_exposure.cpp
    src/portfolio_historical_risk.cpp
    src/random.cpp
    src/returns.cpp
    src/risk_attribution.cpp
    src/statistics.cpp
    src/covariance_matrix.cpp
    src/portfolio_volatility.cpp
    src/portfolio_optimizer.cpp
    src/validation.cpp
)

target_include_directories(quant_lab_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(quant_lab src/main.cpp)
target_link_libraries(quant_lab PRIVATE quant_lab_core)

if(BUILD_TESTING)
    add_executable(quant_lab_tests tests/pricing_tests.cpp)
    target_link_libraries(quant_lab_tests PRIVATE quant_lab_core)
    add_test(NAME quant_lab_tests COMMAND quant_lab_tests)

    add_executable(quant_lab_portfolio_tests tests/portfolio_tests.cpp)
    target_link_libraries(quant_lab_portfolio_tests PRIVATE quant_lab_core)
    add_test(NAME quant_lab_portfolio_tests COMMAND quant_lab_portfolio_tests)

    add_executable(quant_lab_returns_tests tests/returns_tests.cpp)
    target_link_libraries(quant_lab_returns_tests PRIVATE quant_lab_core)
    add_test(NAME quant_lab_returns_tests COMMAND quant_lab_returns_tests)

    add_executable(quant_lab_historical_var_tests tests/historical_var_tests.cpp)
    target_link_libraries(quant_lab_historical_var_tests PRIVATE quant_lab_core)
    add_test(NAME quant_lab_historical_var_tests COMMAND quant_lab_historical_var_tests)

    add_executable(quant_lab_historical_risk_tests tests/historical_risk_tests.cpp)
    target_link_libraries(quant_lab_historical_risk_tests PRIVATE quant_lab_core)
    add_test(NAME quant_lab_historical_risk_tests COMMAND quant_lab_historical_risk_tests)

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

    add_executable(
        quant_lab_risk_attribution_tests
        tests/risk_attribution_tests.cpp
    )
    target_link_libraries(
        quant_lab_risk_attribution_tests
        PRIVATE quant_lab_core
    )
    add_test(
        NAME quant_lab_risk_attribution_tests
        COMMAND quant_lab_risk_attribution_tests
    )

    add_executable(
        quant_lab_statistics_tests
        tests/statistics_tests.cpp
    )
    target_link_libraries(
        quant_lab_statistics_tests
        PRIVATE quant_lab_core
    )
    add_test(
        NAME quant_lab_statistics_tests
        COMMAND quant_lab_statistics_tests
    )

    add_executable(
        quant_lab_covariance_matrix_tests
        tests/covariance_matrix_tests.cpp
    )
    target_link_libraries(
        quant_lab_covariance_matrix_tests
        PRIVATE quant_lab_core
    )
    add_test(
        NAME quant_lab_covariance_matrix_tests
        COMMAND quant_lab_covariance_matrix_tests
    )

    add_executable(
        quant_lab_portfolio_volatility_tests
        tests/portfolio_volatility_tests.cpp
    )
    target_link_libraries(
        quant_lab_portfolio_volatility_tests
        PRIVATE quant_lab_core
    )
    add_test(
        NAME quant_lab_portfolio_volatility_tests
        COMMAND quant_lab_portfolio_volatility_tests
    )

    add_executable(
        quant_lab_portfolio_optimizer_tests
        tests/portfolio_optimizer_tests.cpp
    )
    target_link_libraries(
        quant_lab_portfolio_optimizer_tests
        PRIVATE quant_lab_core
    )
    add_test(
        NAME quant_lab_portfolio_optimizer_tests
        COMMAND quant_lab_portfolio_optimizer_tests
    )
endif()
